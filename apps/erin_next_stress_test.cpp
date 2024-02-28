#include "erin_next/erin_next.h"
#include <iostream>
#include <string>
#include <stdint.h>
#include <assert.h>

using namespace erin_next;

int
main(int argc, char** argv)
{
	size_t numComponents = 5000;
	size_t numHours = 8760;
	if (argc == 3)
	{
		numComponents = static_cast<size_t>(std::stoll(std::string{ argv[1] }));
		numHours = static_cast<size_t>(std::stoll(std::string{ argv[2] }));
	}
	std::cout << "Running " << numComponents << " components for "
		<< numHours << " hours" << std::endl;
	auto start = std::chrono::high_resolution_clock::now();
	Model m = {};
	m.RandFn = []() { return 0.4; };
	m.FinalTime = 8760.0 * 3600.0;
	std::vector<TimeAndAmount> timesAndLoads = {};
	timesAndLoads.reserve(numHours + 1);
	for (size_t i = 0; i <= numHours; ++i)
	{
		timesAndLoads.push_back(TimeAndAmount{((double)i) * 3600.0, 1});
	}
	for (size_t i = 0; i < numComponents; ++i)
	{
		auto srcId = Model_AddConstantSource(m, 100);
		auto loadId = Model_AddScheduleBasedLoad(m, timesAndLoads);
		auto srcToLoadConn = Model_AddConnection(m, srcId, 0, loadId, 0);
	}
	auto stopConstr = std::chrono::high_resolution_clock::now();
	auto durationConstr =
		std::chrono::duration_cast<std::chrono::microseconds>(
			stopConstr - start);
	std::cout << "Construction time: "
		<< ((double)durationConstr.count() / 1000.0)
		<< " ms" << std::endl;
	auto results = Simulate(m, false);
	assert(results.size() == numHours + 1
		&& "Results is not of expected length");
	auto stop = std::chrono::high_resolution_clock::now();
	auto durationSim =
		std::chrono::duration_cast<std::chrono::microseconds>(
			stop - stopConstr);
	std::cout << "Sim time: " << ((double)durationSim.count() / 1000.0)
		<< " ms" << std::endl;
	auto duration =
		std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << "Total time: " << ((double)duration.count() / 1000.0)
		<< " ms" << std::endl;
	return EXIT_SUCCESS;
}
