#include "../vendor/toml11/toml.hpp"
#include "erin/version.h"
#include "erin_next/erin_next_simulation_info.h"
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_simulation.h"
#include "erin_next/erin_next.h"
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
		// Simulation Info
		if (!maybeSimInfo.has_value())
		{
			return EXIT_FAILURE;
		}
		erin_next::SimulationInfo simInfo = std::move(maybeSimInfo.value());
		std::cout << simInfo << std::endl;
		// Loads
		toml::value const& loadTable = data.at("loads");
		auto maybeLoads = erin_next::ParseLoads(loadTable.as_table());
		if (!maybeLoads.has_value())
		{
			return EXIT_FAILURE;
		}
		std::vector<erin_next::Load> loads = std::move(maybeLoads.value());
		std::cout << "Loads:" << std::endl;
		for (size_t i = 0; i < loads.size(); ++i)
		{
			std::cout << i << ": " << loads[i] << std::endl;
		}
		// Components
		toml::value const& compTable = data.at("components");
		erin_next::Simulation s = {};
		Simulation_RegisterFlow(s, "");
		erin_next::Model m = {};
		m.FinalTime = simInfo.MaxTime;
		m.RandFn = []() { return 0.4; };
		bool parseSuccess =
			erin_next::ParseComponents(s, m, compTable.as_table());
		if (!parseSuccess)
		{
			return EXIT_FAILURE;
		}
		std::cout << "Components:" << std::endl;
		Simulation_PrintComponents(s, m);
		std::cout << "Scenarios:" << std::endl;
		Simulation_PrintScenarios(s);
	}
	else
	{
		PrintUsage(std::string{ argv[0] });
	}
	return EXIT_SUCCESS;
}