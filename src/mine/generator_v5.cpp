#include "mine/generator_v5.hpp"

#include <algorithm>

#include "lang/program_util.hpp"
#include "mine/generator.hpp"
#include "mine/stats.hpp"

GeneratorV5::GeneratorV5(const Config &config, const Stats &stats)
    : Generator(config, stats),
      blocks(stats.blocks),
      dist(blocks.rates.begin(), blocks.rates.end()),
      length(config.length) {
  // free rates in blocks object (not needed anymore)
  blocks.rates.clear();
}

Program GeneratorV5::generateProgram() {
  Program block;
  Program result;
  Blocks::Interface int1, int2;
  int1.inputs.insert(0);
  int1.all.insert(0);
  size_t depth = 0;
  while (true) {
    // randomly inject lpb
    if (Random::get().gen() % 5 == 0)  // magic number
    {
      int64_t r = Random::get().gen() % int1.all.size();
      for (auto it : int1.all) {
        if (r-- == 0) {
          result.push_back(Operation::Type::LPB, Operand::Type::DIRECT, it,
                           Operand::Type::CONSTANT, 1);
          depth++;
          break;
        }
      }
    }

    // choose block
    for (size_t i = 0; i < 1000; i++) {
      block = blocks.getBlock(dist(Random::get().gen));
      int2 = Blocks::Interface(block);
      if (std::includes(int1.all.begin(), int1.all.end(), int2.inputs.begin(),
                        int2.inputs.end())) {
        break;
      }
    }

    // append block
    for (auto &op : block.ops) {
      result.ops.push_back(op);
      int1.extend(op);
    }

    // randomly inject lpe
    if (depth > 0 && Random::get().gen() % 2 == 0) {
      result.push_back(Operation::Type::LPE, Operand::Type::CONSTANT, 0,
                       Operand::Type::CONSTANT, 0);
      depth--;
    }

    // enough?
    if (result.ops.size() >= length && Random::get().gen() % 2 == 0) {
      break;
    }
  }

  // close loops
  while (depth > 0) {
    result.push_back(Operation::Type::LPE, Operand::Type::CONSTANT, 0,
                     Operand::Type::CONSTANT, 0);
    depth--;
  }

  return result;
}

std::pair<Operation, double> GeneratorV5::generateOperation() {
  throw std::runtime_error("unsupported operation");
}

bool GeneratorV5::supportsRestart() const { return true; }

bool GeneratorV5::isFinished() const { return false; };
