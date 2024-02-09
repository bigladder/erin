#include "../vendor/toml11/toml.hpp"
#include "erin/version.h"
#include "erin_next/erin_next_simulation_info.h"
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_simulation.h"
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_scenario.h"
#include "erin_next/erin_next_units.h"
#include <iostream>
#include <string>
#include <filesystem>

void
PrintUsage(std::string const& progName)
{
	std::cout << "USAGE: " << progName << " <toml-input-file>" << std::endl;
}

int
main(int argc, char** argv)
{
	using namespace erin_next;
	std::cout << "ERIN version " << erin::version::version_string << std::endl;
	std::cout
		<< "Copyright (C) 2020-2024 Big Ladder Software LLC." << std::endl;
	std::cout << "See LICENSE.txt file for license information." << std::endl;
	if (argc == 2)
	{
		std::string fname{ argv[1] };
		std::cout << "input file: " << fname << std::endl;
		std::ifstream ifs(fname, std::ios_base::binary);
		if (!ifs.good())
		{
			std::cout << "Could not open input file stream on input file"
				<< std::endl;
			return EXIT_FAILURE;
		}
		auto nameOnly = std::filesystem::path(fname).filename();
		auto data = toml::parse(ifs, nameOnly.string());
		ifs.close();
		std::cout << data << std::endl;
		Simulation s = {};
		Simulation_Init(s);
		// Simulation Info
		if (!data.contains("simulation_info"))
		{
			std::cout << "Required section [simulation_info] not found"
				<< std::endl;
			return EXIT_FAILURE;
		}
		toml::value const& simInfoTable = data.at("simulation_info");
		auto maybeSimInfo = ParseSimulationInfo(simInfoTable.as_table());
		if (!maybeSimInfo.has_value())
		{
			return EXIT_FAILURE;
		}
		SimulationInfo simInfo = std::move(maybeSimInfo.value());
		std::cout << simInfo << std::endl;
		s.Info = simInfo;
		// Loads
		toml::value const& loadTable = data.at("loads");
		auto maybeLoads = ParseLoads(loadTable.as_table());
		if (!maybeLoads.has_value())
		{
			return EXIT_FAILURE;
		}
		std::vector<Load> loads = std::move(maybeLoads.value());
		Simulation_RegisterAllLoads(s, loads);
		Model m = {};
		m.FinalTime = simInfo.MaxTime;
		m.RandFn = []() { return 0.4; };
		// Components
		if (data.contains("components") && data.at("components").is_table())
		{
			if (!ParseComponents(
					s, m, data.at("components").as_table()))
			{
				return EXIT_FAILURE;
			}
		}
		else
		{
			std::cout << "required field 'components' not found"
				<< std::endl;
		}
		// Distributions
		if (data.contains("dist") && data.at("dist").is_table())
		{
			ParseDistributions(m.DistSys, data.at("dist").as_table());
		}
		else
		{
			std::cout << "required field 'dist' not found" << std::endl;
			return EXIT_FAILURE;
		}
		// Networks
		if (data.contains("networks") && data.at("networks").is_table())
		{
			ParseNetworks(s.FlowTypeMap, m, data.at("networks").as_table());
		}
		else
		{
			std::cout << "required field 'networks' not found" << std::endl;
			return EXIT_FAILURE;
		}
		// Scenarios
		if (data.contains("scenarios") && data.at("scenarios").is_table())
		{
			std::vector<size_t> scenarioIds =
				ParseScenarios(
					s.ScenarioMap,
					m.DistSys,
					data.at("scenarios").as_table());
		}
		else
		{
			std::cout << "required field 'scenarios' not found or not a table"
				<< std::endl;
			return EXIT_FAILURE;
		}
		// PRINT OUT
		if (true)
		{
			std::cout << "-----------------" << std::endl;
			std::cout << "\nLoads:" << std::endl;
			Simulation_PrintLoads(s);
			std::cout << "\nComponents:" << std::endl;
			Simulation_PrintComponents(s, m);
			std::cout << "\nScenarios:" << std::endl;
			Simulation_PrintScenarios(s);
			std::cout << "\nDistributions:" << std::endl;
			m.DistSys.print_distributions();
			std::cout << "\nConnections:" << std::endl;
			Model_PrintConnections(m, s.FlowTypeMap);
			std::cout << "\nScenarios:" << std::endl;
			Scenario_Print(s.ScenarioMap, m.DistSys);
		}
		std::cout << "-----------------" << std::endl;
		// TODO: check the components and network:
		// -- that all components are hooked up to something
		// -- that no port is double linked
		// -- that all connections have the correct flows
		// -- that required ports are linked
		// -- check that we have a proper acyclic graph?
		// NOW, we want to do a simulation for each scenario
		// TODO: generate reliability information from time = 0 to final time
		// ... from sim info. Those schedules will be clipped to the times of
		// ... each scenario's instance.
		// TODO: generate a data structure to hold all results.
		// TODO: set random function for Model based on SimInfo
		for (size_t scenIdx = 0;
			scenIdx < Simulation_ScenarioCount(s);
			++scenIdx)
		{
			// for this scenario, ensure all schedule-based components
			// have the right schedule set for this scenario
			for (size_t sblIdx=0; sblIdx < m.ScheduledLoads.size(); ++sblIdx)
			{
				if (m.ScheduledLoads[sblIdx].ScenarioIdToLoadId.contains(scenIdx))
				{
					auto loadId =
						m.ScheduledLoads[sblIdx].ScenarioIdToLoadId.at(scenIdx);
					// TODO: set to correct time units of seconds
					std::vector<TimeAndLoad> schedule{};
					size_t numEntries = s.LoadMap.Loads[loadId].size();
					schedule.reserve(numEntries);
					for (size_t i=0; i < numEntries; ++i)
					{
						TimeAndLoad tal{};
						// TODO: need to merge Load and LoadType as we're
						// missing critical fields. Suggest:
						// LoadType => LoadDict (SOA) and keep
						// vector<Load> as AOS. See second arg below.
						tal.Time = Time_ToSeconds(
							s.LoadMap.Loads[loadId][i].Time,
							loads[loadId].TimeUnit);
						tal.Load = s.LoadMap.Loads[loadId][i].Load;
						schedule.push_back(std::move(tal));
					}
					m.ScheduledLoads[sblIdx].TimesAndLoads = schedule;
				}
			}
			// TODO: implement occurrences of the scenario in time.
			// for now, we know a priori that we have a max occurrence of 1
			std::vector<double> occurrenceTimes_s = { 0.0 };
			for (double t : occurrenceTimes_s)
			{
				double duration_s = Time_ToSeconds(
					s.ScenarioMap.Durations[scenIdx],
					s.ScenarioMap.TimeUnits[scenIdx]);
				double tEnd = t + duration_s;
				// TODO: clip reliability schedules here
				m.FinalTime = duration_s;
				SimulationState ss{};
				// TODO: add an optional verbosity flag to SimInfo
				// -- use that to set things like the print flag below
				auto results = Simulate(m, ss, true);
			}
		}
	}
	else
	{
		PrintUsage(std::string{ argv[0] });
	}
	return EXIT_SUCCESS;
}