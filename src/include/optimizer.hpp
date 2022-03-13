#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Optimizer {
 public:
  Optimizer(const Settings &settings) : settings(settings) {}

  bool optimize(Program &p) const;

  bool removeNops(Program &p) const;

  bool removeEmptyLoops(Program &p) const;

  bool mergeOps(Program &p) const;

  bool simplifyOperations(Program &p) const;

  bool reduceMemoryCells(Program &p) const;

  bool swapMemoryCells(Program &p) const;

  bool canChangeVariableOrder(const Program &p) const;

  bool partialEval(Program &p) const;

  bool shouldSwapOperations(const Operation &first,
                            const Operation &second) const;

  bool sortOperations(Program &p) const;

  bool mergeLoops(Program &p) const;

  bool pullUpMov(Program &p) const;

  static constexpr size_t NUM_RESERVED_CELLS = 1;
  static constexpr size_t NUM_INITIALIZED_CELLS = 1;

 private:
  Settings settings;

  bool doPartialEval(Program &p, size_t op_index,
                     std::map<int64_t, Operand> &values) const;
};
