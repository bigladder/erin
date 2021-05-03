/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/input_reader.h"
#include "erin_csv.h"
#include "debug_utils.h"
#include "toml_helper.h"
#include "erin_generics.h"

namespace ERIN
{
  std::unordered_map<std::string, std::vector<RealTimeType>>
  calc_scenario_schedule(
      const RealTimeType max_time_s, 
      const std::unordered_map<std::string, Scenario>& scenarios,
      const erin::distribution::DistributionSystem& ds,
      const std::function<double()>& rand_fn)
  {
    std::unordered_map<std::string, std::vector<RealTimeType>> scenario_sch{};
    if (scenarios.size() == 0) {
      return scenario_sch;
    }
    for (const auto& s : scenarios) {
      const auto& scenario_id = s.first;
      const auto& scenario = s.second;
      const auto dist_id = scenario.get_occurrence_distribution_id();
      const auto max_occurrences = scenario.get_max_occurrences();
      RealTimeType t{0};
      int num_occurrences{0};
      std::vector<RealTimeType> start_times{};
      while (true) {
        t += ds.next_time_advance(dist_id, rand_fn());
        if (t <= max_time_s) {
          num_occurrences += 1;
          if ((max_occurrences != -1) && (num_occurrences > max_occurrences)) {
            break;
          }
          start_times.emplace_back(t);
        } else {
          break;
        }
      }
      scenario_sch.emplace(scenario_id, std::move(start_times));
    }
    return scenario_sch;
  }

  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<erin::fragility::FailureProbAndRepair>>>
  generate_failure_fragilities(
    const std::unordered_map<std::string, Scenario>& scenarios,
    const std::unordered_map<std::string, std::unique_ptr<Component>>& components)
  {
    namespace EF = erin::fragility;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<erin::fragility::FailureProbAndRepair>>> out{};
    for (const auto& s: scenarios) {
      const auto& intensities = s.second.get_intensities();
      std::unordered_map<std::string, std::vector<EF::FailureProbAndRepair>> m{};
      if (intensities.size() > 0) {
        for (const auto& c_pair: components) {
          const auto& c = c_pair.second;
          if (!(c->is_fragile())) {
            continue;
          }
          auto probs = c->apply_intensities(intensities);
          // Sort with the highest probability first; that way if a 1.0 is
          // present, we can end early...
          std::sort(
              probs.begin(), probs.end(),
              [](const EF::FailureProbAndRepair& a, const EF::FailureProbAndRepair& b){
                return a.failure_probability > b.failure_probability;
              });
          if (probs.size() == 0) {
            // no need to add an empty vector of probabilities
            // note: probabilitie of 0.0 (i.e., never fail) are not added in
            // apply_intensities
            continue;
          }
          m[c_pair.first] = probs;
        }
      }
      out[s.first] = m;
    }
    return out;
  }

  InputReader::InputReader(toml::value v):
      sim_info{},
      components{},
      networks{},
      scenarios{},
      reliability_schedule{},
      scenario_schedules{},
      fragility_info_by_comp_tag_by_instance_by_scenario_tag{}
  {
    namespace EF = erin::fragility;
    TomlInputReader reader{v};
    sim_info = reader.read_simulation_info();
    // Read data into private class fields
    const auto loads_by_id = reader.read_loads();
    const auto fragility_curves = reader.read_fragility_curve_data();
    erin::distribution::DistributionSystem ds{};
    // dists is map<string, size_type>
    auto dists = reader.read_distributions(ds);
    // TODO: ensure that fragility modes reference correct fragility curves and distributions
    auto fragility_modes = reader.read_fragility_modes(dists, fragility_curves);
    ReliabilityCoordinator rc{};
    // fms is map<string, size_type>
    auto fms = reader.read_failure_modes(dists, rc);
    // components needs to be modified to add component_id as size_type?
    components = reader.read_components(
      loads_by_id, fragility_curves, fragility_modes, fms, rc);
    networks = reader.read_networks();
    scenarios = reader.read_scenarios(dists);
    const auto max_time_s{sim_info.get_max_time_in_seconds()};
    auto rand_fn = sim_info.make_random_function();
    reliability_schedule = rc.calc_reliability_schedule_by_component_tag(
        rand_fn, ds, max_time_s);
    scenario_schedules = calc_scenario_schedule(
        max_time_s, scenarios, ds, rand_fn);
    const auto failure_probs_by_comp_id_by_scenario_id = generate_failure_fragilities(
      scenarios, components);
    fragility_info_by_comp_tag_by_instance_by_scenario_tag =
      erin::fragility::calc_fragility_schedules(
        scenario_schedules,
        failure_probs_by_comp_id_by_scenario_id,
        rand_fn,
        ds);
  }

  InputReader::InputReader(const std::string& path):
    InputReader(toml::parse(path))
  {
  }

  InputReader::InputReader(std::istream& in):
    InputReader(toml::parse(in, "<input from istream>"))
  {
  }

  std::unordered_map<std::string, std::unique_ptr<Component>>
  InputReader::get_components() const
  {
    std::unordered_map<std::string, std::unique_ptr<Component>> out{};
    for (const auto& tag_comp : components) {
      out[tag_comp.first] = tag_comp.second->clone();
    }
    return out;
  }

  ////////////////////////////////////////////////////////////
  // TomlInputReader
  TomlInputReader::TomlInputReader(toml::value data_):
    data{std::move(data_)}
  {
    check_top_level_entries();
  }

  TomlInputReader::TomlInputReader(const std::string& path):
    data{}
  {
    data = toml::parse(path);
    check_top_level_entries();
  }

  TomlInputReader::TomlInputReader(std::istream& in):
    data{}
  {
    data = toml::parse(in, "<input from istream>");
    check_top_level_entries();
  }

  SimulationInfo
  TomlInputReader::read_simulation_info()
  {
    const auto& sim_info = toml::find(data, "simulation_info");
    const auto& tt = toml::find<toml::table>(data, "simulation_info");
    const std::string rate_unit =
      toml::find_or(sim_info, "rate_unit", "kW");
    const std::string quantity_unit =
      toml::find_or(sim_info, "quantity_unit", "kJ");
    const std::string time_tag =
      toml::find_or(sim_info, "time_unit", "years");
    const auto time_units = tag_to_time_units(time_tag);
    bool has_fixed{false};
    auto fixed_random = read_fixed_random_for_sim_info(tt, has_fixed);
    bool has_series{false};
    auto fixed_series = read_fixed_series_for_sim_info(tt, has_series);
    bool has_seed{false};
    auto seed = read_random_seed_for_sim_info(tt, has_seed);
    const RealTimeType max_time =
      static_cast<RealTimeType>(toml::find_or(sim_info, "max_time", 1000));
    auto ri = make_random_info(
        has_fixed, fixed_random,
        has_seed, seed,
        has_series, fixed_series);
    return SimulationInfo{
      rate_unit, quantity_unit, time_units, max_time, std::move(ri)};
  }

  double
  TomlInputReader::read_fixed_random_for_sim_info(
      const toml::table& tt, bool& has_fixed) const
  {
    auto it_fixed_random = tt.find("fixed_random");
    has_fixed = (it_fixed_random != tt.end());
    double fixed_random{0.0};
    if (has_fixed) {
      fixed_random = read_number(it_fixed_random->second);
    }
    return fixed_random;
  }

  std::vector<double>
  TomlInputReader::read_fixed_series_for_sim_info(
      const toml::table& tt, bool& has_series) const
  {
    auto it_fixed_series = tt.find("fixed_random_series");
    has_series = (it_fixed_series != tt.end());
    auto fixed_series = std::vector<double>{};
    if (has_series) {
      const auto& xs =
        toml::get<std::vector<toml::value>>(it_fixed_series->second);
      for (const auto& x : xs) {
        fixed_series.emplace_back(read_number(x));
      }
    }
    else {
      fixed_series.emplace_back(0.0);
    }
    return fixed_series;
  }

