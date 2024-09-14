#pragma once

#include <map>

#include "eval/interpreter.hpp"

class PartialEvaluator {
 public:
  explicit PartialEvaluator(const Settings& settings) : interpreter(settings) {}

  void initZeros(size_t from, size_t to);

  bool checkValue(int64_t cell, int64_t expected_value) const;

  bool doPartialEval(Program& p, size_t op_index);

  Operand resolveOperand(const Operand& op) const;

  void removeReferences(const Operand& op);

  std::map<int64_t, Operand> values;

 private:
  Interpreter interpreter;
};
