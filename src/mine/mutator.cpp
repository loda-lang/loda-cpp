#include "mine/mutator.hpp"

#include "lang/program_util.hpp"

#define CONSTANTS_START -100
#define CONSTANTS_END 1000

Mutator::Mutator(const Stats &stats, double mutation_rate, bool mutate_comment)
    : random_program_ids(stats),
      mutation_rate(mutation_rate),
      mutate_comment(mutate_comment) {
  // initialize constants distribution from stats
  constants.resize(CONSTANTS_END - CONSTANTS_START + 1);
  for (int64_t i = 0; i <= CONSTANTS_END - CONSTANTS_START; i++) {
    constants[i] = i + CONSTANTS_START;
  }
  constants_dist = constantsDist(constants, stats);

  // initialize operation types distribution from stats
  for (auto type : Operation::Types) {
    if (ProgramUtil::isArithmetic(type)) {
      operation_types.push_back(type);
    }
  }
  operation_types_dist = operationDist(stats, operation_types);
}

int64_t getRandomPos(const Program &program) {
  int64_t pos = Random::get().gen() % program.ops.size();
  if (program.ops[pos].type == Operation::Type::LPB &&
      static_cast<size_t>(pos + 1) < program.ops.size()) {
    pos++;
  }
  if (program.ops[pos].type == Operation::Type::LPE && pos > 0) {
    pos--;
  }
  return pos;
}

void Mutator::mutateRandom(Program &program) {
  int64_t num_mutations, pos;
  size_t i;

  // get number of used memory cells
  std::unordered_set<int64_t> used_cells;
  int64_t num_cells = 0;
  if (!ProgramUtil::getUsedMemoryCells(program, used_cells, num_cells, -1)) {
    // If we can't determine used cells, fall back to direct cell method
    num_cells = ProgramUtil::getLargestDirectMemoryCell(program);
  }
  num_cells++;  // convert from largest index to count
  static const int64_t new_cells = 2;

  // calculate the number of mutations to apply
  num_mutations = static_cast<int64_t>(program.ops.size() * mutation_rate) + 1;
  num_mutations = Random::get().gen() % num_mutations;  // could be zero
  if (mutation_rate > 0.0) {
    num_mutations++;  // at least one mutation
  }

  // mutate only operations with comments?
  if (mutate_comment) {
    tmp_comment_positions.clear();
    for (i = 0; i < program.ops.size(); i++) {
      if (!program.ops[i].comment.empty()) {
        tmp_comment_positions.push_back(i);
      }
    }
    if (tmp_comment_positions.empty()) {
      throw std::runtime_error("missing program comments for mutation");
    }
  }

  // mutate existing operations or add new ones
  static const Operation add_zero(Operation::Type::ADD,
                                  Operand(Operand::Type::DIRECT, 0),
                                  Operand(Operand::Type::CONSTANT, 0));
  for (; num_mutations > 0; num_mutations--) {
    // choose whether to add a new operation or mutate an existing one
    if (Random::get().gen() % 2 == 0 || program.ops.empty()) {
      // add new operation
      if (mutate_comment) {
        i = Random::get().gen() % tmp_comment_positions.size();
        pos = tmp_comment_positions[i];
        for (; i < tmp_comment_positions.size(); i++) {
          tmp_comment_positions[i]++;
        }
      } else {
        pos = Random::get().gen() % program.ops.size();
      }
      program.ops.insert(program.ops.begin() + pos, add_zero);
    } else {
      // mutate existing operation
      if (mutate_comment) {
        pos = tmp_comment_positions[Random::get().gen() %
                                    tmp_comment_positions.size()];
      } else {
        pos = getRandomPos(program);
      }
    }
    mutateOperation(program.ops[pos], num_cells, new_cells);
  }
}

void Mutator::mutateOperation(Operation &op, int64_t num_cells,
                              int64_t new_cells) {
  if (ProgramUtil::isArithmetic(op.type)) {
    // op.comment = "mutated from " + ProgramUtil::operationToString(op);
    op.type = operation_types.at(operation_types_dist(Random::get().gen));
    if ((Random::get().gen() % 3) != 0) {
      op.source = Operand(Operand::Type::CONSTANT,
                          constants.at(constants_dist(Random::get().gen)));
    } else {
      op.source = Operand(Operand::Type::DIRECT,
                          Random::get().gen() % (num_cells + new_cells));
    }
    if (op.type == Operation::Type::MOV &&
        Random::get().gen() % 4 > 0) {  // magic number
      // avoid overwriting
      op.target = Operand(Operand::Type::DIRECT,
                          (Random::get().gen() % new_cells) + num_cells);
    } else {
      op.target = Operand(Operand::Type::DIRECT,
                          Random::get().gen() % (num_cells + new_cells));
    }
    ProgramUtil::avoidNopOrOverflow(op);
  } else if (op.type == Operation::Type::SEQ) {
    // op.comment = "mutated from " + ProgramUtil::operationToString(op);
    op.source =
        Operand(Operand::Type::CONSTANT, random_program_ids.get().castToInt());
  }
}

void Mutator::mutateCopiesRandom(const Program &program, size_t num_results,
                                 std::stack<Program> &result) {
  for (size_t i = 0; i < num_results; i++) {
    auto p = program;
    mutateRandom(p);
    result.push(p);
  }
}

void Mutator::mutateCopiesConstants(const Program &program, size_t num_results,
                                    std::stack<Program> &result) {
  std::vector<size_t> indices;
  for (size_t i = 0; i < program.ops.size(); i++) {
    if (Operation::Metadata::get(program.ops[i].type).num_operands == 2 &&
        program.ops[i].source.type == Operand::Type::CONSTANT) {
      indices.resize(indices.size() + 1);
      indices[indices.size() - 1] = i;
    }
  }
  if (indices.empty()) {
    return;
  }
  int64_t var = std::max<int64_t>(1, num_results / indices.size());
  for (size_t i : indices) {
    if (program.ops[i].source.value.getNumUsedWords() > 1) {
      continue;
    }
    int64_t b = program.ops[i].source.value.asInt();
    int64_t s = b - std::min<int64_t>(var / 2, b);
    for (int64_t v = s; v <= s + var; v++) {
      if (v != b) {
        auto p = program;
        p.ops[i].source.value = Number(v);
        result.push(p);
      }
    }
  }
}
