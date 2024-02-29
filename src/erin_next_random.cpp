#include "erin_next/erin_next_random.h"
#include <random>
#include <chrono>

namespace erin_next
{
	double
	FixedRandom::operator()() const
	{
		return FixedValue;
	}

	double
	FixedSeries::operator()()
	{
		if (Series.size() == 0)
		{
			return 0.0;
		}
		Idx = Idx % Series.size();
		double result = Series[Idx];
		++Idx;
		return result;
	}

	double
	Random::operator()()
	{
		return Distribution(Generator);
	}

	Random
	CreateRandom()
	{
		Random r{};
		auto now = std::chrono::high_resolution_clock::now();
		auto d = now.time_since_epoch();
		constexpr unsigned int range = std::numeric_limits<unsigned int>::max()
			- std::numeric_limits<unsigned int>::min();
		r.Seed = d.count() % range;
		r.Generator.seed(r.Seed);
		return r;
	}

	Random
	CreateRandomWithSeed(unsigned int seed)
	{
		Random r{};
		r.Seed = seed;
		r.Generator.seed(seed);
		return r;
	}
}
