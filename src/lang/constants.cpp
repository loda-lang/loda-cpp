#include "lang/constants.hpp"

#include "lang/program_util.hpp"

std::set<Number> Constants::getAllConstants(const Program &p,
                                            bool arithmetic_only) {
  std::set<Number> result;
  for (const auto &op : p.ops) {
    auto num_operands = Operation::Metadata::get(op.type).num_operands;
    if (num_operands > 1 && op.source.type == Operand::Type::CONSTANT &&
        (ProgramUtil::isArithmetic(op.type) || !arithmetic_only)) {
      result.insert(op.source.value);
    }
  }
  return result;
}

Number Constants::getLargestConstant(const Program &p) {
  Number largest(-1);
  for (const auto &op : p.ops) {
    if (op.source.type == Operand::Type::CONSTANT &&
        largest < op.source.value) {
      largest = op.source.value;
    }
  }
  return largest;
}

Constants::LoopInfo Constants::findConstantLoop(const Program &p) {
  // assumes that the program is optimized already
  Constants::LoopInfo info;
  std::map<Number, Number> values;
  for (size_t i = 0; i < p.ops.size(); i++) {
    auto &op = p.ops[i];
    if (op.target.type != Operand::Type::DIRECT) {
      values.clear();
      continue;
    }
    if (op.type == Operation::Type::MOV) {
      if (op.source.type == Operand::Type::CONSTANT) {
        values[op.target.value] = op.source.value;
      } else {
        values.erase(op.target.value);
      }
    } else if (op.type == Operation::Type::LPB) {
      auto it = values.find(op.target.value);
      if (it != values.end()) {
        // constant loop found!
        info.has_constant_loop = true;
        info.index_lpb = i;
        info.constant_value = it->second;
        return info;
      }
      values.clear();
    } else if (op.type == Operation::Type::LPE) {
      values.clear();
    } else if (ProgramUtil::isArithmetic(op.type)) {
      values.erase(op.target.value);
    }
  }
  // no constant loop found
  info.has_constant_loop = false;
  return info;
}
