#pragma once

#include "lang/program.hpp"
#include "sys/util.hpp"

class Optimizer {
 public:
  explicit Optimizer(const Settings &settings) : settings(settings) {}

  bool optimize(Program &p) const;

  bool removeNops(Program &p) const;

  bool removeEmptyLoops(Program &p) const;

  bool mergeOps(Program &p) const;

  bool mergeRepeated(Program &p) const;

  bool simplifyOperations(Program &p) const;

  bool fixSandwich(Program &p) const;

  bool reduceMemoryCells(Program &p) const;

  bool canChangeVariableOrder(const Program &p) const;

  bool partialEval(Program &p) const;

  bool sortOperations(Program &p) const;

  bool mergeLoops(Program &p) const;

  bool collapseLoops(Program &p) const;

  bool pullUpMov(Program &p) const;

  bool removeCommutativeDetour(Program &p) const;

  static constexpr size_t NUM_RESERVED_CELLS = 1;
  static constexpr size_t NUM_INITIALIZED_CELLS = 1;

 private:
  /*
   * Helper class for moving operations.
   */
  class OperationMover {
   public:
    void init(Program &prog);
    bool up(size_t i);
    bool down(size_t i);
    int64_t getTotalScore() const;

   private:
    int64_t scoreNeighbors(size_t i) const;
    void updateScore(size_t i);
    void updateNeighborhood(size_t i);

    Program *prog;
    std::vector<int64_t> opScores;
    int64_t totalScore;
  };

  Settings settings;
  mutable OperationMover opMover;

  bool doPartialEval(Program &p, size_t op_index,
                     std::map<int64_t, Operand> &values) const;
};
