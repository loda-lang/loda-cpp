#include "form/range_generator.hpp"

#include <set>
#include <stdexcept>
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
  loop_states = {};
  ranges.clear();
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
  std::vector<RangeMap> collected;
  if (!collect(program, collected)) {
    return false;
  }
  ranges = collected.back();
  return true;
}

bool RangeGenerator::annotate(Program& program) {
  std::vector<RangeMap> collected;
  bool ok = collect(program, collected);
  for (size_t i = 0; i < collected.size(); ++i) {
    auto& op = program.ops[i];
    if (op.type != Operation::Type::NOP) {
      op.comment = collected[i].toString(getTargetCell(program, i));
      // op.comment = collected[i].toString();
    }
  }
  return ok;
}

bool RangeGenerator::collect(const Program& program,
                             std::vector<RangeMap>& collected) {
  // compute ranges for the program
  RangeMap ranges;
  if (!init(program, ranges)) {
    return false;
  }
  bool ok = true, hasLoops = false;
  for (auto& op : program.ops) {
    if (!update(op, ranges)) {
      ok = false;
      break;
    }
    collected.push_back(ranges);
    hasLoops = hasLoops || op.type == Operation::Type::LPB;
  }
  // compute fixed point if the program has loops
  for (size_t i = 0; i < program.ops.size() && ok && hasLoops; ++i) {
    ranges = {};
    init(program, ranges);
    for (size_t j = 0; j < program.ops.size(); ++j) {
      auto& op = program.ops[j];
      if (op.type == Operation::Type::LPB) {
        auto loop = ProgramUtil::getEnclosingLoop(program, j);
        ranges = collected[loop.second];
      }
      if (!update(op, ranges)) {
        ok = false;
        break;
      }
      collected[j] = ranges;
    }
  }
  // remove unbounded ranges
  for (auto& ranges : collected) {
    ranges.prune();
  }
  return ok;
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
      RangeGenerator gen;
      RangeMap tmp;
      if (!gen.generate(program_cache.get(id), tmp)) {
        return false;
      }
      target = tmp.get(Program::OUTPUT_CELL);
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
      auto rangeBefore = loop_states.top().rangesBefore.get(targetCell);
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
    auto rangeBefore = loop_states.top().rangesBefore.get(targetCell);
    if (targetCell == loop_states.top().counterCell) {
      target.lower_bound = Number::ZERO;
    } else {
      if (target.lower_bound > rangeBefore.lower_bound) {
        target.lower_bound = rangeBefore.lower_bound;
      } else if (target.lower_bound < rangeBefore.lower_bound) {
        target.lower_bound = Number::INF;
      }
      if (target.upper_bound > rangeBefore.upper_bound) {
        target.upper_bound = Number::INF;
      } else if (target.upper_bound < rangeBefore.upper_bound) {
        target.upper_bound = rangeBefore.upper_bound;
      }
    }
  }
  return true;
}

int64_t RangeGenerator::getTargetCell(const Program& program,
                                      size_t index) const {
  auto op = program.ops[index];
  if (op.type == Operation::Type::LPE) {
    auto loop = ProgramUtil::getEnclosingLoop(program, index);
    op = program.ops[loop.first];
  }
  return op.target.value.asInt();
}

int64_t RangeGenerator::getTargetCell(const Operation& op) const {
  if (op.type == Operation::Type::LPE) {
    if (loop_states.empty()) {
      throw std::runtime_error("No loop state available for LPE operation");
    }
    return loop_states.top().counterCell;
  } else {
    return op.target.value.asInt();
  }
}
