#include "form/formula_gen.hpp"

namespace {
constexpr int64_t FACTORIAL_SEQ_ID = 142;
}

#include <map>
#include <set>
#include <stdexcept>

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "form/variant.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/log.hpp"

FormulaGenerator::FormulaGenerator()
    : interpreter(settings), incEval(interpreter) {}

std::string FormulaGenerator::newName() {
  std::string name = "a" + std::to_string(freeNameIndex);
  freeNameIndex++;
  return name;
}

std::string FormulaGenerator::getCellName(int64_t cell) const {
  auto it = cellNames.find(cell);
  if (it == cellNames.end()) {
    throw std::runtime_error("no name registered for $" + std::to_string(cell));
  }
  return it->second;
}

std::string canonicalName(int64_t index) {
  if (index < 0) {
    throw std::runtime_error("negative index of memory cell");
  }
  constexpr char MAX_CHAR = 5;
  if (index > static_cast<int64_t>(MAX_CHAR)) {
    return std::string(1, 'a' + MAX_CHAR) +
           std::to_string(index - static_cast<int64_t>(MAX_CHAR));
  }
  return std::string(1, 'a' + static_cast<char>(index));
}

Expression FormulaGenerator::operandToExpression(Operand op) const {
  switch (op.type) {
    case Operand::Type::CONSTANT: {
      return Expression(Expression::Type::CONSTANT, "", op.value);
    }
    case Operand::Type::DIRECT: {
      return ExpressionUtil::newFunction(getCellName(op.value.asInt()));
    }
    case Operand::Type::INDIRECT: {
      throw std::runtime_error("indirect operation not supported");
    }
  }
  throw std::runtime_error("internal error");  // unreachable
}

Expression FormulaGenerator::divToFraction(
    const Expression& numerator, const Expression& denominator) const {
  Expression frac(Expression::Type::FRACTION, "", {numerator, denominator});
  std::string func = "floor";
  if (ExpressionUtil::canBeNegative(numerator, offset) ||
      ExpressionUtil::canBeNegative(denominator, offset)) {
    func = "truncate";
  }
  Expression wrapper(Expression::Type::FUNCTION, func, {frac});
  return wrapper;
}

bool FormulaGenerator::bitfunc(const std::string& name, const Expression& a,
                               const Expression& b, Expression& res) const {
  if (ExpressionUtil::canBeNegative(a, offset) ||
      ExpressionUtil::canBeNegative(b, offset)) {
    // TODO: remove this limitation
    return false;
  }
  res = Expression(Expression::Type::FUNCTION, name, {a, b});
  return true;
}

bool FormulaGenerator::update(const Operation& op) {
  auto source = operandToExpression(op.source);
  auto target = operandToExpression(op.target);
  if (source.type == Expression::Type::FUNCTION) {
    source = formula.entries[source];
  }
  auto prevTarget = formula.entries[target];
  auto& res = formula.entries[target];  // reference to result
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
      Expression negated(Expression::Type::PRODUCT, "",
                         {ExpressionUtil::newConstant(-1), source});
      res = Expression(Expression::Type::SUM, "", {prevTarget, negated});
      break;
    }
    case Operation::Type::MUL: {
      res = Expression(Expression::Type::PRODUCT, "", {prevTarget, source});
      break;
    }
    case Operation::Type::DIV: {
      res = divToFraction(prevTarget, source);
      break;
    }
    case Operation::Type::POW: {
      res = Expression(Expression::Type::POWER, "", {prevTarget, source});
      if (ExpressionUtil::canBeNegative(source, offset)) {
        Expression wrapper(Expression::Type::FUNCTION, "truncate", {res});
        res = wrapper;
      }
      break;
    }
    case Operation::Type::MOD: {
      auto c1 = prevTarget;
      auto c2 = source;
      if (ExpressionUtil::canBeNegative(c1, offset) ||
          ExpressionUtil::canBeNegative(c2, offset)) {
        Expression wrapper(Expression::Type::SUM);
        wrapper.newChild(c1);
        wrapper.newChild(Expression::Type::PRODUCT);
        auto frac = divToFraction(c1, c2);
        wrapper.children[1].newChild(
            Expression(Expression::Type::CONSTANT, "", Number(-1)));
        wrapper.children[1].newChild(c2);
        wrapper.children[1].newChild(frac);
        res = wrapper;
      } else {
        res = Expression(Expression::Type::MODULUS, "", {c1, c2});
      }
      break;
    }
    case Operation::Type::BIN: {
      res = Expression(Expression::Type::FUNCTION, "binomial",
                       {prevTarget, source});
      break;
    }
    case Operation::Type::LOG: {
      res = Expression(Expression::Type::FUNCTION, "logint",
                       {prevTarget, source});
      break;
    }
    case Operation::Type::NRT: {
      if (source.type == Expression::Type::CONSTANT &&
          source.value == Number(2)) {
        res = Expression(Expression::Type::FUNCTION, "sqrtint", {prevTarget});
      } else {
        res = Expression(Expression::Type::FUNCTION, "sqrtnint",
                         {prevTarget, source});
      }
      break;
    }
    case Operation::Type::GCD: {
      res = Expression(Expression::Type::FUNCTION, "gcd", {prevTarget, source});
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
    case Operation::Type::BAN: {
      okay = bitfunc("bitand", prevTarget, source, res);
      break;
    }
    case Operation::Type::BOR: {
      okay = bitfunc("bitor", prevTarget, source, res);
      break;
    }
    case Operation::Type::BXO: {
      okay = bitfunc("bitxor", prevTarget, source, res);
      break;
    }
    case Operation::Type::SEQ: {
      res = Expression(Expression::Type::FUNCTION,
                       ProgramUtil::idStr(source.value.asInt()), {prevTarget});
      break;
    }
    case Operation::Type::TRN: {
      Expression negated(Expression::Type::PRODUCT, "",
                         {ExpressionUtil::newConstant(-1), source});
      res = Expression(
          Expression::Type::FUNCTION, "max",
          {Expression(Expression::Type::SUM, "", {prevTarget, negated}),
           ExpressionUtil::newConstant(0)});
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
                     " updated formula to " + formula.toString());
  }
  return okay;
}