  unsigned int
  TomlInputReader::read_random_seed_for_sim_info(
      const toml::table& tt, bool& has_seed) const
  {
    auto it_random_seed = tt.find("random_seed");
    has_seed = (it_random_seed != tt.end());
    unsigned int seed{0};
    if (has_seed) {
      try {
        seed = toml::get<unsigned int>(it_random_seed->second);
      }
      catch (toml::exception&) {
        seed = static_cast<unsigned int>(
            toml::get<double>(it_random_seed->second));
      }
    }
    return seed;
  }

  std::unordered_map<std::string, std::string>
  TomlInputReader::read_streams(const SimulationInfo&)
  {
    const auto toml_streams = toml::find<toml::table>(data, "streams");
    std::unordered_map<std::string, std::string> stream_types_map;
    for (const auto& s: toml_streams) {
      toml::value t = s.second;
      toml::table tt = toml::get<toml::table>(t);
      auto other_rate_units = std::unordered_map<std::string,FlowValueType>();
      auto other_quantity_units =
        std::unordered_map<std::string,FlowValueType>();
      auto it1 = tt.find("other_rate_units");
      if (it1 != tt.end()) {
        const auto oru = toml::get<toml::table>(it1->second);
        for (const auto& p: oru) {
          other_rate_units.insert(
              std::pair<std::string, FlowValueType>(
                p.first,
                read_number(p.second)));
        }
      }
      auto it2  = tt.find("other_quantity_units");
      if (it2 != tt.end()) {
        const auto oqu = toml::get<toml::table>(it2->second);
        for (const auto& p: oqu)
          other_quantity_units.insert(std::pair<std::string,FlowValueType>(
                p.first,
                read_number(p.second)));
      }
      const std::string stream_type{toml::find<std::string>(t, "type")};
      stream_types_map.insert(std::make_pair(s.first, stream_type));
    }
    if constexpr (debug_level >= debug_level_high) {
      for (const auto& x: stream_types_map) {
        std::cerr << "stream type: " << x.first << "\n";
      }
    }
    return stream_types_map;
  }

  std::unordered_map<std::string, std::vector<LoadItem>>
  TomlInputReader::read_loads()
  {
    std::unordered_map<std::string, std::vector<LoadItem>> loads_by_id;
    const auto toml_loads = toml::find<toml::table>(data, "loads");
    if constexpr (debug_level >= debug_level_high) {
      std::cout << toml_loads.size() << " load entries found\n";
    }
    for (const auto& load_item: toml_loads) {
      toml::value tv = load_item.second;
      toml::table tt = toml::get<toml::table>(tv);
      std::vector<LoadItem> the_loads{};
      auto load_id = load_item.first;
      auto it = tt.find("time_rate_pairs");
      if (it == tt.end()) {
        auto it2 = tt.find("csv_file");
        if (it2 == tt.end()) {
          std::ostringstream oss;
          oss << "BadInputError: missing field 'csv_file' in "
              << "'loads' section for " << load_id << "\"";
          throw std::runtime_error(oss.str());
        }
        auto csv_file = toml::get<std::string>(it2->second);
        the_loads = load_loads_from_csv(csv_file);
        if constexpr (debug_level >= debug_level_high) {
          int load_num{0};
          for (const auto& ld : the_loads) {
            std::cout << "- " << load_num << ": " << ld << "\n";
            ++load_num;
          }
        }
      }
      else {
        const auto load_array = toml::get<std::vector<toml::value>>(it->second);
        const std::string not_found{"not_found"};
        const auto time_unit_tag = toml::find_or(
            tv, "time_unit", not_found);
        if (time_unit_tag == not_found) {
          std::ostringstream oss;
          oss << "required field 'time_unit' not found for load '"
              << load_id << "'\n";
          throw std::runtime_error(oss.str());
        }
        TimeUnits time_unit{TimeUnits::Hours};
        try {
          time_unit = tag_to_time_units(time_unit_tag);
        }
        catch (std::invalid_argument& e) {
          std::ostringstream oss;
          oss << "unhandled time unit '" << time_unit_tag << "' "
              << "for load '" << load_id << "'\n"; 
          oss << "original error: " << e.what() << "\n";
          throw std::runtime_error(oss.str());
        }
        const auto rate_unit_tag = toml::find_or(
            tv, "rate_unit", not_found);
        if (rate_unit_tag == not_found) {
          std::ostringstream oss;
          oss << "required field 'rate_unit' not found for load '"
              << load_id << "'\n";
          throw std::runtime_error(oss.str());
        }
        auto rate_unit = RateUnits::KiloWatts;
        try {
          rate_unit = tag_to_rate_units(rate_unit_tag);
        }
        catch (std::invalid_argument& e) {
          std::ostringstream oss;
          oss << "unhandled rate unit '" << rate_unit_tag << "' "
              << "for load '" << load_id << "'\n"; 
          oss << "original error: " << e.what() << "\n";
          throw std::runtime_error(oss.str());
        }
        if (rate_unit != RateUnits::KiloWatts) {
          std::ostringstream oss;
          oss << "unsupported 'rate_unit' '" << rate_unit_tag << " for load '"
              << load_id << "'\n";
          oss << "supported rate units: 'kW'\n";
          throw std::runtime_error(oss.str());
        }
        try {
          the_loads = get_loads_from_array(load_array, time_unit, rate_unit);
        }
        catch (std::runtime_error& e) {
          std::ostringstream oss;
          oss << "could not read loads array for load '" << load_id << "'\n";
          oss << "orignal error: " << e.what() << "\n";
          throw std::runtime_error(e.what());
        }
      }
      loads_by_id.insert(std::make_pair(load_item.first, the_loads));
    }
    return loads_by_id;
  }

  double
  TomlInputReader::read_number(const toml::value& v) const
  {
    if (v.is_floating()) {
      return v.as_floating(std::nothrow);
    }
    return static_cast<double>(v.as_integer());
  }

  double
  TomlInputReader::read_number(const std::string& v) const
  {
    double out{0.0};
    try {
      out = std::stod(v);
    }
    catch (std::invalid_argument&) {
      out = static_cast<double>(std::stoi(v));
    }
    return out;
  }


  std::optional<double>
  TomlInputReader::read_optional_number(const toml::table& tt, const std::string& key)
  {
    const auto& it = tt.find(key);
    if (it == tt.end()) {
      return {};
    }
    return read_number(it->second);
  }


  double
  TomlInputReader::read_number_at(const toml::table& tt, const std::string& key)
  {
    const auto& it = tt.find(key);
    if (it == tt.end()) {
      throw std::runtime_error("expected key '" + key + "' to exist");
    }
    return read_number(it->second);
  }


  std::vector<LoadItem>
  TomlInputReader::get_loads_from_array(
      const std::vector<toml::value>& load_array,
      TimeUnits time_units,
      RateUnits rate_units) const
  {
    if (rate_units != RateUnits::KiloWatts) {
      std::ostringstream oss;
      oss << "unhandled rate units '" << rate_units_to_tag(rate_units) << "'";
      throw std::runtime_error(oss.str());
    }
    std::vector<LoadItem> the_loads{};
    for (const auto& item: load_array) {
      const auto& xs = toml::get<std::vector<toml::value>>(item);
      const auto size = xs.size();
      RealTimeType the_time{};
      FlowValueType the_value{};
      if (size != 2) {
        std::ostringstream oss;
        oss << "time_rate_pairs must be 2 elements in length;"
            << "size = " << size << "\n";
        throw std::runtime_error(oss.str());
      }
      the_time = time_to_seconds(read_number(xs[0]), time_units);
      the_value = static_cast<FlowValueType>(read_number(xs[1]));
      the_loads.emplace_back(LoadItem{the_time, the_value});
    }
    return the_loads;
  }

