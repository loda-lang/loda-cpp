#include "form/pari.hpp"

#include <fstream>
#include <sstream>

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "seq/seq_util.hpp"
#include "sys/log.hpp"
#include "sys/process.hpp"
#include "sys/util.hpp"

bool convertExprToPari(Expression& expr, const Formula& f, bool as_vector) {
  // convert bottom-up!
  for (auto& c : expr.children) {
    if (!convertExprToPari(c, f, as_vector)) {
      return false;
    }
  }
  auto functions = FormulaUtil::getDefinitions(f, Expression::Type::FUNCTION);
  if (expr.type == Expression::Type::FUNCTION && as_vector &&
      std::find(functions.begin(), functions.end(), expr.name) !=
          functions.end()) {
    expr.type = Expression::Type::VECTOR;
  }
  return true;
}

void countFuncs(const Formula& f, const Expression& e,
                std::map<Expression, size_t>& count) {
  auto functions = FormulaUtil::getDefinitions(f, Expression::Type::FUNCTION);
  if (e.type == Expression::Type::FUNCTION &&
      std::find(functions.begin(), functions.end(), e.name) !=
          functions.end()) {
    if (count.find(e) == count.end()) {
      count[e] = 1;
    } else {
      count[e]++;
    }
  }
  for (const auto& c : e.children) {
    countFuncs(f, c, count);
  }
}

bool addLocalVars(Formula& f) {
  std::map<Expression, size_t> count;
  bool changed = false;
  for (auto& e : f.entries) {
    count.clear();
    countFuncs(f, e.second, count);
    size_t i = 1;
    for (auto& c : count) {
      if (c.second < 2) {
        continue;
      }
      std::string name = "l" + std::to_string(i++);
      auto r = e.second;  // copy
      r.replaceAll(c.first, Expression(Expression::Type::PARAMETER, name, {}));
      Expression local(Expression::Type::LOCAL, name, {c.first, r});
      Log::get().debug("Added local variable: " + name + " = " +
                       c.first.toString());
      e.second = local;
      changed = true;
    }
  }
  return changed;
}

void convertInitialTermsToIf(Formula& formula, const Expression::Type type) {
  auto it = formula.entries.begin();
  while (it != formula.entries.end()) {
    auto left = it->first;
    auto general = ExpressionUtil::newFunction(left.name);
    general.type = type;
    auto general_it = formula.entries.find(general);
    if (ExpressionUtil::isInitialTerm(left) &&
        general_it != formula.entries.end()) {
      auto index_expr = left.children.front();
      general_it->second =
          Expression(Expression::Type::IF, "",
                     {index_expr, it->second, general_it->second});
      it = formula.entries.erase(it);
    } else {
      it++;
    }
  }
}

bool PariFormula::convert(const Formula& formula, int64_t offset,
                          bool as_vector, PariFormula& pari_formula) {
  pari_formula = {};
  pari_formula.as_vector = as_vector;
  auto defs = FormulaUtil::getDefinitions(formula, Expression::Type::FUNCTION);
  for (const auto& entry : formula.entries) {
    auto left = entry.first;
    auto right = entry.second;
    if (as_vector && left.type == Expression::Type::FUNCTION) {
      left.type = Expression::Type::VECTOR;
    }
    // TODO: remove this limitation
    if (as_vector &&
        ExpressionUtil::hasNonRecursiveFunctionReference(right, defs, 0)) {
      return false;
    }
    if (!convertExprToPari(right, formula, as_vector)) {
      return false;
    }
    pari_formula.main_formula.entries[left] = right;
  }
  if (as_vector) {
    convertInitialTermsToIf(pari_formula.main_formula,
                            Expression::Type::VECTOR);
  } else {
    addLocalVars(pari_formula.main_formula);
    convertInitialTermsToIf(pari_formula.main_formula,
                            Expression::Type::FUNCTION);
  }
  return true;
}

std::string PariFormula::toString() const {
  if (as_vector) {
    std::stringstream buf;
    auto sorted = FormulaUtil::getDefinitions(main_formula,
                                              Expression::Type::VECTOR, true);
    for (size_t i = 0; i < sorted.size(); i++) {
      const auto& f = sorted.at(i);
      auto key = ExpressionUtil::newFunction(f);
      key.type = Expression::Type::VECTOR;
      if (i > 0) {
        buf << "; ";
      }
      auto expr = main_formula.entries.at(key);
      expr.replaceInside(ExpressionUtil::newParameter(),
                         Expression(Expression::Type::SUM, "",
                                    {ExpressionUtil::newParameter(),
                                     ExpressionUtil::newConstant(1)}),
                         Expression::Type::VECTOR);
      ExpressionUtil::normalize(expr);
      buf << f << "[n+1] = " << expr.toString();
    }
    return buf.str();
  } else {
    return main_formula.toString("; ", true);
  }
}

std::string PariFormula::printEvalCode(int64_t offset, int64_t numTerms) const {
  std::stringstream out;

  if (as_vector) {
    // declare vectors
    auto functions =
        FormulaUtil::getDefinitions(main_formula, Expression::Type::VECTOR);
    for (const auto& f : functions) {
      out << f << " = vector(" << numTerms << ")" << std::endl;
    }
  } else {
    // main function
    out << toString() << std::endl;
  }
  const int64_t end = offset + numTerms - 1;
  out << "for(n=" << offset << "," << end << ",";
  if (as_vector) {
    out << toString() << "; ";
    out << "print(a[n+1])";
  } else {
    out << "print(a(n))";
  }
  out << ")" << std::endl << "quit" << std::endl;

  return out.str();
}

bool PariFormula::eval(int64_t offset, int64_t numTerms, int timeoutSeconds,
                       Sequence& result) const {
  const std::string tmpFileId = std::to_string(Random::get().gen() % 1000);
  const std::string gpPath("pari-loda-" + tmpFileId + ".gp");
  const std::string gpResult("pari-result-" + tmpFileId + ".txt");
  const int64_t maxparisize = 256;  // in MB
  std::vector<std::string> args = {
      "gp", "-s", std::to_string(maxparisize) + "M", "-q", gpPath};
  std::string evalCode = printEvalCode(offset, numTerms);
  return SequenceUtil::evalFormulaWithExternalTool(
      evalCode, getName(), gpPath, gpResult, args, timeoutSeconds, result);
}
