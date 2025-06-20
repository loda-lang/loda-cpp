#include "form/range_generator.hpp"

#include <set>
#include <unordered_set>

#include "eval/semantics.hpp"
#include "lang/program_util.hpp"
#include "sys/log.hpp"

bool RangeGenerator::init(const Program& program, RangeMap& ranges) {
  ProgramUtil::validate(program);
  if (ProgramUtil::hasIndirectOperand(program)) {
    return false;
  }
  std::unordered_set<int64_t> used_cells;
  int64_t largest_used = 0;
  if (!ProgramUtil::getUsedMemoryCells(program, used_cells, largest_used, -1)) {
    return false;
  }
  ranges.clear();
  loop_states = {};
  int64_t offset = ProgramUtil::getOffset(program);
  for (auto cell : used_cells) {
    if (cell == Program::INPUT_CELL) {
      ranges[cell] = Range(Number(offset), Number::INF);
    } else {
      ranges[cell] = Range(Number::ZERO, Number::ZERO);
    }
  }
  return true;
}

bool RangeGenerator::generate(const Program& program, RangeMap& ranges) {
  if (!init(program, ranges)) {
    return false;
  }
  for (auto& op : program.ops) {
    if (!update(op, ranges)) {
      return false;
    }
  }
  ranges.prune();
  return true;
}

void RangeGenerator::generate(Program& program, RangeMap& ranges,
                              bool annotate) {
  if (!init(program, ranges)) {
    return;
  }
  for (auto& op : program.ops) {
    auto targetCell = getTargetCell(op);
    if (!update(op, ranges)) {
      return;
    }
    if (op.type != Operation::Type::NOP && annotate) {
      op.comment = ranges.toString(targetCell);
    }
  }
  ranges.prune();
}

bool RangeGenerator::update(const Operation& op, RangeMap& ranges) {
  Range source;
  if (op.source.type == Operand::Type::CONSTANT) {
    source = Range(op.source.value, op.source.value);
  } else {  // direct memory access
    auto it = ranges.find(op.source.value.asInt());
    if (it != ranges.end()) {
      source = it->second;
    } else {
      source = Range(Number::INF, Number::INF);  // unknown source
    }
  }
  auto targetCell = getTargetCell(op);
  auto it = ranges.find(targetCell);
  if (it == ranges.end()) {
    return false;  // should not happen, but just in case
  }
  auto& target = it->second;
  switch (op.type) {
    case Operation::Type::NOP:
    case Operation::Type::DBG:
      break;  // no operation, nothing to do
    case Operation::Type::MOV:
      target = source;
      break;
    case Operation::Type::ADD:
      target += source;
      break;
    case Operation::Type::SUB:
      target -= source;
      break;
    case Operation::Type::TRN:
      target.trn(source);
      break;
    case Operation::Type::MUL:
      target *= source;
      break;
    case Operation::Type::DIV:
      target /= source;
      break;
    case Operation::Type::DIF:
      target.dif(source);
      break;
    case Operation::Type::DIR:
      target.dir(source);
      break;
    case Operation::Type::MOD:
      target %= source;
      break;
    case Operation::Type::POW:
      target.pow(source);
      break;
    case Operation::Type::GCD:
      target.gcd(source);
      break;
    case Operation::Type::LEX:
      target.lex(source);
      break;
    case Operation::Type::BIN:
      target.bin(source);
      break;
    case Operation::Type::LOG:
      target.log(source);
      break;
    case Operation::Type::NRT:
      target.nrt(source);
      break;
    case Operation::Type::DGS:
      target.dgs(source);
      break;
    case Operation::Type::DGR:
      target.dgr(source);
      break;
    case Operation::Type::EQU:
    case Operation::Type::NEQ:
    case Operation::Type::LEQ:
    case Operation::Type::GEQ:
      target = Range(Number::ZERO, Number::ONE);
      break;
    case Operation::Type::MIN:
      target.min(source);
      break;
    case Operation::Type::MAX:
      target.max(source);
      break;
    case Operation::Type::BAN:
    case Operation::Type::BOR:
    case Operation::Type::BXO:
      target.binary(source);
      break;
    case Operation::Type::SEQ: {
      if (op.source.type != Operand::Type::CONSTANT) {
        return false;  // sequence operation requires a constant source
      }
      auto id = op.source.value.asInt();
      program_cache.collect(id);  // ensures that there is no recursion
      RangeMap tmp;
      if (!generate(program_cache.get(id), tmp)) {
        return false;
      }
      auto tmp_it = tmp.find(Program::OUTPUT_CELL);
      if (tmp_it != tmp.end()) {
        target = tmp_it->second;
      } else {
        target = Range(Number::INF, Number::INF);
      }
      break;
    }
    case Operation::Type::LPB: {
      if (op.source.type != Operand::Type::CONSTANT ||
          op.source.value != Number::ONE) {
        return false;
      }
      loop_states.push({targetCell, ranges});
      target.lower_bound = Number::ZERO;
      break;
    }
    case Operation::Type::LPE: {
      auto rangeBefore = loop_states.top().rangesBefore.at(targetCell);
      target.lower_bound =
          Semantics::min(rangeBefore.lower_bound, Number::ZERO);
      loop_states.pop();
      break;
    }
    case Operation::Type::CLR:
    case Operation::Type::PRG:
    case Operation::Type::__COUNT:
      return false;  // unsupported operation type for range generation
  }
  // extra work inside loops
  if (!loop_states.empty() &&
      (ProgramUtil::isArithmetic(op.type) || op.type == Operation::Type::SEQ)) {
    auto rangeBefore = loop_states.top().rangesBefore.at(targetCell);
    if (targetCell == loop_states.top().counterCell) {
      target.lower_bound = Number::ZERO;
    } else {
      if (target.lower_bound < rangeBefore.lower_bound) {
        target.lower_bound = Number::INF;
      }
      if (target.upper_bound > rangeBefore.upper_bound) {
        target.upper_bound = Number::INF;
      }
    }
  }
  return true;
}

int64_t RangeGenerator::getTargetCell(const Operation& op) const {
  return op.type == Operation::Type::LPE ? loop_states.top().counterCell
                                         : op.target.value.asInt();
}
