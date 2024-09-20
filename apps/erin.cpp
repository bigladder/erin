#include "erin/version.h"
#include "erin/logging.h"
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
#include "erin_next/erin_next_utils.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_graph.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <toml.hpp>
#include <CLI/CLI.hpp>
#include "compilation_settings.h"

erin::Log
get_standard_log(erin::Logger& logger)
{
    using namespace erin;
    Log log = Log_MakeFromCourier(logger);
    // NOTE: overriding default error functionality as it throws.
    //       instead, error conditions and exiting are handled explicitly
    //       by the library.
    log.error = [&](std::string const& tag, std::string const& msg)
    {
        if (tag.empty())
        {
            std::cout << fmt::format("[{}] {}", "ERROR", msg) << std::endl;
        }
        else
        {
            std::cout << fmt::format("[{}] {}: {}", "ERROR", tag, msg)
                      << std::endl;
        }
    };
    return log;
}

CLI::App*
add_version(CLI::App& app)
{
    auto subcommand = app.add_subcommand("version", "Display version");

    auto version = [&]()
    {
        std::cout << "Version: " << erin::version::version_string << "\n";
        std::cout << "Build Type: " << build_type << "\n";
    };

    subcommand->callback([&]() { version(); });

    return subcommand;
}

CLI::App*
add_limits(CLI::App& app)
{
    auto subcommand = app.add_subcommand("limits", "Display limits");

    auto limits = [&]()
    {
        std::cout << "Limits: " << std::endl;
        std::cout << "- value of max_flow_W: " << erin::max_flow_W << std::endl;
        std::cout << "- max_flow_W ==     9223372036854776: "
                  << (erin::max_flow_W == 9'223'372'036'854'776ULL)
                  << std::endl;
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
                  << std::numeric_limits<unsigned long long>::max()
                  << std::endl;
    };

    subcommand->callback([&]() { limits(); });

    return subcommand;
}

CLI::App*
add_run(CLI::App& app)
{
    auto subcommand = app.add_subcommand("run", "Run a simulation");

    static std::string tomlFilename;
    subcommand->add_option("toml_file", tomlFilename, "TOML filename")
        ->required();

    static std::string eventsFilename = "out.csv";
    subcommand->add_option(
        "-e,--events", eventsFilename, "Events csv filename; default:out.csv"
    );

    static std::string statsFilename = "stats.csv";
    subcommand->add_option(
        "-s,--statistics",
        statsFilename,
        "Statistics csv filename; default:stats.csv"
    );

    static double time_step_h = -1.0;
    subcommand
        ->add_option(
            "-t,--time_step_h",
            time_step_h,
            "Report with uniform time step (hours)"
        )
        ->check(CLI::PositiveNumber);

    static bool verbose = false;
    subcommand->add_flag("-v,--verbose", verbose, "Verbose output");

    static bool no_aggregate_groups = false;
    subcommand->add_flag(
        "-n,--no-group", no_aggregate_groups, "Suppress group aggregation"
    );

    static bool save_reliability_curves = false;
    subcommand->add_flag(
        "-r,--save-reliability",
        save_reliability_curves,
        "Save reliability curves"
    );

    auto run = [&]()
    {
        using namespace erin;
        Logger logger{};
        Log log = get_standard_log(logger);
        bool aggregate_groups = !no_aggregate_groups;
        if (verbose)
        {
            std::cout << "input file: " << tomlFilename << std::endl;
            std::cout << "events file: " << eventsFilename << std::endl;
            std::cout << "statistics file: " << statsFilename << std::endl;
            if (time_step_h > 0.0)
            {
                std::cout << "time step (h): " << time_step_h << std::endl;
            }
            std::cout << "save reliability curves: "
                      << (save_reliability_curves ? "true" : "false")
                      << std::endl;
            std::cout << "verbose: " << (verbose ? "true" : "false")
                      << std::endl;
            std::cout << "groups: " << (aggregate_groups ? "true" : "false")
                      << std::endl;
        }
        std::ifstream ifs(tomlFilename, std::ios_base::binary);
        if (!ifs.good())
        {
            Log_Error(log, "Could not open input file stream on input file");
            return EXIT_FAILURE;
        }
        auto nameOnly = std::filesystem::path(tomlFilename).filename();
        toml::value data = toml::parse(ifs, nameOnly.string());
        ifs.close();
        std::unordered_set<std::string> componentTagsInUse =
            TOMLTable_ParseComponentTagsInUse(data);
        auto validationInfo = SetupGlobalValidationInfo();
        auto maybeSim = Simulation_ReadFromToml(
            data, validationInfo, componentTagsInUse, log
        );
        if (!maybeSim.has_value())
        {
            Log_Error(log, "Simulation returned without value");
            return EXIT_FAILURE;
        }
        Simulation s = std::move(maybeSim.value());
        if (verbose)
        {
            Simulation_Print(s);
            Log_Info(log, "-----------------");
        }
        Simulation_Run(
            s,
            log,
            eventsFilename,
            statsFilename,
            time_step_h,
            aggregate_groups,
            save_reliability_curves,
            verbose
        );
        return EXIT_SUCCESS;
    };

    subcommand->callback([&]() { run(); });

    return subcommand;
}

CLI::App*
add_graph(CLI::App& app)
{
    auto subcommand = app.add_subcommand("graph", "Graph a simulation");

    static std::string tomlFilename;
    subcommand->add_option("toml_file", tomlFilename, "TOML filename")
        ->required();

    static std::string outputFilename = "graph.dot";
    subcommand->add_option("-o,--out", outputFilename, "Graph output filename");

    static bool useHtml = true;
    subcommand->add_flag("-s,--simple", useHtml, "Create a simpler graph view");

    auto graph = [&]()
    {
        using namespace erin;
        Logger logger{};
        Log log = get_standard_log(logger);
        std::ifstream ifs(tomlFilename, std::ios_base::binary);
        if (!ifs.good())
        {
            Log_Error(log, "Could not open input file stream on input file");
            return EXIT_FAILURE;
        }
        auto name_only = std::filesystem::path(tomlFilename).filename();
        auto data = toml::parse(ifs, name_only.string());
        ifs.close();
        std::unordered_set<std::string> componentTagsInUse =
            TOMLTable_ParseComponentTagsInUse(data);
        auto validation_info = SetupGlobalValidationInfo();
        auto maybe_sim = Simulation_ReadFromToml(
            data, validation_info, componentTagsInUse, log
        );
        if (!maybe_sim.has_value())
        {
            Log_Error(log, "Could not parse sim data from TOML");
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
    };

    subcommand->callback([&]() { graph(); });

    return subcommand;
}

CLI::App*
add_checkNetwork(CLI::App& app)
{
    auto subcommand = app.add_subcommand("check", "Check network for issues");

    static std::string tomlFilename;
    subcommand
        ->add_option("toml_input_file", tomlFilename, "TOML input file name")
        ->required();

    auto checkNetwork = [&]()
    {
        using namespace erin;
        Logger logger{};
        Log log = get_standard_log(logger);
        std::ifstream ifs(tomlFilename, std::ios_base::binary);
        if (!ifs.good())
        {
            Log_Error(log, "Could not open input file stream on input file");
            return EXIT_FAILURE;
        }
        auto nameOnly = std::filesystem::path(tomlFilename).filename();
        auto data = toml::parse(ifs, nameOnly.string());
        ifs.close();
        std::unordered_set<std::string> componentTagsInUse =
            TOMLTable_ParseComponentTagsInUse(data);
        auto validationInfo = SetupGlobalValidationInfo();
        auto maybeSim = Simulation_ReadFromToml(
            data, validationInfo, componentTagsInUse, log
        );
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
    };

    subcommand->callback([&]() { checkNetwork(); });

    return subcommand;
}

CLI::App*
add_update(CLI::App& app)
{
    auto subcommand =
        app.add_subcommand("update", "Update an ERIN 0.55 file to current");

    static std::string inputFilename;
    subcommand
        ->add_option("toml_input_file", inputFilename, "TOML input file name")
        ->required();

    static std::string outputFilename = "out.toml";
    subcommand->add_option(
        "toml_output_file", outputFilename, "TOML output file name"
    );

    static bool stripIds = false;
    subcommand->add_flag(
        "-s,--strip-ids",
        stripIds,
        "If specified, strips ids from the input file"
    );

    auto update = [&]()
    {
        std::ifstream ifs(inputFilename, std::ios_base::binary);
        if (!ifs.good())
        {
            std::cout << "Could not open input file stream on input file"
                      << std::endl;
            return EXIT_FAILURE;
        }
        using namespace erin;
        auto nameOnly = std::filesystem::path(inputFilename).filename();
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
                              << ".max_discharge = " << maxDischarge
                              << std::endl;
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
                if (stripIds && comp.contains("id"))
                {
                    comp.erase("id");
                    std::cout << "REMOVE components." << compName << ".id"
                              << std::endl;
                }
                if (compType == "converter"
                    && comp.contains("constant_efficiency"))
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
        if (stripIds)
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
                        std::cout << "REMOVE fragility_curve." << fcName
                                  << ".id" << std::endl;
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

        std::ofstream ofs(outputFilename, std::ios_base::binary);
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
    };

    subcommand->callback([&]() { update(); });

    return subcommand;
}

CLI::App*
add_packLoads(CLI::App& app)
{
    auto subcommand =
        app.add_subcommand("pack-loads", "Pack loads into a single csv file");

    static std::string tomlFilename;
    subcommand->add_option("toml_file", tomlFilename, "TOML filename")
        ->required();

    static std::string loadsFilename = "packed-loads.csv";
    subcommand->add_option(
        "-o,--outcsv",
        loadsFilename,
        "Packed-loads csv filename; default:packed-loads.csv"
    );

    static bool verbose = false;
    subcommand->add_flag("-v,--verbose", verbose, "Verbose output");

    auto packLoads = [&]()
    {
        erin::Logger logger{};
        erin::Log log = get_standard_log(logger);
        if (verbose)
        {
            std::cout << "input file: " << tomlFilename << std::endl;
            std::cout << "verbose: " << (verbose ? "true" : "false")
                      << std::endl;
        }
        std::ifstream ifs(tomlFilename, std::ios_base::binary);
        if (!ifs.good())
        {
            erin::Log_Error(
                log, "Could not open input file stream on input file"
            );
            return EXIT_FAILURE;
        }
        auto tomlFilenameOnly = std::filesystem::path(tomlFilename).filename();
        auto data = toml::parse(ifs, tomlFilenameOnly.string());
        ifs.close();
        auto const& loadTable = data.at("loads").as_table();
        auto validationInfo = erin::SetupGlobalValidationInfo();
        erin::ValidationInfo explicitValidation =
            validationInfo.Load_01Explicit;
        erin::ValidationInfo fileValidation = validationInfo.Load_02FileBased;
        auto maybeLoads =
            ParseLoads(loadTable, explicitValidation, fileValidation, log);
        if (!maybeLoads.has_value())
        {
            return EXIT_FAILURE;
        }
        std::vector<erin::Load> loads = std::move(maybeLoads.value());
        return erin::WritePackedLoads(loads, loadsFilename);
    };

    subcommand->callback([&]() { packLoads(); });

    return subcommand;
}

int
main(int argc, char** argv)
{
    int result = EXIT_SUCCESS;

    CLI::App app{"erin"};
    app.require_subcommand(0);

    add_version(app);
    add_limits(app);
    add_run(app);
    add_graph(app);
    add_checkNetwork(app);
    add_update(app);
    add_packLoads(app);

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
