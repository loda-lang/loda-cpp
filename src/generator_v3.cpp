#include "generator_v3.hpp"

#include "generator.hpp"
#include "log.hpp"
#include "stats.hpp"

inline size_t getIndex(size_t pos, size_t len) {
  if (pos >= len) {
  }
  return (((len - 1) * len) / 2) + pos;
}

GeneratorV3::GeneratorV3(const Config &config, const Stats &stats)
    : Generator(config, stats) {
  size_t i;
  std::vector<double> probs;

  // resize operation distribution vector
  size_t max_len = 0;
  for (size_t len = 0; len < stats.num_programs_per_length.size(); len++) {
    if (stats.num_programs_per_length[len] > 0) {
      max_len = len;
    }
  }
  if (max_len == 0) {
    Log::get().error("Maximum  program length is zero", true);
  }
  operation_dists.resize(getIndex(max_len - 1, max_len) + 1);

  // initialize operation distributions
  OpProb p;
  for (const auto &it : stats.num_operation_positions) {
    i = getIndex(it.first.pos, it.first.len);
    auto &op_dist = operation_dists.at(i);
    p.operation = it.first.op;
    p.partial_sum = it.second;
    if (!op_dist.empty()) {
      p.partial_sum += op_dist.back().partial_sum;
    }
    op_dist.push_back(p);
  }

  // program length distribution
  probs.resize(stats.num_programs_per_length.size());
  for (i = 0; i < stats.num_programs_per_length.size(); i++) {
    probs[i] = stats.num_programs_per_length[i];
  }
  length_dist = std::discrete_distribution<>(probs.begin(), probs.end());
}

Program GeneratorV3::generateProgram() {
  Program p;
  const size_t len = length_dist(Random::get().gen);
  size_t num_loops = 0;
  Operation::Type op_type;
  for (size_t pos = 0; pos < len; pos++) {
    size_t sample, left, right;
    auto &op_dist = operation_dists.at(getIndex(pos, len));
    if (op_dist.empty() || op_dist.back().partial_sum == 0) {
      Log::get().error("Invalid operation distribution at position " +
                           std::to_string(pos) + "," + std::to_string(len),
                       true);
    }
    sample = Random::get().gen() % op_dist.back().partial_sum;
    left = 0;
    right = op_dist.size() - 1;
    while (right - left > 1) {
      size_t mid = (left + right) / 2;
      if (sample > op_dist.at(mid).partial_sum) {
        left = mid;
      } else {
        right = mid;
      }
    }
    op_type = op_dist.at(left).operation.type;
    if (op_type != Operation::Type::LPE || num_loops > 0) {
      p.ops.push_back(op_dist[left].operation);
      if (op_type == Operation::Type::LPB) {
        num_loops++;
      } else if (op_type == Operation::Type::LPE) {
        num_loops--;
      }
    }
  }
  while (num_loops > 0) {
    p.ops.push_back(Operation::Type::LPE);
    num_loops--;
  }
  applyPostprocessing(p);
  return p;
}

std::pair<Operation, double> GeneratorV3::generateOperation() {
  std::pair<Operation, double> next_op;
  while (true) {
    auto &op_dist =
        operation_dists.at(Random::get().gen() % operation_dists.size());
    if (!op_dist.empty()) {
      next_op.first =
          op_dist.at(Random::get().gen() % op_dist.size()).operation;
      next_op.second = (double)(Random::get().gen() % 100) / 100.0;
      return next_op;
    }
  }
  return {};
}
