#include "lang/program_util.hpp"

#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stack>

#include "lang/program.hpp"
#include "sys/file.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

bool ProgramUtil::hasOp(const Program &p, Operation::Type type) {
  return std::any_of(p.ops.begin(), p.ops.end(),
                     [&](const Operation &op) { return op.type == type; });
}

void ProgramUtil::removeOps(Program &p, Operation::Type type) {
  auto it = p.ops.begin();
  while (it != p.ops.end()) {
    if (it->type == type) {
      it = p.ops.erase(it);
    } else {
      it++;
    }
  }
}

bool ProgramUtil::replaceOps(Program &p, Operation::Type oldType,
                             Operation::Type newType) {
  bool result = false;
  for (Operation &op : p.ops) {
    if (op.type == oldType) {
      op.type = newType;
      result = true;
    }
  }
  return result;
}

bool ProgramUtil::isNop(const Operation &op) {
  if (op.type == Operation::Type::NOP || op.type == Operation::Type::DBG) {
    return true;
  } else if (op.source == op.target && (op.type == Operation::Type::MOV ||
                                        op.type == Operation::Type::MIN ||
                                        op.type == Operation::Type::MAX)) {
    return true;
  } else if (op.source.type == Operand::Type::CONSTANT &&
             op.source.value == Number::ZERO &&
             (op.type == Operation::Type::ADD ||
              op.type == Operation::Type::SUB ||
              op.type == Operation::Type::CLR ||
              op.type == Operation::Type::FIL ||
              op.type == Operation::Type::ROL ||
              op.type == Operation::Type::ROR)) {
    return true;
  } else if (op.source.type == Operand::Type::CONSTANT &&
             op.source.value == Number::ONE &&
             ((op.type == Operation::Type::MUL ||
               op.type == Operation::Type::DIV ||
               op.type == Operation::Type::DIF ||
               op.type == Operation::Type::DIR ||
               op.type == Operation::Type::POW ||
               op.type == Operation::Type::BIN ||
               op.type == Operation::Type::ROL ||
               op.type == Operation::Type::ROR))) {
    return true;
  }
  return false;
}

size_t ProgramUtil::numOps(const Program &p, bool withNops) {
  if (withNops) {
    return p.ops.size();
  } else {
    return std::count_if(p.ops.begin(), p.ops.end(), [](const Operation &op) {
      return (op.type != Operation::Type::NOP);
    });
  }
}

size_t ProgramUtil::numOps(const Program &p, Operation::Type type) {
  return std::count_if(p.ops.begin(), p.ops.end(), [type](const Operation &op) {
    return (op.type == type);
  });
}

size_t ProgramUtil::numOps(const Program &p, Operand::Type type) {
  size_t num_ops = 0;
  for (const auto &op : p.ops) {
    const auto &m = Operation::Metadata::get(op.type);
    if (m.num_operands == 1 && op.target.type == type) {
      num_ops++;
    } else if (m.num_operands == 2 &&
               (op.source.type == type || op.target.type == type)) {
      num_ops++;
    }
  }
  return num_ops;
}

bool ProgramUtil::isArithmetic(Operation::Type t) {
  return (t != Operation::Type::NOP && t != Operation::Type::DBG &&
          t != Operation::Type::LPB && t != Operation::Type::LPE &&
          t != Operation::Type::CLR && t != Operation::Type::FIL &&
          t != Operation::Type::ROL && t != Operation::Type::ROR &&
          t != Operation::Type::SEQ && t != Operation::Type::PRG);
}

bool ProgramUtil::isCommutative(Operation::Type t) {
  return (t == Operation::Type::ADD || t == Operation::Type::MUL ||
          t == Operation::Type::MIN || t == Operation::Type::MAX ||
          t == Operation::Type::GCD || t == Operation::Type::EQU ||
          t == Operation::Type::NEQ);
}

bool ProgramUtil::isCommutative(const Program &p, int64_t cell) {
  auto update_type = Operation::Type::NOP;
  for (const auto &op : p.ops) {
    const auto meta = Operation::Metadata::get(op.type);
    const auto target = op.target.value.asInt();
    if (target == cell) {
      if (!ProgramUtil::isCommutative(op.type)) {
        return false;
      }
      if (update_type == Operation::Type::NOP) {
        update_type = op.type;
      } else if (update_type != op.type) {
        return false;
      }
    }
    if (meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT) {
      const auto source = op.source.value.asInt();
      if (source == cell) {
        return false;
      }
    }
  }
  return true;
}

