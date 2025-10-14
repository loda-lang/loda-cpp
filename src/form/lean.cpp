#include "form/lean.hpp"

#include <sstream>

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "seq/seq_util.hpp"
#include "sys/util.hpp"

bool convertToLean(Expression& expr, const Formula& f) {
  // Check children recursively
  for (auto& c : expr.children) {
    if (!convertToLean(c, f)) {
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
    case Expression::Type::FUNCTION: {
      if (expr.name == "min" || expr.name == "max") {
        break;
      }
      if (expr.name == "gcd") {
        if (expr.children.size() != 2) return false;
        Expression f(Expression::Type::FUNCTION, "Int.gcd", {expr.children[0], expr.children[1]});
        expr = f;
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

bool LeanFormula::convert(const Formula& formula, bool as_vector,
                          LeanFormula& lean_formula) {
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
  for (const auto& entry : formula.entries) {
    auto left = entry.first;
    auto right = entry.second;
    if (!convertToLean(right, formula)) {
      return false;
    }
    if (left.type != Expression::Type::FUNCTION || left.children.size() != 1) {
      return false;
    }
    auto& arg = left.children.front();
    if (arg.type != Expression::Type::PARAMETER) {
      return false;
    }
    lean_formula.main_formula.entries[left] = right;
  }
  return lean_formula.main_formula.entries.size() == 1;
}

std::string LeanFormula::toString() const {
  std::stringstream buf;
  std::string funcName;
  Expression mainExpr;
  for (const auto& entry : main_formula.entries) {
    funcName = entry.first.name;
    mainExpr = entry.second;
    break;
  }
  bool usesParameter = mainExpr.contains(Expression::Type::PARAMETER);
  std::string arg = usesParameter ? "n" : "_";
  buf << "def " << funcName << " (" << arg
      << " : Int) : Int := " << mainExpr.toString(true);
  return buf.str();
}

std::string LeanFormula::printEvalCode(int64_t offset, int64_t numTerms) const {
  std::stringstream out;
  out << toString() << std::endl;
  out << std::endl;
  out << "def main : IO Unit := do" << std::endl;
  out << "  let offset : Int := " << offset << std::endl;
  out << "  let num_terms : Nat := " << numTerms << std::endl;
  out << std::endl;
  out << "  let rec loop (count : Nat) (n : Int) : IO Unit := do" << std::endl;
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
  const std::string tmpFileId = std::to_string(Random::get().gen() % 1000);
  const std::string leanPath("lean-loda-" + tmpFileId + ".lean");
  const std::string leanResult("lean-result-" + tmpFileId + ".txt");
  std::vector<std::string> args = {"lean", "--run", leanPath};
  std::string evalCode = printEvalCode(offset, numTerms);
  return SequenceUtil::evalFormulaWithExternalTool(
      evalCode, getName(), leanPath, leanResult, args, timeoutSeconds, result);
}
