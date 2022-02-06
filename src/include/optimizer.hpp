#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Optimizer {
 public:
  Optimizer(const Settings &settings) : settings(settings) {}

  bool optimize(Program &p, size_t num_reserved_cells,
                size_t num_initialized_cells) const;

  bool removeNops(Program &p) const;

  bool removeEmptyLoops(Program &p) const;

  bool mergeOps(Program &p) const;

  bool simplifyOperations(Program &p, size_t num_initialized_cells) const;

  bool reduceMemoryCells(Program &p, size_t num_reserved_cells) const;

  bool swapMemoryCells(Program &p, size_t num_reserved_cells) const;

  bool canChangeVariableOrder(const Program &p) const;

  bool partialEval(Program &p, size_t num_initialized_cells) const;

  bool shouldSwapOperations(const Operation &first,
                            const Operation &second) const;

  bool sortOperations(Program &p) const;

  bool mergeLoops(Program &p) const;

  bool pullUpMov(Program &p) const;

 private:
  Settings settings;

  bool doPartialEval(Program &p, size_t op_index,
                     std::map<int64_t, Operand> &values) const;
};
