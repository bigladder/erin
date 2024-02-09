#include "../vendor/toml11/toml.hpp"
#include "erin/version.h"
#include "erin_next/erin_next_simulation_info.h"
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_simulation.h"
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_scenario.h"
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
		if (!data.contains("simulation_info"))
		{
			std::cout << "Required section [simulation_info] not found"
				<< std::endl;
			return EXIT_FAILURE;
		}
		toml::value const& simInfoTable = data.at("simulation_info");
		auto maybeSimInfo = erin_next::ParseSimulationInfo(simInfoTable.as_table());
		std::cout << "-----------------" << std::endl;
		erin_next::Simulation s = {};
		// Simulation Info
		if (!maybeSimInfo.has_value())
		{
			return EXIT_FAILURE;
		}
		erin_next::SimulationInfo simInfo = std::move(maybeSimInfo.value());
		std::cout << simInfo << std::endl;
		s.Info = simInfo;
		// Loads
		toml::value const& loadTable = data.at("loads");
		auto maybeLoads = erin_next::ParseLoads(loadTable.as_table());
		if (!maybeLoads.has_value())
		{
			return EXIT_FAILURE;
		}
		std::vector<erin_next::Load> loads = std::move(maybeLoads.value());
		Simulation_RegisterAllLoads(s, loads);
		// TODO: below code needs to get wrapped into a
		// `Simulation_Init(Simulation)` function; this gives the "0 value"
		// flow which allows one to "opt out" of the flow declaration stuff.
		Simulation_RegisterFlow(s, "");
		erin_next::Model m = {};
		m.FinalTime = simInfo.MaxTime;
		m.RandFn = []() { return 0.4; };
		// Components
		if (data.contains("components") && data.at("components").is_table())
		{
			if (!erin_next::ParseComponents(
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
	else
	{
		PrintUsage(std::string{ argv[0] });
	}
	return EXIT_SUCCESS;
}