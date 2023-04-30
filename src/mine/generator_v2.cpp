#include "mine/generator_v2.hpp"

#include "mine/generator.hpp"
#include "mine/stats.hpp"

GeneratorV2::GeneratorV2(const Config &config, const Stats &stats)
    : Generator(config, stats) {
  size_t i;
  std::vector<double> probs;

  // program length distribution
  probs.resize(stats.num_programs_per_length.size());
  for (i = 0; i < stats.num_programs_per_length.size(); i++) {
    probs[i] = stats.num_programs_per_length[i];
  }
  length_dist = std::discrete_distribution<>(probs.begin(), probs.end());

  // operations distribution
  operations.resize(stats.num_operations.size());
  probs.resize(stats.num_operations.size());
  i = 0;
  for (const auto &it : stats.num_operations) {
    operations[i] = it.first;
    probs[i] = it.second;
    i++;
  }
  operation_dist = std::discrete_distribution<>(probs.begin(), probs.end());
}

std::pair<Operation, double> GeneratorV2::generateOperation() {
  std::pair<Operation, double> next_op;
  next_op.first = operations.at(operation_dist(Random::get().gen));
  next_op.second = (double)(Random::get().gen() % 100) / 100.0;
  return next_op;
}

Program GeneratorV2::generateProgram() {
  Program p;
  int64_t length = length_dist(Random::get().gen);
  generateStateless(p, length);
  applyPostprocessing(p);
  return p;
}

bool GeneratorV2::supportsRestart() const { return true; }
