#pragma once

#include <map>
#include <set>
#include <unordered_set>

#include "base/uid.hpp"
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

  static bool isNonTrivialClear(const Operation &op);

  static bool isReadingCell(const Operation &op, int64_t cell);

  static bool isWritingRegion(Operation::Type t);

  static bool hasRegionOperation(const Program &p);

  static bool hasIndirectOperand(const Operation &op);

  static bool hasIndirectOperand(const Program &p);

  static bool areIndependent(const Operation &op1, const Operation &op2);

  static bool getUsedMemoryCells(const Program &p,
                                 std::map<UID, Program> *prg_refs,
                                 std::unordered_set<int64_t> *used_cells,
                                 int64_t &larged_used, int64_t max_memory);

  static int64_t getLargestDirectMemoryCellWithoutRegions(const Program &p);

  static int64_t getLargestDirectMemoryCellWithRegions(const Program &p);

  static pair<int64_t, int64_t> getTargetMemoryRange(int64_t start, int64_t length);

  static pair<int64_t, int64_t> getTargetMemoryRange(const Operation &op);

  static bool getUsedUninitializedCells(const Program &p,
                                        std::set<int64_t> &initialized,
                                        std::set<int64_t> &uninitialized,
                                        size_t start_pos = 0);

  static bool swapDirectOperandCells(Program &p, int64_t cell1, int64_t cell2);

  static std::pair<int64_t, int64_t> getEnclosingLoop(const Program &p,
                                                      int64_t op_index);

  static std::string operandToString(const Operand &op);

  static std::string operationToString(const Operation &op);

  static void print(const Program &p, std::ostream &out,
                    const std::string &newline = "\n");

  static void print(const Operation &op, std::ostream &out, int indent = 0);

  static size_t hash(const Program &p);

  static size_t hash(const Operation &op);

  static size_t hash(const Operand &op);

  static void validate(const Program &p);

  static void avoidNopOrOverflow(Operation &op);

  static std::string dirStr(UID id);

  static std::string getProgramsDir(char domain);

  static std::string getProgramPath(UID id, bool local = false);

  static int64_t getOffset(const Program &p);

  static int64_t setOffset(Program &p, int64_t offset);

  static int64_t getLoopDepth(const Program &p, int64_t pos);
};