  std::vector<LoadItem>
  TomlInputReader::load_loads_from_csv(const std::string& csv_path) const
  {
    std::vector<LoadItem> the_loads{};
    TimeUnits time_units{TimeUnits::Hours};
    RateUnits rate_units{RateUnits::KiloWatts};
    RealTimeType t{0};
    FlowValueType v{0.0};
    std::ifstream ifs{csv_path};
    if (!ifs.is_open()) {
      std::ostringstream oss;
      oss << "input file stream on \"" << csv_path
          << "\" failed to open for reading\n";
      throw std::runtime_error(oss.str());
    }
    bool in_header{true};
    for (int row{0}; ifs.good(); ++row) {
      auto cells = erin_csv::read_row(ifs);
      if (cells.size() == 0) {
        break;
      }
      if (in_header) {
        in_header = false;
        if (cells.size() < 2) {
          std::ostringstream oss;
          oss << "badly formatted file \"" << csv_path << "\"\n";
          oss << "row: " << row << "\n";
          erin_csv::stream_out(oss, cells);
          ifs.close();
          throw std::runtime_error(oss.str());
        }
        try {
          time_units = tag_to_time_units(cells[0]);
        }
        catch (std::invalid_argument& e) {
          std::ostringstream oss;
          oss << "in file '" << csv_path
            << "', first column should be a time unit tag but is '"
            << cells[0] << "'";
          oss << "row: " << row << "\n";
          erin_csv::stream_out(oss, cells);
          oss << "original error: " << e.what() << "\n";
          ifs.close();
          throw std::runtime_error(oss.str());
        }
        try {
          rate_units = tag_to_rate_units(cells[1]);
        }
        catch (std::invalid_argument& e) {
          std::ostringstream oss;
          oss << "in file '" << csv_path
              << "', second column should be a rate unit tag but is '"
              << cells[1] << "'";
          oss << "row: " << "\n";
          erin_csv::stream_out(oss, cells);
          oss << "original error: " << e.what() << "\n";
          ifs.close();
          throw std::runtime_error(oss.str());
        }
        if (rate_units != RateUnits::KiloWatts) {
          std::ostringstream oss{};
          oss << "rate units other than kW are not currently supported\n"
              << "rate_units = " << rate_units_to_tag(rate_units) << "\n";
          throw std::runtime_error(oss.str());
        }
        continue;
      }
      try {
        t = time_to_seconds(read_number(cells[0]), time_units);
      }
      catch (const std::invalid_argument&) {
        std::ostringstream oss;
        oss << "failed to convert string to int on row " << row << ".\n";
        oss << "t = std::stoi(" << cells[0] << ");\n";
        oss << "row: " << row << "\n";
        erin_csv::stream_out(oss, cells);
        std::cerr << oss.str();
        ifs.close();
        throw;
      }
      try {
        v = read_number(cells[1]);
      }
      catch (const std::invalid_argument&) {
        std::ostringstream oss;
        oss << "failed to convert string to double on row " << row << ".\n";
        oss << "v = std::stod(" << cells[1] << ");\n";
        oss << "row: " << row << "\n";
        erin_csv::stream_out(oss, cells);
        std::cerr << oss.str();
        ifs.close();
        throw;
      }
      the_loads.emplace_back(LoadItem{t, v});
    }
    if (the_loads.empty()) {
      std::ostringstream oss;
      oss << "the_loads is empty.\n";
      ifs.close();
      throw std::runtime_error(oss.str());
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "loads read in:\n";
      for (const auto ld: the_loads) {
        std::cout << "  {t: " << ld.time;
        std::cout << ", v: " << ld.value << "}\n";
      }
    }
    ifs.close();
    return the_loads;
  }

  std::unordered_map<std::string, std::unique_ptr<Component>>
  TomlInputReader::read_components(
      const std::unordered_map<std::string, std::vector<LoadItem>>& loads_by_id,
      const std::unordered_map<std::string, erin::fragility::FragilityCurve>&
        fragility_curves,
      const std::unordered_map<std::string, erin::fragility::FragilityMode>&
        fragility_modes,
      const std::unordered_map<std::string, size_type>& failure_modes,
      ReliabilityCoordinator& rc)
  {
    namespace ef = erin::fragility;
    const auto& toml_comps = toml::find<toml::table>(data, "components");
    if constexpr (debug_level >= debug_level_high) {
      std::cout << toml_comps.size() << " components found\n";
    }
    std::unordered_map<std::string, std::unique_ptr<Component>> components{};
    std::string field_read;
    for (const auto& c: toml_comps) {
      const auto& comp_id = c.first;
      toml::value t = c.second;
      const toml::table& tt = toml::get<toml::table>(t);
      auto component_type = read_component_type(tt, comp_id);
      auto stream_ids = read_stream_ids(tt, comp_id);
      const auto& input_stream_id = stream_ids.input_stream_id;
      const auto& output_stream_id = stream_ids.output_stream_id;
      const auto& lossflow_stream_id = stream_ids.lossflow_stream_id;
      auto frags = read_component_fragilities(
        tt, comp_id, fragility_curves, fragility_modes);
      const auto& failure_mode_tags = toml::find_or<std::vector<std::string>>(
          t, "failure_modes", std::vector<std::string>{});
      size_type comp_numeric_id{0};
      if (failure_mode_tags.size() > 0) {
        comp_numeric_id = rc.register_component(comp_id);
      }
      for (const auto& fm_tag : failure_mode_tags) {
        auto fm_it = failure_modes.find(fm_tag);
        if (fm_it == failure_modes.end()) {
          std::ostringstream oss{};
          oss << "unable to find failure mode with tag `"
              << fm_tag << "`. Is it defined in the input file?";
          throw std::runtime_error(oss.str());
        }
        const auto& fm_id = fm_it->second;
        rc.link_component_with_failure_mode(comp_numeric_id, fm_id);
      }
      switch (component_type) {
        case ComponentType::Source:
          read_source_component(
              tt,
              comp_id,
              output_stream_id,
              components,
              std::move(frags));
          break;
        case ComponentType::Load:
          read_load_component(
              tt,
              comp_id,
              input_stream_id,
              loads_by_id,
              components,
              std::move(frags));
          break;
        case ComponentType::Muxer:
          read_muxer_component(
              tt,
              comp_id,
              input_stream_id,
              components,
              std::move(frags));
          break;
        case ComponentType::Converter:
          {
            if constexpr (debug_level >= debug_level_high) {
              std::cout << "ComponentType::Converter found!\n"
                << "comp_id = " << comp_id << "\n"
                << "frags.size() = " << frags.size() << "\n";
              for (const auto& f : frags) {
                std::cout << "f.first = " << f.first << "\n";
                std::cout << "f.second.size()" << f.second.size() << "\n";
              }
            }
            read_converter_component(
                tt,
                comp_id,
                input_stream_id,
                output_stream_id,
                lossflow_stream_id,
                components,
                std::move(frags));
            break;
          }
        case ComponentType::PassThrough:
          read_passthrough_component(
              tt,
              comp_id,
              input_stream_id,
              components,
              std::move(frags));
          break;
        case ComponentType::Storage:
          read_storage_component(
              tt,
              comp_id,
              input_stream_id,
              components,
              std::move(frags));
          break;
        case ComponentType::UncontrolledSource:
          read_uncontrolled_source_component(
              tt,
              comp_id,
              output_stream_id,
              loads_by_id,
              components,
              std::move(frags));
          break;
        case ComponentType::Mover:
          read_mover_component(
              tt,
              comp_id,
              output_stream_id,
              components,
              std::move(frags));
          break;
        default:
          {
            std::ostringstream oss;
            oss << "unhandled component type\n";
            oss << "type = " << static_cast<int>(component_type) << "\n";
            throw std::runtime_error(oss.str());
            break;
          }
      }
    }
    if constexpr (debug_level >= debug_level_high) {
      for (const auto& c: components) {
        std::cout << "comp[" << c.first << "]:\n";
        std::cout << "\t" << c.second->get_id() << "\n";
      }
    }
    return components;
  }

  std::unordered_map<std::string, std::unique_ptr<Component>>
  TomlInputReader::read_components(
      const std::unordered_map<std::string, std::vector<LoadItem>>&
        loads_by_id)
  {
    ReliabilityCoordinator rc{};
    return read_components(
        loads_by_id,
        {},
        {},
        {},
        rc);
  }

