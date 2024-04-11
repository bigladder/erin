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
#include "erin_next/erin_next_graph.h"
#include <iostream>
#include <string>
#include <filesystem>
#include "../vendor/CLI11/include/CLI/CLI.hpp" //TODO CLI11 license

int
versionCommand()
{
    std::cout << "Version: " << erin::version::version_string;
    return EXIT_SUCCESS;
}

int
limitsCommand()
{
    std::cout << "Limits: " << "not available";
    return EXIT_SUCCESS;
}

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
    Simulation_Print(s);
    std::cout << "-----------------" << std::endl;
    Simulation_Run(s, eventsFilename, statsFilename,verbose);

    return EXIT_SUCCESS;
}

int
graphCommand(
    std::string const& inputFilename,
    std::string const& outputFilename
)
{
    std::ifstream ifs(inputFilename, std::ios_base::binary);
    if (!ifs.good())
    {
        std::cout << "Could not open input file stream on input file"
                  << std::endl;
        return EXIT_FAILURE;
    }

    using namespace erin;
    auto name_only = std::filesystem::path(inputFilename).filename();
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
    std::ofstream ofs(outputFilename, std::ios_base::binary);
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

int
main(int argc, char** argv)
{
    int result = EXIT_SUCCESS;

    // TODO: show program name, copyright info
    // process subcommands
    CLI::App app{"erin"};
    app.require_subcommand(0);

    // "version" command
    auto version = app.add_subcommand("version", "Display version");
    version->callback([&]() { versionCommand(); });

    // "limits" command
    auto limits = app.add_subcommand("limits", "Display limits");
    limits->callback([&]() { limitsCommand(); });

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

    // "graph" command
    auto graph = app.add_subcommand("graph", "Graph a simulation");
    graph->add_option("toml_file", tomlFilename, "TOML filename")->required();

    std::string outputFilename = "graph.dot";
    graph->add_option("-o, --out", outputFilename, "Graph output filename");

    graph->callback([&]()
                    { result = graphCommand(tomlFilename, outputFilename); });

    CLI11_PARSE(app, argc, argv);

    // call with no subcommands is equivalent to subcommand "help"
    if (argc == 1)
    {
        std::cerr << app.help() << std::flush;
    }

    return result;
}
