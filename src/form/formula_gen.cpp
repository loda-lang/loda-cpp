#include "form/formula_gen.hpp"

#include <set>
#include <stdexcept>

#include "form/expression_util.hpp"
#include "lang/evaluator_inc.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/log.hpp"

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

Expression getParamExpr() {
  return Expression(Expression::Type::PARAMETER, "n");
}

Expression getFuncExpr(const std::string& name) {
  return Expression(Expression::Type::FUNCTION, name, {getParamExpr()});
}

Expression FormulaGenerator::operandToExpression(Operand op) const {
  switch (op.type) {
    case Operand::Type::CONSTANT: {
      return Expression(Expression::Type::CONSTANT, "", op.value);
    }
    case Operand::Type::DIRECT: {
      return getFuncExpr(getCellName(op.value.asInt()));
    }
    case Operand::Type::INDIRECT: {
      throw std::runtime_error("indirect operation not supported");
    }
  }
  throw std::runtime_error("internal error");  // unreachable
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
      Expression minus_one(Expression::Type::CONSTANT, "", Number(-1));
      Expression negated(Expression::Type::PRODUCT, "", {minus_one, source});
      res = Expression(Expression::Type::SUM, "", {prevTarget, negated});
      break;
    }
    case Operation::Type::MUL: {
      res = Expression(Expression::Type::PRODUCT, "", {prevTarget, source});
      break;
    }
    case Operation::Type::DIV: {
      res = Expression(Expression::Type::FRACTION, "", {prevTarget, source});
      break;
    }
    case Operation::Type::POW: {
      res = Expression(Expression::Type::POWER, "", {prevTarget, source});
      break;
    }
    case Operation::Type::MOD: {
      res = Expression(Expression::Type::MODULUS, "", {prevTarget, source});
      break;
    }
    case Operation::Type::BIN: {
      res = Expression(Expression::Type::FUNCTION, "binomial",
                       {prevTarget, source});
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
    case Operation::Type::SEQ: {
      res =
          Expression(Expression::Type::FUNCTION,
                     OeisSequence(source.value.asInt()).id_str(), {prevTarget});
      break;
    }
    case Operation::Type::TRN: {
      Expression minus_one(Expression::Type::CONSTANT, "", Number(-1));
      Expression negated(Expression::Type::PRODUCT, "", {minus_one, source});
      res = Expression(
          Expression::Type::FUNCTION, "max",
          {Expression(Expression::Type::SUM, "", {prevTarget, negated}),
           Expression(Expression::Type::CONSTANT, "", Number::ZERO)});
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
  for (auto& op : p.ops) {
    if (!update(op)) {
      return false;  // operation not supported
    }
  }
  return true;
}

bool FormulaGenerator::resolve(const Alternatives& alt, const Expression& left,
                               Expression& right) const {
  if (right.type == Expression::Type::FUNCTION) {
    auto lookup = getFuncExpr(right.name);
    if (lookup != left) {
      auto range = alt.equal_range(lookup);
      for (auto it = range.first; it != range.second; it++) {
        auto replacement = it->second;
        replacement.replaceAll(getParamExpr(), *right.children[0]);
        ExpressionUtil::normalize(replacement);
        auto range2 = alt.equal_range(left);
        bool exists = false;
        for (auto it2 = range2.first; it2 != range2.second; it2++) {
          if (it2->second == replacement) {
            exists = true;
            break;
          }
        }
        if (!exists) {
          right = replacement;
          return true;  // must stop here
        }
      }
    }
  }
  bool resolved = false;
  for (auto c : right.children) {
    if (resolve(alt, left, *c)) {
      resolved = true;
    }
  }
  ExpressionUtil::normalize(right);
  return resolved;
}

int64_t getNumInitialTermsNeeded(int64_t cell, const std::string& funcName,
                                 const Formula& f,
                                 const IncrementalEvaluator& ie,
                                 Interpreter& interpreter) {
  Memory mem;
  interpreter.run(ie.getPreLoop(), mem);
  int64_t loopCounterOffset =
      std::max<int64_t>(0, -(mem.get(ie.getLoopCounterCell()).asInt()));
  auto stateful = ie.getStatefulCells();
  stateful.insert(ie.getOutputCells().begin(), ie.getOutputCells().end());
  // stateful.erase(Program::OUTPUT_CELL);
  if (stateful.find(cell) != stateful.end()) {
    return loopCounterOffset + stateful.size();
  }
  return 0;
}

void FormulaGenerator::initFormula(int64_t numCells, bool use_ie) {
  formula.clear();
  const auto paramExpr = getParamExpr();
  for (int64_t cell = 0; cell < numCells; cell++) {
    auto key = operandToExpression(Operand(Operand::Type::DIRECT, cell));
    if (cell == 0) {
      formula.entries[key] = paramExpr;
    } else {
      if (use_ie) {
        formula.entries[key] = key;
        Expression prev(Expression::Type::SUM, "",
                        {paramExpr, Expression(Expression::Type::CONSTANT, "",
                                               Number(-1))});
        formula.entries[key].replaceAll(paramExpr, prev);
      } else {
        formula.entries[key] =
            Expression(Expression::Type::CONSTANT, "", Number::ZERO);
      }
    }
  }
}

bool FormulaGenerator::findAlternatives(Alternatives& alt) const {
  auto newAlt = alt;  // copy
  bool found = false;
  for (auto& e : alt) {
    auto right = e.second;  // copy
    if (resolve(newAlt, e.first, right)) {
      std::pair<Expression, Expression> p(e.first, right);
      Log::get().debug("Found alternative " + p.first.toString() + " = " +
                       p.second.toString());
      newAlt.insert(p);
      found = true;
    }
  }
  if (found) {
    alt = newAlt;
  }
  return found;
}

bool FormulaGenerator::applyAlternatives(const Alternatives& alt,
                                         Formula& f) const {
  bool applied = false;
  for (auto& e : f.entries) {
    auto range = alt.equal_range(e.first);
    for (auto it = range.first; it != range.second; it++) {
      if (it->second == e.second) {
        continue;
      }
      Formula g = f;  // copy
      g.entries[e.first] = it->second;
      auto depsOld = f.getFunctionDeps(true, true);
      auto depsNew = g.getFunctionDeps(true, true);
      std::string debugMsg =
          " alternative " + e.first.toString() + " = " + it->second.toString();
      if (depsNew.size() < depsOld.size()) {
        e.second = it->second;
        applied = true;
        Log::get().debug("Applied" + debugMsg);
      } else {
        Log::get().debug("Skipped" + debugMsg);
      }
    }
  }
  return applied;
}

bool FormulaGenerator::generateSingle(const Program& p) {
  // indirect operands are not supported
  if (ProgramUtil::hasIndirectOperand(p)) {
    return false;
  }
  const int64_t numCells = ProgramUtil::getLargestDirectMemoryCell(p) + 1;

  Settings settings;
  Interpreter interpreter(settings);
  IncrementalEvaluator ie(interpreter);
  bool use_ie = ie.init(p);

  if (use_ie) {
    // TODO: remove this limitation
    if (ie.getLoopCounterCell() != 0) {
      return false;
    }
    // TODO: remove this limitation
    if (ie.getLoopCounterDecrement() != 1) {
      return false;
    }
    // TODO: remove this limitation
    for (const auto& op : ie.getPreLoop().ops) {
      if (op.type == Operation::Type::MUL || op.type == Operation::Type::DIV ||
          op.type == Operation::Type::POW || op.type == Operation::Type::TRN) {
        return false;
      }
    }
  }

  // initialize function names for memory cells
  cellNames.clear();
  for (int64_t cell = 0; cell < numCells; cell++) {
    cellNames[cell] = newName();
  }

  // initialize expressions for memory cells
  initFormula(numCells, false);
  if (use_ie) {
    // update formula based on pre-loop code
    if (!update(ie.getPreLoop())) {
      return false;
    }
    auto param =
        operandToExpression(Operand(Operand::Type::DIRECT, Number::ZERO));
    auto saved = formula.entries[param];
    initFormula(numCells, true);
    formula.entries[param] = saved;
  }
  Log::get().debug("Initialized formula to " + formula.toString());

  // update formula based on main program / loop body
  Program main;
  if (use_ie) {
    main = ie.getLoopBody();
  } else {
    main = p;
  }
  if (!update(main)) {
    return false;
  }
  Log::get().debug("Updated formula:  " + formula.toString());

  // additional work for IE programs
  if (use_ie) {
    // find and choose alternative function definitions
    Alternatives alt;
    alt.insert(formula.entries.begin(), formula.entries.end());
    while (true) {
      if (!findAlternatives(alt)) {
        break;
      }
      if (!applyAlternatives(alt, formula)) {
        break;
      }
      Log::get().debug("Updated formula: " + formula.toString());
    };

    // determine number of initial terms needed
    std::vector<int64_t> numTerms(numCells);
    int64_t maxNumTerms = 0;
    for (int64_t cell = 0; cell < numCells; cell++) {
      numTerms[cell] = getNumInitialTermsNeeded(cell, getCellName(cell),
                                                formula, ie, interpreter);
      maxNumTerms = std::max(maxNumTerms, numTerms[cell]);
    }

    // evaluate program and add initial terms to formula
    for (int64_t offset = 0; offset < maxNumTerms; offset++) {
      ie.next();
      const auto state = ie.getLoopStates().front();
      for (int64_t cell = 0; cell < numCells; cell++) {
        if (offset < numTerms[cell]) {
          Expression index(Expression::Type::CONSTANT, "", Number(offset));
          Expression func(Expression::Type::FUNCTION, getCellName(cell),
                          {index});
          Expression val(Expression::Type::CONSTANT, "", state.get(cell));
          formula.entries[func] = val;
          Log::get().debug("Added intial term: " + func.toString() + " = " +
                           val.toString());
        }
      }
    }

    // prepare post-loop processing
    for (int64_t cell = 0; cell < numCells; cell++) {
      auto name = newName();
      auto left = getFuncExpr(name);
      auto right = getFuncExpr(getCellName(cell));
      if (cell == ie.getLoopCounterCell()) {
        auto tmp = right;
        right = Expression(
            Expression::Type::FUNCTION, "min",
            {right, Expression(Expression::Type::CONSTANT, "", Number::ZERO)});
      }
      formula.entries[left] = right;
      cellNames[cell] = name;
    }
    Log::get().debug("Prepared post-loop: " + formula.toString());

    // handle post-loop code
    auto post = ie.getPostLoop();
    if (!update(post)) {
      return false;
    }
    Log::get().debug("Processed post-loop: " + formula.toString());
  }

  // resolve linear functions
  formula.resolveSimpleRecursions();
  Log::get().debug("Resolved simple recursions: " + formula.toString());

  // resolve linear functions
  formula.resolveSimpleFunctions();
  Log::get().debug("Resolved simple functions: " + formula.toString());

  // extract main formula (filter out irrelant memory cells)
  Formula tmp;
  formula.collectEntries(getCellName(Program::OUTPUT_CELL), tmp);
  formula = tmp;
  Log::get().debug("Pruned formula: " + formula.toString());

  // resolve identities
  formula.resolveIdentities();
  Log::get().debug("Resolved identities: " + formula.toString());

  // success
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
  for (auto& n : names) {
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
  for (auto op : p.ops) {
    if (op.type == Operation::Type::SEQ) {
      auto id = op.source.value.asInt();
      if (ids.find(id) == ids.end()) {
        ids.insert(id);
        OeisSequence seq(id);
        try {
          auto q = parser.parse(seq.getProgramPath());
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
  formula.clear();
  freeNameIndex = 0;
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
      OeisSequence seq(id2);
      Log::get().debug("Adding dependency " + seq.id_str());
      Program p2;
      try {
        p2 = parser.parse(seq.getProgramPath());
      } catch (const std::exception&) {
        result.clear();
        return false;
      }
      if (!generateSingle(p2)) {
        result.clear();
        return false;
      }
      auto from = getCellName(Program::INPUT_CELL);
      auto to = seq.id_str();
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
  return true;
}