bool FormulaGenerator::update(const Program& p) {
  for (const auto& op : p.ops) {
    if (!update(op)) {
      return false;  // operation not supported
    }
  }
  return true;
}

int64_t getNumInitialTermsNeeded(int64_t cell, const std::string& fname,
                                 const Formula& formula,
                                 const IncrementalEvaluator& ie) {
  auto stateful = ie.getStatefulCells();
  for (const auto& out : ie.getOutputCells()) {
    if (ie.getInputDependentCells().find(out) ==
        ie.getInputDependentCells().end()) {
      stateful.insert(out);
    }
  }
  // stateful.erase(Program::OUTPUT_CELL);
  int64_t terms_needed = 0;
  if (stateful.find(cell) != stateful.end()) {
    terms_needed = (ie.getLoopCounterDecrement() * stateful.size());
  }
  return terms_needed;
}

void FormulaGenerator::initFormula(int64_t numCells, bool useIncEval) {
  formula.clear();
  for (int64_t cell = 0; cell < numCells; cell++) {
    auto key = operandToExpression(Operand(Operand::Type::DIRECT, cell));
    if (useIncEval) {
      if (cell == incEval.getSimpleLoop().counter) {
        formula.entries[key] = ExpressionUtil::newParameter();
      } else if (incEval.getInputDependentCells().find(cell) ==
                 incEval.getInputDependentCells().end()) {
        formula.entries[key] = key;
        Expression prev(
            Expression::Type::SUM, "",
            {ExpressionUtil::newParameter(),
             ExpressionUtil::newConstant(-incEval.getLoopCounterDecrement())});
        formula.entries[key].replaceAll(ExpressionUtil::newParameter(), prev);
      }
    } else {
      formula.entries[key] = (cell == Program::INPUT_CELL)
                                 ? ExpressionUtil::newParameter()
                                 : ExpressionUtil::newConstant(0);
    }
  }
}

