#include "../vendor/toml11/toml.hpp"
#include "erin/version.h"
#include "erin_next/erin_next_simulation_info.h"
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
		// HERE: begin pulling apart the input file and
		// creating a model to simulate
		if (!data.contains("simulation_info"))
		{
			std::cout << "Required section [simulation_info] not found"
				<< std::endl;
			return EXIT_FAILURE;
		}
		toml::value const& simInfoTable = data.at("simulation_info");
		auto simInfo = erin_next::ParseSimulationInfo(simInfoTable.as_table());
		std::cout << "-----------------" << std::endl;
		if (simInfo.has_value())
		{
			std::cout << simInfo.value() << std::endl;
		}
	}
	else
	{
		PrintUsage(std::string{ argv[0] });
	}
	return EXIT_SUCCESS;
}