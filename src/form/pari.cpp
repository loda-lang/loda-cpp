#include "form/pari.hpp"

#include <fstream>
#include <sstream>

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "sys/log.hpp"

bool convertExprToPari(Expression& expr, const Formula& f, bool as_vector) {
  // convert bottom-up!
  for (auto& c : expr.children) {
    if (!convertExprToPari(c, f, as_vector)) {
      return false;
    }
  }
  if (expr.type == Expression::Type::FUNCTION && expr.name == "binomial") {
    // TODO: check feedback from PARI team to avoid this limitation
    // note also that we should use the proper offset here
    if (ExpressionUtil::canBeNegative(expr.children.at(1), 0)) {
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

bool PariFormula::convert(const Formula& formula, bool as_vector,
                          PariFormula& pari_formula) {
  pari_formula = PariFormula();
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
    FormulaUtil::convertInitialTermsToIf(pari_formula.main_formula,
                                         Expression::Type::VECTOR);
  } else {
    addLocalVars(pari_formula.main_formula);
    FormulaUtil::convertInitialTermsToIf(pari_formula.main_formula,
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

void PariFormula::printEvalCode(int64_t offset, int64_t numTerms,
                                std::ostream& out) const {
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
}

Sequence PariFormula::eval(int64_t offset, int64_t numTerms) const {
  const std::string gpPath("pari-loda.gp");
  const std::string gpResult("pari-result.txt");
  const int64_t maxparisize = 256;  // in MB
  std::ofstream gp(gpPath);
  if (!gp) {
    throw std::runtime_error("error generating gp file");
  }
  printEvalCode(offset, numTerms, gp);
  gp.close();
  std::string cmd = "gp -s " + std::to_string(maxparisize) + "M -q " + gpPath +
                    " > " + gpResult;
  if (system(cmd.c_str()) != 0) {
    Log::get().error("Error evaluating PARI code: " + gpPath, true);
  }

  // read result from file
  Sequence seq;
  std::ifstream resultIn(gpResult);
  std::string buf;
  if (!resultIn) {
    Log::get().error("Error reading PARI output", true);
  }
  while (std::getline(resultIn, buf)) {
    seq.push_back(Number(buf));
  }

  // clean up
  std::remove(gpPath.c_str());
  std::remove(gpResult.c_str());

  return seq;
}