bool ProgramUtil::isCommutative(const Program &p,
                                const std::set<int64_t> &cells) {
  return std::all_of(cells.begin(), cells.end(),
                     [&](int64_t c) { return isCommutative(p, c); });
}

bool ProgramUtil::isAdditive(Operation::Type t) {
  return (t == Operation::Type::ADD || t == Operation::Type::SUB);
}

bool ProgramUtil::isNonTrivialLoopBegin(const Operation &op) {
  return (op.type == Operation::Type::LPB &&
          (op.source.type != Operand::Type::CONSTANT ||
           op.source.value != Number::ONE));
}

bool ProgramUtil::isNonTrivialClear(const Operation &op) {
  return (
      op.type == Operation::Type::CLR &&
      (op.source.type != Operand::Type::CONSTANT ||
       (Number::ONE < op.source.value || op.source.value < Number::MINUS_ONE)));
}

bool ProgramUtil::isReadingCell(const Operation &op, int64_t cell) {
  const auto &m = Operation::Metadata::get(op.type);
  const Operand c(Operand::Type::DIRECT, cell);
  return (m.num_operands > 0 && op.target == c && m.is_reading_target) ||
         (m.num_operands > 1 && op.source == c);
}

bool ProgramUtil::isWritingRegion(Operation::Type t) {
  return (t == Operation::Type::CLR || t == Operation::Type::FIL ||
          t == Operation::Type::ROL || t == Operation::Type::ROR ||
          t == Operation::Type::PRG);
}

bool ProgramUtil::hasRegionOperation(const Program &p) {
  return std::any_of(p.ops.begin(), p.ops.end(), [](const Operation &op) {
    return isWritingRegion(op.type);
  });
}

bool ProgramUtil::hasIndirectOperand(const Operation &op) {
  const auto num_ops = Operation::Metadata::get(op.type).num_operands;
  return (num_ops > 0 && op.target.type == Operand::Type::INDIRECT) ||
         (num_ops > 1 && op.source.type == Operand::Type::INDIRECT);
}

bool ProgramUtil::hasIndirectOperand(const Program &p) {
  return std::any_of(p.ops.begin(), p.ops.end(), [&](const Operation &op) {
    return hasIndirectOperand(op);
  });
}

bool isIndependentCandidate(const Operation &op) {
  // must be an arithmetic operation or a sequence, and must not have indirect
  // operands
  return (
      (ProgramUtil::isArithmetic(op.type) || op.type == Operation::Type::SEQ) &&
      !ProgramUtil::hasIndirectOperand(op));
}

bool haveOverlappingOperands(const Operation &op1, const Operation &op2) {
  // source of the second operand is the same as the target of the first operand
  return (op2.source.type == Operand::Type::DIRECT &&
          op1.target.type == Operand::Type::DIRECT &&
          op1.target.value == op2.source.value);
}

bool ProgramUtil::areIndependent(const Operation &op1, const Operation &op2) {
  if (!isIndependentCandidate(op1) || !isIndependentCandidate(op2)) {
    return false;
  }
  if (op1.target.value == op2.target.value &&
      !(isAdditive(op1.type) && isAdditive(op2.type) &&
        !(op1.type == op2.type && isCommutative(op1.type)))) {
    return false;
  }
  if (haveOverlappingOperands(op1, op2) || haveOverlappingOperands(op2, op1)) {
    return false;
  }
  return true;
}

