#include "eval/range_generator.hpp"

#include <set>
#include <stdexcept>
#include <unordered_set>

#include "eval/semantics.hpp"
#include "lang/program_util.hpp"
#include "sys/log.hpp"

bool RangeGenerator::init(const Program& program, RangeMap& ranges,
                          Number inputUpperBound) {
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
      ranges[cell] = Range(Number(offset), inputUpperBound);
    } else {
      ranges[cell] = Range(Number::ZERO, Number::ZERO);
    }
  }
  return true;
}

bool RangeGenerator::generate(const Program& program, RangeMap& ranges,
                              Number inputUpperBound) {
  std::vector<RangeMap> collected;
  if (!collect(program, collected, inputUpperBound) || collected.empty()) {
    return false;
  }
  ranges = collected.back();
  return true;
}

bool RangeGenerator::annotate(Program& program, Number inputUpperBound) {
  std::vector<RangeMap> collected;
  bool ok = collect(program, collected, inputUpperBound);
  for (size_t i = 0; i < collected.size(); ++i) {
    auto& op = program.ops[i];
    if (op.type != Operation::Type::NOP) {
      op.comment = collected[i].toString(getTargetCell(program, i));
    }
  }
  return ok;
}

bool RangeGenerator::collect(const Program& program,
                             std::vector<RangeMap>& collected,
                             Number inputUpperBound) {
  // compute ranges for the program
  RangeMap ranges;
  if (!init(program, ranges, inputUpperBound)) {
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
    init(program, ranges, inputUpperBound);
    for (size_t j = 0; j < program.ops.size(); ++j) {
      auto& op = program.ops[j];
      if (op.type == Operation::Type::LPB) {
        auto loop = ProgramUtil::getEnclosingLoop(program, j);
        auto end = collected[loop.second];
        for (auto& it : ranges) {
          mergeLoopRange(end.get(it.first), it.second);
        }
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
  if (Operation::Metadata::get(op.type).num_operands > 1) {
    if (op.source.type == Operand::Type::CONSTANT) {
      source = Range(op.source.value, op.source.value);
    } else {  // direct memory access
      source = ranges.get(op.source.value.asInt());
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
      return true;  // no operation, nothing to do
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
    case Operation::Type::FAC: {
      target.fac(source);
      break;
    }
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
      auto it = seq_range_cache.find(id);
      if (it != seq_range_cache.end()) {
        target = it->second;
      } else {
        program_cache.collect(id);  // ensures that there is no recursion
        RangeGenerator gen;
        RangeMap tmp;
        if (!gen.generate(program_cache.getProgram(id), tmp)) {
          return false;
        }
        target = tmp.get(Program::OUTPUT_CELL);
        seq_range_cache[id] = target;
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
  if (!loop_states.empty()) {
    auto before = loop_states.top().rangesBefore.get(targetCell);
    mergeLoopRange(before, target);
  }
  return true;
}

void RangeGenerator::mergeLoopRange(const Range& before, Range& target) const {
  if (target.lower_bound > before.lower_bound) {
    target.lower_bound = before.lower_bound;
  } else if (target.lower_bound < before.lower_bound ||
             before.lower_bound == Number::INF) {
    target.lower_bound = Number::INF;
  }
  if (target.upper_bound > before.upper_bound ||
      before.upper_bound == Number::INF) {
    target.upper_bound = Number::INF;
  } else if (target.upper_bound < before.upper_bound) {
    target.upper_bound = before.upper_bound;
  }
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
      throw std::runtime_error("no loop state available at lpe");
    }
    return loop_states.top().counterCell;
  } else {
    return op.target.value.asInt();
  }
}