  ComponentType
  TomlInputReader::read_component_type(
      const toml::table& tt,
      const std::string& comp_id) const
  {
    std::string field_read{};
    std::string comp_type_tag{};
    try {
      comp_type_tag = toml_helper::read_required_table_field<std::string>(
          tt, {"type"}, field_read);
    }
    catch (std::out_of_range& e) {
      std::ostringstream oss;
      oss << "original error: " << e.what() << "\n";
      oss << "failed to find 'type' for component " << comp_id << "\n";
      throw std::runtime_error(oss.str());
    }
    ComponentType component_type;
    try {
      component_type = tag_to_component_type(comp_type_tag);
    }
    catch (std::invalid_argument& e) {
      std::ostringstream oss;
      oss << "original error: " << e.what() << "\n";
      oss << "could not understand 'type' \""
        << comp_type_tag << "\" for component "
        << comp_id << "\n";
      throw std::runtime_error(oss.str());
    }
    return component_type;
  }

  erin::distribution::DistType
  TomlInputReader::read_dist_type(
      const toml::table& tt,
      const std::string& dist_id) const
  {
    std::string field_read{};
    std::string dist_type_tag{};
    try {
      dist_type_tag = toml_helper::read_required_table_field<std::string>(
          tt, {"type"}, field_read);
    }
    catch (std::out_of_range& e) {
      std::ostringstream oss{};
      oss << "original error: " << e.what() << "\n";
      oss << "failed to find 'type' for dist " << dist_id << "\n";
      throw std::runtime_error(oss.str());
    }
    erin::distribution::DistType dist_type{};
    try {
      dist_type = erin::distribution::tag_to_dist_type(dist_type_tag);
    }
    catch (std::invalid_argument& e) {
      std::ostringstream oss{};
      oss << "original error: " << e.what() << "\n";
      oss << "could not understand 'type' \""
        << dist_type_tag << "\" for component "
        << dist_id << "\n";
      throw std::runtime_error(oss.str());
    }
    return dist_type;
  }

  void
  TomlInputReader::check_top_level_entries() const
  {
    const std::unordered_set<std::string> valid_entries{
      "simulation_info",
      "loads",
      "components",
      "fragility_mode",
      "fragility_curve",
      "dist",
      "failure_mode",
      "networks",
      "scenarios",
    };
    const auto& tt = toml::get<toml::table>(data);
    for (const auto& entry : tt) {
      const auto& tag = entry.first;
      if (valid_entries.find(tag) == valid_entries.end()) {
        std::vector<std::string> valid_entries_vec{};
        for (const auto& an_entry : valid_entries) {
          valid_entries_vec.emplace_back(an_entry);
        }
        std::sort(valid_entries_vec.begin(), valid_entries_vec.end());
        std::ostringstream oss{};
        oss << "the top-level entry `" << tag << "` is invalid\n"
            << "valid top-level entries are:\n"
            << "  "
            << vec_to_string<std::string>(valid_entries_vec)
            << "\n";
        oss << "The current top level entries in your input are:\n";
        for (const auto& e : tt) {
          oss << "- " << e.first << "\n";
        }
        throw std::invalid_argument(oss.str());
      }
    }
  }


  StreamIDs
  TomlInputReader::read_stream_ids(
      const toml::table& tt,
      const std::string& comp_id) const
  {
    std::string field_read;
    std::string input_stream_id;
    try {
      input_stream_id =
        toml_helper::read_required_table_field<std::string>(
            tt,
            { "input_stream",
              "inflow_stream",
              "inflow",
              "stream",
              "flow",
              "output_stream",
              "outflow_stream",
              "outflow"},
            field_read);
    }
    catch (std::out_of_range& e) {
      std::ostringstream oss;
      oss << "original error: " << e.what() << "\n";
      oss << "failed to find 'input_stream', 'stream', "
          << "or 'output_stream' for component \"" << comp_id << "\"\n";
      throw std::runtime_error(oss.str());
    }
    auto output_stream_id = input_stream_id;
    if ((field_read != "stream") && (field_read != "flow")) {
      output_stream_id = toml_helper::read_optional_table_field<std::string>(
          tt,
          { "output_stream",
            "outflow_stream",
            "outflow"},
          input_stream_id,
          field_read);
    }
    auto lossflow_stream_id = input_stream_id;
    if ((field_read != "stream") && (field_read != "flow")) {
      lossflow_stream_id = toml_helper::read_optional_table_field<std::string>(
          tt,
          { "lossflow_stream", "lossflow"},
          input_stream_id,
          field_read);
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "comp: " << comp_id << ".input_stream_id  = "
        << input_stream_id << "\n";
      std::cout << "comp: " << comp_id << ".output_stream_id = "
        << output_stream_id << "\n";
      std::cout << "comp: " << comp_id << ".output_stream_id = "
        << lossflow_stream_id << "\n";
    }
    return StreamIDs{input_stream_id, output_stream_id, lossflow_stream_id};
  }

  fragility_map
  TomlInputReader::read_component_fragilities(
      const toml::table& tt,
      const std::string& comp_id,
      const std::unordered_map<
        std::string,
        erin::fragility::FragilityCurve>& fragility_curves,
      const std::unordered_map<
        std::string,
        erin::fragility::FragilityMode>& fragility_modes) const
  {
    namespace ef = erin::fragility;
    std::string field_read{};
    std::vector<std::string> fragility_mode_ids =
      toml_helper::read_optional_table_field<std::vector<std::string>>(
          tt, {"fragility_modes"}, std::vector<std::string>(0), field_read);
    fragility_map frags{};
    for (const auto& fm_id : fragility_mode_ids) {
      auto fm_it = fragility_modes.find(fm_id);
      if (fm_it == fragility_modes.end()) {
        std::ostringstream oss{};
        oss << "component '" << comp_id << "' specified fragility mode '"
          << fm_id << "' but that fragility mode isn't declared "
          << "in the input file. Please check your input file.";
        throw std::runtime_error(oss.str());
      }
      const auto& fm = (*fm_it).second;
      auto fc_it = fragility_curves.find(fm.fragility_curve_tag);
      if (fc_it == fragility_curves.end()) {
        std::ostringstream oss{};
        oss << "fragility mode '" << fm_id
          << "' specified fragility_curve '"
          << fm.fragility_curve_tag
          << "' but that fragility curve isn't declared in the input file. "
          << "Please check your input file.";
        throw std::runtime_error(oss.str());
      }
      const auto& fc = (*fc_it).second;
      auto it = frags.find(fc.vulnerable_to);
      if (it == frags.end()) {
        std::vector<FragilityCurveAndRepair> cs;
        cs.emplace_back(FragilityCurveAndRepair{fc.curve->clone(), fm.repair_dist_id});
        frags.emplace(std::make_pair(fc.vulnerable_to, std::move(cs)));
      }
      else {
        (*it).second.emplace_back(
          FragilityCurveAndRepair{fc.curve->clone(), fm.repair_dist_id});
      }
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "comp: " << comp_id << ".fragility_modes = [";
      bool first{true};
      for (const auto& fm_id: fragility_mode_ids) {
        if (first) {
          std::cout << "[";
          first = false;
        }
        else {
          std::cout << ", ";
        }
        std::cout << fm_id;
      }
      std::cout << "]\n";
    }
    return frags;
  }

  std::unordered_map<std::string, ::erin::fragility::FragilityCurve>
  TomlInputReader::read_fragility_curve_data()
  {
    namespace ef = erin::fragility;
    std::unordered_map<std::string, ef::FragilityCurve> out;
    std::string field_read;
    const auto& tt = toml_helper::read_optional_table_field<toml::table>(
        toml::get<toml::table>(data),
        {"fragility_curve"},
        toml::table{},
        field_read);
    if (tt.empty()) {
      return out;
    }
    for (const auto& pair : tt) {
      const auto& curve_id = pair.first;
      const auto& data_table = toml::get<toml::table>(pair.second);
      std::string curve_type_tag{};
      try {
        curve_type_tag = toml_helper::read_required_table_field<std::string>(
          data_table, {"type"}, field_read);
      }
      catch (std::out_of_range&) {
        std::ostringstream oss{};
        oss << "fragility curve '" << curve_id << "' does not "
            << "declare its 'type'\n";
        throw std::runtime_error(oss.str());
      }
      const auto curve_type = ef::tag_to_curve_type(curve_type_tag);
      std::string vulnerable_to;
      try {
        vulnerable_to = toml_helper::read_required_table_field<std::string>(
            data_table, {"vulnerable_to"}, field_read);
      }
      catch (std::out_of_range&) {
        std::ostringstream oss;
        oss << "required field 'vulnerable_to' for fragility curve '"
            << vulnerable_to << "' missing";
        throw std::runtime_error(oss.str());
      }
      switch (curve_type) {
        case ef::CurveType::Linear:
          {
            auto lower_bound = read_number_at(data_table, "lower_bound");
            auto upper_bound = read_number_at(data_table, "upper_bound");
            std::unique_ptr<ef::Curve> the_curve = std::make_unique<ef::Linear>(
                lower_bound, upper_bound);
            ef::FragilityCurve fc{vulnerable_to, std::move(the_curve)};
            out.insert(std::make_pair(curve_id, std::move(fc)));
            break;
          }
        default:
          {
            std::ostringstream oss;
            oss << "unhandled curve type '" << static_cast<int>(curve_type) << "'";
            throw std::runtime_error(oss.str());
          }
      }
    }
    return out;
  }

