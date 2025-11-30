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

// cannot use "lean" here because it is a reserved package name
const std::string LEAN_PROJECT_NAME = "loda-lean";

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

bool LeanFormula::isLocalOrSeqFunc(const std::string& funcName) const {
  return std::find(funcNames.begin(), funcNames.end(), funcName) !=
             funcNames.end() ||
         UID::valid(funcName);
}

bool LeanFormula::needsIntToNat(const Expression& expr) const {
  // Check if expression contains operations that return Int
  return expr.contains(Expression::Type::FUNCTION, "Int.fdiv") ||
         expr.contains(Expression::Type::FUNCTION, "Int.tdiv") ||
         expr.contains(Expression::Type::FUNCTION, "Int.gcd");
}

bool LeanFormula::convertToLean(Expression& expr, int64_t offset,
                                Number patternOffset, bool insideOfLocalFunc) {
  bool childInsideOfLocalFunc =
      insideOfLocalFunc ||
      (expr.type == Expression::Type::FUNCTION && isLocalOrSeqFunc(expr.name));
  // Check children recursively
  for (auto& c : expr.children) {
    if (!convertToLean(c, offset, patternOffset, childInsideOfLocalFunc)) {
      return false;
    }
  }
  switch (expr.type) {
    case Expression::Type::IF:
    case Expression::Type::LOCAL:
    case Expression::Type::VECTOR:
      return false;
    case Expression::Type::FACTORIAL: {
      if (expr.children.size() != 1) {
        return false;
      }
      auto arg = expr.children[0];
      if (ExpressionUtil::canBeNegative(arg, offset)) {
        return false;
      }
      Expression toNat(Expression::Type::FUNCTION, "Int.toNat", {arg});
      Expression factorial(Expression::Type::FUNCTION, "Nat.factorial",
                           {toNat});
      Expression result(Expression::Type::FUNCTION, "Int.ofNat", {factorial});
      expr = result;
      imports.insert("Mathlib.Data.Nat.Factorial.Basic");
      break;
    }
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
      // Allow calls to locally defined functions and sequences
      if (isLocalOrSeqFunc(expr.name)) {
        // When domain is Nat, wrap arguments with Int.toNat to convert Int to
        // Nat Only wrap if the argument needs it (contains Int-returning
        // operations) or is not a plain PARAMETER
        if (domain == "Nat") {
          for (auto& arg : expr.children) {
            if (arg.type != Expression::Type::PARAMETER && needsIntToNat(arg)) {
              Expression toNat(Expression::Type::FUNCTION, "Int.toNat", {arg});
              arg = toNat;
            }
          }
        }
        break;
      }
      return false;
    }
    case Expression::Type::POWER: {
      // Support only non-negative exponents
      if (expr.children.size() != 2) {
        return false;
      }
      if (ExpressionUtil::canBeNegative(expr.children[1], offset)) {
        return false;
      }
      // Wrap non-constant exponent with Int.toNat for LEAN compatibility
      if (expr.children[1].type != Expression::Type::CONSTANT) {
        Expression toNat(Expression::Type::FUNCTION, "Int.toNat",
                         {expr.children[1]});
        expr.children[1] = toNat;
      }
      break;
    }
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

  // Check for mutual recursion - LEAN can't prove termination automatically
  // for mutually recursive functions without explicit termination proofs
  if (FormulaUtil::hasMutualRecursion(formula)) {
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
    if (!lean_formula.convertToLean(right, offset, patternOffset, false)) {
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
  Expression generalRHS;

  for (const auto& entry : main_formula.entries) {
    if (entry.first.name != funcName) {
      continue;
    }
    const auto& arg = entry.first.children.front();
    if (arg.type == Expression::Type::CONSTANT) {
      // store RHS keyed by the constant value; map keeps keys sorted
      baseCases[arg.value.asInt()] = entry.second;
    } else {
      generalRHS = entry.second;
    }
  }

  bool usesParameter = generalRHS.contains(Expression::Type::PARAMETER);
  std::string arg = usesParameter ? "n" : "_";

  // Recursive case with base cases - use pattern matching syntax
  if (!baseCases.empty()) {
    buf << "def " << funcName << " : " << domain << " -> Int";
    for (const auto& kv : baseCases) {
      const auto constValue = kv.first;
      buf << " | " << Number(constValue).to_string() << " => "
          << kv.second.toString(true);
    }
    if (domain == "Nat" && !baseCases.empty()) {
      int64_t maxBaseCase = baseCases.rbegin()->first;
      int64_t patternOffset = maxBaseCase + 1;
      buf << " | " << arg << "+" << patternOffset << " => "
          << generalRHS.toString(true);
    } else {
      buf << " | " << arg << " => " << generalRHS.toString(true);
    }
  } else {
    buf << "def " << funcName << " (" << arg << " : " << domain
        << ") : Int := " << generalRHS.toString(true);
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
  auto dir = Setup::getCacheHome() + LEAN_PROJECT_NAME + FILE_SEP;
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

  const std::string cacheHome = Setup::getCacheHome();
  const std::string projectDir = cacheHome + LEAN_PROJECT_NAME + FILE_SEP;
  const std::string lakeFilePath = projectDir + "lakefile.lean";

  // Check if project is already initialized
  if (isFile(lakeFilePath)) {
    initialized = true;
    return true;
  }

  Log::get().info("Initializing LEAN project at " + projectDir);

  // Use lake to create a new Lean project with Mathlib dependency
  // The command is: lake +leanprover-community/mathlib4:lean-toolchain new
  // <project-name> math
  std::vector<std::string> initArgs = {
      "lake", "+leanprover-community/mathlib4:lean-toolchain", "new",
      LEAN_PROJECT_NAME, "math"};

  int initTimeout = 600;  // 10 minutes for initial setup
  int exitCode = execWithTimeout(initArgs, initTimeout, "", cacheHome);
  if (exitCode != 0) {
    Log::get().warn("lake new failed with exit code " +
                    std::to_string(exitCode));
    return false;
  }

  // Build the project to download and compile dependencies
  std::vector<std::string> buildArgs = {"lake", "build"};
  int buildTimeout = 1200;  // 20 minutes for building (mathlib can be large)
  exitCode = execWithTimeout(buildArgs, buildTimeout, "", projectDir);
  if (exitCode != 0) {
    Log::get().warn("lake build failed with exit code " +
                    std::to_string(exitCode));
    return false;
  }

  // Download precompiled Mathlib cache to avoid lengthy compilation
  std::vector<std::string> cacheArgs = {"lake", "exe", "cache", "get"};
  int cacheTimeout = 1200;  // 20 minutes for downloading cache
  exitCode = execWithTimeout(cacheArgs, cacheTimeout, "", projectDir);
  if (exitCode != 0) {
    Log::get().warn("lake exe cache get failed with exit code " +
                    std::to_string(exitCode) + ", continuing anyway");
    // Don't fail here - the project might still work without cache
  }

  initialized = true;
  return true;
}
