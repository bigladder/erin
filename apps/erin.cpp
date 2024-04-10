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
#include "../vendor/CLI11/include/CLI/CLI.hpp"

int runCommand(std::string const& tomlFilename)
{
\
    std::cout << "input file: " << tomlFilename << std::endl;
    std::ifstream ifs(tomlFilename, std::ios_base::binary);
    if (!ifs.good())
    {
        std::cout << "Could not open input file stream on input file" << std::endl;
        return EXIT_FAILURE;
    }

    using namespace erin;
    auto nameOnly = std::filesystem::path(tomlFilename).filename();
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

    return EXIT_SUCCESS;
}

void
helpCommand(std::string const& progName)
{
    std::cout << "USAGE: " << progName << " " << "<toml-input-file> "
              << "<optional:output csv; default:out.csv> "
              << "<optional:statistics.csv; default:stats.csv> "
              << "<optional:scenario; default: run all>" << std::endl;
}

int
main(int argc, char** argv)
{
    int result = EXIT_SUCCESS;
    CLI::App app{"erin"};
    app.require_subcommand(1);

    auto run = app.add_subcommand("run", "Run a simulation");
    std::string tomlFilename;
    run->add_option("toml_file", tomlFilename, "TOML filename");
    run->callback([&]() {
        result = runCommand(tomlFilename);
    }
    );

    auto help = app.add_subcommand("help", "Display command-line help");
    help->callback([&]() {
                       helpCommand(std::string{argv[0]});
    }
    );

    CLI11_PARSE(app, argc, argv);
    return result;
}