  void
  TomlInputReader::read_source_component(
      const toml::table& tt,
      const std::string& id,
      const std::string& stream,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    const auto max_outflow = toml_helper::read_number_from_table_as_double(
        tt, "max_outflow", -1.0);
    bool is_limited{max_outflow != -1.0};
    Limits lim{};
    if (is_limited) {
      FlowValueType min_outflow{0.0};
      lim = Limits{min_outflow, max_outflow};
    }
    std::unique_ptr<Component> source_comp = std::make_unique<SourceComponent>(
        id, std::string(stream), std::move(frags), lim);
    components.insert(std::make_pair(id, std::move(source_comp)));
  }

  void
  TomlInputReader::read_load_component(
      const toml::table& tt,
      const std::string& id,
      const std::string& stream,
      const std::unordered_map<
        std::string, std::vector<LoadItem>>& loads_by_id,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    const std::string key_loads_by_scenario{"loads_by_scenario"};
    std::unordered_map<std::string,std::vector<LoadItem>>
      loads_by_scenario{};
    auto it = tt.find(key_loads_by_scenario);
    const auto tt_end = tt.end();
    if (it != tt_end) {
      const auto& loads = toml::get<toml::table>(it->second);
      if constexpr (debug_level >= debug_level_high) {
        std::cout << loads.size()
                  << " load profile(s) by scenario"
                  << " for component " << id << "\n";
      }
      for (const auto& lp: loads) {
        const std::string load_id{toml::get<toml::string>(lp.second)};
        auto the_loads_it = loads_by_id.find(load_id);
        if (the_loads_it != loads_by_id.end()) {
          loads_by_scenario.insert(
              std::make_pair(lp.first, the_loads_it->second));
        }
        else {
          std::ostringstream oss;
          oss << "Input File Error reading load: "
            << "could not find load_id = \"" << load_id << "\"";
          throw std::runtime_error(oss.str());
        }
      }
      if constexpr (debug_level >= debug_level_high) {
        std::cout << loads_by_scenario.size()
          << " scenarios with loads\n";
      }
      if constexpr (debug_level >= debug_level_high) {
        for (const auto& ls: loads_by_scenario) {
          std::cout << ls.first << ": [";
          for (const auto& li: ls.second) {
            std::cout << "(" << li.time;
            std::cout << ", " << li.value << "), ";
          }
          std::cout << "]\n";
        }
      }
      std::unique_ptr<Component> load_comp = std::make_unique<LoadComponent>(
            id, std::string(stream), loads_by_scenario, std::move(frags));
      components.insert(std::make_pair(id, std::move(load_comp)));
    }
    else {
      std::ostringstream oss;
      oss << "BadInputError: could not find \""
        << key_loads_by_scenario
        << "\" in component type \"load\"";
      throw std::runtime_error(oss.str());
    }
  }

  void
  TomlInputReader::read_muxer_component(
      const toml::table& tt,
      const std::string& id,
      const std::string& stream,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    std::string field_read;
    auto num_inflows = static_cast<int>(
        toml_helper::read_number_from_table_as_int(tt, "num_inflows", 1));
    auto num_outflows = static_cast<int>(
        toml_helper::read_number_from_table_as_int(tt, "num_outflows", 1));
    auto out_disp_tag = toml_helper::read_optional_table_field<std::string>(
        tt, {"dispatch_strategy", "outflow_dispatch_strategy"}, "in_order", field_read);
    auto out_disp = tag_to_muxer_dispatch_strategy(out_disp_tag);
    std::unique_ptr<Component> mux_comp = std::make_unique<MuxerComponent>(
        id, std::string(stream), num_inflows, num_outflows, std::move(frags),
        out_disp);
    components.insert(
        std::make_pair(id, std::move(mux_comp)));
  }

  void TomlInputReader::read_passthrough_component(
      const toml::table& tt,
      const std::string& id,
      const std::string& stream,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    bool has_limits{false};
    std::string field_read{};
    FlowValueType min_outflow{0.0};
    auto max_outflow = toml_helper::read_number_from_table_as_double(
        tt, "max_outflow", -1.0);
    has_limits = (max_outflow != -1.0);
    std::unique_ptr<Component> pass_through_comp;
    if (has_limits) {
      pass_through_comp = std::make_unique<PassThroughComponent>(
          id, std::string(stream), Limits{min_outflow, max_outflow}, std::move(frags));
    }
    else {
      pass_through_comp = std::make_unique<PassThroughComponent>(
          id, std::string(stream), std::move(frags));
    }
    components.insert(
        std::make_pair(id, std::move(pass_through_comp)));
  }

  void
  TomlInputReader::read_mover_component(
      const toml::table& tt,
      const std::string& id,
      const std::string& outflow,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    std::string field_read{};
    auto inflow0 = toml_helper::read_required_table_field<std::string>(
        tt, {"inflow0"}, field_read);
    auto inflow1 = toml_helper::read_required_table_field<std::string>(
        tt, {"inflow1"}, field_read);
    auto COP = toml_helper::read_required_table_field<FlowValueType>(
        tt, {"COP","cop"}, field_read);
    auto m = std::make_unique<MoverComponent>(
        id, inflow0, inflow1, outflow, COP, std::move(frags));
    components.insert(std::make_pair(id, std::move(m)));
  }

  void
  TomlInputReader::read_uncontrolled_source_component(
      const toml::table& tt,
      const std::string& id,
      const std::string& outflow,
      const std::unordered_map<
        std::string, std::vector<LoadItem>>& profiles_by_id,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    const std::string key_supply_by_scenario{"supply_by_scenario"};
    std::unordered_map<std::string,std::vector<LoadItem>>
      supply_by_scenario{};
    auto it = tt.find(key_supply_by_scenario);
    const auto tt_end = tt.end();
    if (it != tt_end) {
      const auto& profiles = toml::get<toml::table>(it->second);
      for (const auto& p: profiles) {
        const std::string profile_id{toml::get<toml::string>(p.second)};
        auto the_profile_it = profiles_by_id.find(profile_id);
        if (the_profile_it != profiles_by_id.end()) {
          supply_by_scenario.insert(
              std::make_pair(p.first, the_profile_it->second));
        }
        else {
          std::ostringstream oss{};
          oss << "Input File Error reading load: "
              << "could not find supply_id = \"" << profile_id << "\"";
          throw std::runtime_error(oss.str());
        }
      }
      std::unique_ptr<Component> uncontrolled_src_comp = std::make_unique<UncontrolledSourceComponent>(
          id, std::string(outflow), supply_by_scenario, std::move(frags));
      components.insert(std::make_pair(id, std::move(uncontrolled_src_comp)));
    }
    else {
      std::ostringstream oss{};
      oss << "BadInputError: could not find \""
          << key_supply_by_scenario
          << "\" in component type \"uncontrolled_source\"";
      throw std::runtime_error(oss.str());
    }
  }

