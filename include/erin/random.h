// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_RANDOM_H
#define ERIN_RANDOM_H
#include <vector>
#include <random>

namespace erin
{

enum class RandomType
{
    fixed_random,
    fixed_series,
    random_from_seed,
    random_from_clock,
};

struct FixedRandom
{
    double fixed_value = 0.0;
    double operator()() const;
};

struct FixedSeries
{
    size_t index = 0;
    std::vector<double> series;
    double operator()();
};

struct Random
{
    unsigned int seed = 0;
    std::mt19937 generator;
    std::uniform_real_distribution<double> distribution {0.0, 1.0};
    double operator()();
};

Random create_random();

Random create_random_with_seed(unsigned int seed);

} // namespace erin

#endif
