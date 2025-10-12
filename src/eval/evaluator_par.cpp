#include "eval/evaluator_par.hpp"

#include "lang/program_util.hpp"

void PartialEvaluator::removeReferences(const Operand &op) {
  auto it = values.begin();
  // do we need to do this transitively?
  while (it != values.end()) {
    if (it->second == op) {
      it = values.erase(it);
    } else {
      it++;
    }
  }
}

Operand PartialEvaluator::resolveOperand(const Operand &op) const {
  if (op.type == Operand::Type::DIRECT) {
    auto it = values.find(op.value.asInt());
    if (it != values.end()) {
      return it->second;
    }
  }
  return op;
}

void PartialEvaluator::initZeros(size_t from, size_t to) {
  values.clear();
  for (size_t i = from; i <= to; i++) {
    values[i] = Operand(Operand::Type::CONSTANT, 0);
  }
}

bool PartialEvaluator::checkValue(int64_t cell, int64_t expected_value) const {
  auto it = values.find(cell);
  if (it == values.end()) {
    return false;
  }
  return it->second.value.asInt() == expected_value;
}

bool PartialEvaluator::doPartialEval(Program &p, size_t op_index) {
  // make sure there is not indirect memory access
  Operation &op = p.ops[op_index];
  if (ProgramUtil::hasIndirectOperand(op)) {
    values.clear();
    return false;
  }

  // resolve source and target operands
  auto source = resolveOperand(op.source);
  auto target = resolveOperand(op.target);

  // calculate new value
  bool has_result = false;
  auto num_ops = Operation::Metadata::get(op.type).num_operands;
  switch (op.type) {
    case Operation::Type::NOP:
    case Operation::Type::DBG:
    case Operation::Type::SEQ: {
      break;
    }

    case Operation::Type::LPB:
    case Operation::Type::LPE: {
      // remove values from cells that are modified in the loop
      auto loop = ProgramUtil::getEnclosingLoop(p, op_index);
      for (int64_t i = loop.first + 1; i < loop.second; i++) {
        if (ProgramUtil::isWritingRegion(p.ops[i].type) ||
            ProgramUtil::hasIndirectOperand(p.ops[i])) {
          values.clear();
          break;
        }
        if (Operation::Metadata::get(p.ops[i].type).is_writing_target) {
          values.erase(p.ops[i].target.value.asInt());
          removeReferences(p.ops[i].target);
        }
      }
      return false;
    }
    case Operation::Type::CLR:
    case Operation::Type::FIL:
    case Operation::Type::ROL:
    case Operation::Type::ROR:
    case Operation::Type::PRG: {
      values.clear();
      return false;
    }

    case Operation::Type::MOV: {
      target = source;
      has_result = true;
      break;
    }

    default: {
      if (target.type == Operand::Type::CONSTANT &&
          (num_ops == 1 || source.type == Operand::Type::CONSTANT)) {
        target.value = Interpreter::calc(op.type, target.value, source.value);
        has_result = true;
      }
      break;
    }
  }

  // update target value
  if (num_ops > 0) {
    if (has_result) {
      values[op.target.value.asInt()] = target;
    } else {
      values.erase(op.target.value.asInt());
    }
    // remove references to target because they are out-dated now
    removeReferences(op.target);
  }
  return has_result;
}
