#include "form/lean.hpp"

#include <algorithm>
#include <sstream>

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "seq/seq_util.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/process.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

std::string convertBitfuncToLean(const std::string& bitfunc) {
  if (bitfunc == "bitand") {
    return "Int.land";
  } else if (bitfunc == "bitor") {
    return "Int.lor";
  } else if (bitfunc == "bitxor") {
    return "Int.xor";
  }
  return "";
}

bool LeanFormula::isLocalFunc(const std::string& funcName) const {
  return std::find(funcNames.begin(), funcNames.end(), funcName) !=
         funcNames.end();
}

bool LeanFormula::convertToLean(Expression& expr, Number patternOffset,
                                bool insideOfLocalFunc) {
  bool childInsideOfLocalFunc =
      insideOfLocalFunc ||
      (expr.type == Expression::Type::FUNCTION && isLocalFunc(expr.name));
  // Check children recursively
  for (auto& c : expr.children) {
    if (!convertToLean(c, patternOffset, childInsideOfLocalFunc)) {
      return false;
    }
  }
  switch (expr.type) {
    case Expression::Type::IF:
    case Expression::Type::LOCAL:
    case Expression::Type::VECTOR:
    case Expression::Type::FACTORIAL:
      return false;
    case Expression::Type::EQUAL:
    case Expression::Type::NOT_EQUAL:
    case Expression::Type::LESS_EQUAL:
    case Expression::Type::GREATER_EQUAL: {
      Expression f(Expression::Type::FUNCTION, "Bool.toInt", {expr});
      expr = f;
      break;
    }
    case Expression::Type::PARAMETER: {
      if (domain == "Nat" && !insideOfLocalFunc) {
        Expression cast(Expression::Type::FUNCTION, "Int.ofNat", {expr});
        expr = cast;
      }
      if (patternOffset != Number::ZERO && patternOffset != Number::INF) {
        Expression sum(
            Expression::Type::SUM, "",
            {expr, ExpressionUtil::newConstant(patternOffset.asInt())});
        expr = sum;
      }
      break;
    }
    case Expression::Type::FUNCTION: {
      if (expr.name == "min" || expr.name == "max") {
        break;
      }
      if (expr.name == "gcd") {
        expr.name = "Int.gcd";
        break;
      }
      // Convert floor and truncate functions to LEAN equivalents
      if (expr.name == "floor" || expr.name == "truncate") {
        // These functions should have a single FRACTION argument
        if (expr.children.size() == 1 &&
            expr.children[0].type == Expression::Type::FRACTION &&
            expr.children[0].children.size() == 2) {
          // Extract numerator and denominator from fraction
          auto numerator = expr.children[0].children[0];
          auto denominator = expr.children[0].children[1];
          // Replace with Int.fdiv or Int.tdiv
          expr.name = (expr.name == "floor") ? "Int.fdiv" : "Int.tdiv";
          expr.children.clear();
          expr.children.push_back(numerator);
          expr.children.push_back(denominator);
          break;
        }
        // If not a simple fraction, reject
        return false;
      }
      // Convert bitwise functions
      auto bitfunc = convertBitfuncToLean(expr.name);
      if (!bitfunc.empty()) {
        expr.name = bitfunc;
        imports.insert("Mathlib.Data.Int.Bitwise");
        break;
      }
      // Allow calls to locally defined functions incl. recursions
      if (isLocalFunc(expr.name)) {
        break;
      }
      return false;
    }
    case Expression::Type::POWER:
      // Support only non-negative constants as exponents
      if (expr.children.size() != 2 ||
          expr.children[1].type != Expression::Type::CONSTANT ||
          expr.children[1].value < Number::ZERO) {
        return false;
      }
      break;
    default:
      break;
  }
  ExpressionUtil::normalize(expr);
  return true;
}

