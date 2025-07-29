#include "eval/evaluator_vir.hpp"

#include <iostream>
#include <limits>

#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"

const int64_t MAX_EMBEDDED_PROGRAMS = 10;

VirtualEvaluator::VirtualEvaluator(const Settings &settings)
    : interpreter(settings) {}

void extractEmbedded(Program &refactored, Program &extracted, int64_t dummy_id,
                     const EmbeddedSequenceProgram &info) {
  // extract the embedded sequence program
  extracted.ops = {refactored.ops.begin() + info.start_pos,
                   refactored.ops.begin() + info.end_pos + 1};
  // update the input cell in the extracted program
  ProgramUtil::swapDirectOperandCells(extracted, info.input_cell,
                                      Program::INPUT_CELL);
  if (info.output_cell != Program::OUTPUT_CELL) {
    extracted.push_back(Operation::Type::MOV, Operand::Type::DIRECT,
                        Number(Program::OUTPUT_CELL), Operand::Type::DIRECT,
                        Number(info.output_cell));
  }
  // remove the extracted program from the refactored program
  refactored.ops.erase(refactored.ops.begin() + info.start_pos,
                       refactored.ops.begin() + info.end_pos);
  // insert seq operation to call the extracted program
  refactored.ops[info.start_pos] =
      Operation(Operation::Type::SEQ,
                Operand(Operand::Type::DIRECT, Number(info.input_cell)),
                Operand(Operand::Type::CONSTANT, Number(dummy_id)));
  // move the result to the output cell
  if (info.input_cell != info.output_cell) {
    refactored.ops.insert(
        refactored.ops.begin() + info.start_pos + 1,
        Operation(Operation::Type::MOV,
                  Operand(Operand::Type::DIRECT, Number(info.output_cell)),
                  Operand(Operand::Type::DIRECT, Number(info.input_cell))));
  }
}

bool VirtualEvaluator::init(const Program &p) {
  interpreter.clearCaches();
  refactored = p;
  bool changed = false;
  int64_t dummy_id = std::numeric_limits<int64_t>::max();
  Program extracted;
  for (int64_t i = 0; i < MAX_EMBEDDED_PROGRAMS; i++) {
    auto found = Subprogram::findEmbeddedSequencePrograms(refactored, 3, 1, 1);
    if (found.empty()) {
      break;
    }
    auto &info = found.front();
    extractEmbedded(refactored, extracted, dummy_id, info);
    interpreter.program_cache.insert(dummy_id, extracted);
    dummy_id--;
    changed = true;
  }
  if (!changed) {
    refactored = {};
  }
  return changed;
}

std::pair<Number, size_t> VirtualEvaluator::eval(const Number &input) {
  tmp_memory.clear();
  tmp_memory.set(Program::INPUT_CELL, input);
  auto steps = interpreter.run(refactored, tmp_memory);
  auto output = tmp_memory.get(Program::OUTPUT_CELL);
  return {output, steps};
}
