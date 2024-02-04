#pragma once

#include <map>
#include <set>
#include <unordered_set>

#include "lang/number.hpp"
#include "lang/program.hpp"

class ProgramUtil {
 public:
  static bool hasOp(const Program &p, Operation::Type type);

  static void removeOps(Program &p, Operation::Type type);

  static bool replaceOps(Program &p, Operation::Type oldType,
                         Operation::Type newType);

  static bool isNop(const Operation &op);

  static size_t numOps(const Program &p, bool withNops);

  static size_t numOps(const Program &p, Operation::Type type);

  static size_t numOps(const Program &p, Operand::Type type);

  static bool isArithmetic(Operation::Type t);

  static bool isCommutative(Operation::Type t);

  static bool isCommutative(const Program &p, int64_t cell);

  static bool isCommutative(const Program &p, const std::set<int64_t> &cells);

  static bool isAdditive(Operation::Type t);

  static bool isNonTrivialLoopBegin(const Operation &op);

  static bool hasIndirectOperand(const Operation &op);

  static bool hasIndirectOperand(const Program &p);

  static bool areIndependent(const Operation &op1, const Operation &op2);

  static bool getUsedMemoryCells(const Program &p,
                                 std::unordered_set<int64_t> &used_cells,
                                 int64_t &larged_used, int64_t max_memory);

  static int64_t getLargestDirectMemoryCell(const Program &p);

  static std::set<Number> getAllConstants(const Program &p,
                                          bool arithmetic_only);

  static Number getLargestConstant(const Program &p);

  class ConstantLoopInfo {
   public:
    bool has_constant_loop;
    size_t index_lpb;
    Number constant_value;
  };

  static ConstantLoopInfo findConstantLoop(const Program &p);

  static std::pair<int64_t, int64_t> getEnclosingLoop(const Program &p,
                                                      int64_t op_index);

  static std::string operandToString(const Operand &op);

  static std::string operationToString(const Operation &op);

  static void print(const Program &p, std::ostream &out,
                    std::string newline = std::string("\n"));

  static void print(const Operation &op, std::ostream &out, int indent = 0);

  static void exportToDot(const Program &p, std::ostream &out);

  static size_t hash(const Program &p);

  static size_t hash(const Operation &op);

  static size_t hash(const Operand &op);

  static void validate(const Program &p);

  static void avoidNopOrOverflow(Operation &op);
};
