#pragma once

#include <random>

std::discrete_distribution<> EqualDist(size_t size);

std::discrete_distribution<> ExpDist(size_t size);

void printDist(const std::discrete_distribution<>& d);

std::discrete_distribution<> AddDist(const std::discrete_distribution<>& d1,
		const std::discrete_distribution<>& d2);

void MutateDist(std::discrete_distribution<>& d, std::mt19937& gen);
