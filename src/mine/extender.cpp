#include "mine/extender.hpp"

#include "lang/program_util.hpp"
#include "lang/semantics.hpp"
#include "sys/util.hpp"

void add_or_sub(Program &p, const Number &c) {
  if (Number::ZERO < c) {
    p.push_back(Operation::Type::ADD, Operand::Type::DIRECT,
                Program::OUTPUT_CELL, Operand::Type::CONSTANT, c);
  } else if (c < Number::ZERO) {
    auto d = c;
    d.negate();
    p.push_back(Operation::Type::SUB, Operand::Type::DIRECT,
                Program::OUTPUT_CELL, Operand::Type::CONSTANT, d);
  }
}

bool Extender::linear1(Program &p, line_t inverse, line_t target) {
  if (inverse.offset == target.offset && inverse.factor == target.factor) {
    return true;
  }
  if (inverse.offset != Number::ZERO) {
    add_or_sub(p, Semantics::sub(Number::ZERO, inverse.offset));
  }
  if (Number::ONE < inverse.factor && Number::ONE < target.factor &&
      Semantics::mod(target.factor, inverse.factor) == Number::ZERO) {
    target.factor = Semantics::div(target.factor, inverse.factor);
    inverse.factor = Number::ONE;  // order is important!!
  }
  if (Number::ONE < inverse.factor && Number::ONE < target.factor &&
      Semantics::mod(inverse.factor, target.factor) == Number::ZERO) {
    inverse.factor = Semantics::div(inverse.factor, target.factor);
    target.factor = Number::ONE;  // order is important!!
  }
  if (inverse.factor != Number::ONE) {
    p.push_back(Operation::Type::DIV, Operand::Type::DIRECT,
                Program::OUTPUT_CELL, Operand::Type::CONSTANT, inverse.factor);
  }
  if (target.factor != Number::ONE) {
    p.push_back(Operation::Type::MUL, Operand::Type::DIRECT,
                Program::OUTPUT_CELL, Operand::Type::CONSTANT, target.factor);
  }
  if (target.offset != Number::ZERO) {
    add_or_sub(p, target.offset);
  }
  return true;
}

bool Extender::linear2(Program &p, line_t inverse, line_t target) {
  if (inverse.factor == target.factor && inverse.offset == target.offset) {
    return true;
  }
  if (inverse.factor != Number::ONE) {
    p.push_back(Operation::Type::DIV, Operand::Type::DIRECT,
                Program::OUTPUT_CELL, Operand::Type::CONSTANT, inverse.factor);
  }
  add_or_sub(p, Semantics::sub(target.offset, inverse.offset));
  if (target.factor != Number::ONE) {
    p.push_back(Operation::Type::MUL, Operand::Type::DIRECT,
                Program::OUTPUT_CELL, Operand::Type::CONSTANT, target.factor);
  }
  return true;
}