bool ProgramUtil::getUsedMemoryCells(const Program &p,
                                     std::unordered_set<int64_t> *used_cells,
                                     int64_t &largest_used,
                                     int64_t max_memory) {
  for (const auto &op : p.ops) {
    int64_t region_length = 1;
    if (op.source.type == Operand::Type::INDIRECT ||
        op.target.type == Operand::Type::INDIRECT ||
        op.type == Operation::Type::PRG) {
      return false;
    }
    if (op.type == Operation::Type::LPB || op.type == Operation::Type::CLR ||
        op.type == Operation::Type::FIL || op.type == Operation::Type::ROL ||
        op.type == Operation::Type::ROR) {
      if (op.source.type == Operand::Type::CONSTANT) {
        region_length = op.source.value.asInt();
        // Handle negative region lengths
        if (region_length < 0) {
          return false;
        }
      } else {
        return false;
      }
    }
    if (region_length > max_memory && max_memory >= 0) {
      return false;
    }
    if (op.source.type == Operand::Type::DIRECT) {
      for (int64_t i = 0; i < region_length; i++) {
        if (used_cells) {
          used_cells->insert(op.source.value.asInt() + i);
        }
      }
    }
    if (op.target.type == Operand::Type::DIRECT) {
      for (int64_t i = 0; i < region_length; i++) {
        if (used_cells) {
          used_cells->insert(op.target.value.asInt() + i);
        }
      }
    }
  }
  if (used_cells) {
    const auto max = [](int64_t a, int64_t b) { return std::max(a, b); };
    largest_used =
        std::accumulate(used_cells->begin(), used_cells->end(), (int64_t)0, max);
  } else {
    // If used_cells is null, we still need to compute largest_used
    largest_used = 0;
    for (const auto &op : p.ops) {
      int64_t region_length = 1;
      if (op.type == Operation::Type::LPB || op.type == Operation::Type::CLR ||
          op.type == Operation::Type::FIL || op.type == Operation::Type::ROL ||
          op.type == Operation::Type::ROR) {
        if (op.source.type == Operand::Type::CONSTANT) {
          region_length = op.source.value.asInt();
        }
      }
      if (op.source.type == Operand::Type::DIRECT) {
        largest_used = std::max(largest_used, op.source.value.asInt() + region_length - 1);
      }
      if (op.target.type == Operand::Type::DIRECT) {
        largest_used = std::max(largest_used, op.target.value.asInt() + region_length - 1);
      }
    }
  }
  return true;
}

bool ProgramUtil::getUsedUninitializedCells(const Program &p,
                                            std::set<int64_t> &initialized,
                                            std::set<int64_t> &uninitialized,
                                            size_t start_pos) {
  // find cells that are read and uninitialized
  for (size_t i = start_pos; i < p.ops.size(); i++) {
    const auto &op = p.ops[i];
    // undecidable if there are indirect operands
    if (hasIndirectOperand(op)) {
      return false;
    }
    const auto &meta = Operation::Metadata::get(op.type);
    // check target memory cell
    if (meta.num_operands > 0 && op.target.type == Operand::Type::DIRECT) {
      auto t = op.target.value.asInt();
      if (meta.is_reading_target && initialized.find(t) == initialized.end()) {
        uninitialized.insert(t);
      }
      if (meta.is_writing_target) {
        initialized.insert(t);
      }
    }
    // check source memory cell
    if (meta.num_operands > 1 && op.source.type == Operand::Type::DIRECT) {
      auto s = op.source.value.asInt();
      if (initialized.find(s) == initialized.end()) {
        uninitialized.insert(s);
      }
    }
    // check region operations
    if (op.type == Operation::Type::CLR || op.type == Operation::Type::FIL ||
        op.type == Operation::Type::ROL || op.type == Operation::Type::ROR) {
      if (op.source.type == Operand::Type::CONSTANT) {
        // start of region (direct memory cell)
        const auto t = op.target.value.asInt();
        // region length (constant value)
        const auto s = op.source.value.asInt();
        for (int64_t i = 0; i < s; i++) {
          initialized.insert(t + i);
        }
      } else {
        // undecidable if the region length it not constant
        return false;
      }
    } else if (op.type == Operation::Type::PRG) {
      // TODO: handle prg operations
      return false;
    }
  }
  return true;
}

int64_t ProgramUtil::getLargestDirectMemoryCell(const Program &p) {
  int64_t largest = 0;
  for (const auto &op : p.ops) {
    if (op.source.type == Operand::Type::DIRECT) {
      largest = std::max<int64_t>(largest, op.source.value.asInt());
    }
    if (op.target.type == Operand::Type::DIRECT) {
      largest = std::max<int64_t>(largest, op.target.value.asInt());
    }
  }
  return largest;
}

bool ProgramUtil::swapDirectOperandCells(Program &p, int64_t cell1,
                                         int64_t cell2) {
  if (cell1 == cell2) {
    return false;
  }
  bool changed = false;
  for (auto &op : p.ops) {
    const auto &meta = Operation::Metadata::get(op.type);
    if (meta.num_operands > 1 && op.source.type == Operand::Type::DIRECT) {
      auto src = op.source.value.asInt();
      if (src == cell1) {
        op.source.value = Number(cell2);
        changed = true;
      } else if (src == cell2) {
        op.source.value = Number(cell1);
        changed = true;
      }
    }
    if (meta.num_operands > 0 && op.target.type == Operand::Type::DIRECT) {
      auto trg = op.target.value.asInt();
      if (trg == cell1) {
        op.target.value = Number(cell2);
        changed = true;
      } else if (trg == cell2) {
        op.target.value = Number(cell1);
        changed = true;
      }
    }
  }
  return changed;
}

