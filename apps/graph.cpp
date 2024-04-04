/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/version.h"
#include "erin_next/erin_next_graph.h"
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
#include <string>
#include <iostream>
#include <filesystem>

void
print_usage(std::string prog_name)
{
    std::cout << "\nUSAGE: " << prog_name
              << " <toml-input-file> <output-dot-file>\n"
              << "Creates a graphviz-compatible graph from network"
              << std::endl;
}

int
main(int argc, char** argv)
{
    using namespace erin;
    std::cout << "ERIN version " << erin::version::version_string << std::endl;
    std::cout << "Copyright (C) 2020-2024 Big Ladder Software LLC."
              << std::endl;
    std::cout << "See LICENSE.txt file for license information." << std::endl;
    if (argc != 3)
    {
        std::filesystem::path prog_name =
            std::filesystem::path(std::string(argv[0])).filename();
        std::string prog_name_str = prog_name.string();
        print_usage(prog_name_str);
        return EXIT_SUCCESS;
    }
    std::cout << "Creating graph..." << std::endl;
    std::string inputfile_path(argv[1]);
    std::string outputfile_path(argv[2]);
    std::cout << "input file : " << inputfile_path << std::endl;
    std::cout << "output file: " << outputfile_path << std::endl;
    std::ifstream ifs(inputfile_path, std::ios_base::binary);
    if (!ifs.good())
    {
        std::cout << "Could not open input file stream on input file"
                  << std::endl;
        return EXIT_FAILURE;
    }
    auto name_only = std::filesystem::path(inputfile_path).filename();
    auto data = toml::parse(ifs, name_only.string());
    ifs.close();
    auto validation_info = SetupGlobalValidationInfo();
    auto maybe_sim = Simulation_ReadFromToml(data, validation_info);
    if (!maybe_sim.has_value())
    {
        return EXIT_FAILURE;
    }
    Simulation s = std::move(maybe_sim.value());
    std::string dot_data = network_to_dot(
        s.TheModel.Connections, s.TheModel.ComponentMap.Tag, "", true
    );
    // save string from network_to_dot
    std::ofstream ofs(outputfile_path, std::ios_base::binary);
    if (!ofs.good())
    {
        std::cout << "Could not open output file stream on output file"
                  << std::endl;
        return EXIT_FAILURE;
    }
    ofs << dot_data << std::endl;
    ofs.close();
    return EXIT_SUCCESS;
}
