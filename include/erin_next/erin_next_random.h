/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_RANDOM_H
#define ERIN_RANDOM_H
#include <vector>
#include <random>

namespace erin
{
	enum class RandomType
	{
		FixedRandom,
		FixedSeries,
		RandomFromSeed,
		RandomFromClock,
	};

	struct FixedRandom
	{
		double FixedValue = 0.0;
		double
		operator()() const;
	};

	struct FixedSeries
	{
		size_t Idx = 0;
		std::vector<double> Series;
		double
		operator()();
	};

	struct Random
	{
		unsigned int Seed = 0;
		std::mt19937 Generator;
		std::uniform_real_distribution<double> Distribution{0.0, 1.0};
		double
		operator()();
	};

	Random
	CreateRandom();

	Random
	CreateRandomWithSeed(unsigned int seed);
} // namespace erin

#endif
