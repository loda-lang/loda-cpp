#include "mutator.hpp"

#include "program_util.hpp"

#define CONSTANTS_START -100
#define CONSTANTS_END 1000

Mutator::Mutator(const Stats &stats) : found_programs(stats.found_programs) {
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
      pos + 1 < program.ops.size()) {
    pos++;
  }
  if (program.ops[pos].type == Operation::Type::LPE && pos > 0) {
    pos--;
  }
  return pos;
}

int64_t Mutator::getRandomProgramId() {
  // TODO: avoid duplicate function
  int64_t id;
  do {
    id = Random::get().gen() % found_programs.size();
  } while (!found_programs[id]);
  return id;
}

void Mutator::mutateRandom(Program &program) {
  Log::get().info("Mutating program randomly");

  // mutate existing operations
  if (!program.ops.empty()) {
    int64_t num_mutations = (program.ops.size() / 5) + 1;
    num_mutations = (Random::get().gen() % num_mutations) + 1;
    for (; num_mutations > 0; num_mutations--) {
      Operation &op = program.ops[getRandomPos(program)];
      if (ProgramUtil::isArithmetic(op.type)) {
        op.comment = "mutated from " + ProgramUtil::operationToString(op);
        op.type = operation_types.at(operation_types_dist(Random::get().gen));
        if ((Random::get().gen() % 4) != 0) {
          op.source = Operand(Operand::Type::CONSTANT,
                              constants.at(constants_dist(Random::get().gen)));
        } else {
          // TODO
        }
        // TODO

      } else if (op.type == Operation::Type::SEQ) {
        op.comment = "mutated from " + ProgramUtil::operationToString(op);
        op.source.value = getRandomProgramId();
      }
    }
  }

  // add new operations
  // TODO
}

void Mutator::mutateConstants(const Program &program, size_t num_results,
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
