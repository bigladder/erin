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
#include "erin/csv.h"
#include "compilation_settings.h"

int exit_code = EXIT_SUCCESS;

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

    static bool show_seed = false;
    subcommand->add_flag("-k,--show_seed", show_seed, "Show random seed used for simulation");

    static bool no_aggregate_groups = false;
    subcommand->add_flag("-n,--no-group", no_aggregate_groups, "Suppress group aggregation");

    static bool save_reliability_curves = false;
    subcommand->add_flag(
        "-r,--save-reliability", save_reliability_curves, "Save reliability curves");

    auto run_it = [&]()
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
            exit_code = EXIT_FAILURE;
            return;
        }
        auto name_only = std::filesystem::path(toml_filename).filename();
        toml::value data = toml::parse(ifs, name_only.string());
        ifs.close();
        std::unordered_set<std::string> component_tags_in_use =
            TOMLTable_parse_component_tags_in_use(data);
        auto validation_info = setup_global_validation_info();
        auto maybe_sim = read_from_toml(data, validation_info, component_tags_in_use, log);
        if (!maybe_sim.has_value())
        {
            Log_error(log, "Simulation returned without value");
            exit_code = EXIT_FAILURE;
            return;
        }
        Simulation s = std::move(maybe_sim.value());
        if (verbose)
        {
            print(s);
            Log_info(log, "-----------------");
        }
        run(s,
            log,
            events_filename,
            stats_filename,
            time_step_h,
            aggregate_groups,
            save_reliability_curves,
            verbose,
            show_seed);
    };

    subcommand->callback([&]() { run_it(); });

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
            exit_code = EXIT_FAILURE;
            return;
        }
        auto name_only = std::filesystem::path(toml_filename).filename();
        auto data = toml::parse(ifs, name_only.string());
        ifs.close();
        std::unordered_set<std::string> component_tags_in_use =
            TOMLTable_parse_component_tags_in_use(data);
        auto validation_info = setup_global_validation_info();
        auto maybe_sim = read_from_toml(data, validation_info, component_tags_in_use, log);
        if (!maybe_sim.has_value())
        {
            Log_error(log, "Could not parse sim data from TOML");
            exit_code = EXIT_FAILURE;
            return;
        }
        Simulation s = std::move(maybe_sim.value());
        std::string dot_data =
            network_to_dot(s.the_model.connection, s.the_model.component.tag, "", use_html);
        // save string from network_to_dot
        std::ofstream ofs(output_filename, std::ios_base::binary);
        if (!ofs.good())
        {
            std::cout << "Could not open output file stream on output file" << std::endl;
            exit_code = EXIT_FAILURE;
            return;
        }
        ofs << dot_data << std::endl;
        ofs.close();
    };

    subcommand->callback([&]() { graph(); });

    return subcommand;
}

CLI::App* add_check_network(CLI::App& app)
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
            exit_code = EXIT_FAILURE;
            return;
        }
        auto name_only = std::filesystem::path(toml_filename).filename();
        auto data = toml::parse(ifs, name_only.string());
        ifs.close();
        std::unordered_set<std::string> component_tags_in_use =
            TOMLTable_parse_component_tags_in_use(data);
        auto validationInfo = setup_global_validation_info();
        auto maybe_sim = read_from_toml(data, validationInfo, component_tags_in_use, log);
        if (!maybe_sim.has_value())
        {
            exit_code = EXIT_FAILURE;
            return;
        }
        Simulation s = std::move(maybe_sim.value());
        std::vector<std::string> issues = erin::check_network(s.the_model);
        if (issues.size() > 0)
        {
            std::cout << "ISSUES FOUND:" << std::endl;
            for (std::string const& issue : issues)
            {
                std::cout << issue << std::endl;
            }
            exit_code = EXIT_FAILURE;
            return;
        }
        std::cout << "No issues found with network." << std::endl;
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
            exit_code = EXIT_FAILURE;
            return;
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
            exit_code = EXIT_FAILURE;
            return;
        }
        ofs << data;
        ofs.close();
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
            exit_code = EXIT_FAILURE;
            return;
        }
        auto toml_filename_only = std::filesystem::path(toml_filename).filename();
        auto data = toml::parse(ifs, toml_filename_only.string());
        ifs.close();
        auto const& load_table = data.at("loads").as_table();
        auto validation_info = erin::setup_global_validation_info();
        erin::ValidationInfo explicit_validation = validation_info.load_explicit;
        erin::ValidationInfo file_validation = validation_info.load_file_based;
        auto maybeLoads = parse_loads(load_table, explicit_validation, file_validation, log);
        if (!maybeLoads.has_value())
        {
            exit_code = EXIT_FAILURE;
            return;
        }
        std::vector<erin::Load> loads = std::move(maybeLoads.value());
        exit_code = erin::write_packed_loads(loads, loads_filename);
    };

    subcommand->callback([&]() { pack_loads(); });

    return subcommand;
}