std::pair<int64_t, int64_t> ProgramUtil::getEnclosingLoop(const Program &p,
                                                          int64_t op_index) {
  int64_t open_loops;
  std::pair<int64_t, int64_t> loop(-1, -1);
  // find start
  if (p.ops.at(op_index).type != Operation::Type::LPB) {
    if (p.ops.at(op_index).type == Operation::Type::LPE) {
      op_index--;  // get inside the loop
    }
    for (open_loops = 1; op_index >= 0 && open_loops; op_index--) {
      auto t = p.ops.at(op_index).type;
      if (t == Operation::Type::LPB) {
        open_loops--;
      } else if (t == Operation::Type::LPE) {
        open_loops++;
      }
    }
    if (open_loops) {
      return loop;
    }
    op_index++;
  }
  loop.first = op_index;
  // find end
  op_index++;
  for (open_loops = 1;
       op_index < static_cast<int64_t>(p.ops.size()) && open_loops;
       op_index++) {
    auto t = p.ops.at(op_index).type;
    if (t == Operation::Type::LPB) {
      open_loops++;
    } else if (t == Operation::Type::LPE) {
      open_loops--;
    }
  }
  op_index--;
  if (open_loops) {
    print(p, std::cout);
    throw std::runtime_error("invalid program");
  }
  loop.second = op_index;
  // final check
  if (p.ops.at(loop.first).type != Operation::Type::LPB ||
      p.ops.at(loop.second).type != Operation::Type::LPE) {
    throw std::runtime_error("internal error");
  }
  return loop;
}

std::string getIndent(int indent) {
  std::string s;
  for (int i = 0; i < indent; i++) {
    s += " ";
  }
  return s;
}

std::string ProgramUtil::operandToString(const Operand &op) {
  switch (op.type) {
    case Operand::Type::CONSTANT:
      return op.value.to_string();
    case Operand::Type::DIRECT:
      return "$" + op.value.to_string();
    case Operand::Type::INDIRECT:
      return "$$" + op.value.to_string();
  }
  return "";
}

std::string ProgramUtil::operationToString(const Operation &op) {
  auto &metadata = Operation::Metadata::get(op.type);
  std::string str;
  if (metadata.num_operands == 0 && op.type != Operation::Type::NOP) {
    str = metadata.name;
  } else if (metadata.num_operands == 1 ||
             (op.type == Operation::Type::LPB &&
              op.source.type == Operand::Type::CONSTANT &&
              op.source.value == 1))  // lpb has an optional second argument
  {
    str = metadata.name + " " + operandToString(op.target);
  } else if (metadata.num_operands == 2) {
    str = metadata.name + " " + operandToString(op.target) + "," +
          operandToString(op.source);
  }
  if (!op.comment.empty()) {
    if (!str.empty()) {
      str = str + " ";
    }
    str = str + "; " + op.comment;
  }
  return str;
}

void ProgramUtil::print(const Operation &op, std::ostream &out, int indent) {
  out << getIndent(indent) << operationToString(op);
}

void ProgramUtil::print(const Program &p, std::ostream &out,
                        const std::string &newline) {
  size_t i = 0;
  bool last_has_comment = false;
  for (i = 0; i < p.ops.size() && p.ops[i].type == Operation::Type::NOP; i++) {
    print(p.ops[i], out, 0);
    out << newline;
    last_has_comment = !p.ops[i].comment.empty();
  }
  if (!p.directives.empty()) {
    if (i > 0 && last_has_comment) {
      out << newline;
    }
    for (const auto &d : p.directives) {
      out << "#" << d.first << " " << d.second << newline;
    }
    out << newline;
  }
  int indent = 0;
  for (; i < p.ops.size(); i++) {
    const auto &op = p.ops[i];
    if (op.type == Operation::Type::LPE) {
      indent -= 2;
    }
    print(op, out, indent);
    out << newline;
    if (op.type == Operation::Type::LPB) {
      indent += 2;
    }
  }
}

size_t ProgramUtil::hash(const Program &p) {
  size_t h = 0;
  for (const auto &op : p.ops) {
    if (op.type != Operation::Type::NOP) {
      h = (h * 3) + hash(op);
    }
  }
  return h;
}

size_t ProgramUtil::hash(const Operation &op) {
  auto &meta = Operation::Metadata::get(op.type);
  size_t h = static_cast<size_t>(op.type);
  if (meta.num_operands > 0) {
    h = (5 * h) + hash(op.target);
  }
  if (meta.num_operands > 1) {
    h = (7 * h) + hash(op.source);
  }
  return h;
}

