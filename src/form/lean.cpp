#include "form/lean.hpp"

#include <fstream>
#include <sstream>

#ifndef _WIN64
#include <sys/stat.h>
#endif

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "seq/seq_util.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/process.hpp"
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
      // Convert bitxor to LEAN Int.xor
      if (expr.name == "bitxor") {
        expr.name = "Int.xor";
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
  // Add imports if needed
  std::string imports = getImports();
  if (!imports.empty()) {
    out << imports << std::endl;
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

bool LeanFormula::eval(int64_t offset, int64_t numTerms, int timeoutSeconds,
                       Sequence& result) const {
  // Initialize LEAN project if needed (only once)
  if (needsBitwiseImport() && !initializeLeanProject()) {
    Log::get().warn("Failed to initialize LEAN project, continuing without it");
  }

  const std::string tmpFileId = std::to_string(Random::get().gen() % 1000);
  std::string leanPath;
  std::string leanResult;
  std::vector<std::string> args;

  // If we need imports, we need to use a LEAN project structure
  if (needsBitwiseImport()) {
    const std::string projectDir = getTmpDir() + "lean-project/";
    leanPath = projectDir + "Main.lean";
    leanResult = projectDir + "lean-result-" + tmpFileId + ".txt";

    // Create a wrapper script to run lake in the project directory
    std::string runScript = projectDir + "run-" + tmpFileId + ".sh";
    std::ofstream scriptFile(runScript);
    if (!scriptFile) {
      Log::get().error("Failed to create run script", true);
    }
    scriptFile << "#!/bin/sh\n";
    scriptFile << "cd " + projectDir + "\n";
    scriptFile << "lake env lean --run Main.lean\n";
    scriptFile.close();

    // Make script executable on Unix systems
#ifndef _WIN64
    chmod(runScript.c_str(), 0755);
#endif

    args = {runScript};
  } else {
    leanPath = "lean-loda-" + tmpFileId + ".lean";
    leanResult = "lean-result-" + tmpFileId + ".txt";
    args = {"lean", "--run", leanPath};
  }

  std::string evalCode = printEvalCode(offset, numTerms);

  // For project-based eval, we need to handle it differently
  if (needsBitwiseImport()) {
    const std::string projectDir = getTmpDir() + "lean-project/";
    std::string runScript = projectDir + "run-" + tmpFileId + ".sh";

    // Write the code to Main.lean
    std::ofstream leanFile(leanPath);
    if (!leanFile) {
      Log::get().error("Error generating LEAN file", true);
    }
    leanFile << evalCode;
    leanFile.close();

    // Execute with timeout
    int exitCode = execWithTimeout(args, timeoutSeconds, leanResult);
    if (exitCode != 0) {
      std::remove(leanPath.c_str());
      std::remove(runScript.c_str());
      if (exitCode == PROCESS_ERROR_TIMEOUT) {
        return false;  // timeout
      }
      Log::get().error(
          "Error evaluating LEAN code: exit code " + std::to_string(exitCode),
          true);
    }

    // Read the result
    std::ifstream resultIn(leanResult);
    if (!resultIn) {
      std::remove(leanPath.c_str());
      std::remove(leanResult.c_str());
      std::remove(runScript.c_str());
      Log::get().error("Error reading LEAN result", true);
    }
    result.clear();
    std::string line;
    while (std::getline(resultIn, line)) {
      try {
        result.push_back(Number(line));
      } catch (...) {
        resultIn.close();
        std::remove(leanPath.c_str());
        std::remove(leanResult.c_str());
        std::remove(runScript.c_str());
        Log::get().error("Error parsing LEAN output: " + line, true);
      }
    }
    resultIn.close();
    std::remove(leanPath.c_str());
    std::remove(leanResult.c_str());
    std::remove(runScript.c_str());
    return true;
  } else {
    return SequenceUtil::evalFormulaWithExternalTool(evalCode, getName(),
                                                     leanPath, leanResult, args,
                                                     timeoutSeconds, result);
  }
}

bool LeanFormula::needsBitwiseImport() const {
  for (const auto& entry : main_formula.entries) {
    if (entry.second.contains(Expression::Type::FUNCTION, "Int.xor")) {
      return true;
    }
  }
  return false;
}

std::string LeanFormula::getImports() const {
  if (needsBitwiseImport()) {
    return "import Mathlib.Data.Int.Bitwise";
  }
  return "";
}

bool LeanFormula::initializeLeanProject() {
  static bool initialized = false;
  if (initialized) {
    return true;
  }

  const std::string projectDir = getTmpDir() + "lean-project/";

  // Check if project directory already exists and is initialized
  if (isDir(projectDir) && isFile(projectDir + "lakefile.lean")) {
    initialized = true;
    return true;
  }

  // Create project directory
  ensureDir(projectDir);

  // Create lakefile.lean
  std::ofstream lakefile(projectDir + "lakefile.lean");
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
  // timeout) Note: This requires lake to be run from the project directory We
  // use a wrapper script to change directory first The script also copies the
  // Mathlib toolchain to match the dependency
  std::string updateScript = projectDir + "update.sh";
  std::ofstream scriptFile(updateScript);
  if (!scriptFile) {
    Log::get().warn("Failed to create update script");
    return false;
  }
  scriptFile << "#!/bin/sh\n";
  scriptFile << "cd " + projectDir + "\n";
  scriptFile << "lake update\n";
  scriptFile << "if [ -f .lake/packages/mathlib/lean-toolchain ]; then\n";
  scriptFile << "  cp .lake/packages/mathlib/lean-toolchain ./lean-toolchain\n";
  scriptFile << "fi\n";
  scriptFile.close();

  // Make script executable on Unix systems
#ifndef _WIN64
  chmod(updateScript.c_str(), 0755);
#endif

  std::vector<std::string> updateArgs = {updateScript};
  int exitCode = execWithTimeout(updateArgs, 300);
  std::remove(updateScript.c_str());

  if (exitCode != 0) {
    Log::get().warn("lake update failed with exit code " +
                    std::to_string(exitCode));
    return false;
  }

  initialized = true;
  return true;
}
