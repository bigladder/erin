// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_RANDOM_H
#define ERIN_RANDOM_H
#include <vector>
#include <random>

namespace erin
{

// PUBLIC
enum class RandomType
{
    FixedRandom,
    FixedSeries,
    RandomFromSeed,
    RandomFromClock,
};

// PUBLIC
struct FixedRandom
{
    double FixedValue = 0.0;
    double operator()() const;
};

// PUBLIC
struct FixedSeries
{
    size_t Idx = 0;
    std::vector<double> Series;
    double operator()();
};

// PUBLIC
struct Random
{
    unsigned int Seed = 0;
    std::mt19937 Generator;
    std::uniform_real_distribution<double> Distribution {0.0, 1.0};
    double operator()();
};

// PUBLIC
Random CreateRandom();

// PUBLIC
Random CreateRandomWithSeed(unsigned int seed);

} // namespace erin

#endif
