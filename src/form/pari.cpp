#include "form/pari.hpp"

#include <fstream>
#include <sstream>

#include "form/expression_util.hpp"
#include "sys/log.hpp"

void convertInitialTermsToIf(Formula& formula) {
  auto it = formula.entries.begin();
  while (it != formula.entries.end()) {
    auto left = it->first;
    Expression general(Expression::Type::FUNCTION, left.name,
                       {Expression(Expression::Type::PARAMETER, "n")});
    auto general_it = formula.entries.find(general);
    if (left.type == Expression::Type::FUNCTION && left.children.size() == 1 &&
        left.children.front().type == Expression::Type::CONSTANT &&
        general_it != formula.entries.end()) {
      general_it->second =
          Expression(Expression::Type::IF, "",
                     {left.children.front(), it->second, general_it->second});
      it = formula.entries.erase(it);
    } else {
      it++;
    }
  }
}

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
  if (expr.type == Expression::Type::FUNCTION && as_vector &&
      f.containsFunctionDef(expr.name)) {
    expr.type = Expression::Type::VECTOR;
  }
  return true;
}

void countFuncs(const Formula& f, const Expression& e,
                std::map<Expression, size_t>& count) {
  if (e.type == Expression::Type::FUNCTION && f.containsFunctionDef(e.name)) {
    if (count.find(e) == count.end()) {
      count[e] = 1;
    } else {
      count[e]++;
    }
  }
  for (auto& c : e.children) {
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
  addLocalVars(f);
  convertInitialTermsToIf(f);
  return true;
}

Sequence Pari::eval(const Formula& f, int64_t start, int64_t end) {
  const std::string gpPath("pari-loda.gp");
  const std::string gpResult("pari-result.txt");
  const int64_t maxparisize = 256;  // in MB
  std::ofstream gp(gpPath);
  if (!gp) {
    throw std::runtime_error("error generating gp file");
  }
  gp << f.toString("; ", true) << std::endl;
  gp << "for (n = " << start << ", " << end << ", print(a(n)))" << std::endl;
  gp << "quit" << std::endl;
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
