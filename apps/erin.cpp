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

void
PrintUsage(std::string const& progName)
{
    std::cout << "USAGE: " << progName << " " << "<toml-input-file> "
              << "<optional:output csv; default:out.csv> "
              << "<optional:statistics.csv; default:stats.csv> "
              << "<optional:scenario; default: run all>" << std::endl;
}

int
main(int argc, char** argv)
{
    CLI::App app{"erin"};
    app.require_subcommand(1);

    auto help = app.add_subcommand("help", "Display command-line help");

    help->callback([&]() {
        PrintUsage(std::string{argv[0]});
    }
    );

    CLI11_PARSE(app, argc, argv);
     return EXIT_SUCCESS;
}
