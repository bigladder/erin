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
#include "erin_next/erin_next_csv.h"
#include <iostream>
#include <limits>
#include <string>
#include <filesystem>
#include "../vendor/CLI11/include/CLI/CLI.hpp"

int
versionCommand() {
    std::cout << "Version: " << erin::version::version_string;
    return EXIT_SUCCESS;
}

int
limitsCommand() {
    std::cout << "Limits: " << std::endl;
    std::cout << "- sizeof(uint64_t): " << (sizeof(uint64_t)) << std::endl;
    std::cout << "- std::numeric_limits<uint64_t>::max(): "
              << std::numeric_limits<uint64_t>::max() << std::endl;
    std::cout << "- sizeof(uint32_t): " << (sizeof(uint32_t)) << std::endl;
    std::cout << "- std::numeric_limits<uint32_t>::max(): "
              << std::numeric_limits<uint32_t>::max() << std::endl;
    std::cout << "- sizeof(unsigned int): " << (sizeof(unsigned int))
              << std::endl;
    std::cout << "- std::numeric_limits<unsigned int>::max(): "
              << std::numeric_limits<unsigned int>::max() << std::endl;
    std::cout << "- sizeof(unsigned long): " << (sizeof(unsigned long))
              << std::endl;
    std::cout << "- std::numeric_limits<unsigned long>::max(): "
              << std::numeric_limits<unsigned long>::max() << std::endl;
    std::cout << "- sizeof(unsigned long long): "
              << (sizeof(unsigned long long)) << std::endl;
    std::cout << "- std::numeric_limits<unsigned long long>::max(): "
              << std::numeric_limits<unsigned long long>::max() << std::endl;
    return EXIT_SUCCESS;
}

int
runCommand(
        std::string const &tomlFilename,
        std::string const &eventsFilename,
        std::string const &statsFilename,
        bool verbose
) {
    if (verbose) {
        std::cout << "input file: " << tomlFilename << std::endl;
        std::cout << "events file: " << eventsFilename << std::endl;
        std::cout << "statistics file: " << statsFilename << std::endl;
        std::cout << "verbose: " << (verbose ? "true" : "false") << std::endl;
    }

    std::ifstream ifs(tomlFilename, std::ios_base::binary);
    if (!ifs.good()) {
        std::cout << "Could not open input file stream on input file"
                  << std::endl;
        return EXIT_FAILURE;
    }

    using namespace erin;
    auto nameOnly = std::filesystem::path(tomlFilename).filename();
    auto data = toml::parse(ifs, nameOnly.string());
    ifs.close();
    auto validationInfo = SetupGlobalValidationInfo();
    auto maybeSim = Simulation_ReadFromToml(data, validationInfo);
    if (!maybeSim.has_value()) {
        return EXIT_FAILURE;
    }
    Simulation s = std::move(maybeSim.value());
    if (verbose) {
        Simulation_Print(s);
        std::cout << "-----------------" << std::endl;
    }
    Simulation_Run(s, eventsFilename, statsFilename, verbose);

    return EXIT_SUCCESS;
}