bool LeanFormula::convert(const Formula& formula, int64_t offset,
                          bool as_vector, LeanFormula& lean_formula) {
  // Check if conversion is supported.
  if (as_vector) {
    return false;
  }
  lean_formula = {};
  lean_formula.domain = "Int";
  lean_formula.funcNames =
      FormulaUtil::getDefinitions(formula, Expression::Type::FUNCTION);
  if (lean_formula.funcNames.empty()) {
    return false;
  }
  for (const auto& f : lean_formula.funcNames) {
    if (FormulaUtil::isRecursive(formula, f)) {
      if (offset != 0 ||
          FormulaUtil::getMinimumBaseCase(formula, f) != Number::ZERO) {
        return false;
      }
      lean_formula.domain = "Nat";
    }
  }

  std::map<std::string, int64_t> maxBaseCases;
  for (const auto& entry : formula.entries) {
    if (entry.first.type != Expression::Type::FUNCTION ||
        entry.first.children.size() != 1) {
      continue;
    }
    const auto& arg = entry.first.children.front();
    if (arg.type == Expression::Type::CONSTANT) {
      if (maxBaseCases[entry.first.name] < arg.value.asInt()) {
        maxBaseCases[entry.first.name] = arg.value.asInt();
      }
    }
  }

  for (const auto& entry : formula.entries) {
    auto left = entry.first;
    auto right = entry.second;
    Number patternOffset = Number::INF;
    if (ExpressionUtil::isSimpleFunction(left, true) &&
        maxBaseCases.find(left.name) != maxBaseCases.end()) {
      patternOffset = maxBaseCases[left.name] + 1;
    }
    if (!lean_formula.convertToLean(right, patternOffset, false)) {
      return false;
    }
    lean_formula.main_formula.entries[left] = right;
  }

  return lean_formula.main_formula.entries.size() >= 1;
}

std::string LeanFormula::toString() const {
  auto functions = FormulaUtil::getDefinitions(main_formula);

  std::stringstream buf;
  if (functions.size() == 1) {
    buf << printFunction(functions[0]);
  } else {
    buf << "mutual\n";
    for (size_t i = 0; i < functions.size(); ++i) {
      buf << "  " << printFunction(functions[i]) << "\n";
    }
    buf << "end";
  }
  return buf.str();
}

std::string LeanFormula::printFunction(const std::string& funcName) const {
  std::stringstream buf;

  // Collect base cases (constant arguments) and recursive case (parameter
  // argument)
  // Map from base case constant value -> RHS expression. Using std::map
  // automatically keeps base cases sorted by the integer key.
  std::map<int64_t, Expression> baseCases;
  Expression recursiveCase;
  Expression recursiveRHS;

  for (const auto& entry : main_formula.entries) {
    if (entry.first.name != funcName) {
      continue;
    }
    const auto& arg = entry.first.children.front();
    if (arg.type == Expression::Type::CONSTANT) {
      // store RHS keyed by the constant value; map keeps keys sorted
      baseCases[arg.value.asInt()] = entry.second;
    } else {
      recursiveCase = entry.first;
      recursiveRHS = entry.second;
    }
  }

  const bool isRecursive = FormulaUtil::isRecursive(main_formula, funcName);
  if (isRecursive) {
    // Recursive case with base cases - use pattern matching syntax
    buf << "def " << funcName << " : " << domain << " -> Int";

    // Generate pattern matching cases for base cases. The map is sorted by
    // integer key so iteration yields increasing constant values.
    for (const auto& kv : baseCases) {
      const auto constValue = kv.first;
      buf << " | " << Number(constValue).to_string() << " => "
          << kv.second.toString(true);
    }

    // Add the recursive case
    if (!recursiveCase.name.empty()) {
      // For Nat domain, use n+k pattern where k is one more than the largest
      // base case
      if (domain == "Nat" && !baseCases.empty()) {
        // Find the largest base case value (map is ordered ascending)
        int64_t maxBaseCase = baseCases.rbegin()->first;
        int64_t patternOffset = maxBaseCase + 1;
        buf << " | n+" << patternOffset << " => "
            << recursiveRHS.toString(true);
      } else {
        buf << " | n => " << recursiveRHS.toString(true);
      }
    }
  } else {
    // Non-recursive case (original behavior)
    bool usesParameter = recursiveRHS.contains(Expression::Type::PARAMETER);
    std::string arg = usesParameter ? "n" : "_";
    buf << "def " << funcName << " (" << arg << " : " << domain
        << ") : Int := " << recursiveRHS.toString(true);
  }

  return buf.str();
}