bool FormulaGenerator::generateSingle(const Program& p) {
  // indirect operands are not supported
  if (ProgramUtil::hasIndirectOperand(p)) {
    return false;
  }
  const int64_t numCells = ProgramUtil::getLargestDirectMemoryCell(p) + 1;
  const bool useIncEval =
      incEval.init(p, true, true);  // skip input transformations and offset

  // initialize function names for memory cells
  cellNames.clear();
  for (int64_t cell = 0; cell < numCells; cell++) {
    cellNames[cell] = newName();
  }

  // initialize expressions for memory cells
  initFormula(numCells, false);
  std::map<int64_t, Expression> preloopExprs;
  if (useIncEval) {
    // TODO: remove this limitation
    if (incEval.getInputDependentCells().size() > 1 &&
        ProgramUtil::numOps(incEval.getSimpleLoop().body,
                            Operation::Type::MOV) > 0) {
      return false;
    }
    // TODO: remove this limitation
    if (incEval.getLoopCounterLowerBound()) {
      return false;
    }
    // update formula based on pre-loop code
    if (!update(incEval.getSimpleLoop().pre_loop)) {
      return false;
    }
    for (auto cell : incEval.getInputDependentCells()) {
      auto op = Operand(Operand::Type::DIRECT, Number(cell));
      auto param = operandToExpression(op);
      preloopExprs[cell] = formula.entries[param];
    }
    initFormula(numCells, true);
  }
  Log::get().debug("Initialized formula to " + formula.toString());

  // update formula based on main program / loop body
  Program main;
  if (useIncEval) {
    main = incEval.getSimpleLoop().body;
  } else {
    main = p;
  }
  if (!update(main)) {
    return false;
  }
  Log::get().debug("Updated formula:  " + formula.toString());

  // additional work for IE programs
  if (useIncEval) {
    // determine number of initial terms needed
    std::map<std::string, int64_t> numTerms;
    for (int64_t cell = 0; cell < numCells; cell++) {
      auto name = getCellName(cell);
      numTerms[name] = getNumInitialTermsNeeded(cell, name, formula, incEval);
    }

    // find and choose alternative function definitions
    simplifyFormulaUsingVariants(formula, numTerms);

    // evaluate program and add initial terms to formula
    const int64_t offset = ProgramUtil::getOffset(p);
    if (!addInitialTerms(numCells, offset, numTerms)) {
      return false;
    }

    // prepare post-loop processing
    prepareForPostLoop(numCells, offset, preloopExprs);
    Log::get().debug("Prepared post-loop: " + formula.toString());

    // handle post-loop code
    if (!update(incEval.getSimpleLoop().post_loop)) {
      return false;
    }
    Log::get().debug("Processed post-loop: " + formula.toString());
  }

  // resolve simple recursions
  FormulaUtil::resolveSimpleRecursions(formula);
  Log::get().debug("Resolved simple recursions: " + formula.toString());

  // resolve simple functions
  FormulaUtil::resolveSimpleFunctions(formula);
  Log::get().debug("Resolved simple functions: " + formula.toString());

  // extract main formula (filter out irrelant memory cells)
  Formula tmp;
  formula.collectEntries(getCellName(Program::OUTPUT_CELL), tmp);
  formula = tmp;
  Log::get().debug("Pruned formula: " + formula.toString());

  // resolve identities
  FormulaUtil::resolveIdentities(formula);
  Log::get().debug("Resolved identities: " + formula.toString());

  // success
  return true;
}

void FormulaGenerator::prepareForPostLoop(
    int64_t numCells, int64_t offset,
    const std::map<int64_t, Expression>& preloopExprs) {
  // prepare post-loop processing
  auto preloopCounter = preloopExprs.at(incEval.getSimpleLoop().counter);
  for (int64_t cell = 0; cell < numCells; cell++) {
    auto name = newName();
    auto left = ExpressionUtil::newFunction(name);
    Expression right;
    if (cell == incEval.getSimpleLoop().counter) {
      auto last = ExpressionUtil::newConstant(0);
      if (incEval.getLoopCounterDecrement() > 1) {
        auto loop_dec =
            ExpressionUtil::newConstant(incEval.getLoopCounterDecrement());
        last = Expression(Expression::Type::MODULUS, "",
                          {preloopCounter, loop_dec});
      }
      right =
          Expression(Expression::Type::FUNCTION, "min", {preloopCounter, last});
    } else if (incEval.getInputDependentCells().find(cell) !=
               incEval.getInputDependentCells().end()) {
      right = preloopExprs.at(cell);
    } else {
      auto safe_param = preloopCounter;
      if (ExpressionUtil::canBeNegative(safe_param, offset)) {
        auto tmp = safe_param;
        safe_param = Expression(Expression::Type::FUNCTION, "max",
                                {tmp, ExpressionUtil::newConstant(0)});
      }
      right = Expression(Expression::Type::FUNCTION, getCellName(cell),
                         {safe_param});
    }
    formula.entries[left] = right;
    cellNames[cell] = name;
  }
}

