#include "eval/range_generator.hpp"

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
  if (!ProgramUtil::getUsedMemoryCells(program, nullptr, &used_cells,
                                       largest_used, -1)) {
    return false;
  }
  loop_states = {};
  ranges.clear();
  int64_t offset = ProgramUtil::getOffset(program);
  for (auto cell : used_cells) {
    if (cell == Program::INPUT_CELL) {
      ranges[cell] = Range(Number(offset), input_upper_bound);
    } else {
      ranges[cell] = Range(Number::ZERO, Number::ZERO);
    }
  }
  return true;
}

bool RangeGenerator::generate(const Program& program, RangeMap& ranges) {
  std::vector<RangeMap> collected;
  if (!collect(program, collected) || collected.empty()) {
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
    if (is_range_before_op) {
      collected.push_back(ranges);
    }
    if (!update(op, ranges)) {
      ok = false;
      break;
    }
    if (!is_range_before_op) {
      collected.push_back(ranges);
    }
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
        auto end = collected[loop.second];
        for (auto& it : ranges) {
          mergeLoopRange(end.get(it.first), it.second);
        }
      }
      if (is_range_before_op) {
        collected[j] = ranges;
      }
      if (!update(op, ranges)) {
        ok = false;
        break;
      }
      if (!is_range_before_op) {
        collected[j] = ranges;
      }
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
      if (!handleSeqOperation(op, target)) {
        return false;
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
    case Operation::Type::FIL:
    case Operation::Type::ROL:
    case Operation::Type::ROR:
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

// Helper function to handle SEQ operation case in update()
bool RangeGenerator::handleSeqOperation(const Operation& op, Range& target) {
  if (op.source.type != Operand::Type::CONSTANT) {
    return false;  // sequence operation requires a constant source
  }
  const auto uid = UID::castFromInt(op.source.value.asInt());
  auto it = seq_range_cache.find(uid);
  if (it != seq_range_cache.end()) {
    target = it->second;
  } else {
    program_cache.collect(uid);  // ensures that there is no recursion
    RangeGenerator gen;
    RangeMap tmp;
    if (!gen.generate(program_cache.getProgram(uid), tmp)) {
      return false;
    }
    target = tmp.get(Program::OUTPUT_CELL);
    seq_range_cache[uid] = target;
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

bool RangeGenerator::collect(const SimpleLoopProgram& loop,
                             std::vector<RangeMap>& pre_loop_ranges,
                             std::vector<RangeMap>& body_ranges,
                             std::vector<RangeMap>& post_loop_ranges) {
  if (!loop.is_simple_loop) {
    return false;
  }
  // Reconstruct the full program: pre_loop + lpb + body + lpe + post_loop
  Program full;
  full.ops.insert(full.ops.end(), loop.pre_loop.ops.begin(),
                  loop.pre_loop.ops.end());
  Operation lpb(Operation::Type::LPB,
                Operand(Operand::Type::DIRECT, loop.counter),
                Operand(Operand::Type::CONSTANT, 1));
  full.ops.push_back(lpb);
  full.ops.insert(full.ops.end(), loop.body.ops.begin(), loop.body.ops.end());
  full.ops.push_back(Operation(Operation::Type::LPE));
  full.ops.insert(full.ops.end(), loop.post_loop.ops.begin(),
                  loop.post_loop.ops.end());
  // Compute ranges for the full program
  std::vector<RangeMap> full_ranges;
  if (!collect(full, full_ranges) || full_ranges.size() != full.ops.size()) {
    return false;
  }
  // Fill output vectors
  size_t pre_size = loop.pre_loop.ops.size();
  size_t body_size = loop.body.ops.size();
  size_t post_size = loop.post_loop.ops.size();
  pre_loop_ranges.assign(full_ranges.begin(), full_ranges.begin() + pre_size);
  body_ranges.assign(full_ranges.begin() + pre_size + 1,
                     full_ranges.begin() + pre_size + 1 + body_size);
  post_loop_ranges.assign(full_ranges.end() - post_size, full_ranges.end());
  return true;
}
