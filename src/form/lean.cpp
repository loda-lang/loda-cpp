#include "form/lean.hpp"

#include <sstream>

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "seq/seq_util.hpp"
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
    if (offset <= 0) {
      return false;
    }
    lean_formula.domain = "Nat";
  } else {
    lean_formula.domain = "Int";
  }
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
    lean_formula.main_formula.entries[left] = right;
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

    // Generate pattern matching cases
    for (const auto& bc : baseCases) {
      const auto& constValue = bc.first.children.front().value;
      buf << " | " << constValue.to_string() << " => "
          << bc.second.toString(true);
    }

    // Add the recursive case
    if (!recursiveCase.name.empty()) {
      buf << " | n => " << recursiveRHS.toString(true);
    }
  }
}

std::string LeanFormula::printEvalCode(int64_t offset, int64_t numTerms) const {
  std::stringstream out;
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
  const std::string tmpFileId = std::to_string(Random::get().gen() % 1000);
  const std::string leanPath("lean-loda-" + tmpFileId + ".lean");
  const std::string leanResult("lean-result-" + tmpFileId + ".txt");
  std::vector<std::string> args = {"lean", "--run", leanPath};
  std::string evalCode = printEvalCode(offset, numTerms);
  return SequenceUtil::evalFormulaWithExternalTool(
      evalCode, getName(), leanPath, leanResult, args, timeoutSeconds, result);
}