  void TomlInputReader::read_storage_component(
      const toml::table& tt,
      const std::string& id,
      const std::string& stream,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    std::string field_read;
    auto capacity = toml_helper::read_required_table_field<FlowValueType>(
        tt, {"capacity"}, field_read);
    field_read = "";
    auto capacity_units_tag = toml_helper::read_optional_table_field<std::string>(
        tt, {"capacity_units", "capacity_unit"}, "kJ", field_read);
    auto capacity_units = tag_to_work_units(capacity_units_tag);
    capacity = work_to_kJ(capacity, capacity_units);
    auto max_charge_rate = toml_helper::read_required_table_field<FlowValueType>(
        tt, {"max_inflow", "max_charge", "max_charge_rate"}, field_read);
    std::unique_ptr<Component> store = std::make_unique<StorageComponent>(
        id, std::string(stream), capacity, max_charge_rate, std::move(frags));
    components.insert(
        std::make_pair(id, std::move(store)));
  }

  void
  TomlInputReader::read_converter_component(
      const toml::table& tt,
      const std::string& id,
      const std::string& input_stream,
      const std::string& output_stream,
      const std::string& lossflow_stream,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    std::string field_read{};
    auto const_eff_val = toml_helper::read_required_table_field<toml::value>(
        tt, {"constant_efficiency"}, field_read);
    auto const_eff = read_number(const_eff_val);
    std::unique_ptr<Component> converter_comp =
      std::make_unique<ConverterComponent>(
          id,
          std::string(input_stream),
          std::string(output_stream),
          std::string(lossflow_stream),
          const_eff,
          std::move(frags));
    components.insert(
        std::make_pair(id, std::move(converter_comp)));
  }

  std::unordered_map<
    std::string,
    std::vector<::erin::network::Connection>>
  TomlInputReader::read_networks()
  {
    namespace enw = ::erin::network;
    namespace ep = ::erin::port;
    std::unordered_map<
      std::string,
      std::vector<enw::Connection>> networks;
    const auto toml_nets = toml::find<toml::table>(data, "networks");
    if constexpr (debug_level >= debug_level_high)
      std::cout << toml_nets.size() << " networks found\n";
    for (const auto& n: toml_nets) {
      std::vector<enw::Connection> nw_list;
      std::vector<std::vector<std::string>> raw_connects;
      auto nested_nw_table = toml::get<toml::table>(n.second);
      auto nested_nw_it = nested_nw_table.find("connections");
      if (nested_nw_it != nested_nw_table.end())
        raw_connects = toml::get<std::vector<std::vector<std::string>>>(
            nested_nw_it->second);
      int item_num{0};
      for (const auto& connect: raw_connects) {
        auto num_items = connect.size();
        const decltype(num_items) expected_num_items{3};
        if (num_items != expected_num_items) {
          std::ostringstream oss{};
          oss << "network " << nested_nw_it->first << " "
              << "connection[" << item_num << "] "
              << "doesn't have " << expected_num_items << " items\n"
              << "num_items: " << num_items << "\n";
          throw std::invalid_argument(oss.str());
        }
        const auto& comp_01_tag = connect[0];
        const auto& comp_02_tag = connect[1];
        const auto& stream_id = connect[2];
        const auto comp_01_id = parse_component_id(comp_01_tag);
        const auto comp_01_port = parse_component_port(comp_01_tag);
        const auto comp_01_port_num = parse_component_port_num(comp_01_tag);
        const auto comp_02_id = parse_component_id(comp_02_tag);
        const auto comp_02_port = parse_component_port(comp_02_tag);
        const auto comp_02_port_num = parse_component_port_num(comp_02_tag);
        nw_list.emplace_back(
            enw::Connection{
              enw::ComponentAndPort{comp_01_id, comp_01_port, comp_01_port_num},
              enw::ComponentAndPort{comp_02_id, comp_02_port, comp_02_port_num},
              stream_id});
        ++item_num;
      }
      networks.emplace(std::make_pair(n.first, nw_list));
    }
    if constexpr (debug_level >= debug_level_high) {
      for (const auto& nw: networks) {
        std::cout << "network[" << nw.first << "]:\n";
        for (const auto& connection: nw.second) {
          std::cout << "\tedge: ("
                    << connection.first.component_id
                    << "["
                    << ::erin::port::type_to_tag(connection.first.port_type)
                    << "] <==> "
                    << connection.second.component_id
                    << "["
                    << ::erin::port::type_to_tag(connection.second.port_type)
                    << "])\n";
        }
      }
    }
    return networks;
  }

  std::unordered_map<std::string, Scenario>
  TomlInputReader::read_scenarios(
      const std::unordered_map<std::string, ERIN::size_type>& dists
      )
  {
    namespace eg = erin_generics;
    const std::string occur_dist_tag{"occurrence_distribution"};
    const std::string default_time_units{"hours"};
    std::unordered_map<std::string, Scenario> scenarios;
    const auto& toml_scenarios = toml::find<toml::table>(data, "scenarios");
    if constexpr (debug_level >= debug_level_high) {
      std::cout << toml_scenarios.size() << " scenarios found\n";
    }
    for (const auto& s: toml_scenarios) {
      const auto& tt = toml::get<toml::table>(s.second);
      const auto next_occurrence_dist = toml::find<std::string>(
          s.second, occur_dist_tag);
      auto it = dists.find(next_occurrence_dist);
      if (it == dists.end()) {
        std::ostringstream oss{};
        oss << occur_dist_tag << " '" << next_occurrence_dist
            << "' does not appear in the list of available dist "
            << "declarations. A typo?\n";
        throw std::runtime_error(oss.str());
      }
      const auto occurrence_dist_id = it->second;
      const auto& time_unit_str = toml::find_or(
          s.second, "time_unit", default_time_units);
      const auto time_units = tag_to_time_units(time_unit_str);
      RealTimeType time_multiplier{1};
      switch (time_units) {
        case TimeUnits::Seconds:
          time_multiplier = 1;
          break;
        case TimeUnits::Minutes:
          time_multiplier = rtt_seconds_per_minute;
          break;
        case TimeUnits::Hours:
          time_multiplier = rtt_seconds_per_hour;
          break;
        case TimeUnits::Days:
          time_multiplier = rtt_seconds_per_day;
          break;
        case TimeUnits::Years:
          time_multiplier = rtt_seconds_per_year;
          break;
        default:
          {
            std::ostringstream oss{};
            oss << "unhandled time units '" << static_cast<int>(time_units)
                << "'";
            throw std::runtime_error(oss.str());
          }
      }
      const auto duration =
          toml_helper::read_number_from_table_as_int(tt, "duration", -1)
          * time_multiplier;
      if (duration < 0) {
        std::ostringstream oss;
        oss << "duration must be present and > 0 in '" << s.first << "'\n";
        throw std::runtime_error(oss.str());
      }
      const auto max_occurrences = toml_helper::read_number_from_table_as_int(
          tt, "max_occurrences", -1);
      const auto network_id = toml::find<std::string>(s.second, "network");
      std::unordered_map<std::string,double> intensity{};
      auto intensity_tmp =
        toml::find_or(
            s.second, "intensity",
            std::unordered_map<std::string,toml::value>{});
      for (const auto& pair: intensity_tmp) {
        double v{0.0};
        if (pair.second.is_floating()) {
          v = pair.second.as_floating();
        }
        else {
          v = static_cast<double>(pair.second.as_integer());
        }
        intensity.insert(std::make_pair(pair.first, v));
      }
      bool calc_reliability =
        toml::find_or(s.second, "calculate_reliability", false);
      scenarios.insert(
          std::make_pair(
            s.first,
            Scenario{
              s.first,
              network_id,
              duration,
              static_cast<int>(max_occurrences),
              occurrence_dist_id,
              intensity,
              calc_reliability}));
    }
    if constexpr (debug_level >= debug_level_high) {
      for (const auto& s: scenarios) {
        std::cout << "scenario[" << s.first << "]\n";
        std::cout << "\tname      : " << s.second.get_name() << "\n";
        std::cout << "\tnetwork_id: " << s.second.get_network_id() << "\n";
        std::cout << "\tduration  : " << s.second.get_duration() << "\n";
        std::cout << "\tmax occur : " << s.second.get_max_occurrences() << "\n";
        std::cout << "\tnum occur : " << s.second.get_number_of_occurrences()
                  << "\n";
      }
    }
    return scenarios;
  }

