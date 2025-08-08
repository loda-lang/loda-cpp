#include "eval/evaluator_vir.hpp"

// #include <iostream>
#include <limits>

#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"
#include "sys/log.hpp"

const int64_t MAX_EMBEDDED_PROGRAMS = 10;

VirtualEvaluator::VirtualEvaluator(const Settings &settings)
    : interpreter(settings), is_debug(Log::get().level == Log::Level::DEBUG) {}

int64_t extractEmbedded(Program &refactored, Program &extracted,
                        int64_t dummy_id, const EmbeddedSequenceProgram &info) {
  int64_t overhead = 0;
  // extract the embedded sequence program
  extracted.ops = {refactored.ops.begin() + info.start_pos,
                   refactored.ops.begin() + info.end_pos + 1};
  // update the input cell in the extracted program
  ProgramUtil::swapDirectOperandCells(extracted, info.input_cell,
                                      Program::INPUT_CELL);
  if (info.output_cell != Program::OUTPUT_CELL &&
      info.output_cell != info.input_cell) {
    extracted.push_back(Operation::Type::MOV, Operand::Type::DIRECT,
                        Number(Program::OUTPUT_CELL), Operand::Type::DIRECT,
                        Number(info.output_cell));
    overhead -= 1;  // account for the mov operation
  }
  // remove the extracted program from the refactored program
  refactored.ops.erase(refactored.ops.begin() + info.start_pos,
                       refactored.ops.begin() + info.end_pos);
  // insert seq operation to call the extracted program
  refactored.ops[info.start_pos] =
      Operation(Operation::Type::SEQ,
                Operand(Operand::Type::DIRECT, Number(info.output_cell)),
                Operand(Operand::Type::CONSTANT, Number(dummy_id)));
  overhead -= 1;  // account for the seq operation
  // move the result to the output cell
  if (info.input_cell != info.output_cell) {
    refactored.ops.insert(
        refactored.ops.begin() + info.start_pos,
        Operation(Operation::Type::MOV,
                  Operand(Operand::Type::DIRECT, Number(info.output_cell)),
                  Operand(Operand::Type::DIRECT, Number(info.input_cell))));
    overhead -= 1;  // account for the mov operation
  }
  return overhead;
}

bool VirtualEvaluator::init(const Program &p) {
  interpreter.clearCaches();
  refactored = p;
  int64_t dummy_id = std::numeric_limits<int64_t>::max();
  Program extracted;
  auto &program_cache = interpreter.program_cache;
  int64_t num_embedded_seqs = 0;
  for (int64_t i = 0; i < MAX_EMBEDDED_PROGRAMS; i++) {
    auto found = Subprogram::findEmbeddedSequencePrograms(refactored, 3, 1, 1);
    if (found.empty()) {
      break;
    }
    auto &info = found.front();
    auto overhead = extractEmbedded(refactored, extracted, dummy_id, info);
    /*
    std::cout << "\nRefactored:" << std::endl;
    ProgramUtil::print(refactored, std::cout);
    std::cout << "\nExtracted:" << std::endl;
    ProgramUtil::print(extracted, std::cout);
    */
    program_cache.insert(dummy_id, extracted);
    program_cache.setOverhead(dummy_id, overhead);
    dummy_id--;
    num_embedded_seqs++;
  }
  if (num_embedded_seqs) {
    if (is_debug) {
      Log::get().debug("Successfully initialized virtual evaluator with " +
                       std::to_string(num_embedded_seqs) +
                       " embedded sequence program(s)");
    }
  } else {
    refactored = {};
    if (is_debug) {
      Log::get().debug("Virtual evaluation not supported");
    }
  }
  return num_embedded_seqs > 0;
}

std::pair<Number, size_t> VirtualEvaluator::eval(const Number &input) {
  tmp_memory.clear();
  tmp_memory.set(Program::INPUT_CELL, input);
  auto steps = interpreter.run(refactored, tmp_memory);
  auto output = tmp_memory.get(Program::OUTPUT_CELL);
  return {output, steps};
}

void VirtualEvaluator::reset() {
  refactored = {};
  interpreter.clearCaches();
  tmp_memory.clear();
}