std::string LeanFormula::printEvalCode(int64_t offset, int64_t numTerms) const {
  std::stringstream out;
  if (!imports.empty()) {
    for (const auto& imp : imports) {
      out << "import " << imp << std::endl;
    }
    out << std::endl;
  }
  out << toString() << std::endl;
  out << std::endl;
  out << "def main : IO Unit := do" << std::endl;
  out << "  let offset : " << domain << " := " << offset << std::endl;
  out << "  let num_terms : Nat := " << numTerms << std::endl;
  out << std::endl;
  out << "  let rec loop (count : Nat) (n : " << domain << ") : IO Unit := do"
      << std::endl;
  out << "    if count < num_terms then" << std::endl;
  out << "      IO.println (toString (a n))" << std::endl;
  out << "      loop (count + 1) (n + 1)" << std::endl;
  out << "    else" << std::endl;
  out << "      pure ()" << std::endl;
  out << std::endl;
  out << "  loop 0 offset" << std::endl;
  return out.str();
}

std::string getLeanProjectDir() {
  auto dir = Setup::getCacheHome() + "lean" + FILE_SEP;
  ensureDir(dir);
  return dir;
}

bool LeanFormula::eval(int64_t offset, int64_t numTerms, int timeoutSeconds,
                       Sequence& result) const {
  // Initialize LEAN project if needed (only once)
  bool needsProject = !imports.empty();
  if (needsProject && !initializeLeanProject()) {
    Log::get().error("Failed to initialize LEAN project", true);
  }

  const std::string tmpFileId = std::to_string(Random::get().gen() % 1000);
  const std::string projectDir = getLeanProjectDir();
  const std::string leanPath = projectDir + "Main.lean";
  const std::string leanResult = projectDir + "result-" + tmpFileId + ".txt";
  const std::string evalCode = printEvalCode(offset, numTerms);
  std::vector<std::string> args;

  // If we need imports, we need to use a LEAN project structure
  if (needsProject) {
    args = {"lake", "env", "lean", "--run", "Main.lean"};
    timeoutSeconds = std::max<int>(timeoutSeconds, 600);  // 10 minutes
  } else {
    args = {"lean", "--run", leanPath};
  }
  return SequenceUtil::evalFormulaWithExternalTool(
      evalCode, getName(), leanPath, leanResult, args, timeoutSeconds, result,
      projectDir);
}

bool LeanFormula::initializeLeanProject() {
  static bool initialized = false;
  if (initialized) {
    return true;
  }
  // Check if project is already initialized
  const std::string projectDir = getLeanProjectDir();
  const std::string lakeFilePath = projectDir + "lakefile.lean";
  if (isFile(lakeFilePath)) {
    initialized = true;
    return true;
  }

  Log::get().info("Initializing LEAN project at " + projectDir);

  // Create lakefile.lean
  std::ofstream lakefile(lakeFilePath);
  if (!lakefile) {
    return false;
  }
  lakefile << "import Lake\n";
  lakefile << "open Lake DSL\n\n";
  lakefile << "package \"lean-loda\" where\n";
  lakefile << "  version := v!\"0.1.0\"\n\n";
  lakefile << "require mathlib from git\n";
  lakefile << "  \"https://github.com/leanprover-community/mathlib4.git\"\n\n";
  lakefile << "@[default_target]\n";
  lakefile << "lean_exe «lean-loda» where\n";
  lakefile << "  root := `Main\n";
  lakefile.close();

  // Run lake update to download dependencies and create toolchain (with
  // timeout). Use a larger timeout here because fetching mathlib and
  // preparing the toolchain can be slow on first run.
  std::vector<std::string> updateArgs = {"lake", "update"};
  int updateTimeout = 300;  // 5 minutes
  int exitCode = execWithTimeout(updateArgs, updateTimeout, "", projectDir);
  if (exitCode != 0) {
    Log::get().warn("lake update failed with exit code " +
                    std::to_string(exitCode));
    std::remove(lakeFilePath.c_str());
    return false;
  }
  updateArgs = {"lake", "exe", "cache", "get"};
  exitCode = execWithTimeout(updateArgs, updateTimeout, "", projectDir);
  if (exitCode != 0) {
    Log::get().warn("lake exe cache get failed with exit code " +
                    std::to_string(exitCode));
    std::remove(lakeFilePath.c_str());
    return false;
  }

  initialized = true;
  return true;
}
