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

bool Pari::convertToPari(Formula& f, bool as_vector) {
  Formula tmp;
  for (auto& entry : f.entries) {
    auto left = entry.first;
    auto right = entry.second;
    if (as_vector && left.type == Expression::Type::FUNCTION) {
      left.type = Expression::Type::VECTOR;
    }
    if (!convertExprToPari(right, f, as_vector)) {
      return false;
    }
    tmp.entries[left] = right;
  }
  f = tmp;
  if (!as_vector) {
    addLocalVars(f);
    FormulaUtil::convertInitialTermsToIf(f);
  }
  return true;
}

void Pari::printEvalCode(const Formula& f, int64_t start, int64_t end,
                         std::ostream& out, bool as_vector) {
  if (as_vector) {
    // TODO: print initial terms
  } else {
    out << f.toString("; ", true) << std::endl;
  }
  out << "for (n = " << start << ", " << end << ", ";
  if (as_vector) {
    // TODO: print function definition
    out << "print(a[n])";
  } else {
    out << "print(a(n))";
  }
  out << ")" << std::endl << "quit" << std::endl;
}

Sequence Pari::eval(const Formula& f, int64_t start, int64_t end,
                    bool as_vector) {
  const std::string gpPath("pari-loda.gp");
  const std::string gpResult("pari-result.txt");
  const int64_t maxparisize = 256;  // in MB
  std::ofstream gp(gpPath);
  if (!gp) {
    throw std::runtime_error("error generating gp file");
  }
  printEvalCode(f, start, end, gp, as_vector);
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
