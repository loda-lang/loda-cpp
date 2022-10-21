#include "formula_gen.hpp"

#include <stdexcept>

#include "evaluator_inc.hpp"
#include "expression_util.hpp"
#include "log.hpp"
#include "program_util.hpp"

std::string memoryCellToName(Number index) {
  int64_t i = index.asInt();
  if (i < 0) {
    throw std::runtime_error("negative index of memory cell");
  }
  constexpr char MAX_CHAR = 5;
  if (i > static_cast<int64_t>(MAX_CHAR)) {
    return std::string(1, 'a' + MAX_CHAR) +
           std::to_string(i - static_cast<int64_t>(MAX_CHAR));
  }
  return std::string(1, 'a' + static_cast<char>(i));
}

Expression operandToExpression(Operand op) {
  switch (op.type) {
    case Operand::Type::CONSTANT: {
      return Expression(Expression::Type::CONSTANT, "", op.value);
    }
    case Operand::Type::DIRECT: {
      return Expression(Expression::Type::FUNCTION, memoryCellToName(op.value),
                        {Expression(Expression::Type::PARAMETER, "n")});
    }
    case Operand::Type::INDIRECT: {
      throw std::runtime_error("indirect operation not supported");
    }
  }
  throw std::runtime_error("internal error");  // unreachable
}

Expression fraction(const Expression& num, const Expression& den,
                    bool pariMode) {
  auto frac = Expression(Expression::Type::FRACTION, "", {num, den});
  if (pariMode) {
    std::string func = "floor";
    if (ExpressionUtil::canBeNegative(num) ||
        ExpressionUtil::canBeNegative(den)) {
      func = "truncate";
    }
    return Expression(Expression::Type::FUNCTION, func, {frac});
  }
  return frac;
}

bool update(Formula& f, const Operation& op, bool pariMode) {
  auto source = operandToExpression(op.source);
  auto target = operandToExpression(op.target);
  if (source.type == Expression::Type::FUNCTION) {
    source = f.entries[source];
  }
  auto prevTarget = f.entries[target];
  auto& res = f.entries[target];  // reference to result
  bool okay = true;
  switch (op.type) {
    case Operation::Type::NOP: {
      break;
    }
    case Operation::Type::MOV: {
      res = source;
      break;
    }
    case Operation::Type::ADD: {
      res = Expression(Expression::Type::SUM, "", {prevTarget, source});
      break;
    }
    case Operation::Type::SUB: {
      res = Expression(Expression::Type::DIFFERENCE, "", {prevTarget, source});
      break;
    }
    case Operation::Type::MUL: {
      res = Expression(Expression::Type::PRODUCT, "", {prevTarget, source});
      break;
    }
    case Operation::Type::DIV: {
      res = fraction(prevTarget, source, pariMode);
      break;
    }
    case Operation::Type::POW: {
      auto pow = Expression(Expression::Type::POWER, "", {prevTarget, source});
      if (pariMode && ExpressionUtil::canBeNegative(source)) {
        res = Expression(Expression::Type::FUNCTION, "truncate", {pow});
      } else {
        res = pow;
      }
      break;
    }
    case Operation::Type::MOD: {
      if (pariMode && (ExpressionUtil::canBeNegative(prevTarget) ||
                       ExpressionUtil::canBeNegative(source))) {
        res = Expression(Expression::Type::DIFFERENCE);
        res.newChild(prevTarget);
        res.newChild(Expression::Type::PRODUCT);
        res.children[1]->newChild(source);
        res.children[1]->newChild(fraction(prevTarget, source, pariMode));
      } else {
        res = Expression(Expression::Type::MODULUS, "", {prevTarget, source});
      }
      break;
    }
    case Operation::Type::BIN: {
      res = Expression(Expression::Type::FUNCTION, "binomial",
                       {prevTarget, source});
      break;
    }
    case Operation::Type::MIN: {
      res = Expression(Expression::Type::FUNCTION, "min", {prevTarget, source});
      break;
    }
    case Operation::Type::MAX: {
      res = Expression(Expression::Type::FUNCTION, "max", {prevTarget, source});
      break;
    }
    default: {
      okay = false;
      break;
    }
  }
  if (okay) {
    ExpressionUtil::normalize(res);
    Log::get().debug("Operation " + ProgramUtil::operationToString(op) +
                     " updated formula to " + f.toString(false));
  }
  return okay;
}

