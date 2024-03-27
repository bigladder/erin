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
#include "erin_next/erin_next_result.h"
#include "erin_next/erin_next_validation.h"
#include <iostream>
#include <string>
#include <filesystem>

void
PrintUsage(std::string const& progName)
{
	std::cout << "USAGE: " << progName << " "
		<< "<toml-input-file> "
		<< "<optional:output csv; default:out.csv> "
		<< "<optional:statistics.csv; default:stats.csv> "
		<< "<optional:scenario; default: run all>"
		<< std::endl;
}

int
main(int argc, char** argv)
{
	using namespace erin;
	std::cout << "ERIN version " << erin::version::version_string << std::endl;
	std::cout
		<< "Copyright (C) 2020-2024 Big Ladder Software LLC." << std::endl;
	std::cout << "See LICENSE.txt file for license information." << std::endl;
	if (argc >= 2)
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
		auto validationInfo = SetupGlobalValidationInfo();
		auto maybeSim = Simulation_ReadFromToml(data, validationInfo);
		if (!maybeSim.has_value())
		{
			return EXIT_FAILURE;
		}
		Simulation s = std::move(maybeSim.value());
		Simulation_Print(s);
		std::cout << "-----------------" << std::endl;
		Simulation_Run(s);
	}
	else
	{
		PrintUsage(std::string{ argv[0] });
	}
	return EXIT_SUCCESS;
}
