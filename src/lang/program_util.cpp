#include "lang/program_util.hpp"

#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stack>

#include "lang/program.hpp"
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

size_t ProgramUtil::replaceSubprogams(Program &p, const Program &search,
                                      const Program &replace) {
  if (p.ops.empty() || search.ops.empty() || search.ops.size() > p.ops.size()) {
    return 0;
  }
  size_t count = 0;
  const size_t max_start = p.ops.size() - search.ops.size();
  for (size_t i = 0; i < max_start; i++) {
    bool matches = true;
    for (size_t j = 0; j < search.ops.size(); j++) {
      if (p.ops[i + j] != search.ops[j]) {
        matches = false;
        break;
      }
    }
    if (matches) {
      p.ops.erase(p.ops.begin() + i, p.ops.begin() + i + search.ops.size());
      p.ops.insert(p.ops.begin() + i, replace.ops.begin(), replace.ops.end());
      i += replace.ops.size() - 1;
      count++;
    }
  }
  return count;
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
              op.type == Operation::Type::SRT)) {
    return true;
  } else if (op.source.type == Operand::Type::CONSTANT &&
             op.source.value == Number::ONE &&
             ((op.type == Operation::Type::MUL ||
               op.type == Operation::Type::DIV ||
               op.type == Operation::Type::DIF ||
               op.type == Operation::Type::POW ||
               op.type == Operation::Type::BIN))) {
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
          t != Operation::Type::CLR && t != Operation::Type::SRT &&
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

bool ProgramUtil::isWritingRegion(Operation::Type t) {
  return (t == Operation::Type::CLR || t == Operation::Type::PRG ||
          t == Operation::Type::SRT);
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
                                     std::unordered_set<int64_t> &used_cells,
                                     int64_t &largest_used,
                                     int64_t max_memory) {
  for (const auto &op : p.ops) {
    int64_t region_length = 1;
    if (op.source.type == Operand::Type::INDIRECT ||
        op.target.type == Operand::Type::INDIRECT) {
      return false;
    }
    if (op.type == Operation::Type::LPB ||
        ProgramUtil::isWritingRegion(op.type)) {
      if (op.source.type == Operand::Type::CONSTANT) {
        region_length = op.source.value.asInt();
      } else {
        return false;
      }
    }
    if (region_length > max_memory && max_memory >= 0) {
      return false;
    }
    if (op.source.type == Operand::Type::DIRECT) {
      for (int64_t i = 0; i < region_length; i++) {
        used_cells.insert(op.source.value.asInt() + i);
      }
    }
    if (op.target.type == Operand::Type::DIRECT) {
      for (int64_t i = 0; i < region_length; i++) {
        used_cells.insert(op.target.value.asInt() + i);
      }
    }
  }
  const auto max = [](int64_t a, int64_t b) { return std::max(a, b); };
  largest_used = std::accumulate(used_cells.begin(), used_cells.end(), 0, max);
  return true;
}

bool ProgramUtil::getUsedUninitializedCells(const Program &p,
                                            std::set<int64_t> &initialized,
                                            std::set<int64_t> &uninitialized) {
  // find cells that are read and uninitialized
  for (const auto &op : p.ops) {
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
    if (op.type == Operation::Type::CLR) {
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

std::set<Number> ProgramUtil::getAllConstants(const Program &p,
                                              bool arithmetic_only) {
  std::set<Number> result;
  for (const auto &op : p.ops) {
    auto num_operands = Operation::Metadata::get(op.type).num_operands;
    if (num_operands > 1 && op.source.type == Operand::Type::CONSTANT &&
        (isArithmetic(op.type) || !arithmetic_only)) {
      result.insert(op.source.value);
    }
  }
  return result;
}

Number ProgramUtil::getLargestConstant(const Program &p) {
  Number largest(-1);
  for (const auto &op : p.ops) {
    if (op.source.type == Operand::Type::CONSTANT &&
        largest < op.source.value) {
      largest = op.source.value;
    }
  }
  return largest;
}

ProgramUtil::ConstantLoopInfo ProgramUtil::findConstantLoop(const Program &p) {
  // assumes that the program is optimized already
  ProgramUtil::ConstantLoopInfo info;
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

void ProgramUtil::exportToDot(const Program &p, std::ostream &out) {
  out << "digraph G {" << std::endl;

  // merge operations
  std::vector<std::vector<Operation>> merged;
  merged.push_back({});
  for (auto op : p.ops) {
    if (op.type == Operation::Type::NOP) {
      continue;
    }
    if (!merged.back().empty() && !areIndependent(op, merged.back().back())) {
      merged.push_back({});
    }
    op.comment.clear();
    merged.back().push_back(op);
  }

  // insert forks and joins
  for (size_t i = 0; i < merged.size(); i++) {
    if (merged[i].size() > 1) {
      merged.insert(merged.begin() + i,
                    {Operation(Operation::Type::NOP, {}, {}, "triangle")});
      merged.insert(merged.begin() + i + 2,
                    {Operation(Operation::Type::NOP, {}, {}, "invtriangle")});
      i += 2;
    }
  }

  // nodes
  for (size_t i = 0; i < merged.size(); i++) {
    for (size_t j = 0; j < merged[i].size(); j++) {
      std::string shape = "ellipse";
      std::string color = "black";
      std::string fontname = "courier";
      std::string label = operationToString(merged[i][j]);
      if (merged[i][j].type == Operation::Type::NOP) {
        shape = merged[i][j].comment;
        label.clear();
      } else if (merged[i][j].type == Operation::Type::MOV) {
        color = "blue";
      } else if (isArithmetic(merged[i][j].type)) {
        color = "green";
      } else {
        color = "red";
      }
      out << "  o" << i << "_" << j << " [label=\"" << label
          << "\",shape=" + shape + ",color=" + color + ",fontname=\"" +
                 fontname + "\"];"
          << std::endl;
    }
  }

  // edges
  std::stack<std::string> lpbs;
  std::vector<std::string> targets;

  for (size_t i = 0; i < merged.size(); i++) {
    for (size_t j = 0; j < merged[i].size(); j++) {
      // current source node
      std::string src = "o" + std::to_string(i) + "_" + std::to_string(j);
      // target nodes
      targets.clear();
      if (i + 1 < merged.size())  // edge to next node
      {
        for (size_t k = 0; k < merged[i + 1].size(); k++) {
          targets.push_back("o" + std::to_string(i + 1) + "_" +
                            std::to_string(k));
        }
      }
      if (merged[i][j].type == Operation::Type::LPE)  // edge back to loop start
      {
        targets.push_back(lpbs.top());
        lpbs.pop();
      }
      if (!targets.empty()) {
        out << "  " << src << " -> {";
        for (const auto &t : targets) {
          out << " " << t;
        }
        out << " }" << std::endl;
      }
      if (merged[i][j].type == Operation::Type::LPB) {
        lpbs.push("o" + std::to_string(i) + "_" + std::to_string(j));
      }
    }
  }
  out << "}" << std::endl;
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
         op.type == Operation::Type::DIF || op.type == Operation::Type::MOD ||
         op.type == Operation::Type::POW || op.type == Operation::Type::GCD ||
         op.type == Operation::Type::BIN)) {
      op.source.value = 2;
    }
  } else if (op.source.type == Operand::Type::DIRECT) {
    if ((op.source.value == op.target.value) &&
        (op.type == Operation::Type::MOV || op.type == Operation::Type::DIV ||
         op.type == Operation::Type::DIF || op.type == Operation::Type::MOD ||
         op.type == Operation::Type::GCD || op.type == Operation::Type::BIN)) {
      op.target.value += Number::ONE;
    }
  }
}