size_t ProgramUtil::hash(const Operand &op) {
  return (11 * static_cast<size_t>(op.type)) + op.value.hash();
}

void throwInvalidLoop(const Program &p) {
  throw std::runtime_error("invalid loop");
}

void ProgramUtil::validate(const Program &p) {
  // validate number of open/closing loops
  int64_t open_loops = 0;
  auto it = p.ops.begin();
  while (it != p.ops.end()) {
    if (it->type == Operation::Type::LPB) {
      open_loops++;
    } else if (it->type == Operation::Type::LPE) {
      if (open_loops == 0) {
        throwInvalidLoop(p);
      }
      open_loops--;
    }
    it++;
  }
  if (open_loops != 0) {
    throwInvalidLoop(p);
  }
}

void swapCells(Operand &o, int64_t old_cell, int64_t new_cell) {
  if (o == Operand(Operand::Type::DIRECT, old_cell)) {
    o.value = new_cell;
  } else if (o == Operand(Operand::Type::DIRECT, new_cell)) {
    o.value = old_cell;
  }
}

void ProgramUtil::avoidNopOrOverflow(Operation &op) {
  if (op.source.type == Operand::Type::CONSTANT) {
    if (op.source.value == 0 &&
        (op.type == Operation::Type::ADD || op.type == Operation::Type::SUB ||
         op.type == Operation::Type::LPB)) {
      op.source.value = 1;
    }
    if ((op.source.value == 0 || op.source.value == 1) &&
        (op.type == Operation::Type::MUL || op.type == Operation::Type::DIV ||
         op.type == Operation::Type::DIF || op.type == Operation::Type::DIR ||
         op.type == Operation::Type::MOD || op.type == Operation::Type::POW ||
         op.type == Operation::Type::GCD || op.type == Operation::Type::BIN)) {
      op.source.value = 2;
    }
  } else if (op.source.type == Operand::Type::DIRECT) {
    if ((op.source.value == op.target.value) &&
        (op.type == Operation::Type::MOV || op.type == Operation::Type::DIV ||
         op.type == Operation::Type::DIF || op.type == Operation::Type::DIR ||
         op.type == Operation::Type::MOD || op.type == Operation::Type::GCD ||
         op.type == Operation::Type::BIN)) {
      op.target.value += Number::ONE;
    }
  }
}

std::string ProgramUtil::getProgramsDir(char domain) {
  std::string dir;
  switch (domain) {
    case 'A':
      dir = "oeis";
      break;
    case 'P':
      dir = "prg";
      break;
    case 'V':
      dir = "virt";
      break;
    default:
      throw std::runtime_error("Unknown domain: " + std::string(1, domain));
  }
  return Setup::getProgramsHome() + dir + FILE_SEP;
}

std::string ProgramUtil::dirStr(UID id) {
  std::stringstream s;
  s << std::setw(3) << std::setfill('0') << (id.number() / 1000);
  return s.str();
}

std::string ProgramUtil::getProgramPath(UID id, bool local) {
  if (local || id.domain() == 'U') {
    return Setup::getProgramsHome() + "local" + FILE_SEP + id.string() + ".asm";
  } else {
    auto dir = getProgramsDir(id.domain());
    return dir + dirStr(id) + FILE_SEP + id.string() + ".asm";
  }
}

int64_t ProgramUtil::getOffset(const Program &p) {
  return p.getDirective("offset", 0);
}

int64_t ProgramUtil::setOffset(Program &p, int64_t offset) {
  const int64_t current = p.getDirective("offset", 0);
  const int64_t delta = offset - current;
  if (delta > 0) {
    p.ops.insert(p.ops.begin(),
                 Operation(Operation::Type::SUB,
                           Operand(Operand::Type::DIRECT, Program::INPUT_CELL),
                           Operand(Operand::Type::CONSTANT, delta)));
  } else if (delta < 0) {
    p.ops.insert(p.ops.begin(),
                 Operation(Operation::Type::ADD,
                           Operand(Operand::Type::DIRECT, Program::INPUT_CELL),
                           Operand(Operand::Type::CONSTANT, -delta)));
  }
  if (offset != 0) {
    p.directives["offset"] = offset;
  } else {
    p.directives.erase("offset");
  }
  return delta;
}

int64_t ProgramUtil::getLoopDepth(const Program &p, int64_t pos) {
  int64_t depth = 0;
  for (int64_t i = 0; i < pos; i++) {
    if (p.ops[i].type == Operation::Type::LPB) {
      depth++;
    } else if (p.ops[i].type == Operation::Type::LPE) {
      depth--;
    }
  }
  return depth;
}