int
graphCommand(
        std::string const &inputFilename,
        std::string const &outputFilename
) {
    std::ifstream ifs(inputFilename, std::ios_base::binary);
    if (!ifs.good()) {
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
    if (!maybe_sim.has_value()) {
        return EXIT_FAILURE;
    }
    Simulation s = std::move(maybe_sim.value());
    std::string dot_data = network_to_dot(
            s.TheModel.Connections, s.TheModel.ComponentMap.Tag, "", true
    );
    // save string from network_to_dot
    std::ofstream ofs(outputFilename, std::ios_base::binary);
    if (!ofs.good()) {
        std::cout << "Could not open output file stream on output file"
                  << std::endl;
        return EXIT_FAILURE;
    }
    ofs << dot_data << std::endl;
    ofs.close();
    return EXIT_SUCCESS;
}

int
readCommand(
        std::string const &tomlFilename,
        std::string const &mode,
        bool verbose
) {
    if (verbose) {
        std::cout << "input file: " << tomlFilename << std::endl;
        std::cout << "verbose: " << (verbose ? "true" : "false") << std::endl;
    }

    std::ifstream ifs(tomlFilename, std::ios_base::binary);
    if (!ifs.good()) {
        std::cout << "Could not open input file stream on input file"
                  << std::endl;
        return EXIT_FAILURE;
    }

    auto tomlFilenameOnly = std::filesystem::path(tomlFilename).filename();
    auto data = toml::parse(ifs, tomlFilenameOnly.string());
    ifs.close();

    if (!data.contains("read_files")) {
        return EXIT_FAILURE;
    }
    auto read_files = data.at("read_files");

    std::vector<std::string> multi_names = toml::find<std::vector<std::string>>(read_files, "multi_names");
    auto single_filename = toml::find<std::string>(read_files, "single_name");
    auto num_single_read = toml::find<int>(read_files, "num_single_read");
    auto mixed_filename = toml::find<std::string>(read_files, "mixed_name");

    unsigned num_read = 0;
    using namespace erin;

    if (mode == "multi") {
        for (auto &filename: multi_names) {
            std::ifstream inFile;
            inFile.open(filename);
            if (inFile.good()) {
                std::vector<std::string> filerow;
                do {
                    filerow = read_row(inFile);
                } while (!inFile.eof());
                ++num_read;
                //std::cout << " read " << num_read << ": "<< filename << "\n";
            }
            inFile.close();
        }
    }

    if (mode == "single") {
        for (int i = 0; i < num_single_read; ++i) {
            std::ifstream inFile;
            inFile.open(single_filename);
            if (inFile.good()) {
                std::vector<std::string> filerow;
                do {
                    filerow = read_row(inFile);
                } while (!inFile.eof());
                ++num_read;
                //std::cout << " read " << num_read << ": " << single_filename << "\n";
            }
            inFile.close();
        }
    }

    if (mode == "mixed") {
        std::ifstream inFile;
        inFile.open(mixed_filename);
        if (inFile.good()) {
            std::vector<std::string> filerow;
            do {
                filerow = read_row(inFile);
            } while (!inFile.eof());
            ++num_read;
            //std::cout << " read " << num_read << ": " << mixed_filename << "\n";
        }
        inFile.close();
    }

    if (verbose) {
        std::cout << "mode: " << mode << std::endl;
        std::cout << "# files read: " << num_read << std::endl;
        std::cout << "-----------------" << std::endl;
    }

    return EXIT_SUCCESS;
}

int
main(int argc, char **argv) {
    int result = EXIT_SUCCESS;

    CLI::App app{"erin"};
    app.require_subcommand(0);

    auto version = app.add_subcommand("version", "Display version");
    version->callback([&]() { versionCommand(); });

    auto limits = app.add_subcommand("limits", "Display limits");
    limits->callback([&]() { limitsCommand(); });

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
    run->add_flag("-v,--verbose", verbose, "Verbose output");

    run->callback(
            [&]() {
                result = runCommand(
                        tomlFilename, eventsFilename, statsFilename, verbose
                );
            }
    );

    auto graph = app.add_subcommand("graph", "Graph a simulation");
    graph->add_option("toml_file", tomlFilename, "TOML filename")->required();

    std::string outputFilename = "graph.dot";
    graph->add_option("-o,--out", outputFilename, "Graph output filename");

    graph->callback([&]() { result = graphCommand(tomlFilename, outputFilename); });

    //
    auto read = app.add_subcommand("read", "Read mixed speed test");
    read->add_option("toml_file", tomlFilename, "TOML filename")->required();

    std::string mode = "multi";
    read->add_option("mode", mode, "mode")->required();

    read->add_flag("-v,--verbose", verbose, "Verbose output");

    read->callback([&]() { result = readCommand(tomlFilename, mode, verbose); });

    CLI11_PARSE(app, argc, argv);

    // call with no subcommands is equivalent to subcommand "help"
    if (argc == 1) {
        std::cout << "ERIN - Energy Resilience of Interacting Networks\n"
                  << "Version " << erin::version::version_string << "\n"
                  << std::endl;
        std::cout << app.help() << std::endl;
    }

    return result;
}