  std::unordered_map<std::string, size_type>
  TomlInputReader::read_distributions(
      erin::distribution::DistributionSystem& cds)
  {
    const auto& toml_dists = toml::find_or(data, "dist", toml::table{});
    std::unordered_map<std::string, size_type> out{};
    if (toml_dists.size() == 0) {
      return out;
    }
    for (const auto& toml_dist : toml_dists) {
      const auto& dist_string_id = toml_dist.first;
      const auto& t = toml_dist.second;
      const auto& tt = toml::get<toml::table>(t);
      const auto& dist_type = read_dist_type(tt, dist_string_id);
      switch (dist_type) {
        case erin::distribution::DistType::Fixed:
          {
            std::string field_read{""};
            auto value =
              toml_helper::read_number_from_table_as_double(tt, "value", -1.0);
            if (value < 0) {
              std::ostringstream oss{};
              oss << "dist '" << dist_string_id << "' doesn't have a valid 'value' field\n"
                  << "field 'value' must be present and >= 0\n"
                  << "value = " << value << "\n";
              throw std::invalid_argument(oss.str());
            }
            std::string time_tag =
              toml_helper::read_optional_table_field<std::string>(
                  tt, {"time_unit", "time_units"}, std::string{"hours"},
                  field_read);
            auto tu = tag_to_time_units(time_tag);
            auto dist_id = cds.add_fixed(
                dist_string_id, time_to_seconds(value, tu));
            out[dist_string_id] = dist_id;
            break;
          }
        case erin::distribution::DistType::Uniform:
          {
            std::string field_read{""};
            auto lower_bound =
              toml_helper::read_number_from_table_as_double(tt, "lower_bound", -1);
            if (lower_bound < 0) {
              std::ostringstream oss{};
              oss << "dist '" << dist_string_id << "' doesn't have a valid 'lower_bound' field\n"
                  << "field 'lower_bound' must be present and >= 0\n"
                  << "lower_bound = " << lower_bound << "\n";
              throw std::invalid_argument(oss.str());
            }
            auto upper_bound =
              toml_helper::read_number_from_table_as_double(tt, "upper_bound", -1);
            if (upper_bound < 0) {
              std::ostringstream oss{};
              oss << "dist '" << dist_string_id << "' doesn't have a valid 'upper_bound' field\n"
                  << "field 'upper_bound' must be present and >= 0\n"
                  << "upper_bound = " << upper_bound << "\n";
              throw std::invalid_argument(oss.str());
            }
            std::string time_tag =
              toml_helper::read_optional_table_field<std::string>(
                  tt, {"time_unit", "time_units"}, std::string{"hours"},
                  field_read);
            auto tu = tag_to_time_units(time_tag);
            auto dist_id = cds.add_uniform(
                dist_string_id,
                time_to_seconds(lower_bound, tu),
                time_to_seconds(upper_bound, tu));
            out[dist_string_id] = dist_id;
            break;
          }
        case erin::distribution::DistType::Normal:
          {
            std::string field_read{""};
            auto mean =
              toml_helper::read_number_from_table_as_double(tt, "mean", -1);
            if (mean < 0) {
              std::ostringstream oss{};
              oss << "dist '" << dist_string_id << "' doesn't have a valid 'mean' field\n"
                  << "field 'mean' must be present and >= 0\n"
                  << "mean = " << mean << "\n";
              throw std::invalid_argument(oss.str());
            }
            auto std_dev =
              toml_helper::read_number_from_table_as_double(tt, "standard_deviation", -1);
            if (std_dev < 0) {
              std::ostringstream oss{};
              oss << "dist '" << dist_string_id << "' doesn't have a valid 'standard_deviation' field\n"
                  << "field 'standard_deviation' must be present and >= 0\n"
                  << "standard_deviation = " << std_dev << "\n";
              throw std::invalid_argument(oss.str());
            }
            std::string time_tag =
              toml_helper::read_optional_table_field<std::string>(
                  tt, {"time_unit", "time_units"}, std::string{"hours"},
                  field_read);
            auto tu = tag_to_time_units(time_tag);
            auto dist_id = cds.add_normal(
                dist_string_id,
                time_to_seconds(mean, tu),
                time_to_seconds(std_dev, tu));
            out[dist_string_id] = dist_id;
            break;
          }
        case erin::distribution::DistType::Weibull:
          {
            std::string field_read{""};
            auto shape =
              toml_helper::read_number_from_table_as_double(tt, "shape", -1.0);
            if (shape <= 0) {
              std::ostringstream oss{};
              oss << "dist '" << dist_string_id << "' doesn't have a valid 'shape' field\n"
                  << "field 'shape' must be present and > 0\n"
                  << "shape = " << shape << "\n";
              throw std::invalid_argument(oss.str());
            }
            auto scale =
              toml_helper::read_number_from_table_as_double(tt, "scale", -1.0);
            if (scale < 0) {
              std::ostringstream oss{};
              oss << "dist '" << dist_string_id << "' doesn't have a valid "
                  << "'scale' field\n"
                  << "field 'scale' must be present and >= 0\n"
                  << "scale = " << scale << "\n";
              throw std::invalid_argument(oss.str());
            }
            auto location =
              toml_helper::read_number_from_table_as_double(tt, "location", 0.0);
            if (location < 0) {
              std::ostringstream oss{};
              oss << "dist '" << dist_string_id << "' doesn't have a "
                  << "valid 'location' field\n"
                  << "field 'location' is optional and defaults to 0. If "
                  << "present it must be >= 0\n"
                  << "location = " << location << "\n";
              throw std::invalid_argument(oss.str());
            }
            std::string time_tag =
              toml_helper::read_optional_table_field<std::string>(
                  tt, {"time_unit", "time_units"}, std::string{"hours"},
                  field_read);
            auto tu = tag_to_time_units(time_tag);
            auto dist_id = cds.add_weibull(
                dist_string_id,
                shape,
                static_cast<double>(time_to_seconds(scale, tu)),
                static_cast<double>(time_to_seconds(location, tu)));
            out[dist_string_id] = dist_id;
            break;
          }
        case erin::distribution::DistType::QuantileTable:
          {
            /* [dist.my-table]
             * type = "quantile_table"
             * variate_time_pairs = [[0.0, 1500.0], [1.0, 2000.0]]
             * time_unit = "hours"
             * # OR
             * [dist.my-table]
             * type = "quantile_table"
             * csv_file = "my-data.csv"
             */
            std::string field_read{""};
            std::vector<double> xs{};
            std::vector<double> dtimes{};
            std::string time_tag =
              toml_helper::read_optional_table_field<std::string>(
                  tt, {"time_unit"}, std::string{"hours"},
                  field_read);
            auto time_units = tag_to_time_units(time_tag);
            auto it = tt.find("variate_time_pairs");
            const auto tt_end = tt.end();
            if (it == tt_end) {
              auto it2 = tt.find("csv_file");
              if (it2 == tt_end) {
                std::ostringstream oss{};
                oss << "dist '" << dist_string_id
                    << "' must define 'variate_time_pairs' or "
                    << "'csv_file' but has neither\n";
                throw std::invalid_argument(oss.str());
              }
              const auto& csv_path = toml::get<std::string>(it2->second);
              std::ifstream ifs{csv_path};
              xs = std::vector<double>{};
              dtimes = std::vector<double>{};
              if (!ifs.is_open()) {
                std::ostringstream oss;
                oss << "input file stream on \"" << csv_path
                  << "\" failed to open for reading\n";
                throw std::runtime_error(oss.str());
              }
              bool in_header{true};
              for (int row{0}; ifs.good(); ++row) {
                auto cells = erin_csv::read_row(ifs);
                if (cells.size() == 0) {
                  break;
                }
                if (in_header) {
                  in_header = false;
                  if (cells.size() < 2) {
                    std::ostringstream oss;
                    oss << "badly formatted file \"" << csv_path << "\"\n";
                    oss << "row: " << row << "\n";
                    erin_csv::stream_out(oss, cells);
                    ifs.close();
                    throw std::runtime_error(oss.str());
                  }
                  try {
                    time_units = tag_to_time_units(cells[1]);
                  }
                  catch (std::invalid_argument& e) {
                    std::ostringstream oss;
                    oss << "in file '" << csv_path
                      << "', second column should be a time unit tag but is '"
                      << cells[1] << "'";
                    oss << "row: " << row << "\n";
                    erin_csv::stream_out(oss, cells);
                    oss << "original error: " << e.what() << "\n";
                    ifs.close();
                    throw std::runtime_error(oss.str());
                  }
                  continue;
                }
                try {
                  xs.emplace_back(read_number(cells[0]));
                  //t = time_to_seconds(read_number(cells[0]), time_units);
                }
                catch (const std::invalid_argument&) {
                  std::ostringstream oss;
                  oss << "failed to convert string to number on row " << row << ".\n";
                  oss << "t = " << cells[0] << ";\n";
                  oss << "row: " << row << "\n";
                  erin_csv::stream_out(oss, cells);
                  std::cerr << oss.str();
                  ifs.close();
                  throw std::runtime_error(oss.str());
                }
                try {
                  dtimes.emplace_back(
                      time_to_seconds(read_number(cells[1]), time_units));
                }
                catch (const std::invalid_argument&) {
                  std::ostringstream oss;
                  oss << "failed to convert string to double on row " << row << ".\n";
                  oss << "dtime = " << cells[1] << ";\n";
                  oss << "row: " << row << "\n";
                  erin_csv::stream_out(oss, cells);
                  std::cerr << oss.str();
                  ifs.close();
                  throw std::runtime_error(oss.str());
                }
              }
              if (dtimes.empty()) {
                std::ostringstream oss{};
                oss << "CSV data in " << csv_path << " cannot be empty!\n";
                ifs.close();
                throw std::runtime_error(oss.str());
              }
              if (dtimes.size() != xs.size()) {
                std::ostringstream oss;
                oss << "xs and dtimes must have the same size but"
                    << " xs.size() = " << xs.size()
                    << " dtimes.size() = " << dtimes.size() << "!\n";
                ifs.close();
                throw std::runtime_error(oss.str());
              }
              ifs.close();
            }
            else {
              const auto& xdts = toml::get<std::vector<toml::value>>(
                  it->second);
              for (const auto& item : xdts) {
                const auto& xdt = toml::get<std::vector<toml::value>>(item);
                const auto size = xdt.size();
                if (size != 2) {
                  std::ostringstream oss{};
                  oss << "dist '" << dist_string_id
                      << "': error reading 'variate_time_pairs'; "
                      << "not all elements have length of 2\n";
                  throw std::invalid_argument(oss.str());
                }
                xs.emplace_back(read_number(xdt[0]));
                dtimes.emplace_back(
                    time_to_seconds(read_number(xdt[1]), time_units));
              }
            }
            auto dist_id = cds.add_quantile_table(dist_string_id, xs, dtimes);
            out[dist_string_id] = dist_id;
            break;
          }
        default:
          {
            std::ostringstream oss{};
            oss << "unhandled dist_type `" << static_cast<int>(dist_type) << "`";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    return out;
  }

  std::unordered_map<std::string, size_type>
  TomlInputReader::read_failure_modes(
      const std::unordered_map<std::string, size_type>& dist_ids,
      ReliabilityCoordinator& rc)
  {
    const auto& toml_fms = toml::find_or(
        data, "failure_mode", toml::table{});
    std::unordered_map<std::string, size_type> out{};
    if (toml_fms.size() == 0) {
      return out;
    }
    for (const auto& toml_fm : toml_fms) {
      const auto& fm_string_id = toml_fm.first;
      toml::value t = toml_fm.second;
      const toml::table& tt = toml::get<toml::table>(t);
      std::string field_read{};
      const auto& failure_dist_tag =
        toml_helper::read_required_table_field<std::string>(
            tt, {"failure_dist"}, field_read);
      const auto& repair_dist_tag =
        toml_helper::read_required_table_field<std::string>(
            tt, {"repair_dist"}, field_read);
      auto it = dist_ids.find(failure_dist_tag);
      if (it == dist_ids.end()) {
        std::ostringstream oss{};
        oss << "could not find distribution (dist) corresponding to tag `"
            << failure_dist_tag << "`";
        throw std::runtime_error(oss.str());
      }
      const auto& failure_dist_id = it->second;
      it = dist_ids.find(repair_dist_tag);
      if (it == dist_ids.end()) {
        std::ostringstream oss{};
        oss << "could not find distribution (dist) corresponding to tag `"
            << repair_dist_tag << "`";
        throw std::runtime_error(oss.str());
      }
      const auto& repair_dist_id = it->second;
      auto fm_id = rc.add_failure_mode(
          fm_string_id,
          failure_dist_id,
          repair_dist_id);
      out[fm_string_id] = fm_id;
    }
    return out;
  }

  std::unordered_map<std::string, erin::fragility::FragilityMode>
  TomlInputReader::read_fragility_modes(
      const std::unordered_map<std::string, size_type>& dist_ids,
      const std::unordered_map<std::string, erin::fragility::FragilityCurve>& fragility_curves)
  {
    const auto& toml_fms = toml::find_or(
        data, "fragility_mode", toml::table{});
    std::unordered_map<std::string, erin::fragility::FragilityMode> out{};
    if (toml_fms.size() == 0) {
      return out;
    }
    for (const auto& toml_fm : toml_fms) {
      const auto& fm_string_id = toml_fm.first;
      toml::value t = toml_fm.second;
      const toml::table& tt = toml::get<toml::table>(t);
      std::string field_read{};
      const auto& fragility_curve_tag =
        toml_helper::read_required_table_field<std::string>(
            tt, {"fragility_curve"}, field_read);
      const std::string empty_tag{""};
      const auto& repair_dist_tag =
        toml_helper::read_optional_table_field<std::string>(
            tt, {"repair_dist"}, empty_tag, field_read);
      auto it = fragility_curves.find(fragility_curve_tag);
      if (it == fragility_curves.end()) {
        std::ostringstream oss{};
        oss << "For fragility_mode `"
            << fm_string_id << "`"
            << ", could not find fragility_curve corresponding to tag `"
            << fragility_curve_tag << "`";
        throw std::runtime_error(oss.str());
      }
      std::int64_t repair_dist_id{erin::fragility::no_repair_distribution};
      if (repair_dist_tag != empty_tag) {
        auto it2 = dist_ids.find(repair_dist_tag);
        if (it2 == dist_ids.end()) {
          std::ostringstream oss{};
          oss << "For fragility_mode `"
              << fm_string_id << "`"
              << ", could not find distribution (dist) corresponding to tag `"
              << repair_dist_tag << "`\n"
              << "Specifying a repair distribution for a fragility mode is optional\n"
              << "However, if you specify one, its tag must correspond to a declared "
              << "distribution (dist)";
          throw std::runtime_error(oss.str());
        }
        else {
          repair_dist_id = static_cast<std::int64_t>(it2->second);
        }
      }
      out[fm_string_id] = erin::fragility::FragilityMode{
        fragility_curve_tag, repair_dist_id};
    }
    return out;
  }

  std::string
  parse_component_id(const std::string& tag)
  {
    const std::string delimiter{":"};
    std::string id{tag};
    auto pos = tag.find(delimiter);
    if (pos == std::string::npos)
      return id;
    return tag.substr(0, pos);
  }

  erin::port::Type
  parse_component_port(const std::string& tag)
  {
    if (tag.find(":IN(") != std::string::npos)
      return erin::port::Type::Inflow;
    if (tag.find(":OUT(") != std::string::npos)
      return erin::port::Type::Outflow;
    std::ostringstream oss{};
    oss << "could not find :IN(.) or :OUT(.) in tag\n"
        << "tag: " << tag << "\n";
    throw std::invalid_argument(oss.str());
  }

  int
  parse_component_port_num(const std::string& tag)
  {
    int port_num{0};
    auto pos = tag.find('(');
    if (pos == std::string::npos)
      return port_num;
    auto substr = tag.substr(pos + 1, tag.size());
    std::istringstream iss{substr};
    iss >> port_num;
    return port_num;
  }
}