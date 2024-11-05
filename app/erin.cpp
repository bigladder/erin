// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <CLI/CLI.hpp>
#include <toml.hpp>

#include "erin/all.h"
#include "compilation_settings.h"

erin::Log get_standard_log(erin::Logger& logger)
{
    using namespace erin;
    Log log = Log_make_from_courier(logger);
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
            std::cout << fmt::format("[{}] {}: {}", "ERROR", tag, msg) << std::endl;
        }
    };
    return log;
}

CLI::App* add_version(CLI::App& app)
{
    auto subcommand = app.add_subcommand("version", "Display version");

    auto version = [&]()
    {
        std::cout << "Version: " << erin::version::version_string << "\n";
        std::cout << "(git hash: " << erin::version::git_hash << ")\n";
        std::cout << "Build Type: " << build_type << "\n";
    };

    subcommand->callback([&]() { version(); });

    return subcommand;
}

CLI::App* add_limits(CLI::App& app)
{
    auto subcommand = app.add_subcommand("limits", "Display limits");

    auto limits = [&]()
    {
        std::cout << "Limits: " << std::endl;
        std::cout << "- value of max_flow_W: " << erin::max_flow_W << std::endl;
        std::cout << "- max_flow_W ==     9223372036854776: "
                  << (erin::max_flow_W == 9'223'372'036'854'776ULL) << std::endl;
        std::cout << "- max_flow_W == 18446744073709551615: "
                  << (erin::max_flow_W == 18'446'744'073'709'551'615ULL) << std::endl;
        std::cout << "- sizeof(uint64_t): " << (sizeof(uint64_t)) << std::endl;
        std::cout << "- std::numeric_limits<uint64_t>::max(): "
                  << std::numeric_limits<uint64_t>::max() << std::endl;
        std::cout << "- sizeof(uint32_t): " << (sizeof(uint32_t)) << std::endl;
        std::cout << "- std::numeric_limits<uint32_t>::max(): "
                  << std::numeric_limits<uint32_t>::max() << std::endl;
        std::cout << "- sizeof(unsigned int): " << (sizeof(unsigned int)) << std::endl;
        std::cout << "- std::numeric_limits<unsigned int>::max(): "
                  << std::numeric_limits<unsigned int>::max() << std::endl;
        std::cout << "- sizeof(unsigned long): " << (sizeof(unsigned long)) << std::endl;
        std::cout << "- std::numeric_limits<unsigned long>::max(): "
                  << std::numeric_limits<unsigned long>::max() << std::endl;
        std::cout << "- sizeof(unsigned long long): " << (sizeof(unsigned long long)) << std::endl;
        std::cout << "- std::numeric_limits<unsigned long long>::max(): "
                  << std::numeric_limits<unsigned long long>::max() << std::endl;
    };

    subcommand->callback([&]() { limits(); });

    return subcommand;
}

CLI::App* add_run(CLI::App& app)
{
    auto subcommand = app.add_subcommand("run", "Run a simulation");

    static std::string toml_filename;
    subcommand->add_option("toml_file", toml_filename, "TOML filename")->required();

    static std::string events_filename = "out.csv";
    subcommand->add_option("-e,--events", events_filename, "Events csv filename; default:out.csv");

    static std::string stats_filename = "stats.csv";
    subcommand->add_option(
        "-s,--statistics", stats_filename, "Statistics csv filename; default:stats.csv");

    static double time_step_h = -1.0;
    subcommand->add_option("-t,--time_step_h", time_step_h, "Report with uniform time step (hours)")
        ->check(CLI::PositiveNumber);

    static bool verbose = false;
    subcommand->add_flag("-v,--verbose", verbose, "Verbose output");

    static bool no_aggregate_groups = false;
    subcommand->add_flag("-n,--no-group", no_aggregate_groups, "Suppress group aggregation");

    static bool save_reliability_curves = false;
    subcommand->add_flag(
        "-r,--save-reliability", save_reliability_curves, "Save reliability curves");

    auto run = [&]()
    {
        using namespace erin;
        Logger logger {};
        Log log = get_standard_log(logger);
        bool aggregate_groups = !no_aggregate_groups;
        if (verbose)
        {
            std::cout << "input file: " << toml_filename << std::endl;
            std::cout << "events file: " << events_filename << std::endl;
            std::cout << "statistics file: " << stats_filename << std::endl;
            if (time_step_h > 0.0)
            {
                std::cout << "time step (h): " << time_step_h << std::endl;
            }
            std::cout << "save reliability curves: " << (save_reliability_curves ? "true" : "false")
                      << std::endl;
            std::cout << "verbose: " << (verbose ? "true" : "false") << std::endl;
            std::cout << "groups: " << (aggregate_groups ? "true" : "false") << std::endl;
        }
        std::ifstream ifs(toml_filename, std::ios_base::binary);
        if (!ifs.good())
        {
            Log_error(log, "Could not open input file stream on input file");
            return EXIT_FAILURE;
        }
        auto name_only = std::filesystem::path(toml_filename).filename();
        toml::value data = toml::parse(ifs, name_only.string());
        ifs.close();
        std::unordered_set<std::string> component_tags_in_use =
            TOMLTable_parse_component_tags_in_use(data);
        auto validation_info = setup_global_validation_info();
        auto maybe_sim =
            Simulation_read_from_toml(data, validation_info, component_tags_in_use, log);
        if (!maybe_sim.has_value())
        {
            Log_error(log, "Simulation returned without value");
            return EXIT_FAILURE;
        }
        Simulation s = std::move(maybe_sim.value());
        if (verbose)
        {
            Simulation_print(s);
            Log_info(log, "-----------------");
        }
        Simulation_run(s,
                       log,
                       events_filename,
                       stats_filename,
                       time_step_h,
                       aggregate_groups,
                       save_reliability_curves,
                       verbose);
        return EXIT_SUCCESS;
    };

    subcommand->callback([&]() { run(); });

    return subcommand;
}

CLI::App* add_graph(CLI::App& app)
{
    auto subcommand = app.add_subcommand("graph", "Graph a simulation");

    static std::string toml_filename;
    subcommand->add_option("toml_file", toml_filename, "TOML filename")->required();

    static std::string output_filename = "graph.dot";
    subcommand->add_option("-o,--out", output_filename, "Graph output filename");

    static bool use_html = true;
    subcommand->add_flag("-s,--simple", use_html, "Create a simpler graph view");

    auto graph = [&]()
    {
        using namespace erin;
        Logger logger {};
        Log log = get_standard_log(logger);
        std::ifstream ifs(toml_filename, std::ios_base::binary);
        if (!ifs.good())
        {
            Log_error(log, "Could not open input file stream on input file");
            return EXIT_FAILURE;
        }
        auto name_only = std::filesystem::path(toml_filename).filename();
        auto data = toml::parse(ifs, name_only.string());
        ifs.close();
        std::unordered_set<std::string> component_tags_in_use =
            TOMLTable_parse_component_tags_in_use(data);
        auto validation_info = setup_global_validation_info();
        auto maybe_sim =
            Simulation_read_from_toml(data, validation_info, component_tags_in_use, log);
        if (!maybe_sim.has_value())
        {
            Log_error(log, "Could not parse sim data from TOML");
            return EXIT_FAILURE;
        }
        Simulation s = std::move(maybe_sim.value());
        std::string dot_data =
            network_to_dot(s.TheModel.connection, s.TheModel.component.tag, "", use_html);
        // save string from network_to_dot
        std::ofstream ofs(output_filename, std::ios_base::binary);
        if (!ofs.good())
        {
            std::cout << "Could not open output file stream on output file" << std::endl;
            return EXIT_FAILURE;
        }
        ofs << dot_data << std::endl;
        ofs.close();
        return EXIT_SUCCESS;
    };

    subcommand->callback([&]() { graph(); });

    return subcommand;
}

CLI::App* add_checkNetwork(CLI::App& app)
{
    auto subcommand = app.add_subcommand("check", "Check network for issues");

    static std::string toml_filename;
    subcommand->add_option("toml_input_file", toml_filename, "TOML input file name")->required();

    auto check_network = [&]()
    {
        using namespace erin;
        Logger logger {};
        Log log = get_standard_log(logger);
        std::ifstream ifs(toml_filename, std::ios_base::binary);
        if (!ifs.good())
        {
            Log_error(log, "Could not open input file stream on input file");
            return EXIT_FAILURE;
        }
        auto name_only = std::filesystem::path(toml_filename).filename();
        auto data = toml::parse(ifs, name_only.string());
        ifs.close();
        std::unordered_set<std::string> component_tags_in_use =
            TOMLTable_parse_component_tags_in_use(data);
        auto validationInfo = setup_global_validation_info();
        auto maybe_sim =
            Simulation_read_from_toml(data, validationInfo, component_tags_in_use, log);
        if (!maybe_sim.has_value())
        {
            return EXIT_FAILURE;
        }
        Simulation s = std::move(maybe_sim.value());
        std::vector<std::string> issues = erin::Model_check_network(s.TheModel);
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

    subcommand->callback([&]() { check_network(); });

    return subcommand;
}

CLI::App* add_update(CLI::App& app)
{
    auto subcommand = app.add_subcommand("update", "Update an ERIN 0.55 file to current");

    static std::string input_filename;
    subcommand->add_option("toml_input_file", input_filename, "TOML input file name")->required();

    static std::string output_filename = "out.toml";
    subcommand->add_option("toml_output_file", output_filename, "TOML output file name");

    static bool strip_ids = false;
    subcommand->add_flag(
        "-s,--strip-ids", strip_ids, "If specified, strips ids from the input file");

    auto update = [&]()
    {
        std::ifstream ifs(input_filename, std::ios_base::binary);
        if (!ifs.good())
        {
            std::cout << "Could not open input file stream on input file" << std::endl;
            return EXIT_FAILURE;
        }
        using namespace erin;
        auto name_only = std::filesystem::path(input_filename).filename();
        auto data = toml::parse(ifs, name_only.string());
        ifs.close();

        // rename networks.* to network
        if (data.contains("networks"))
        {
            auto new_node = std::unordered_map<std::string, toml::value> {};
            auto nw = data["networks"].as_table();
            for (auto it = nw.begin(); it != nw.end(); ++it)
            {
                std::string const& nw_name = it->first;
                std::cout << "CHANGE .networks." << nw_name << " to .network" << std::endl;
                auto nw_table = it->second.as_table();
                new_node["connections"] = nw_table["connections"];
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
                std::cout << "ADD simulation_info.input_format_version = " << current_input_version
                          << std::endl;
            }
            sim_info_table["input_format_version"] = current_input_version;
        }
        // add missing fields to components
        if (data.contains("components"))
        {
            auto& components = data["components"].as_table();
            for (auto it = components.begin(); it != components.end(); ++it)
            {
                std::string const& comp_name = it->first;
                auto& comp = it->second.as_table();
                std::string comp_type = comp["type"].as_string();
                if (comp_type == "store" && !comp.contains("max_discharge"))
                {
                    double max_discharge =
                        comp["max_inflow"].is_floating()
                            ? comp["max_inflow"].as_floating()
                            : static_cast<double>(comp["max_inflow"].as_integer());
                    comp["max_discharge"] = toml::value(max_discharge);
                    std::cout << "ADD components." << comp_name
                              << ".max_discharge = " << max_discharge << std::endl;
                }
                if (comp_type == "store" && comp.contains("max_inflow"))
                {
                    double max_inflow = comp["max_inflow"].is_floating()
                                            ? comp["max_inflow"].as_floating()
                                            : static_cast<double>(comp["max_inflow"].as_integer());
                    comp.erase("max_inflow");
                    comp["max_charge"] = toml::value(max_inflow);
                    std::cout << "RENAME components." << comp_name << ".max_inflow to components"
                              << comp_name << ".max_charge" << std::endl;
                }
                if (comp_type == "muxer" && comp.contains("dispatch_strategy"))
                {
                    comp.erase("dispatch_strategy");
                    std::cout << "REMOVE components." << comp_name << ".dispatch_strategy"
                              << std::endl;
                }
                if (strip_ids && comp.contains("id"))
                {
                    comp.erase("id");
                    std::cout << "REMOVE components." << comp_name << ".id" << std::endl;
                }
                if (comp_type == "converter" && comp.contains("constant_efficiency"))
                {
                    double eff =
                        comp["constant_efficiency"].is_floating()
                            ? comp["constant_efficiency"].as_floating()
                            : static_cast<double>(comp["constant_efficiency"].as_integer());
                    if (eff > 1.0)
                    {
                        comp["type"] = toml::value("mover");
                        comp["cop"] = toml::value(eff);
                        comp.erase("constant_efficiency");
                        std::cout << "CHANGE components." << comp_name << ".type to mover"
                                  << std::endl;
                    }
                }
            }
        }
        if (strip_ids)
        {
            if (data.contains("simulation_info"))
            {
                auto& sim_info_table = data["simulation_info"].as_table();
                if (sim_info_table.contains("id"))
                {
                    sim_info_table.erase("id");
                    std::cout << "REMOVE simulation_info.id" << std::endl;
                }
            }
            if (data.contains("fragility_mode"))
            {
                auto& fms = data["fragility_mode"].as_table();
                for (auto it = fms.begin(); it != fms.end(); ++it)
                {
                    std::string const& fm_name = it->first;
                    auto& fm = it->second.as_table();
                    if (fm.contains("id"))
                    {
                        fm.erase("id");
                        std::cout << "REMOVE fragility_mode." << fm_name << ".id" << std::endl;
                    }
                }
            }
            if (data.contains("failure_mode"))
            {
                auto& fms = data["failure_mode"].as_table();
                for (auto it = fms.begin(); it != fms.end(); ++it)
                {
                    std::string const& fm_name = it->first;
                    auto& fm = it->second.as_table();
                    if (fm.contains("id"))
                    {
                        fm.erase("id");
                        std::cout << "REMOVE failure_mode." << fm_name << ".id" << std::endl;
                    }
                }
            }
            if (data.contains("fragility_curve"))
            {
                auto& fcs = data["fragility_curve"].as_table();
                for (auto it = fcs.begin(); it != fcs.end(); ++it)
                {
                    std::string const& fc_name = it->first;
                    auto& fc = it->second.as_table();
                    if (fc.contains("id"))
                    {
                        fc.erase("id");
                        std::cout << "REMOVE fragility_curve." << fc_name << ".id" << std::endl;
                    }
                }
            }
            if (data.contains("dist"))
            {
                auto& dists = data["dist"].as_table();
                for (auto it = dists.begin(); it != dists.end(); ++it)
                {
                    std::string const& dist_name = it->first;
                    auto& dist_table = it->second.as_table();
                    if (dist_table.contains("id"))
                    {
                        dist_table.erase("id");
                        std::cout << "REMOVE dist." << dist_name << ".id" << std::endl;
                    }
                }
            }
        }

        std::ofstream ofs(output_filename, std::ios_base::binary);
        if (!ofs.good())
        {
            std::cout << "Could not open ouptut file stream for output file" << std::endl;
            return EXIT_FAILURE;
        }
        ofs << data;
        ofs.close();
        return EXIT_SUCCESS;
    };

    subcommand->callback([&]() { update(); });

    return subcommand;
}

CLI::App* add_pack_loads(CLI::App& app)
{
    auto subcommand = app.add_subcommand("pack-loads", "Pack loads into a single csv file");

    static std::string toml_filename;
    subcommand->add_option("toml_file", toml_filename, "TOML filename")->required();

    static std::string loads_filename = "packed-loads.csv";
    subcommand->add_option(
        "-o,--outcsv", loads_filename, "Packed-loads csv filename; default:packed-loads.csv");

    static bool verbose = false;
    subcommand->add_flag("-v,--verbose", verbose, "Verbose output");

    auto pack_loads = [&]()
    {
        erin::Logger logger {};
        erin::Log log = get_standard_log(logger);
        if (verbose)
        {
            std::cout << "input file: " << toml_filename << std::endl;
            std::cout << "verbose: " << (verbose ? "true" : "false") << std::endl;
        }
        std::ifstream ifs(toml_filename, std::ios_base::binary);
        if (!ifs.good())
        {
            erin::Log_error(log, "Could not open input file stream on input file");
            return EXIT_FAILURE;
        }
        auto toml_filename_only = std::filesystem::path(toml_filename).filename();
        auto data = toml::parse(ifs, toml_filename_only.string());
        ifs.close();
        auto const& load_table = data.at("loads").as_table();
        auto validation_info = erin::setup_global_validation_info();
        erin::ValidationInfo explicit_validation = validation_info.Load_01Explicit;
        erin::ValidationInfo file_validation = validation_info.Load_02FileBased;
        auto maybeLoads = parse_loads(load_table, explicit_validation, file_validation, log);
        if (!maybeLoads.has_value())
        {
            return EXIT_FAILURE;
        }
        std::vector<erin::Load> loads = std::move(maybeLoads.value());
        return erin::write_packed_loads(loads, loads_filename);
    };

    subcommand->callback([&]() { pack_loads(); });

    return subcommand;
}

int main(int argc, char** argv)
{
    int result = EXIT_SUCCESS;

    CLI::App app {"erin"};
    app.require_subcommand(0);

    add_version(app);
    add_limits(app);
    add_run(app);
    add_graph(app);
    add_checkNetwork(app);
    add_update(app);
    add_pack_loads(app);

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
