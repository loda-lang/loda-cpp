#include "form/lean.hpp"

#include <sstream>

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "seq/seq_util.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/process.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

bool LeanFormula::convertToLean(Expression& expr, const Formula& f,
                                const std::string& funcName) {
  // Check children recursively
  for (auto& c : expr.children) {
    if (!convertToLean(c, f, funcName)) {
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
      if (domain == "Nat") {
        Expression cast(Expression::Type::FUNCTION, "Int.ofNat", {expr});
        expr = cast;
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
      if (expr.name == "bitand" || expr.name == "bitor" ||
          expr.name == "bitxor") {
        expr.name = "Int." + expr.name.substr(3);
        imports.insert("Mathlib.Data.Int.Bitwise");
        break;
      }
      // Allow recursive calls to the main function
      if (expr.name == funcName) {
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
  return true;
}

bool LeanFormula::convert(const Formula& formula, int64_t offset,
                          bool as_vector, LeanFormula& lean_formula) {
  // Check if conversion is supported.
  if (as_vector) {
    return false;
  }
  auto functions =
      FormulaUtil::getDefinitions(formula, Expression::Type::FUNCTION);
  if (functions.size() != 1) {
    return false;
  }
  const std::string funcName = functions[0];
  lean_formula = {};
  if (FormulaUtil::isRecursive(formula, funcName)) {
    if (offset < 0) {
      return false;
    }
    lean_formula.domain = "Nat";
  } else {
    lean_formula.domain = "Int";
  }

  // Collect base cases to check minimum value for Nat domain
  bool hasBaseCase = false;
  Number minBaseCase = Number::ZERO;
  for (const auto& entry : formula.entries) {
    auto left = entry.first;
    auto right = entry.second;
    if (!lean_formula.convertToLean(right, formula, funcName)) {
      return false;
    }
    if (left.type != Expression::Type::FUNCTION || left.children.size() != 1) {
      return false;
    }
    auto& arg = left.children.front();
    if (arg.type != Expression::Type::PARAMETER &&
        arg.type != Expression::Type::CONSTANT) {
      return false;
    }
    if (arg.type == Expression::Type::CONSTANT) {
      if (!hasBaseCase || arg.value < minBaseCase) {
        minBaseCase = arg.value;
        hasBaseCase = true;
      }
    }
    lean_formula.main_formula.entries[left] = right;
  }

  // For Nat domain with recursive formulas, the minimum base case must be 0
  if (lean_formula.domain == "Nat" &&
      FormulaUtil::isRecursive(formula, funcName) && hasBaseCase &&
      minBaseCase != Number::ZERO) {
    return false;
  }

  return lean_formula.main_formula.entries.size() >= 1;
}

std::string LeanFormula::toString() const {
  std::stringstream buf;
  std::string funcName;

  // Collect base cases (constant arguments) and recursive case (parameter
  // argument)
  std::vector<std::pair<Expression, Expression>> baseCases;
  Expression recursiveCase;
  Expression recursiveRHS;

  for (const auto& entry : main_formula.entries) {
    funcName = entry.first.name;
    const auto& arg = entry.first.children.front();
    if (arg.type == Expression::Type::CONSTANT) {
      baseCases.push_back({entry.first, entry.second});
    } else {
      recursiveCase = entry.first;
      recursiveRHS = entry.second;
    }
  }

  // Check if this is a recursive formula (multiple entries)
  if (main_formula.entries.size() == 1) {
    // Non-recursive case (original behavior)
    bool usesParameter = recursiveRHS.contains(Expression::Type::PARAMETER);
    std::string arg = usesParameter ? "n" : "_";
    buf << "def " << funcName << " (" << arg << " : " << domain
        << ") : Int := " << recursiveRHS.toString(true);
  } else {
    // Recursive case with base cases - use pattern matching syntax
    buf << "def " << funcName << " : " << domain << " -> Int";

    // Sort base cases by constant value for consistent output
    std::sort(baseCases.begin(), baseCases.end(),
              [](const auto& a, const auto& b) {
                return a.first.children.front().value <
                       b.first.children.front().value;
              });

    // Generate pattern matching cases for base cases
    for (const auto& bc : baseCases) {
      const auto& constValue = bc.first.children.front().value;
      buf << " | " << constValue.to_string() << " => "
          << bc.second.toString(true);
    }

    // Add the recursive case
    if (!recursiveCase.name.empty()) {
      // For Nat domain, use n+k pattern where k is one more than the largest
      // base case
      if (domain == "Nat" && !baseCases.empty()) {
        // Find the largest base case value
        int64_t maxBaseCase =
            baseCases.back().first.children.front().value.asInt();
        int64_t patternOffset = maxBaseCase + 1;

        // Transform the RHS: replace (n-k) with (n+(patternOffset-k))
        Expression transformedRHS = recursiveRHS;
        transformParameterReferences(transformedRHS, patternOffset, funcName);

        buf << " | n+" << patternOffset << " => "
            << transformedRHS.toString(true);
      } else {
        buf << " | n => " << recursiveRHS.toString(true);
      }
    }
  }
  return buf.str();
}

void LeanFormula::transformParameterReferences(
    Expression& expr, int64_t offset, const std::string& funcName) const {
  // If this is a function call to funcName, transform its argument first
  // before recursing into children
  if (expr.type == Expression::Type::FUNCTION && expr.name == funcName) {
    if (expr.children.size() == 1) {
      auto& arg = expr.children[0];
      // Check if the argument is (Int.ofNat n) + k
      if (arg.type == Expression::Type::SUM && arg.children.size() >= 2) {
        // Look for Int.ofNat cast and constant
        bool hasIntOfNat = false;
        Number constantValue = Number::ZERO;
        std::vector<Expression> otherTerms;

        for (const auto& term : arg.children) {
          if (ExpressionUtil::isSimpleNamedFunction(term, "Int.ofNat")) {
            hasIntOfNat = true;
          } else if (term.type == Expression::Type::CONSTANT) {
            constantValue += term.value;
          } else {
            otherTerms.push_back(term);
          }
        }

        if (hasIntOfNat && otherTerms.empty()) {
          // We have (Int.ofNat n) + constant
          // Transform to n + (offset + constant)
          int64_t newConstant = offset + constantValue.asInt();
          expr.children[0] = ExpressionUtil::createParameterSum(newConstant);
        }
      } else if (ExpressionUtil::isSimpleNamedFunction(arg, "Int.ofNat")) {
        // Simple case: just (Int.ofNat n)
        // Transform to n + offset
        expr.children[0] = ExpressionUtil::createParameterSum(offset);
      }
    }
    // Don't recurse into function call arguments after transforming them
    return;
  }

  // Transform recursively for all children
  for (auto& child : expr.children) {
    transformParameterReferences(child, offset, funcName);
  }

  // After transforming children, if this is a standalone Int.ofNat(n),
  // transform it
  if (ExpressionUtil::isSimpleNamedFunction(expr, "Int.ofNat") && offset > 0) {
    // Replace Int.ofNat(n) with (Int.ofNat n) + offset
    Expression sum(Expression::Type::SUM);
    sum.children.push_back(expr);
    sum.children.push_back(ExpressionUtil::newConstant(offset));
    expr = sum;
  }
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
  int updateTimeout = 900;  // 15 minutes
  int exitCode = execWithTimeout(updateArgs, updateTimeout, "", projectDir);
  if (exitCode != 0) {
    Log::get().warn("lake update failed with exit code " +
                    std::to_string(exitCode));
    std::remove(lakeFilePath.c_str());
    return false;
  }
  initialized = true;
  return true;
}
