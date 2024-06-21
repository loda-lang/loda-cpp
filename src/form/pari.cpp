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
    if (ExpressionUtil::canBeNegative(expr.children.at(1))) {
      return false;
    }
  }
  auto functions = f.getDefinitions(Expression::Type::FUNCTION);
  if (expr.type == Expression::Type::FUNCTION && as_vector &&
      std::find(functions.begin(), functions.end(), expr.name) !=
          functions.end()) {
    expr.type = Expression::Type::VECTOR;
  }
  return true;
}

void countFuncs(const Formula& f, const Expression& e,
                std::map<Expression, size_t>& count) {
  auto functions = f.getDefinitions(Expression::Type::FUNCTION);
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

void PariFormula::extractInitialTerms() {
  initial_terms.clear();
  auto it = main_formula.entries.begin();
  while (it != main_formula.entries.end()) {
    if (ExpressionUtil::isInitialTerm(it->first)) {
      auto key = it->first;
      key.children.front().value += Number::ONE;
      initial_terms.entries[key] = it->second;
      max_initial_terms[key.name] = std::max(
          max_initial_terms[key.name], key.children.front().value.asInt());
      it = main_formula.entries.erase(it);
    } else {
      it++;
    }
  }
}

bool PariFormula::convert(const Formula& formula, bool as_vector,
                          PariFormula& pari_formula) {
  pari_formula = PariFormula();
  pari_formula.as_vector = as_vector;
  for (const auto& entry : formula.entries) {
    auto left = entry.first;
    auto right = entry.second;
    if (as_vector && left.type == Expression::Type::FUNCTION) {
      left.type = Expression::Type::VECTOR;
    }
    if (!convertExprToPari(right, formula, as_vector)) {
      return false;
    }
    pari_formula.main_formula.entries[left] = right;
  }
  if (as_vector) {
    pari_formula.extractInitialTerms();
  } else {
    addLocalVars(pari_formula.main_formula);
    FormulaUtil::convertInitialTermsToIf(pari_formula.main_formula);
  }
  return true;
}

std::string PariFormula::toString() const {
  if (as_vector) {
    return main_formula.toString("; ", false) + "; " +
           initial_terms.toString("; ", false);
  } else {
    return main_formula.toString("; ", true);
  }
}

void PariFormula::printEvalCode(int64_t numTerms, std::ostream& out) const {
  if (as_vector) {
    // declare vectors
    auto functions = main_formula.getDefinitions(Expression::Type::VECTOR);
    for (const auto& f : functions) {
      out << f << " = vector(" << numTerms << ")" << std::endl;
    }
    // initial terms only
    out << initial_terms.toString("\n", false) << std::endl;
  } else {
    // main function
    out << main_formula.toString("; ", true) << std::endl;
  }
  const int64_t start = as_vector ? 1 : 0;
  const int64_t end = numTerms + start - 1;
  out << "for(n=" << start << "," << end << ",";
  if (as_vector) {
    auto sorted = main_formula.getDefinitions(Expression::Type::VECTOR, true);
    for (const auto& f : sorted) {
      auto key = ExpressionUtil::newFunction(f);
      key.type = Expression::Type::VECTOR;
      if (max_initial_terms.find(f) != max_initial_terms.end()) {
        out << "if(n>" << max_initial_terms.at(f) << ", ";
        out << f << "[n] = " << main_formula.entries.at(key).toString()
            << "); ";
      } else {
        out << f << "[n] = " << main_formula.entries.at(key).toString() << "; ";
      }
    }
    out << "print(a[n])";
  } else {
    out << "print(a(n))";
  }
  out << ")" << std::endl << "quit" << std::endl;
}

Sequence PariFormula::eval(int64_t numTerms) const {
  const std::string gpPath("pari-loda.gp");
  const std::string gpResult("pari-result.txt");
  const int64_t maxparisize = 256;  // in MB
  std::ofstream gp(gpPath);
  if (!gp) {
    throw std::runtime_error("error generating gp file");
  }
  printEvalCode(numTerms, gp);
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
