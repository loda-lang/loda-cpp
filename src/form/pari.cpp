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

void convertFracToPari(Expression& frac) {
  std::string func = "floor";
  if (ExpressionUtil::canBeNegative(frac.children.at(0)) ||
      ExpressionUtil::canBeNegative(frac.children.at(1))) {
    func = "truncate";
  }
  Expression wrapper(Expression::Type::FUNCTION, func, {frac});
  frac = wrapper;
}

bool convertExprToPari(Expression& expr) {
  // convert bottom-up!
  for (auto& c : expr.children) {
    if (!convertExprToPari(c)) {
      return false;
    }
  }
  if (expr.type == Expression::Type::FRACTION) {
    convertFracToPari(expr);
  } else if (expr.type == Expression::Type::POWER) {
    if (ExpressionUtil::canBeNegative(expr.children.at(1))) {
      Expression wrapper(Expression::Type::FUNCTION, "truncate", {expr});
      expr = wrapper;
    }
  } else if (expr.type == Expression::Type::MODULUS) {
    auto c1 = expr.children.at(0);
    auto c2 = expr.children.at(1);
    if (ExpressionUtil::canBeNegative(c1) ||
        ExpressionUtil::canBeNegative(c2)) {
      Expression wrapper(Expression::Type::SUM);
      wrapper.newChild(c1);
      wrapper.newChild(Expression::Type::PRODUCT);
      Expression frac(Expression::Type::FRACTION, "", {c1, c2});
      convertFracToPari(frac);
      wrapper.children[1].newChild(
          Expression(Expression::Type::CONSTANT, "", Number(-1)));
      wrapper.children[1].newChild(c2);
      wrapper.children[1].newChild(frac);
      expr = wrapper;
    }
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

bool Pari::convertToPari(Formula& f) {
  for (auto& entry : f.entries) {
    if (!convertExprToPari(entry.second)) {
      return false;
    }
  }
  addLocalVars(f);
  convertInitialTermsToIf(f);
  return true;
}

std::string Pari::toString(const Formula& f) {
  std::string result;
  bool first = true;
  for (auto it = f.entries.rbegin(); it != f.entries.rend(); it++) {
    if (!first) {
      result += "; ";
    }
    if (f.entries.size() > 1) {
      result += "(";
    }
    result += it->first.toString() + " = " + it->second.toString();
    if (f.entries.size() > 1) {
      result += ")";
    }
    first = false;
  }
  return result;
}

Sequence Pari::eval(const Formula& f, int64_t start, int64_t end) {
  const std::string gpPath("pari-loda.gp");
  const std::string gpResult("pari-result.txt");
  const int64_t maxparisize = 256;  // in MB
  std::ofstream gp(gpPath);
  if (!gp) {
    throw std::runtime_error("error generating gp file");
  }
  gp << toString(f) << std::endl;
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
