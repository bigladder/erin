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

int
runCommand(
    std::string const& tomlFilename,
    std::string const& eventsFilename,
    std::string const& statsFilename,
    bool verbose
)
{
    std::cout << "input file: " << tomlFilename << std::endl;
    std::cout << "events file: " << eventsFilename << std::endl;
    std::cout << "statistics file: " << statsFilename << std::endl;
    std::cout << "verbose: " << (verbose ? "true" : "false") << std::endl;

    std::ifstream ifs(tomlFilename, std::ios_base::binary);
    if (!ifs.good())
    {
        std::cout << "Could not open input file stream on input file"
                  << std::endl;
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
    s.eventsFilename = eventsFilename;
    s.statsFilename = statsFilename;
    s.verbose = verbose;
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

    // call with no subcommands is equivalent to subcommand "help"
    if (argc == 1)
    {
        helpCommand(std::string{argv[0]});
        return result;
    }

    // process subcommands
    CLI::App app{"erin"};
    app.require_subcommand(0);

    // "help" command
    auto help = app.add_subcommand("help", "Display command-line help");
    help->callback([&]() { helpCommand(std::string{argv[0]}); });

    // "run" command
    auto run = app.add_subcommand("run", "Run a simulation");

    std::string tomlFilename;
    run->add_option("toml_file", tomlFilename, "TOML filename")->required();

    std::string eventsFilename = "out.csv";
    run->add_option(
        "-e,--events", eventsFilename, "Events csv filename; default:out.csv"
    );

    std::string statsFilename = "stats.csv";
    run->add_option(
        "-s,--statistics",
        statsFilename,
        "Statistics csv filename; default:stats.csv"
    );

    bool verbose = false;
    run->add_flag("-v, --verbose", verbose, "Verbose output");

    run->callback(
        [&]() {
            result = runCommand(
                tomlFilename, eventsFilename, statsFilename, verbose
            );
        }
    );

    CLI11_PARSE(app, argc, argv);
    return result;
}
