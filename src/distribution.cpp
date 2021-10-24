#include "distribution.hpp"

#include "util.hpp"

std::discrete_distribution<> uniformDist(size_t size) {
  std::vector<double> p(size, 100.0);
  return std::discrete_distribution<>(p.begin(), p.end());
}

std::discrete_distribution<> constantsDist(const std::vector<Number> &constants,
                                           const Stats &stats) {
  std::vector<double> p(constants.size());
  for (size_t i = 0; i < constants.size(); i++) {
    int64_t rate = stats.num_constants.at(constants[i]);
    if (rate <= 0) {
      Log::get().error(
          "Unexpected stats for constant: " + constants[i].to_string(), true);
    }
    p[i] = rate;
  }
  return std::discrete_distribution<>(p.begin(), p.end());
}

std::discrete_distribution<> operationDist(
    const Stats &stats, const std::vector<Operation::Type> &operation_types) {
  std::vector<double> p(operation_types.size());
  for (size_t i = 0; i < operation_types.size(); i++) {
    int64_t rate =
        stats.num_ops_per_type.at(static_cast<size_t>(operation_types[i]));
    rate = std::max<int64_t>(rate / 1000, 1);
    p[i] = rate;
  }
  return std::discrete_distribution<>(p.begin(), p.end());
}
