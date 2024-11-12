// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include "erin/random.h"
#include <random>
#include <chrono>

namespace erin
{
double FixedRandom::operator()() const { return fixed_value; }

double FixedSeries::operator()()
{
    if (series.size() == 0)
    {
        return 0.0;
    }
    index = index % series.size();
    double result = series[index];
    ++index;
    return result;
}

double Random::operator()() { return distribution(generator); }

Random create_random()
{
    Random r {};
    auto now = std::chrono::high_resolution_clock::now();
    auto d = now.time_since_epoch();
    constexpr unsigned int range =
        std::numeric_limits<unsigned int>::max() - std::numeric_limits<unsigned int>::min();
    r.seed = d.count() % range;
    r.generator.seed(r.seed);
    return r;
}

Random create_random_with_seed(unsigned int seed)
{
    Random r {};
    r.seed = seed;
    r.generator.seed(seed);
    return r;
}
} // namespace erin