bool FormulaGenerator::addInitialTerms(
    int64_t numCells, int64_t offset,
    const std::map<std::string, int64_t>& numTerms) {
  // calculate maximum offset
  int64_t maxNumTerms = 0;
  for (auto it : numTerms) {
    Log::get().debug("Function " + it.first + "(n) requires " +
                     std::to_string(it.second) + " intial terms");
    maxNumTerms = std::max(maxNumTerms, it.second);
  }
  // evaluate program and add initial terms to formula
  for (int64_t n = 0; n < maxNumTerms; n++) {
    try {
      incEval.next(true, true);  // skip final iteration and post loop code
    } catch (const std::exception&) {
      Log::get().debug("Cannot generate initial terms");
      return false;
    }
    const auto state = incEval.getLoopStates().at(incEval.getPreviousSlice());
    for (int64_t cell = 0; cell < numCells; cell++) {
      auto name = getCellName(cell);
      if (n < numTerms.at(name)) {
        auto arg = n;
        if (cell == Program::INPUT_CELL) {
          arg += offset;
        }
        Expression func(Expression::Type::FUNCTION, name,
                        {ExpressionUtil::newConstant(arg)});
        Expression val(Expression::Type::CONSTANT, "", state.get(cell));
        formula.entries[func] = val;
        Log::get().debug("Added intial term: " + func.toString() + " = " +
                         val.toString());
      }
    }
  }
  return true;
}

void FormulaGenerator::simplifyFunctionNames() {
  std::set<std::string> names;
  for (auto& e : formula.entries) {
    auto& f = e.first;
    if (f.type == Expression::Type::FUNCTION && !f.name.empty() &&
        std::islower(f.name[0])) {
      names.insert(e.first.name);
    }
  }
  formula.replaceName(getCellName(0), canonicalName(0));
  size_t cell = 1;
  for (const auto& n : names) {
    if (n == getCellName(0)) {
      continue;
    }
    auto c = canonicalName(cell++);
    Log::get().debug("Renaming function " + n + " => " + c);
    formula.replaceName(n, c);
  }
}

bool addProgramIds(const Program& p, std::set<int64_t>& ids) {
  // TODO: check for recursion
  Parser parser;
  for (const auto& op : p.ops) {
    if (op.type == Operation::Type::SEQ) {
      auto id = op.source.value.asInt();
      if (ids.find(id) == ids.end()) {
        ids.insert(id);
        try {
          auto q = parser.parse(ProgramUtil::getProgramPath(id));
          addProgramIds(q, ids);
        } catch (const std::exception&) {
          return false;
        }
      }
    }
  }
  return true;
}

bool FormulaGenerator::generate(const Program& p, int64_t id, Formula& result,
                                bool withDeps) {
  if (id > 0) {
    Log::get().debug("Generating formula for " + ProgramUtil::idStr(id));
  }
  formula.clear();
  freeNameIndex = 0;
  offset = ProgramUtil::getOffset(p);
  if (!generateSingle(p)) {
    return false;
  }
  static const std::string mainName = "MAIN";  // must be upper case
  formula.replaceName(getCellName(0), mainName);
  result = formula;
  if (withDeps) {
    std::set<int64_t> ids;
    if (!addProgramIds(p, ids)) {
      return false;
    }
    Parser parser;
    for (auto id2 : ids) {
      if (id2 == FACTORIAL_SEQ_ID) {
        continue;  // Skip dependency for A000142 (factorial)
      }
      Log::get().debug("Adding dependency " + ProgramUtil::idStr(id2));
      Program p2;
      try {
        p2 = parser.parse(ProgramUtil::getProgramPath(id2));
      } catch (const std::exception&) {
        result.clear();
        return false;
      }
      if (!generateSingle(p2)) {
        result.clear();
        return false;
      }
      auto from = getCellName(Program::INPUT_CELL);
      auto to = ProgramUtil::idStr(id2);
      Log::get().debug("Replacing " + from + " by " + to);
      formula.replaceName(from, to);
      result.entries.insert(formula.entries.begin(), formula.entries.end());
    }
  }
  // rename helper functions
  formula = result;
  simplifyFunctionNames();
  formula.replaceName(mainName, canonicalName(0));
  result = formula;

  // replace functions A000142(n) by n! in all formula definitions
  const std::string factorialSeqName = ProgramUtil::idStr(FACTORIAL_SEQ_ID);
  for (auto& entry : result.entries) {
    entry.second.replaceType(Expression::Type::FUNCTION, factorialSeqName, 1,
                             Expression::Type::FACTORIAL);
  }

  return true;
}