bool Extender::delta_one(Program &p, const bool sum) {
  Settings settings;
  std::unordered_set<int64_t> used_cells;
  int64_t largest_used = 0;
  if (!ProgramUtil::getUsedMemoryCells(p, used_cells, largest_used,
                                       settings.max_memory)) {
    return false;
  }
  largest_used = std::max((int64_t)Program::OUTPUT_CELL, largest_used);
  auto saved_arg_cell = largest_used + 1;
  auto saved_result_cell = largest_used + 2;
  auto loop_counter_cell = largest_used + 3;
  auto tmp_counter_cell = largest_used + 4;

  Program prefix;
  prefix.push_back(Operation::Type::MOV, Operand::Type::DIRECT, saved_arg_cell,
                   Operand::Type::DIRECT, Program::INPUT_CELL);
  if (sum) {
    prefix.push_back(Operation::Type::MOV, Operand::Type::DIRECT,
                     loop_counter_cell, Operand::Type::DIRECT,
                     Program::INPUT_CELL);
    prefix.push_back(Operation::Type::ADD, Operand::Type::DIRECT,
                     loop_counter_cell, Operand::Type::CONSTANT, 1);
  } else {
    prefix.push_back(Operation::Type::MOV, Operand::Type::DIRECT,
                     loop_counter_cell, Operand::Type::CONSTANT, 2);
  }
  prefix.push_back(Operation::Type::LPB, Operand::Type::DIRECT,
                   loop_counter_cell, Operand::Type::CONSTANT, 1);
  prefix.push_back(Operation::Type::CLR, Operand::Type::DIRECT,
                   Program::INPUT_CELL, Operand::Type::CONSTANT,
                   largest_used + 1);
  prefix.push_back(Operation::Type::SUB, Operand::Type::DIRECT,
                   loop_counter_cell, Operand::Type::CONSTANT, 1);
  prefix.push_back(Operation::Type::MOV, Operand::Type::DIRECT,
                   Program::INPUT_CELL, Operand::Type::DIRECT, saved_arg_cell);
  if (sum) {
    prefix.push_back(Operation::Type::SUB, Operand::Type::DIRECT,
                     Program::INPUT_CELL, Operand::Type::DIRECT,
                     loop_counter_cell);
  } else {
    prefix.push_back(Operation::Type::ADD, Operand::Type::DIRECT,
                     Program::INPUT_CELL, Operand::Type::DIRECT,
                     loop_counter_cell);
    prefix.push_back(Operation::Type::TRN, Operand::Type::DIRECT,
                     Program::INPUT_CELL, Operand::Type::CONSTANT, 1);
  }
  p.ops.insert(p.ops.begin(), prefix.ops.begin(), prefix.ops.end());

  if (sum) {
    p.push_back(Operation::Type::ADD, Operand::Type::DIRECT, saved_result_cell,
                Operand::Type::DIRECT, Program::OUTPUT_CELL);
  } else {
    p.push_back(Operation::Type::MOV, Operand::Type::DIRECT, tmp_counter_cell,
                Operand::Type::DIRECT, loop_counter_cell);
    p.push_back(Operation::Type::CMP, Operand::Type::DIRECT, tmp_counter_cell,
                Operand::Type::CONSTANT, 1);
    p.push_back(Operation::Type::MUL, Operand::Type::DIRECT, tmp_counter_cell,
                Operand::Type::DIRECT, Program::OUTPUT_CELL);
    p.push_back(Operation::Type::ADD, Operand::Type::DIRECT, saved_result_cell,
                Operand::Type::DIRECT, tmp_counter_cell);
  }
  p.push_back(Operation::Type::LPE, Operand::Type::CONSTANT, 0,
              Operand::Type::CONSTANT, 0);

  if (sum) {
    p.push_back(Operation::Type::MOV, Operand::Type::DIRECT,
                Program::OUTPUT_CELL, Operand::Type::DIRECT, saved_result_cell);
  } else {
    p.push_back(Operation::Type::MIN, Operand::Type::DIRECT, saved_arg_cell,
                Operand::Type::CONSTANT, 1);
    p.push_back(Operation::Type::MUL, Operand::Type::DIRECT, saved_arg_cell,
                Operand::Type::DIRECT, Program::OUTPUT_CELL);
    p.push_back(Operation::Type::MOV, Operand::Type::DIRECT,
                Program::OUTPUT_CELL, Operand::Type::DIRECT, saved_result_cell);
    p.push_back(Operation::Type::SUB, Operand::Type::DIRECT,
                Program::OUTPUT_CELL, Operand::Type::DIRECT, saved_arg_cell);
  }
  return true;
}

bool Extender::delta_it(Program &p, int64_t delta) {
  while (delta < 0) {
    if (!delta_one(p, false)) {
      return false;
    }
    delta++;
  }
  while (delta > 0) {
    if (!delta_one(p, true)) {
      return false;
    }
    delta--;
  }
  return true;
}

bool Extender::digit(Program &p, int64_t num_digits, int64_t offset) {
  if (offset != 0) {
    add_or_sub(p, offset);
  }
  p.push_back(Operation::Type::MOD, Operand::Type::DIRECT, Program::OUTPUT_CELL,
              Operand::Type::CONSTANT, num_digits);
  p.push_back(Operation::Type::ADD, Operand::Type::DIRECT, Program::OUTPUT_CELL,
              Operand::Type::CONSTANT, num_digits);
  p.push_back(Operation::Type::MOD, Operand::Type::DIRECT, Program::OUTPUT_CELL,
              Operand::Type::CONSTANT, num_digits);
  return true;
}
