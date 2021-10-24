#pragma once

#include <random>

#include "stats.hpp"

std::discrete_distribution<> uniformDist(size_t size);

std::discrete_distribution<> constantsDist(const std::vector<Number> &constants,
                                           const Stats &stats);

std::discrete_distribution<> operationDist(
    const Stats &stats, const std::vector<Operation::Type> &operation_types);