bool update(Formula& f, const Program& p, bool pariMode) {
  for (auto& op : p.ops) {
    if (!update(f, op, pariMode)) {
      return false;  // operation not supported
    }
  }
  return true;
}

void resolve(Formula& f, const Expression& left, Expression& right) {
  if (right.type == Expression::Type::FUNCTION) {
    Expression lookup(Expression::Type::FUNCTION, right.name,
                      {Expression(Expression::Type::PARAMETER, "n")});
    if (lookup != left) {
      auto it = f.entries.find(lookup);
      if (it != f.entries.end()) {
        auto replacement = it->second;
        replacement.replaceAll(Expression(Expression::Type::PARAMETER, "n"),
                               *right.children[0]);
        ExpressionUtil::normalize(replacement);
        Log::get().debug("Replacing " + right.toString() + " by " +
                         replacement.toString());
        right = replacement;
        return;  // must stop here
      }
    }
  }
  for (auto c : right.children) {
    resolve(f, left, *c);
  }
}

std::pair<bool, Formula> FormulaGenerator::generate(const Program& p,
                                                    bool pariMode) {
  std::pair<bool, Formula> result;
  result.first = false;
  Formula f, tmp;

  // indirect operands are not supported
  if (ProgramUtil::hasIndirectOperand(p)) {
    return result;
  }
  const int64_t largestCell = ProgramUtil::getLargestDirectMemoryCell(p);

  Settings settings;
  Interpreter interpreter(settings);
  IncrementalEvaluator ie(interpreter);
  bool use_ie = ie.init(p);

  if (use_ie) {
    // TODO: remove this limitation
    if (ie.getLoopCounterCell() != 0) {
      return result;
    }
    // TODO: remove this limitation
    if (!ie.getLoopCounterDependentCells().empty()) {
      return result;
    }
    // TODO: remove this limitation
    for (auto& op : ie.getPreLoop().ops) {
      if (op.type == Operation::Type::MUL || op.type == Operation::Type::DIV) {
        return result;
      }
    }
    // TODO: remove this limitation
    for (auto& op : ie.getLoopBody().ops) {
      if (op.type == Operation::Type::MOV &&
          op.source.type == Operand::Type::CONSTANT) {
        return result;
      }
    }
  }

  // initialize expressions for memory cells
  const Expression paramExpr(Expression::Type::PARAMETER, "n");
  for (int64_t i = 0; i <= largestCell; i++) {
    auto key = operandToExpression(Operand(Operand::Type::DIRECT, i));
    if (use_ie) {
      f.entries[key] = key;
      Expression prev(
          Expression::Type::DIFFERENCE, "",
          {paramExpr, Expression(Expression::Type::CONSTANT, "", Number::ONE)});
      f.entries[key].replaceAll(paramExpr, prev);
    } else {
      if (i == 0) {
        f.entries[key] = paramExpr;
      } else {
        f.entries[key] =
            Expression(Expression::Type::CONSTANT, "", Number::ZERO);
      }
    }
  }
  Log::get().debug("Initialized formula to " + f.toString(false));

  // update formula based on main program / loop body
  Program main;
  if (use_ie) {
    main = ie.getLoopBody();
  } else {
    main = p;
  }
  if (!update(f, main, pariMode)) {
    return result;
  }
  Log::get().debug("Updated formula:  " + f.toString(false));

  // additional work for IE programs
  std::map<std::string, std::string> identities;
  if (use_ie) {
    // resolve function references
    for (auto& e : f.entries) {
      resolve(f, e.first, e.second);
      ExpressionUtil::normalize(e.second);
    }
    Log::get().debug("Resolved formula: " + f.toString(false));

    // handle post-loop code
    auto post = ie.getPostLoop();
    for (auto& op : post.ops) {
      if (op.type == Operation::Type::MOV &&
          op.source.type == Operand::Type::DIRECT) {
        auto t = operandToExpression(op.target);
        auto s = operandToExpression(op.source);
        identities[t.name] = s.name;
        f.entries[t] = s;
      } else {
        // TODO: avoid this limitation
        return result;
      }
    }
    Log::get().debug("Updated formula:  " + f.toString(false));
  }

  // extract main formula (filter out irrelant memory cells)
  tmp.clear();
  f.collectEntries(memoryCellToName(0), tmp);
  f = tmp;
  Log::get().debug("Filtered formula: " + f.toString(false));

  // add initial terms for IE programs
  if (use_ie) {
    // calculate offset
    Memory mem;
    interpreter.run(ie.getPreLoop(), mem);
    int64_t preloopOffset =
        std::max<int64_t>(0, -(mem.get(ie.getLoopCounterCell()).asInt()));
    Log::get().debug("Calculated offset from pre-loop: " +
                     std::to_string(preloopOffset));

    // find out which initial terms are needed
    std::map<int64_t, int64_t> maxOffsets;
    // maximum offsets are bound by number of used memory cells
    for (int64_t offset = 0; offset <= largestCell; offset++) {
      for (int64_t cell = 0; cell <= largestCell; cell++) {
        Expression funcSearch(Expression::Type::FUNCTION,
                              memoryCellToName(cell));
        if (offset == 0) {
          funcSearch.newChild(paramExpr);
        } else {
          funcSearch.newChild(Expression(
              Expression::Type::DIFFERENCE, "",
              {paramExpr,
               Expression(Expression::Type::CONSTANT, "", offset + 1)}));
        }
        if (f.contains(funcSearch)) {
          maxOffsets[cell] =
              std::max<int64_t>(maxOffsets[cell], offset) + preloopOffset;
        }
      }
    }
    for (auto e : maxOffsets) {
      Log::get().debug(
          "Calculated offset from formula: " + memoryCellToName(e.first) +
          " => " + std::to_string(e.second));
    }
    // evaluate program and add initial terms to formula
    for (int64_t offset = 0; offset <= largestCell; offset++) {
      ie.next();
      for (int64_t cell = 0; cell <= largestCell; cell++) {
        if (maxOffsets[cell] < offset) {
          continue;
        }
        Expression index(Expression::Type::CONSTANT, "", Number(offset));
        Expression func(Expression::Type::FUNCTION, memoryCellToName(cell),
                        {index});
        Expression val(Expression::Type::CONSTANT, "",
                       ie.getLoopState().get(cell));
        f.entries[func] = val;
      }
    }
    Log::get().debug("Added intial terms: " + f.toString(false));

    // simplify identical functions
    for (auto& e : identities) {
      f.replaceName(e.second, e.first);
    }

    // need to filter again
    tmp.clear();
    f.collectEntries(memoryCellToName(0), tmp);
    f = tmp;
    Log::get().debug("Filtered formula: " + f.toString(false));
  }

  // TODO: avoid this limitation
  auto mainCell = memoryCellToName(0);
  for (auto& e : f.entries) {
    if (e.first.name != mainCell) {
      return result;
    }
  }

  // pari: convert initial terms to "if"
  if (pariMode) {
    auto it = f.entries.begin();
    while (it != f.entries.end()) {
      Expression left = it->first;
      Expression general(Expression::Type::FUNCTION, left.name, {paramExpr});
      auto general_it = f.entries.find(general);
      if (left.type == Expression::Type::FUNCTION &&
          left.children.size() == 1 &&
          left.children.front()->type == Expression::Type::CONSTANT &&
          general_it != f.entries.end()) {
        general_it->second = Expression(
            Expression::Type::IF, "",
            {*left.children.front(), it->second, general_it->second});
        it = f.entries.erase(it);
      } else {
        it++;
      }
    }
  }

  // success
  result.first = true;
  result.second = f;
  return result;
}
