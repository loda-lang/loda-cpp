#include "form/lean.hpp"

#include <fstream>
#include <sstream>

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "sys/log.hpp"
#include "sys/process.hpp"

// Forward declaration
std::string exprToLeanString(const Expression& expr, const Formula& f,
                              const std::string& funcName);

bool convertExprToLean(Expression& expr, const Formula& f) {
  // convert bottom-up!
  for (auto& c : expr.children) {
    if (!convertExprToLean(c, f)) {
      return false;
    }
  }
  // LEAN doesn't support some advanced features yet
  // We only support simple recursive functions for now
  return true;
}

std::string binaryExpr(const Expression& expr, const Formula& f,
                       const std::string& funcName, const std::string& op) {
  if (expr.children.size() != 2) {
    return "";
  }
  std::stringstream ss;
  ss << "(" << exprToLeanString(expr.children[0], f, funcName) << " " << op
     << " " << exprToLeanString(expr.children[1], f, funcName) << ")";
  return ss.str();
}

std::string naryExpr(const Expression& expr, const Formula& f,
                     const std::string& funcName, const std::string& op) {
  std::stringstream ss;
  ss << "(";
  for (size_t i = 0; i < expr.children.size(); i++) {
    if (i > 0) ss << " " << op << " ";
    ss << exprToLeanString(expr.children[i], f, funcName);
  }
  ss << ")";
  return ss.str();
}

std::string exprToLeanString(const Expression& expr, const Formula& f,
                              const std::string& funcName) {
  std::stringstream ss;
  switch (expr.type) {
    case Expression::Type::CONSTANT:
      ss << expr.value;
      break;
    case Expression::Type::PARAMETER:
      ss << expr.name;
      break;
    case Expression::Type::FUNCTION: {
      auto functions =
          FormulaUtil::getDefinitions(f, Expression::Type::FUNCTION);
      if (std::find(functions.begin(), functions.end(), expr.name) !=
          functions.end()) {
        // Recursive call
        ss << expr.name;
        if (!expr.children.empty()) {
          ss << " " << exprToLeanString(expr.children[0], f, funcName);
        }
      } else {
        // Built-in function - need to map to LEAN syntax
        // For now, we keep the generic function call syntax
        ss << expr.name;
        if (!expr.children.empty()) {
          ss << " (";
          for (size_t i = 0; i < expr.children.size(); i++) {
            if (i > 0) ss << ", ";
            ss << exprToLeanString(expr.children[i], f, funcName);
          }
          ss << ")";
        }
      }
      break;
    }
    case Expression::Type::SUM:
      return naryExpr(expr, f, funcName, "+");
    case Expression::Type::PRODUCT:
      return naryExpr(expr, f, funcName, "*");
    case Expression::Type::FRACTION:
      return binaryExpr(expr, f, funcName, "/");
    case Expression::Type::POWER:
      return binaryExpr(expr, f, funcName, "^");
    case Expression::Type::MODULUS:
      return binaryExpr(expr, f, funcName, "%");
    case Expression::Type::IF:
      // LEAN uses pattern matching, not if-then-else in recursive definitions
      // This is handled at a higher level
      return "";
    default:
      // Unsupported types
      return "";
  }
  return ss.str();
}

bool LeanFormula::convert(const Formula& formula, LeanFormula& lean_formula) {
  lean_formula = LeanFormula();
  
  // Check if this is a simple formula we can convert
  auto functions =
      FormulaUtil::getDefinitions(formula, Expression::Type::FUNCTION);
  if (functions.empty()) {
    return false;  // No functions to convert
  }
  
  // For now, only support single function formulas
  if (functions.size() > 1) {
    return false;  // Multiple functions not yet supported
  }
  
  // Copy the formula
  for (const auto& entry : formula.entries) {
    auto left = entry.first;
    auto right = entry.second;
    
    // Don't convert advanced features yet
    if (right.contains(Expression::Type::LOCAL)) {
      return false;
    }
    if (right.contains(Expression::Type::VECTOR)) {
      return false;
    }
    
    if (!convertExprToLean(right, formula)) {
      return false;
    }
    lean_formula.main_formula.entries[left] = right;
  }
  
  return true;
}

std::string LeanFormula::toString() const {
  std::stringstream buf;
  
  // Get the main function
  auto functions =
      FormulaUtil::getDefinitions(main_formula, Expression::Type::FUNCTION);
  if (functions.empty()) {
    return "";
  }
  
  // For now, only handle single function
  const std::string funcName = functions[0];
  
  // Check if this is a simple direct formula (no recursion)
  bool hasRecursion = false;
  Expression mainExpr;
  
  for (const auto& entry : main_formula.entries) {
    if (entry.first.type == Expression::Type::FUNCTION &&
        entry.first.name == funcName) {
      mainExpr = entry.second;
      // Check if the expression contains a recursive call
      if (mainExpr.contains(Expression::Type::FUNCTION, funcName)) {
        hasRecursion = true;
      }
      break;
    }
  }
  
  // Generate LEAN function definition
  buf << "def " << funcName << " : ℕ → ℕ := fun n => ";
  
  if (!hasRecursion) {
    // Simple non-recursive formula
    buf << exprToLeanString(mainExpr, main_formula, funcName);
  } else {
    // Recursive formula - use pattern matching
    // This is more complex and requires extracting base cases from IF expressions
    // For now, generate a placeholder
    buf << std::endl;
    buf << "  | 0 => 0  -- TODO: extract base case" << std::endl;
    buf << "  | n + 1 => " << exprToLeanString(mainExpr, main_formula, funcName);
  }
  
  return buf.str();
}

void LeanFormula::printEvalCode(int64_t offset, int64_t numTerms,
                                std::ostream& out) const {
  // Generate LEAN code to evaluate the function
  out << toString() << std::endl;
  out << std::endl;
  
  // Generate evaluation code
  out << "#eval List.range " << numTerms << " |>.map (fun i => "
      << "a (i + " << offset << "))" << std::endl;
}

bool LeanFormula::eval(int64_t offset, int64_t numTerms, int timeoutSeconds,
                       Sequence& result) const {
  const std::string leanPath("loda-eval.lean");
  const std::string leanResult("lean-result.txt");
  
  std::ofstream lean(leanPath);
  if (!lean) {
    Log::get().error("Error generating lean file", true);
  }
  printEvalCode(offset, numTerms, lean);
  lean.close();

  std::vector<std::string> args = {"lean", "--run", leanPath};
  int exitCode = execWithTimeout(args, timeoutSeconds, leanResult);
  if (exitCode != 0) {
    std::remove(leanPath.c_str());
    std::remove(leanResult.c_str());
    if (exitCode == PROCESS_ERROR_TIMEOUT) {
      return false;  // timeout
    } else {
      Log::get().error("Error evaluating LEAN code: lean exited with code " +
                           std::to_string(exitCode),
                       true);
    }
  }

  // read result from file
  result.clear();
  std::ifstream resultIn(leanResult);
  std::string buf;
  if (!resultIn) {
    Log::get().error("Error reading LEAN output", true);
  }
  while (std::getline(resultIn, buf)) {
    result.push_back(Number(buf));
  }

  // clean up
  std::remove(leanPath.c_str());
  std::remove(leanResult.c_str());

  return true;
}
