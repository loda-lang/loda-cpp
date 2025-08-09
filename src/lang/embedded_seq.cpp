#include "lang/embedded_seq.hpp"

#include <stdexcept>

#include "lang/comments.hpp"
#include "lang/program_util.hpp"

// Helper class for tracking cell usage in embedded sequence programs
class CellTracker {
 public:
  int64_t input_cell = -1;
  int64_t output_cell = -1;
  int64_t loops = 0;
  int64_t open_loops = 0;
  std::set<int64_t> written_cells;
  std::map<int64_t, int64_t> safely_written_cells;
  std::set<int64_t> overridden_cells;

  bool read(int64_t cell, bool after) {
    if (after) {
      if (written_cells.find(cell) != written_cells.end() &&
          overridden_cells.find(cell) == overridden_cells.end()) {
        if (output_cell == -1 &&
            safely_written_cells.find(cell) != safely_written_cells.end()) {
          output_cell = cell;
        } else if (cell != output_cell) {
          return false;  // multiple output cells found or not safely written
        }
      }
    } else {
      if (input_cell == -1) {
        input_cell = cell;
      } else if (cell != input_cell && safely_written_cells.find(cell) ==
                                           safely_written_cells.end()) {
        return false;  // multiple input cells found
      }
    }
    return true;
  }

  bool update(const Operation &op, bool after) {
    if (!after) {
      if (op.type == Operation::Type::LPB) {
        loops++;
        open_loops++;
        for (auto &it : safely_written_cells) {
          it.second++;
        }
      } else if (op.type == Operation::Type::LPE) {
        open_loops--;
        for (auto it = safely_written_cells.begin();
             it != safely_written_cells.end();) {
          if (it->second > 0) {
            it->second--;
            it++;
          } else {
            it = safely_written_cells.erase(it);
          }
        }
      }
    }
    const auto meta = Operation::Metadata::get(op.type);
    // check the source cell
    if (meta.num_operands > 1 && op.source.type == Operand::Type::DIRECT) {
      if (!read(op.source.value.asInt(), after)) {
        return false;
      }
    }
    // check the target cell
    if (meta.num_operands > 0 && op.target.type == Operand::Type::DIRECT) {
      auto target = op.target.value.asInt();
      if (meta.is_reading_target && !read(op.target.value.asInt(), after)) {
        return false;
      }
      if (meta.is_writing_target) {
        if (after) {
          overridden_cells.insert(target);
        } else {
          written_cells.insert(target);
          // safely written only if mov and outside of loops
          if (!meta.is_reading_target) {
            if (safely_written_cells.find(target) ==
                safely_written_cells.end()) {
              safely_written_cells[target] = 0;
            }
          }
        }
      }
    }
    return open_loops >= 0 || after;
  }

  void reset() {
    input_cell = -1;
    output_cell = -1;
    loops = 0;
    open_loops = 0;
    written_cells.clear();
    safely_written_cells.clear();
    overridden_cells.clear();
  }
};

bool collectAffectedOperations(const Program &p, int64_t start, int64_t end,
                               std::vector<std::vector<Operation>> &result) {
  result.clear();
  const int64_t num_ops = p.ops.size();
  if (end + 1 >= num_ops) {
    return true;
  }
  result.emplace_back();
  for (int64_t i = end + 1; i < num_ops; i++) {
    const auto &op = p.ops[i];
    result[0].push_back(op);
    if (op.type == Operation::Type::LPE) {
      auto loop = ProgramUtil::getEnclosingLoop(p, i);
      if (loop.first < 0) {
        throw std::runtime_error("unmatched loop end operation at " +
                                 std::to_string(i));
      }
      auto copy = result[0];
      for (int64_t j = loop.first; j < start; j++) {
        copy.push_back(p.ops[j]);
      }
      result.push_back(copy);
    }
  }
  return true;
}

std::vector<EmbeddedSeq::Result> EmbeddedSeq::findEmbeddedSequencePrograms(
    const Program &p, int64_t min_length, int64_t min_loops_outside,
    int64_t min_loops_inside) {
  std::vector<EmbeddedSeq::Result> result;
  const int64_t num_ops = p.ops.size();
  if (num_ops == 0 || ProgramUtil::hasIndirectOperand(p)) {
    return result;
  }
  CellTracker tracker;
  std::vector<std::vector<Operation>> affected;
  for (int64_t start = 0; start + 1 < num_ops; start++) {
    if (ProgramUtil::getLoopDepth(p, start) < min_loops_outside) {
      continue;  // skip if not enough loops outside
    }
    tracker.reset();
    int64_t end = start - 1;
    int64_t output_cell = -1;
    for (int64_t i = start; i < num_ops; i++) {
      bool ok = tracker.update(p.ops[i], false);
      if (!ok) {
        break;
      }
      ok = ok && tracker.loops >= min_loops_inside && tracker.open_loops == 0;
      if (ok) {
        // check rest of the program
        collectAffectedOperations(p, start, i, affected);
        tracker.output_cell = -1;
        for (size_t t = 0; t < affected.size(); t++) {
          tracker.overridden_cells.clear();
          for (const auto &op : affected[t]) {
            if (!tracker.update(op, true)) {
              ok = false;
              break;
            }
          }
          const bool is_loop_thread = t > 0;
          if (is_loop_thread &&
              tracker.written_cells.find(tracker.input_cell) !=
                  tracker.written_cells.end() &&
              tracker.overridden_cells.find(tracker.input_cell) ==
                  tracker.overridden_cells.end()) {
            ok = false;
          }
        }
      }
      if (ok) {
        end = i;
        output_cell = tracker.output_cell;
      }
    }
    if (start + min_length <= end && tracker.input_cell != -1 &&
        output_cell != -1 && (result.empty() || result.back().end_pos != end)) {
      EmbeddedSeq::Result esp;
      esp.start_pos = start;
      esp.end_pos = end;
      esp.input_cell = tracker.input_cell;
      esp.output_cell = output_cell;
      result.emplace_back(esp);
    }
  }
  return result;
}

int64_t EmbeddedSeq::annotateEmbeddedSequencePrograms(
    Program &main, int64_t min_length, int64_t min_loops_outside,
    int64_t min_loops_inside) {
  Comments::removeComments(main);
  auto embs = findEmbeddedSequencePrograms(main, min_length, min_loops_outside,
                                           min_loops_outside);
  for (size_t i = 0; i < embs.size(); i++) {
    auto &esp = embs[i];
    main.ops.at(esp.start_pos).comment =
        "begin of embedded sequence " + std::to_string(i + 1) + " with input " +
        ProgramUtil::operandToString(
            Operand(Operand::Type::DIRECT, esp.input_cell));
    main.ops.at(esp.end_pos).comment =
        "end of embedded sequence " + std::to_string(i + 1) + " with output " +
        ProgramUtil::operandToString(
            Operand(Operand::Type::DIRECT, esp.output_cell));
  }
  return embs.size();
}
