#include "../vendor/toml11/toml.hpp"
#include "erin/version.h"
#include "erin_next/erin_next_simulation_info.h"
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_simulation.h"
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_scenario.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_result.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_graph.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include "../vendor/CLI11/include/CLI/CLI.hpp"
#include "toml/exception.hpp"
#include "toml/get.hpp"
#include "compilation_settings.h"

int
versionCommand()
{
    std::cout << "Version: " << erin::version::version_string << "\n";
    std::cout << "Build Type: " << build_type << "\n";
    return EXIT_SUCCESS;
}

int
limitsCommand()
{
    std::cout << "Limits: " << std::endl;
    std::cout << "- value of max_flow_W: " << erin::max_flow_W << std::endl;
    std::cout << "- max_flow_W ==     9223372036854776: "
              << (erin::max_flow_W == 9'223'372'036'854'776ULL) << std::endl;
    std::cout << "- max_flow_W == 18446744073709551615: "
              << (erin::max_flow_W == 18'446'744'073'709'551'615ULL)
              << std::endl;
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
    std::string const& tomlFilename,
    std::string const& eventsFilename,
    std::string const& statsFilename,
    bool verbose
)
{
    if (verbose)
    {
        std::cout << "input file: " << tomlFilename << std::endl;
        std::cout << "events file: " << eventsFilename << std::endl;
        std::cout << "statistics file: " << statsFilename << std::endl;
        std::cout << "verbose: " << (verbose ? "true" : "false") << std::endl;
    }

    std::ifstream ifs(tomlFilename, std::ios_base::binary);
    if (!ifs.good())
    {
        std::cout << "Could not open input file stream on input file"
                  << std::endl;
        return EXIT_FAILURE;
    }

    using namespace erin;
    auto nameOnly = std::filesystem::path(tomlFilename).filename();
    toml::value data = toml::parse(ifs, nameOnly.string());
    ifs.close();
    std::unordered_set<std::string> componentTagsInUse =
        TOMLTable_ParseComponentTagsInUse(data);
    auto validationInfo = SetupGlobalValidationInfo();
    auto maybeSim =
        Simulation_ReadFromToml(data, validationInfo, componentTagsInUse);
    if (!maybeSim.has_value())
    {
        return EXIT_FAILURE;
    }
    Simulation s = std::move(maybeSim.value());
    if (verbose)
    {
        Simulation_Print(s);
        std::cout << "-----------------" << std::endl;
    }
    Simulation_Run(s, eventsFilename, statsFilename, verbose);

    return EXIT_SUCCESS;
}

int
graphCommand(
    std::string const& inputFilename,
    std::string const& outputFilename,
    bool const useHtml
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
    std::unordered_set<std::string> componentTagsInUse =
        TOMLTable_ParseComponentTagsInUse(data);
    auto validation_info = SetupGlobalValidationInfo();
    auto maybe_sim =
        Simulation_ReadFromToml(data, validation_info, componentTagsInUse);
    if (!maybe_sim.has_value())
    {
        return EXIT_FAILURE;
    }
    Simulation s = std::move(maybe_sim.value());
    std::string dot_data = network_to_dot(
        s.TheModel.Connections, s.TheModel.ComponentMap.Tag, "", useHtml
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
checkNetworkCommand(std::string tomlFilename)
{
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
    std::unordered_set<std::string> componentTagsInUse =
        TOMLTable_ParseComponentTagsInUse(data);
    auto validationInfo = SetupGlobalValidationInfo();
    auto maybeSim =
        Simulation_ReadFromToml(data, validationInfo, componentTagsInUse);
    if (!maybeSim.has_value())
    {
        return EXIT_FAILURE;
    }
    Simulation s = std::move(maybeSim.value());
    std::vector<std::string> issues = erin::Model_CheckNetwork(s.TheModel);
    if (issues.size() > 0)
    {
        std::cout << "ISSUES FOUND:" << std::endl;
        for (std::string const& issue : issues)
        {
            std::cout << issue << std::endl;
        }
        return EXIT_FAILURE;
    }
    std::cout << "No issues found with network." << std::endl;
    return EXIT_SUCCESS;
}

int
updateTomlInputCommand(
    std::string inputTomlFilename,
    std::string outputTomlFilename,
    bool removeIds
)
{
    std::ifstream ifs(inputTomlFilename, std::ios_base::binary);
    if (!ifs.good())
    {
        std::cout << "Could not open input file stream on input file"
                  << std::endl;
        return EXIT_FAILURE;
    }
    using namespace erin;
    auto nameOnly = std::filesystem::path(inputTomlFilename).filename();
    auto data = toml::parse(ifs, nameOnly.string());
    ifs.close();

    // rename networks.* to network
    if (data.contains("networks"))
    {
        auto new_node = std::unordered_map<std::string, toml::value>{};
        auto nw = data["networks"].as_table();
        for (auto it = nw.begin(); it != nw.end(); ++it)
        {
            std::string const& nwName = it->first;
            std::cout << "CHANGE .networks." << nwName << " to .network"
                      << std::endl;
            auto nwTable = it->second.as_table();
            new_node["connections"] = nwTable["connections"];
            break;
        }
        data["network"] = new_node;
        data.as_table().erase("networks");
    }
    // add input_format_version = current_input_version
    if (data.contains("simulation_info"))
    {
        auto& sim_info_table = data.at("simulation_info").as_table();
        if (sim_info_table.contains("input_format_version"))
        {
            std::cout << "UPDATE simulation_info.input_format_version from "
                      << sim_info_table["input_format_version"] << " to "
                      << current_input_version << std::endl;
        }
        else
        {
            std::cout << "ADD simulation_info.input_format_version = "
                      << current_input_version << std::endl;
        }
        sim_info_table["input_format_version"] = current_input_version;
    }
    // add missing fields to components
    if (data.contains("components"))
    {
        auto& components = data["components"].as_table();
        for (auto it = components.begin(); it != components.end(); ++it)
        {
            std::string const& compName = it->first;
            auto& comp = it->second.as_table();
            std::string compType = comp["type"].as_string();
            if (compType == "store" && !comp.contains("max_discharge"))
            {
                double maxDischarge = comp["max_inflow"].is_floating()
                    ? comp["max_inflow"].as_floating()
                    : static_cast<double>(comp["max_inflow"].as_integer());
                comp["max_discharge"] = toml::value(maxDischarge);
                std::cout << "ADD components." << compName
                          << ".max_discharge = " << maxDischarge << std::endl;
            }
            if (compType == "store" && comp.contains("max_inflow"))
            {
                double maxInflow = comp["max_inflow"].is_floating()
                    ? comp["max_inflow"].as_floating()
                    : static_cast<double>(comp["max_inflow"].as_integer());
                comp.erase("max_inflow");
                comp["max_charge"] = toml::value(maxInflow);
                std::cout << "RENAME components." << compName
                          << ".max_inflow to components" << compName
                          << ".max_charge" << std::endl;
            }
            if (compType == "muxer" && comp.contains("dispatch_strategy"))
            {
                comp.erase("dispatch_strategy");
                std::cout << "REMOVE components." << compName
                          << ".dispatch_strategy" << std::endl;
            }
            if (removeIds && comp.contains("id"))
            {
                comp.erase("id");
                std::cout << "REMOVE components." << compName << ".id"
                          << std::endl;
            }
            if (compType == "converter" && comp.contains("constant_efficiency"))
            {
                double eff = comp["constant_efficiency"].is_floating()
                    ? comp["constant_efficiency"].as_floating()
                    : static_cast<double>(
                          comp["constant_efficiency"].as_integer()
                      );
                if (eff > 1.0)
                {
                    comp["type"] = toml::value("mover");
                    comp["cop"] = toml::value(eff);
                    comp.erase("constant_efficiency");
                    std::cout << "CHANGE components." << compName
                              << ".type to mover" << std::endl;
                }
            }
        }
    }
    if (removeIds)
    {
        if (data.contains("simulation_info"))
        {
            auto& simInfoTable = data["simulation_info"].as_table();
            if (simInfoTable.contains("id"))
            {
                simInfoTable.erase("id");
                std::cout << "REMOVE simulation_info.id" << std::endl;
            }
        }
        if (data.contains("fragility_mode"))
        {
            auto& fms = data["fragility_mode"].as_table();
            for (auto it = fms.begin(); it != fms.end(); ++it)
            {
                std::string const& fmName = it->first;
                auto& fm = it->second.as_table();
                if (fm.contains("id"))
                {
                    fm.erase("id");
                    std::cout << "REMOVE fragility_mode." << fmName << ".id"
                              << std::endl;
                }
            }
        }
        if (data.contains("failure_mode"))
        {
            auto& fms = data["failure_mode"].as_table();
            for (auto it = fms.begin(); it != fms.end(); ++it)
            {
                std::string const& fmName = it->first;
                auto& fm = it->second.as_table();
                if (fm.contains("id"))
                {
                    fm.erase("id");
                    std::cout << "REMOVE failure_mode." << fmName << ".id"
                              << std::endl;
                }
            }
        }
        if (data.contains("fragility_curve"))
        {
            auto& fcs = data["fragility_curve"].as_table();
            for (auto it = fcs.begin(); it != fcs.end(); ++it)
            {
                std::string const& fcName = it->first;
                auto& fc = it->second.as_table();
                if (fc.contains("id"))
                {
                    fc.erase("id");
                    std::cout << "REMOVE fragility_curve." << fcName << ".id"
                              << std::endl;
                }
            }
        }
        if (data.contains("dist"))
        {
            auto& dists = data["dist"].as_table();
            for (auto it = dists.begin(); it != dists.end(); ++it)
            {
                std::string const& distName = it->first;
                auto& distTable = it->second.as_table();
                if (distTable.contains("id"))
                {
                    distTable.erase("id");
                    std::cout << "REMOVE dist." << distName << ".id"
                              << std::endl;
                }
            }
        }
    }

    std::ofstream ofs(outputTomlFilename, std::ios_base::binary);
    if (!ofs.good())
    {
        std::cout << "Could not open ouptut file stream for output file"
                  << std::endl;
        return EXIT_FAILURE;
    }
    ofs << data;
    ofs.close();
    // go through data and find issues
    return EXIT_SUCCESS;
}

int
packLoadsCommand(
    std::string const& tomlFilename,
    std::string const& loadsFilename,
    bool verbose
)
{
    if (verbose)
    {
        std::cout << "input file: " << tomlFilename << std::endl;
        std::cout << "verbose: " << (verbose ? "true" : "false") << std::endl;
    }

    std::ifstream ifs(tomlFilename, std::ios_base::binary);
    if (!ifs.good())
    {
        std::cout << "Could not open input file stream on input file"
                  << std::endl;
        return EXIT_FAILURE;
    }

    auto tomlFilenameOnly = std::filesystem::path(tomlFilename).filename();
    auto data = toml::parse(ifs, tomlFilenameOnly.string());
    ifs.close();

    auto const& loadTable = data.at("loads").as_table();

    auto validationInfo = erin::SetupGlobalValidationInfo();
    erin::ValidationInfo explicitValidation = validationInfo.Load_01Explicit;
    erin::ValidationInfo fileValidation = validationInfo.Load_02FileBased;
    auto maybeLoads = ParseLoads(loadTable, explicitValidation, fileValidation);
    if (!maybeLoads.has_value())
    {
        return EXIT_FAILURE;
    }
    std::vector<erin::Load> loads = std::move(maybeLoads.value());

    return erin::WritePackedLoads(loads, loadsFilename);
}

int
main(int argc, char** argv)
{
    int result = EXIT_SUCCESS;

    CLI::App app{"erin"};
    app.require_subcommand(0);

    {
        auto version = app.add_subcommand("version", "Display version");
        version->callback([&]() { versionCommand(); });
    }
    {
        auto limits = app.add_subcommand("limits", "Display limits");
        limits->callback([&]() { limitsCommand(); });
    }

    {
        std::string tomlFilename;
        auto run = app.add_subcommand("run", "Run a simulation");
        run->add_option("toml_file", tomlFilename, "TOML filename")->required();

        std::string eventsFilename = "out.csv";
        run->add_option(
            "-e,--events",
            eventsFilename,
            "Events csv filename; default:out.csv"
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
    }

    {
        std::string tomlFilename;
        auto graph = app.add_subcommand("graph", "Graph a simulation");
        graph->add_option("toml_file", tomlFilename, "TOML filename")
            ->required();

        std::string outputFilename = "graph.dot";
        bool useHtml = true;
        graph->add_option("-o,--out", outputFilename, "Graph output filename");
        graph->add_flag("-s,--simple", useHtml, "Create a simpler graph view");

        graph->callback(
            [&]()
            { result = graphCommand(tomlFilename, outputFilename, !useHtml); }
        );

        auto checkNetwork =
            app.add_subcommand("check", "Check network for issues");
        checkNetwork
            ->add_option(
                "toml_input_file", tomlFilename, "TOML input file name"
            )
            ->required();
        checkNetwork->callback([&]()
                               { result = checkNetworkCommand(tomlFilename); });
    }

    {
        std::string tomlFilename;
        std::string tomlOutputFilename = "out.toml";
        bool stripIds = false;
        auto update =
            app.add_subcommand("update", "Update an ERIN 0.55 file to current");
        update
            ->add_option(
                "toml_input_file", tomlFilename, "TOML input file name"
            )
            ->required();
        update->add_option(
            "toml_output_file", tomlOutputFilename, "TOML output file name"
        );
        update->add_flag(
            "-s,--strip-ids",
            stripIds,
            "If specified, strips ids from the input file"
        );
        update->callback(
            [&]() {
                result = updateTomlInputCommand(
                    tomlFilename, tomlOutputFilename, stripIds
                );
            }
        );
    }

    {
        std::string tomlFilename;
        auto packLoads = app.add_subcommand(
            "pack-loads", "Pack loads into a single csv file"
        );
        packLoads->add_option("toml_file", tomlFilename, "TOML filename")
            ->required();

        std::string loadsFilename = "packed-loads.csv";
        packLoads->add_option(
            "-o,--outcsv",
            loadsFilename,
            "Packed-loads csv filename; default:packed-loads.csv"
        );

        bool verbose = false;
        packLoads->add_flag("-v,--verbose", verbose, "Verbose output");

        packLoads->callback(
            [&]()
            { result = packLoadsCommand(tomlFilename, loadsFilename, verbose); }
        );
    }
    CLI11_PARSE(app, argc, argv);

    // call with no subcommands is equivalent to subcommand "help"
    if (argc == 1)
    {
        std::cout << "ERIN - Energy Resilience of Interacting Networks\n"
                  << "Version " << erin::version::version_string << "\n"
                  << std::endl;
        std::cout << app.help() << std::endl;
    }

    return result;
}