CLI::App* add_distribution_sampling(CLI::App& app)
{
    auto subcommand = app.add_subcommand("sample-dist", "Sample statistical distributions");
    static std::string distribution_name;
    subcommand
        ->add_option("distribution_name",
                     distribution_name,
                     "name of distribution: fixed, uniform, normal, weibull, or table")
        ->required();

    static std::string quantile_table_csv = "table.csv";
    subcommand->add_option(
        "-q,--table",
        quantile_table_csv,
        "csv file with the quantile (inverse cumulative distribution function) defined in two "
        "columns: variate and elapsed time (no header) (unit: s)");

    static std::string number_of_samples = "100";
    subcommand->add_option("-n,--number-of-samples",
                           number_of_samples,
                           "the number of times to sample the distribution");

    static std::string fixed_value_s = "3600";
    subcommand->add_option(
        "-f,--fixed", fixed_value_s, "fixed value of a fixed distribution (unit: s)");

    static std::string uniform_lower_bound_s = "0";
    subcommand->add_option("-l,--lower-bound",
                           uniform_lower_bound_s,
                           "lower bound of a uniform distribution (unit: s)");

    static std::string normal_mean_s = "3600";
    subcommand->add_option("-m,--mean", normal_mean_s, "mean of normal distribution (unit: s)");

    static std::string weibull_shape = "3";
    subcommand->add_option(
        "-s,--shape", weibull_shape, "shape parameter of Weibull distribution (unitless)");

    static std::string uniform_upper_bound_s = "3600";
    subcommand->add_option("-u,--upper-bound",
                           uniform_upper_bound_s,
                           "upper bound of a uniform distribution (unit: s)");

    static std::string normal_std_dev_s = "600";
    subcommand->add_option(
        "-d,--std-dev", normal_std_dev_s, "standard deviation of normal distribution (unit: s)");

    static std::string weibull_scale_s = "3600";
    subcommand->add_option(
        "-k,--scale", weibull_scale_s, "scale of Weibull distribution (unit: s)");

    static std::string weibull_location_s = "0";
    subcommand->add_option(
        "-z,--location", weibull_location_s, "location of Weibull distribution (unit: s)");

    auto sample_dist = [&]()
    {
        std::optional<erin::DistType> maybe_dist_type = erin::tag_to_dist_type(distribution_name);
        if (!maybe_dist_type.has_value())
        {
            std::ostringstream oss {};
            oss << "issue parsing distribution type: \"" << distribution_name
                << "\"; must be one of 'fixed', 'uniform', 'normal', 'weibull', or 'table'"
                << std::endl;
            std::cerr << oss.str();
            exit_code = EXIT_FAILURE;
            return;
        }
        erin::DistType dist_type = maybe_dist_type.value();
        erin::DistributionSystem ds {};
        std::string tag = "distribution";
        size_t id;
        size_t num_samples;
        try
        {
            num_samples = static_cast<size_t>(std::stol(number_of_samples));
        }
        catch (const std::exception&)
        {
            std::cerr << "ERROR: number of samples must be convertable to size_t\n";
            exit_code = EXIT_FAILURE;
            return;
        }
        switch (dist_type)
        {
        case erin::DistType::Fixed:
        {
            std::int64_t value_in_seconds {0};
            try
            {
                value_in_seconds = std::stol(fixed_value_s);
            }
            catch (const std::exception&)
            {
                std::cerr << "ERROR: value in seconds must be convertable to int64\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            try
            {
                id = ds.add_fixed(tag, value_in_seconds);
                std::cout << "Fixed Distribution" << std::endl;
                std::cout << "- fixed: " << value_in_seconds << " s" << std::endl;
            }
            catch (const std::exception&)
            {
                std::cerr << "ERROR: could not create a fixed distribution\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            break;
        }
        case erin::DistType::Uniform:
        {
            std::int64_t lower_bound_s = 0;
            std::int64_t upper_bound_s = 3600;
            try
            {
                lower_bound_s = std::stol(uniform_lower_bound_s);
            }
            catch (std::exception&)
            {
                std::cerr << "ERROR: lower bound must be convertable to int64_t\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            try
            {
                upper_bound_s = std::stol(uniform_upper_bound_s);
            }
            catch (std::exception&)
            {
                std::cerr << "ERROR: upper bound must be convertable to int64_t\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            try
            {
                id = ds.add_uniform(tag, lower_bound_s, upper_bound_s);
                std::cout << "Uniform Distribution" << std::endl;
                std::cout << "- lower bound: " << lower_bound_s << " s" << std::endl;
                std::cout << "- upper bound: " << upper_bound_s << " s" << std::endl;
            }
            catch (std::exception&)
            {
                std::cerr << "ERROR: unable to create uniform distribution\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            break;
        }
        case erin::DistType::Normal:
        {
            std::int64_t mean;
            try
            {
                mean = std::stol(normal_mean_s);
            }
            catch (std::exception&)
            {
                std::cerr << "ERROR: unable to parse mean for normal\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            std::int64_t std_dev;
            try
            {
                std_dev = std::stol(normal_std_dev_s);
            }
            catch (std::exception&)
            {
                std::cerr << "ERROR: unable to parse standard deviation for normal\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            try
            {
                id = ds.add_normal(tag, mean, std_dev);
                std::cout << "Normal Distribution" << std::endl;
                std::cout << "- mean              : " << mean << " s" << std::endl;
                std::cout << "- standard deviation: " << std_dev << " s" << std::endl;
            }
            catch (std::exception&)
            {
                std::cerr << "ERROR: unable to create normal distribution\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            break;
        }
        case erin::DistType::Weibull:
        {
            double shape;
            double scale;
            double location;
            try
            {
                shape = std::stod(weibull_shape);
            }
            catch (const std::exception&)
            {
                std::cerr << "ERROR: shape must be convertable to a double for Weibull\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            try
            {
                scale = std::stod(weibull_scale_s);
            }
            catch (const std::exception&)
            {
                std::cerr << "ERROR: scale must be convertable to a double for Weibull\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            try
            {
                location = std::stod(weibull_location_s);
            }
            catch (const std::exception&)
            {
                std::cerr << "ERROR: location must be convertable to a double for Weibull\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            try
            {
                id = ds.add_weibull(tag, shape, scale, location);
            }
            catch (const std::exception&)
            {
                std::cerr << "ERROR: could not create Weibull distribution\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            break;
        }
        case erin::DistType::QuantileTable:
        {
            std::vector<double> xs {};
            std::vector<double> dtimes_s {};
            std::ifstream ifs {quantile_table_csv};
            if (!ifs.is_open())
            {
                std::ostringstream oss {};
                oss << "input file stream on \"" << quantile_table_csv
                    << "\" failed to open for reading\n";
                std::cerr << oss.str() << "\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            for (int row {0}; ifs.good(); ++row)
            {
                std::string delim {""};
                auto cells = erin::read_row(ifs);
                auto csize {cells.size()};
                if (csize == 0)
                {
                    break;
                }
                if (csize != 2)
                {
                    std::ostringstream oss {};
                    oss << "issue reading input file csv \"" << quantile_table_csv
                        << "\"; issue on row " << row << "; number of columns should be 2 but got "
                        << csize << "\n";
                    std::cerr << oss.str() << "\n";
                    exit_code = EXIT_FAILURE;
                    return;
                }
                try
                {
                    xs.emplace_back(std::stod(cells[0]));
                    dtimes_s.emplace_back(std::stod(cells[1]));
                }
                catch (const std::exception&)
                {
                    std::ostringstream oss {};
                    oss << "issue reading input file csv \"" << quantile_table_csv
                        << "\"; issue on row " << row << "; could not conver xs (" << cells[0]
                        << ") or dtimes (" << cells[1] << " to double\n";
                    std::cerr << oss.str() << "\n";
                    exit_code = EXIT_FAILURE;
                    return;
                }
            }
            ifs.close();
            try
            {
                id = ds.add_quantile_table(tag, xs, dtimes_s);
            }
            catch (const std::exception&)
            {
                std::cerr << "ERROR: could not create tabular distribution\n";
                exit_code = EXIT_FAILURE;
                return;
            }
            break;
        }
        default:
        {
            std::ostringstream oss {};
            oss << "unhandled distribution type '" << erin::dist_type_to_tag(dist_type) << "'"
                << std::endl;
            std::cerr << oss.str();
            exit_code = EXIT_FAILURE;
            return;
        }
        }
        std::cout << "data\n";
        for (size_t idx = 0; idx < num_samples; ++idx)
        {
            try
            {
                double dt = ds.next_time_advance(id);
                std::uint64_t time_advance_s = static_cast<std::uint64_t>(dt);
                std::cout << time_advance_s << "\n";
            }
            catch (const std::exception&)
            {
                std::cerr << "ERROR: unknown error attempting to sample distribution on sample "
                          << idx << "\n";
                exit_code = EXIT_FAILURE;
                return;
            }
        }
        return;
    };
    subcommand->callback([&]() { sample_dist(); });

    return subcommand;
}

int main(int argc, char** argv)
{
    CLI::App app {"erin"};
    app.require_subcommand(0, 1);

    add_version(app);
    add_limits(app);
    add_run(app);
    add_graph(app);
    add_check_network(app);
    add_update(app);
    add_pack_loads(app);
    add_distribution_sampling(app);

    CLI11_PARSE(app, argc, argv);

    // call with no subcommands is equivalent to subcommand "help"
    if (argc == 1)
    {
        std::cout << "ERIN - Energy Resilience of Interacting Networks\n"
                  << "Version " << erin::version::version_string << "\n"
                  << std::endl;
        std::cout << app.help() << std::endl;
    }

    return exit_code;
}
