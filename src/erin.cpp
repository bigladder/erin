/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include "debug_utils.h"
#include "erin/erin.h"
#include "erin_csv.h"
#include "erin/fragility.h"
#include "erin/port.h"
#include "erin/type.h"
#include "erin/utils.h"
#include "erin_generics.h"
#include "toml_helper.h"
#include "gsl/pointers"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ERIN
{
  ScenarioStats&
  ScenarioStats::operator+=(const ScenarioStats& other)
  {
    uptime += other.uptime;
    downtime += other.downtime;
    max_downtime = std::max(max_downtime, other.max_downtime);
    load_not_served += other.load_not_served;
    total_energy += other.total_energy;
    return *this;
  }

  ScenarioStats
  operator+(const ScenarioStats& a, const ScenarioStats& b)
  {
    return ScenarioStats{
      a.uptime + b.uptime,
      a.downtime + b.downtime,
      std::max(a.max_downtime, b.max_downtime),
      a.load_not_served + b.load_not_served,
      a.total_energy + b.total_energy};
  }

  bool
  operator==(const ScenarioStats& a, const ScenarioStats& b)
  {
    if ((a.uptime == b.uptime)
        && (a.downtime == b.downtime)
        && (a.max_downtime == b.max_downtime)) {
      const auto diff_lns = std::abs(a.load_not_served - b.load_not_served);
      const auto diff_te = std::abs(a.total_energy - b.total_energy);
      const auto& fvt = flow_value_tolerance;
      return (diff_lns < fvt) && (diff_te < fvt);
    }
    return false;
  }

  bool
  operator!=(const ScenarioStats& a, const ScenarioStats& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const ScenarioStats& s)
  {
    os << "ScenarioStats(uptime=" << s.uptime << ", "
       << "downtime=" << s.downtime << ", "
       << "max_downtime=" << s.max_downtime << ", "
       << "load_not_served=" << s.load_not_served << ", "
       << "total_energy=" << s.total_energy << ")";
    return os;
  }

  ////////////////////////////////////////////////////////////
  // TomlInputReader
  TomlInputReader::TomlInputReader(toml::value data_):
    InputReader(),
    data{std::move(data_)}
  {
    check_top_level_entries();
  }

  TomlInputReader::TomlInputReader(const std::string& path):
    InputReader(),
    data{}
  {
    data = toml::parse(path);
    check_top_level_entries();
  }

  TomlInputReader::TomlInputReader(std::istream& in):
    InputReader(),
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
        fragilities,
      const std::unordered_map<std::string, size_type>& fms,
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
      auto frags = read_component_fragilities(tt, comp_id, fragilities);
      const auto& failure_mode_tags = toml::find_or<std::vector<std::string>>(
          t, "failure_modes", std::vector<std::string>{});
      size_type comp_numeric_id{0};
      if (failure_mode_tags.size() > 0) {
        comp_numeric_id = rc.register_component(comp_id);
      }
      for (const auto& fm_tag : failure_mode_tags) {
        auto fm_it = fms.find(fm_tag);
        if (fm_it == fms.end()) {
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
      "fragility", // TODO: remove
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
        for (const auto& entry : valid_entries) {
          valid_entries_vec.emplace_back(entry);
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
        erin::fragility::FragilityCurve>& fragilities) const
  {
    namespace ef = erin::fragility;
    std::string field_read{};
    std::vector<std::string> fragility_ids =
      toml_helper::read_optional_table_field<std::vector<std::string>>(
          tt, {"fragilities"}, std::vector<std::string>(0), field_read);
    fragility_map frags;
    for (const auto& fid : fragility_ids) {
      auto f_it = fragilities.find(fid);
      if (f_it == fragilities.end()) {
        std::ostringstream oss;
        oss << "component '" << comp_id << "' specified fragility '"
          << fid << "' but that fragility doesn't appear in the"
          << "fragility curve map.";
        throw std::runtime_error(oss.str());
      }
      const auto& fc = (*f_it).second;
      auto it = frags.find(fc.vulnerable_to);
      if (it == frags.end()) {
        std::vector<std::unique_ptr<ef::Curve>> cs;
        cs.emplace_back(fc.curve->clone());
        frags.emplace(std::make_pair(fc.vulnerable_to, std::move(cs)));
      }
      else {
        (*it).second.emplace_back(fc.curve->clone());
      }
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "comp: " << comp_id << ".fragilities = [";
      bool first{true};
      for (const auto& f: fragility_ids) {
        if (first) {
          std::cout << "[";
          first = false;
        }
        else {
          std::cout << ", ";
        }
        std::cout << f;
      }
      std::cout << "]\n";
    }
    return frags;
  }

  // TODO: rename to read_fragility_curve_data()
  std::unordered_map<std::string, ::erin::fragility::FragilityCurve>
  TomlInputReader::read_fragility_curve_data()
  {
    namespace ef = erin::fragility;
    std::unordered_map<std::string, ef::FragilityCurve> out;
    std::string field_read;
    // TODO: change field read from "fragility" to "fragility_curve"
    const auto& tt = toml_helper::read_optional_table_field<toml::table>(
        toml::get<toml::table>(data),
        {"fragility", "fragility_curve"}, // TODO: remove "fragility"
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
        std::ostringstream oss;
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
      const auto& repair_dist_tag =
        toml_helper::read_required_table_field<std::string>(
            tt, {"repair_dist"}, field_read);
      auto it = fragility_curves.find(fragility_curve_tag);
      if (it == fragility_curves.end()) {
        std::ostringstream oss{};
        oss << "could not find fragility_curve corresponding to tag `"
            << fragility_curve_tag << "`";
        throw std::runtime_error(oss.str());
      }
      auto it2 = dist_ids.find(repair_dist_tag);
      if (it2 == dist_ids.end()) {
        std::ostringstream oss{};
        oss << "could not find distribution (dist) corresponding to tag `"
            << repair_dist_tag << "`";
        throw std::runtime_error(oss.str());
      }
      const auto& repair_dist_id = it2->second;
      out[fm_string_id] = erin::fragility::FragilityMode{
        fragility_curve_tag, repair_dist_id};
    }
    return out;
  }


  ////////////////////////////////////////////////////////////
  // ScenarioResults
  ScenarioResults::ScenarioResults():
    ScenarioResults(false,0,0,{},{},{},{})
  {
  }

  ScenarioResults::ScenarioResults(
      bool is_good_,
      RealTimeType scenario_start_time_,
      RealTimeType scenario_duration_,
      std::unordered_map<std::string, std::vector<Datum>> results_,
      std::unordered_map<std::string, std::string> stream_ids_,
      std::unordered_map<std::string, ComponentType> component_types_,
      std::unordered_map<std::string, PortRole> port_roles_):
    is_good{is_good_},
    scenario_start_time{scenario_start_time_},
    scenario_duration{scenario_duration_},
    results{std::move(results_)},
    stream_ids{std::move(stream_ids_)},
    component_types{std::move(component_types_)},
    port_roles_by_port_id{std::move(port_roles_)},
    statistics{},
    keys{}
  {
    for (const auto& r: results) {
      keys.emplace_back(r.first);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& comp_id : keys) {
      statistics[comp_id] = calc_scenario_stats(results[comp_id]);
    }
  }

  std::vector<std::string>
  ScenarioResults::to_csv_lines(
      const std::vector<std::string>& comp_ids,
      bool make_header,
      TimeUnits time_units) const
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "ScenarioResults::to_csv_lines()\n";
      std::cout << "comp_ids:\n";
      for (const auto& id : comp_ids) {
        std::cout << "- " << id << "\n";
      }
      std::cout << "make_header: " << make_header << "\n";
      std::cout << "time_units : " << time_units_to_tag(time_units) << "\n";
      std::cout << "results:\n";
      for (const auto& p : results) {
        std::cout << "- " << p.first << "\n";
        for (const auto& d : p.second) {
          std::cout << "  - " << d << "\n";
        }
      }
    }
    if (!erin::utils::is_superset(comp_ids, keys)) {
      std::ostringstream oss{};
      oss << "comp_ids is not a superset of the keys defined in "
             "this ScenarioResults\n"
             "keys:\n";
      for (const auto& k : keys) {
        oss << k << "\n";
      }
      oss << "comp_ids:\n";
      for (const auto& c_id : comp_ids) {
        oss << c_id << "\n";
      }
      throw std::invalid_argument(oss.str());
    }
    if (!is_good) {
      return std::vector<std::string>{};
    }
    std::vector<std::string> lines;
    std::set<RealTimeType> times_set{scenario_duration};
    std::unordered_map<std::string, std::vector<FlowValueType>> values;
    std::unordered_map<std::string, std::vector<FlowValueType>> requested_values;
    std::unordered_map<std::string, FlowValueType> last_values;
    std::unordered_map<std::string, FlowValueType> last_requested_values;
    for (const auto& k: keys) {
      values[k] = std::vector<FlowValueType>();
      requested_values[k] = std::vector<FlowValueType>();
      last_values[k] = 0.0;
      last_requested_values[k] = 0.0;
      for (const auto& d: results.at(k)) {
        if (d.time > scenario_duration) {
          break;
        }
        else {
          times_set.emplace(d.time);
        }
      }
    }
    std::vector<RealTimeType> times{times_set.begin(), times_set.end()};
    for (const auto& p: results) {
      for (const auto& t: times) {
        auto k{p.first};
        if (t == scenario_duration) {
          values[k].emplace_back(0.0);
          requested_values[k].emplace_back(0.0);
        }
        else {
          bool found{false};
          auto last = last_values[k];
          auto last_request = last_requested_values[k];
          for (const auto& d: p.second) {
            if (d.time == t) {
              found = true;
              values[k].emplace_back(d.achieved_value);
              requested_values[k].emplace_back(d.requested_value);
              last_values[k] = d.achieved_value;
              last_requested_values[k] = d.requested_value;
              break;
            }
            if (d.time > t) {
              break;
            }
          }
          if (!found) {
            values[k].emplace_back(last);
            requested_values[k].emplace_back(last_request);
          }
        }
      }
    }
    if (make_header) {
      std::ostringstream oss;
      oss << "time (" << time_units_to_tag(time_units) << ")";
      for (const auto& k: comp_ids) {
        oss << "," << k << ":achieved (kW)," << k << ":requested (kW)";
      }
      lines.emplace_back(oss.str());
    }
    using size_type = std::vector<RealTimeType>::size_type;
    for (size_type i{0}; i < times.size(); ++i) {
      std::ostringstream oss;
      oss << std::setprecision(precision_for_output);
      oss << convert_time_in_seconds_to(times[i], time_units);
      for (const auto& k: comp_ids) {
        auto it = values.find(k);
        if (it == values.end()) {
          oss << ",,";
          continue;
        }
        oss << ',' << values[k][i] << ',' << requested_values[k][i];
      }
      lines.emplace_back(oss.str());
    }
    return lines;
  }

  std::string
  ScenarioResults::to_csv(TimeUnits time_units) const
  {
    if (!is_good) {
      return std::string{};
    }
    auto lines = to_csv_lines(keys, true, time_units);
    std::ostringstream oss;
    for (const auto& line: lines) {
      oss << line << '\n';
    }
    return oss.str();
  }

  std::unordered_map<std::string, double>
  ScenarioResults::calc_energy_availability() const
  {
    std::unordered_map<std::string, double> out{};
    for (const auto& comp_id : keys) {
      const auto& stats = statistics.at(comp_id);
      out[comp_id] = calc_energy_availability_from_stats(stats);
    }
    return out;
  }

  std::unordered_map<std::string, RealTimeType>
  ScenarioResults::calc_max_downtime()
  {
    return erin_generics::derive_statistic<RealTimeType,Datum,ScenarioStats>(
      results, keys, statistics, calc_scenario_stats,
      [](const ScenarioStats& ss) -> RealTimeType { return ss.downtime; }
    );
  }

  std::unordered_map<std::string, FlowValueType>
  ScenarioResults::calc_load_not_served()
  {
    return erin_generics::derive_statistic<FlowValueType,Datum,ScenarioStats>(
      results, keys, statistics, calc_scenario_stats,
      [](const ScenarioStats& ss) -> FlowValueType {
        return ss.load_not_served;
      }
    );
  }

  std::unordered_map<std::string, FlowValueType>
  ScenarioResults::calc_energy_usage_by_stream(ComponentType ct)
  {
    std::unordered_map<std::string, FlowValueType> out{};
    for (const auto& k: keys) {
      auto stat_it = statistics.find(k);
      if (stat_it == statistics.end()) {
        statistics[k] = calc_scenario_stats(results.at(k));
      }
      auto st_id = stream_ids.at(k);
      auto the_ct = component_types.at(k);
      if (the_ct == ct) {
        auto it = out.find(st_id);
        if (it == out.end()) {
          out[st_id] = 0.0;
        }
        out[st_id] += statistics[k].total_energy;
      }
    }
    return out;
  }

  std::string
  ScenarioResults::to_stats_csv(TimeUnits time_units)
  {
    auto ea = calc_energy_availability();
    auto md = calc_max_downtime();
    auto lns = calc_load_not_served();
    auto eubs_src = calc_energy_usage_by_port_role(
        keys,
        PortRole::SourceOutflow,
        statistics,
        stream_ids,
        port_roles_by_port_id);
    auto eubs_load = calc_energy_usage_by_port_role(
        keys,
        PortRole::LoadInflow,
        statistics,
        stream_ids,
        port_roles_by_port_id);
    auto eubs_waste = calc_energy_usage_by_port_role(
        keys,
        PortRole::WasteInflow,
        statistics,
        stream_ids,
        port_roles_by_port_id);
    auto eubs_storeflow = calc_energy_usage_by_port_role(
        keys,
        PortRole::StorageInflow,
        statistics,
        stream_ids,
        port_roles_by_port_id);
    auto eubs_discharge = calc_energy_usage_by_port_role(
        keys,
        PortRole::StorageOutflow,
        statistics,
        stream_ids,
        port_roles_by_port_id);
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "metrics printout\n";
      std::cout << "results:\n";
      for (const auto& r: results) {
        std::cout << "  " << r.first << ":\n";
        for (const auto& d: r.second) {
          std::cout << "    {";
          print_datum(std::cout, d);
          std::cout << "}\n";
        }
      }
      erin_generics::print_unordered_map("ea", ea);
      erin_generics::print_unordered_map("md", md);
      erin_generics::print_unordered_map("lns", lns);
      erin_generics::print_unordered_map("eubs_src", eubs_src);
      erin_generics::print_unordered_map("eubs_load", eubs_load);
      std::cout << "metrics done...\n";
    }
    std::ostringstream oss;
    oss << "component id,type,stream,energy availability,max downtime ("
        << time_units_to_tag(time_units) << "),load not served (kJ)";
    std::set<std::string> stream_key_set{};
    for (const auto& s: stream_ids)
      stream_key_set.emplace(s.second);
    std::vector<std::string> stream_keys{
      stream_key_set.begin(), stream_key_set.end()};
    // should not need. set is ordered/sorted.
    //std::sort(stream_keys.begin(), stream_keys.end());
    for (const auto& sk: stream_keys)
      oss << "," << sk << " energy used (kJ)";
    oss << "\n";
    for (const auto& k: keys) {
      auto s_id = stream_ids.at(k);
      oss << k
          << "," << component_type_to_tag(component_types.at(k))
          << "," << s_id
          << "," << ea.at(k)
          << "," << convert_time_in_seconds_to(md.at(k), time_units)
          << "," << lns.at(k);
      auto stats = statistics.at(k);
      for (const auto& sk: stream_keys) {
        if (s_id != sk) {
          oss << ",0.0";
          continue;
        }
        oss << "," << stats.total_energy;
      }
      oss << "\n";
    }
    FlowValueType total_source{0.0};
    FlowValueType total_load{0.0};
    FlowValueType total_storage{0.0};
    FlowValueType total_waste{0.0};
    oss << "TOTAL (source),,,,,";
    for (const auto& sk: stream_keys) {
      auto it = eubs_src.find(sk);
      if (it == eubs_src.end()) {
        oss << ",0.0";
        continue;
      }
      oss << "," << (it->second);
      total_source += it->second;
    }
    oss << "\n";
    oss << "TOTAL (load),,,,,";
    for (const auto& sk: stream_keys) {
      auto it = eubs_load.find(sk);
      if (it == eubs_load.end()) {
        oss << ",0.0";
        continue;
      }
      oss << "," << (it->second);
      total_load += it->second;
    }
    oss << "\n";
    oss << "TOTAL (storage),,,,,";
    for (const auto& sk: stream_keys) {
      auto it_store = eubs_storeflow.find(sk);
      auto it_dschg = eubs_discharge.find(sk);
      if ((it_store == eubs_storeflow.end())
          || (it_dschg == eubs_discharge.end())) {
        oss << ",0.0";
        continue;
      }
      auto net_storage = (it_store->second - it_dschg->second);
      oss << "," << net_storage;
      total_storage += net_storage;
    }
    oss << "\n";
    oss << "TOTAL (waste),,,,,";
    for (const auto& sk: stream_keys) {
      auto it = eubs_waste.find(sk);
      if (it == eubs_waste.end()) {
        oss << ",0.0";
        continue;
      }
      oss << "," << (it->second);
      total_waste += it->second;
    }
    oss << "\n";
    oss << "ENERGY BALANCE (source-(load+storage+waste)),";
    oss << (total_source - (total_load + total_storage + total_waste));
    oss << ",,,,";
    auto num_sks{stream_keys.size()};
    for (decltype(num_sks) i{0}; i < num_sks; ++i) {
      oss << ",";
    }
    oss << "\n";
    return oss.str();
  }

  std::unordered_map<std::string, FlowValueType>
  ScenarioResults::total_loads_by_stream_helper(
      const std::function<
        FlowValueType(const std::vector<Datum>& f)>& sum_fn) const
  {
    if constexpr (debug_level >= debug_level_high)
      std::cout << "ScenarioResults::total_loads_by_stream_helper()\n";
    std::unordered_map<std::string, double> out{};
    if (is_good) {
      for (const auto& comp_id : keys) {
        if constexpr (debug_level >= debug_level_high)
          std::cout << "looking up '" << comp_id << "'\n";
        auto comp_type_it = component_types.find(comp_id);
        if (comp_type_it == component_types.end()) {
          std::ostringstream oss{};
          oss << "expected the key '" << comp_id << "' to be in "
              << "component_types but it was not\n"
              << "component_types:\n";
          for (const auto& pair : component_types)
            oss << "\t" << pair.first << " : " << component_type_to_tag(pair.second) << "\n";
          throw std::runtime_error(oss.str());
        }
        const auto& comp_type = comp_type_it->second;
        if (comp_type != ComponentType::Load)
          continue;
        const auto& stream_name = stream_ids.at(comp_id);
        const auto& data = results.at(comp_id);
        const auto sum = sum_fn(data);
        auto it = out.find(stream_name);
        if (it == out.end()) {
          out[stream_name] = sum;
        }
        else {
          it->second += sum;
        }
      }
    }
    return out;
  }

  std::unordered_map<std::string, FlowValueType>
  ScenarioResults::total_requested_loads_by_stream() const
  {
    if constexpr (debug_level >= debug_level_high)
      std::cout << "ScenarioResults::total_requested_loads_by_stream()\n";
    return total_loads_by_stream_helper(sum_requested_load);
  }

  std::unordered_map<std::string, FlowValueType>
  ScenarioResults::total_achieved_loads_by_stream() const
  {
    if constexpr (debug_level >= debug_level_high)
      std::cout << "ScenarioResults::total_achieved_loads_by_stream()\n";
    return total_loads_by_stream_helper(sum_achieved_load);
  }

  std::unordered_map<std::string, FlowValueType>
  ScenarioResults::total_energy_availability_by_stream() const
  {
    if constexpr (debug_level >= debug_level_high)
      std::cout << "ScenarioResults::total_energy_availability_by_stream()\n";
    std::unordered_map<std::string, FlowValueType> out{};
    const auto req = total_requested_loads_by_stream();
    if constexpr (debug_level >= debug_level_high)
      std::cout << "successfully called total_requested_loads_by_stream()\n";
    const auto ach = total_achieved_loads_by_stream();
    if constexpr (debug_level >= debug_level_high)
      std::cout << "successfully called total_achieved_loads_by_stream()\n";
    if (req.size() != ach.size()) {
      std::ostringstream oss;
      oss << "number of requested loads by stream (" << req.size()
          << ") doesn't equal number of achieved loads by stream ("
          << ach.size() << ")";
      throw std::runtime_error(oss.str());
    }
    for (const auto& pair : req) {
      const auto& stream_name = pair.first;
      const auto& requested_load_kJ = pair.second;
      const auto& achieved_load_kJ = ach.at(stream_name);
      if (requested_load_kJ == 0.0) {
        out[stream_name] = 1.0;
      }
      else {
        out[stream_name] = achieved_load_kJ / requested_load_kJ;
      }
    }
    return out;
  }

  bool
  operator==(const ScenarioResults& a, const ScenarioResults& b)
  {
    namespace EG = erin_generics;
    return (a.is_good == b.is_good) &&
      (a.scenario_start_time == b.scenario_start_time) &&
      (a.scenario_duration == b.scenario_duration) &&
      EG::unordered_map_to_vector_equality<std::string, Datum>(
          a.results, b.results) &&
      EG::unordered_map_equality<std::string, std::string>(
          a.stream_ids, b.stream_ids) &&
      EG::unordered_map_equality<std::string, ComponentType>(
          a.component_types, b.component_types);
  }

  bool
  operator!=(const ScenarioResults& a, const ScenarioResults& b)
  {
    return !(a == b);
  }

  ////////////////////////////////////////////////////////////
  // ScenarioStats
  ScenarioStats
  calc_scenario_stats(const std::vector<Datum>& ds)
  {
    RealTimeType uptime{0};
    RealTimeType downtime{0};
    RealTimeType max_downtime{0};
    RealTimeType contiguous_downtime{0};
    RealTimeType t0{0};
    FlowValueType req{0};
    FlowValueType ach{0};
    FlowValueType ach_energy{0};
    FlowValueType load_not_served{0.0};
    bool was_down{false};
    for (const auto& d: ds) {
      if (d.time == 0) {
        req = d.requested_value;
        ach = d.achieved_value;
        continue;
      }
      auto dt = d.time - t0;
      t0 = d.time;
      if (dt <= 0) {
        std::ostringstream oss;
        oss << "calc_scenario_stats";
        oss << "invariant_error: dt <= 0; dt=" << dt << "\n";
        int idx{0};
        for (const auto& dd: ds) {
          oss << "ds[" << idx << "]\n\t.time = " << dd.time << "\n";
          oss << "\t.requested_value = " << dd.requested_value << "\n";
          oss << "\t.achieved_value  = " << dd.achieved_value << "\n";
          ++idx;
        }
        throw std::runtime_error(oss.str());
      }
      auto gap = std::fabs(req - ach);
      if (gap > flow_value_tolerance) {
        downtime += dt;
        if (was_down) {
          contiguous_downtime += dt;
        }
        else {
          contiguous_downtime = dt;
        }
        was_down = true;
      }
      else {
        uptime += dt;
        max_downtime = std::max(contiguous_downtime, max_downtime);
        was_down = false;
        contiguous_downtime = 0;
      }
      load_not_served += static_cast<FlowValueType>(dt) * gap;
      req = d.requested_value;
      ach_energy += ach * static_cast<FlowValueType>(dt);
      ach = d.achieved_value;
    }
    if (was_down) {
      max_downtime = std::max(contiguous_downtime, max_downtime);
    }
    return ScenarioStats{
      uptime, downtime, max_downtime, load_not_served, ach_energy};
  }

  ////////////////////////////////////////////////////////////
  // AllResults
  AllResults::AllResults(bool is_good_):
    AllResults(is_good_, {})
  {
  }

  AllResults::AllResults(
      bool is_good_,
      const std::unordered_map<std::string, std::vector<ScenarioResults>>& results_):
    is_good{is_good_},
    results{results_},
    scenario_id_set{},
    comp_id_set{},
    stream_key_set{},
    scenario_ids{},
    comp_ids{},
    stream_keys{},
    outputs{}
  {
    if (is_good) {
      for (const auto& pair: results) {
        const auto& scenario_id{pair.first};
        const auto& results_for_scenario{pair.second};
        if (results_for_scenario.size() == 0) {
          continue;
        }
        scenario_id_set.emplace(scenario_id);
        // Each scenario should have a network associated with it so the set
        // of components in a scenario is constant.
        bool first{true};
        for (const auto& scenario_results : results_for_scenario) {
          if (first) {
            first = false;
            const auto& comp_types{scenario_results.get_component_types()};
            for (const auto& ct_pair: comp_types) {
              const auto& comp_id{ct_pair.first};
              comp_id_set.emplace(comp_id);
            }
            const auto& stream_ids{scenario_results.get_stream_ids()};
            for (const auto& s: stream_ids) {
              stream_key_set.emplace(s.second);
            }
          }
          auto scenario_start = scenario_results.get_start_time_in_seconds();
          outputs.emplace(
              std::make_pair(
                std::make_pair(scenario_start, scenario_id),
                std::cref(scenario_results)));
        }
      }
      scenario_ids = std::vector<std::string>(
          scenario_id_set.begin(), scenario_id_set.end());
      comp_ids = std::vector<std::string>(
          comp_id_set.begin(), comp_id_set.end());
      stream_keys = std::vector<std::string>(
          stream_key_set.begin(), stream_key_set.end());
    }
  }

  std::string
  AllResults::to_csv() const
  {
    if (is_good) {
      std::ostringstream oss;
      oss << "scenario id,"
             "scenario start time (P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]),"
             "elapsed (hours)";
      for (const auto& comp_id: comp_ids) {
        oss << "," << comp_id << ":achieved (kW)"
            << "," << comp_id << ":requested (kW)";
      }
      oss << '\n';
      for (const auto& op_pair: outputs) {
        const auto& scenario_time{op_pair.first.first};
        const auto& scenario_id{op_pair.first.second};
        const auto scenario_time_str =
          ::erin::utils::time_to_iso_8601_period(scenario_time);
        const ScenarioResults& scenario_results = op_pair.second;
        auto csv_lines = scenario_results.to_csv_lines(comp_ids, false); 
        for (const auto& line: csv_lines) {
          oss << scenario_id << ","
              << scenario_time_str << ","
              << line << '\n';
        }
      }
      return oss.str();
    }
    return "";
  }

  std::string
  AllResults::to_stats_csv() const
  {
    const auto& stats = get_stats();
    if (!is_good) {
      return "";
    }
    std::ostringstream oss;
    write_header_for_stats_csv(oss);
    if (stats.empty()) {
      return oss.str();
    }
    for (const auto& scenario_id: scenario_ids) {
      const auto& all_ss = stats.at(scenario_id);
      auto the_end = all_ss.component_types_by_comp_id.end();
      for (const auto& comp_id: comp_ids) {
        auto it = all_ss.component_types_by_comp_id.find(comp_id);
        if (it == the_end) {
          continue;
        }
        write_component_line_for_stats_csv(oss, all_ss, comp_id, scenario_id);
      }
      write_total_line_for_stats_csv(
          oss, scenario_id, all_ss,
          all_ss.totals_by_stream_id_for_source_kJ, "source");
      write_total_line_for_stats_csv(
          oss, scenario_id, all_ss,
          all_ss.totals_by_stream_id_for_load_kJ, "load");
      write_total_line_for_stats_csv(
          oss, scenario_id, all_ss,
          all_ss.totals_by_stream_id_for_storage_kJ, "storage");
      write_total_line_for_stats_csv(
          oss, scenario_id, all_ss,
          all_ss.totals_by_stream_id_for_waste_kJ, "waste");
      FlowValueType total_source{0.0};
      FlowValueType total_load{0.0};
      FlowValueType total_storage{0.0};
      FlowValueType total_waste{0.0};
      for (const auto& p : all_ss.totals_by_stream_id_for_source_kJ) {
        total_source += p.second;
      }
      for (const auto& p : all_ss.totals_by_stream_id_for_load_kJ) {
        total_load += p.second;
      }
      for (const auto& p : all_ss.totals_by_stream_id_for_storage_kJ) {
        total_storage += p.second;
      }
      for (const auto& p : all_ss.totals_by_stream_id_for_waste_kJ) {
        total_waste += p.second;
      }
      FlowValueType balance{
        total_source - (total_load + total_storage + total_waste)};
      write_energy_balance_line_for_stats_csv(
          oss, scenario_id, all_ss, balance);
    }
    return oss.str();
  }

  void
  AllResults::write_header_for_stats_csv(std::ostream& oss) const
  {
    namespace CSV = erin_csv;
    CSV::write_csv(oss, {
        "scenario id",
        "number of occurrences",
        "total time in scenario (hours)",
        "component id",
        "type",
        "stream",
        "energy availability",
        "max downtime (hours)",
        "load not served (kJ)"}, true, false);
    CSV::write_csv_with_tranform<std::string>(oss, stream_keys,
        [](const std::string& s) -> std::string {
          return s + " energy used (kJ)";
        }, false, true);
  }

  void
  AllResults::write_component_line_for_stats_csv(
      std::ostream& oss,
      const AllScenarioStats& all_ss,
      const std::string& comp_id,
      const std::string& scenario_id) const
  {
    namespace EG = erin_generics;
    const auto& stream_types = all_ss.stream_types_by_comp_id;
    const auto& comp_types = all_ss.component_types_by_comp_id;
    std::string stream_name =
      EG::find_or<std::string, std::string>(stream_types, comp_id, "--");
    std::string comp_type =
      EG::find_and_transform_or<std::string, ComponentType, std::string>(
          comp_types, comp_id, "--",
          [](const ComponentType& ct) -> std::string {
            return component_type_to_tag(ct);
          });
    oss << scenario_id
        << "," << all_ss.num_occurrences
        << "," <<
        convert_time_in_seconds_to(
            all_ss.time_in_scenario_s, TimeUnits::Hours)
        << "," << comp_id
        << "," << comp_type
        << "," << stream_name
        << "," << all_ss.energy_availability_by_comp_id.at(comp_id)
        << "," << convert_time_in_seconds_to(
            all_ss.max_downtime_by_comp_id_s.at(comp_id),
            TimeUnits::Hours)
        << "," << all_ss.load_not_served_by_comp_id_kW.at(comp_id);
    for (const auto& s: stream_keys) {
      if (s != stream_name) {
        oss << ",0.0";
        continue;
      }
      oss << "," << all_ss.total_energy_by_comp_id_kJ.at(comp_id);
    }
    oss  << "\n";
  }

  void
  AllResults::write_total_line_for_stats_csv(
      std::ostream& oss,
      const std::string& scenario_id,
      const AllScenarioStats& all_ss,
      const std::unordered_map<std::string, double>& totals_by_stream,
      const std::string& label) const
  {
    namespace EG = erin_generics;
    oss << scenario_id
        << "," << all_ss.num_occurrences
        << "," << convert_time_in_seconds_to(
            all_ss.time_in_scenario_s, TimeUnits::Hours)
        << "," << "TOTAL (" << label << "),,,,,";
    for (const auto& s: stream_keys) {
      auto val = EG::find_or<std::string,double>(totals_by_stream, s, 0.0);
      if (val == 0.0) {
        oss << ",0.0";
        continue;
      }
      oss << "," << val;
    }
    oss  << "\n";
  }

  void
  AllResults::write_energy_balance_line_for_stats_csv(
      std::ostream& oss,
      const std::string& scenario_id,
      const AllScenarioStats& all_ss,
      const FlowValueType& balance) const
  {
    oss << scenario_id
        << "," << all_ss.num_occurrences
        << "," << convert_time_in_seconds_to(
            all_ss.time_in_scenario_s, TimeUnits::Hours)
        << "," << "ENERGY BALANCE (source-(load+storage+waste)),"
        << balance << ",,,,";
    auto num_sks{stream_keys.size()};
    for (decltype(num_sks) i{0}; i < num_sks; ++i) {
      oss << ",";
    }
    oss  << "\n";
  }

  std::unordered_map<std::string, AllScenarioStats>
  AllResults::get_stats() const
  {
    std::unordered_map<std::string, AllScenarioStats> stats{};
    for (const auto& scenario_id: scenario_ids) {
      const auto& results_for_scenario = results.at(scenario_id);
      const auto num_occurrences{results_for_scenario.size()};
      if (num_occurrences == 0) {
        if constexpr (debug_level >= debug_level_high) {
          std::cout << "num_occurrences of " << scenario_id << " == 0\n"
                    << "continuing...\n";
        }
        continue;
      }
      std::unordered_map<std::string, RealTimeType> max_downtime_by_comp_id_s{};
      std::unordered_map<std::string, double> energy_availability_by_comp_id{};
      std::unordered_map<std::string, double> load_not_served_by_comp_id_kW{};
      std::unordered_map<std::string, double> total_energy_by_comp_id_kJ{};
      const auto& stream_ids = results_for_scenario[0].get_stream_ids();
      const auto& comp_types = results_for_scenario[0].get_component_types();
      const auto& port_roles_by_port_id = results_for_scenario[0].get_port_roles_by_port_id();
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "port_roles_by_port_id: \n";
        for (const auto& p : port_roles_by_port_id) {
          std::cout << "\t" << p.first << " : "
                    << port_role_to_tag(p.second) << "\n";
        }
        std::cout << "----------------------------\n";
      }
      RealTimeType time_in_scenario{0};
      std::unordered_map<std::string, ScenarioStats> stats_by_comp{};
      std::unordered_map<std::string, double> totals_by_stream_source{};
      std::unordered_map<std::string, double> totals_by_stream_load{};
      std::unordered_map<std::string, double> totals_by_stream_storage{};
      std::unordered_map<std::string, double> totals_by_stream_waste{};
      for (const auto& scenario_results: results_for_scenario) {
        time_in_scenario += scenario_results.get_duration_in_seconds();
        const auto& stats_by_comp_temp = scenario_results.get_statistics();
        const auto& the_comp_ids = scenario_results.get_component_ids();
        const auto totals_by_stream_source_temp = calc_energy_usage_by_port_role(
            the_comp_ids,
            PortRole::SourceOutflow,
            stats_by_comp_temp,
            stream_ids,
            port_roles_by_port_id);
        const auto totals_by_stream_load_temp = calc_energy_usage_by_port_role(
            the_comp_ids,
            PortRole::LoadInflow,
            stats_by_comp_temp,
            stream_ids,
            port_roles_by_port_id);
        const auto totals_by_stream_waste_temp = calc_energy_usage_by_port_role(
            the_comp_ids,
            PortRole::WasteInflow,
            stats_by_comp_temp,
            stream_ids,
            port_roles_by_port_id);
        const auto totals_by_stream_storeflow_temp = calc_energy_usage_by_port_role(
            the_comp_ids,
            PortRole::StorageInflow,
            stats_by_comp_temp,
            stream_ids,
            port_roles_by_port_id);
        const auto totals_by_stream_discharge_temp = calc_energy_usage_by_port_role(
            the_comp_ids,
            PortRole::StorageOutflow,
            stats_by_comp_temp,
            stream_ids,
            port_roles_by_port_id);
        for (const auto& cid_stats_pair: stats_by_comp_temp) {
          const auto& comp_id = cid_stats_pair.first;
          const auto& comp_stats = cid_stats_pair.second;
          auto it = stats_by_comp.find(comp_id);
          if (it == stats_by_comp.end()) {
            stats_by_comp[comp_id] = comp_stats;
          }
          else {
            it->second += comp_stats;
          }
        }
        for (const auto& sn_total_pair : totals_by_stream_source_temp) {
          const auto& stream_name = sn_total_pair.first;
          const auto& total = sn_total_pair.second;
          auto it = totals_by_stream_source.find(stream_name);
          if (it == totals_by_stream_source.end()) {
            totals_by_stream_source[stream_name] = total;
          }
          else {
            it->second += total;
          }
        }
        for (const auto& sn_total_pair : totals_by_stream_load_temp) {
          const auto& stream_name = sn_total_pair.first;
          const auto& total = sn_total_pair.second;
          auto it = totals_by_stream_load.find(stream_name);
          if (it == totals_by_stream_load.end()) {
            totals_by_stream_load[stream_name] = total;
          }
          else {
            it->second += total;
          }
        }
        for (const auto& sn_total_pair : totals_by_stream_storeflow_temp) {
          const auto& stream_name = sn_total_pair.first;
          auto dschg = totals_by_stream_discharge_temp.at(stream_name);
          auto total = sn_total_pair.second - dschg;
          auto it = totals_by_stream_storage.find(stream_name);
          if (it == totals_by_stream_storage.end()) {
            totals_by_stream_storage[stream_name] = total;
          }
          else {
            it->second += total;
          }
        }
        for (const auto& sn_total_pair : totals_by_stream_waste_temp) {
          const auto& stream_name = sn_total_pair.first;
          const auto& total = sn_total_pair.second;
          auto it = totals_by_stream_waste.find(stream_name);
          if (it == totals_by_stream_waste.end()) {
            totals_by_stream_waste[stream_name] = total;
          }
          else {
            it->second += total;
          }
        }
      }
      auto stats_by_comp_end = stats_by_comp.end();
      for (const auto& comp_id: comp_ids) {
        auto it_comp_id = stats_by_comp.find(comp_id);
        if (it_comp_id == stats_by_comp_end) {
          continue;
        }
        const auto& comp_stats = it_comp_id->second;
        energy_availability_by_comp_id[comp_id] = calc_energy_availability_from_stats(comp_stats);
        max_downtime_by_comp_id_s[comp_id] = comp_stats.max_downtime;
        load_not_served_by_comp_id_kW[comp_id] = comp_stats.load_not_served;
        total_energy_by_comp_id_kJ[comp_id] = comp_stats.total_energy;
      }
      stats[scenario_id] = AllScenarioStats{
        num_occurrences,
        time_in_scenario,
        std::move(max_downtime_by_comp_id_s),
        stream_ids,
        comp_types,
        port_roles_by_port_id,
        std::move(energy_availability_by_comp_id),
        std::move(load_not_served_by_comp_id_kW),
        std::move(total_energy_by_comp_id_kJ),
        std::move(totals_by_stream_source),
        std::move(totals_by_stream_load),
        std::move(totals_by_stream_storage),
        std::move(totals_by_stream_waste)};
    }
    return stats;
  }

  std::unordered_map<
    std::string, std::vector<ScenarioResults>::size_type>
  AllResults::get_num_results() const
  {
    using size_type = std::vector<ScenarioResults>::size_type;
    std::unordered_map<std::string, size_type> out;
    for (const auto& s: scenario_ids) {
      auto it = results.find(s);
      if (it == results.end()) {
        std::ostringstream oss;
        oss << "scenario id not in results '" << s << "'\n";
        throw std::runtime_error(oss.str());
      }
      out[s] = (it->second).size();
    }
    return out;
  }

  std::unordered_map<
    std::string,
    std::unordered_map<std::string, std::vector<double>>>
  AllResults::get_total_energy_availabilities() const
  {
    if constexpr (debug_level >= debug_level_high)
      std::cout << "AllResults::get_total_energy_availabilities()\n";
    using size_type = std::vector<ScenarioResults>::size_type;
    using T_scenario_id = std::string;
    using T_stream_name = std::string;
    using T_total_energy_availability = FlowValueType;
    using T_total_energy_availability_return =
      std::unordered_map<
        T_scenario_id,
        std::unordered_map<
          T_stream_name,
          std::vector<T_total_energy_availability>>>;
    T_total_energy_availability_return out{};
    for (const auto& scenario_id: scenario_ids) {
      if constexpr (debug_level >= debug_level_high)
        std::cout << "... processing " << scenario_id << "\n";
      const auto& srs = results.at(scenario_id);
      const auto num_srs = srs.size();
      std::unordered_map<
        T_stream_name,
        std::vector<T_total_energy_availability>> data{};
      for (size_type i{0}; i < num_srs; ++i) {
        if constexpr (debug_level >= debug_level_high)
          std::cout << "... scenario_results[" << i << "]\n";
        const auto& sr = srs.at(i);
        if constexpr (debug_level >= debug_level_high)
          std::cout << "... calling sr.total_energy_availability_by_stream()\n";
        const auto teabs = sr.total_energy_availability_by_stream();
        if constexpr (debug_level >= debug_level_high)
          std::cout << "... called sr.total_energy_availability_by_stream()\n";
        for (const auto& pair : teabs) {
          const auto& stream_name = pair.first;
          const auto& tea = pair.second;
          auto it = data.find(stream_name);
          if (it == data.end())
            data[stream_name] = std::vector<FlowValueType>{tea};
          else
            it->second.emplace_back(tea);
        }
      }
      out[scenario_id] = data;
    }
    return out;
  }

  AllResults
  AllResults::with_is_good_as(bool is_good_) const
  {
    return AllResults{is_good_, results};
  }

  bool
  all_results_results_equal(
      const std::unordered_map<std::string,std::vector<ScenarioResults>>& a,
      const std::unordered_map<std::string,std::vector<ScenarioResults>>& b)
  {
    auto a_size = a.size();
    auto b_size = b.size();
    if (a_size != b_size) {
      return false;
    }
    for (const auto& item: a) {
      auto b_it = b.find(item.first);
      if (b_it == b.end()) {
        return false;
      }
      auto a_item_size = item.second.size();
      auto b_item_size = b_it->second.size();
      if (a_item_size != b_item_size) {
        return false;
      }
      for (decltype(a_item_size) i{0}; i < a_item_size; ++i) {
        if (item.second[i] != b_it->second[i]) {
          return false;
        }
      }
    }
    return true;
  }

  bool
  operator==(const AllResults& a, const AllResults& b)
  {
    namespace EG = erin_generics;
    return (a.is_good == b.is_good) &&
      EG::unordered_map_to_vector_equality<std::string, ScenarioResults>(
          a.results, b.results);
  }

  bool
  operator!=(const AllResults& a, const AllResults& b)
  {
    return !(a == b);
  }

  //////////////////////////////////////////////////////////// 
  // Main
  // main class that runs the simulation from file
  Main::Main(const std::string& input_file_path):
    Main(TomlInputReader{input_file_path})
  {
  }

  Main::Main(TomlInputReader reader):
    sim_info{reader.read_simulation_info()},
    failure_probs_by_comp_id_by_scenario_id{}
  {
    // Read data into private class fields
    auto loads_by_id = reader.read_loads();
    auto fragility_curves = reader.read_fragility_curve_data();
    erin::distribution::DistributionSystem ds{};
    // dists is map<string, size_type>
    auto dists = reader.read_distributions(ds);
    auto fragility_modes = reader.read_fragility_modes(dists, fragility_curves);
    ReliabilityCoordinator rc{};
    // fms is map<string, size_type>
    auto fms = reader.read_failure_modes(dists, rc);
    // components needs to be modified to add component_id as size_type?
    components = reader.read_components(loads_by_id, fragility_curves, fms, rc);
    networks = reader.read_networks();
    scenarios = reader.read_scenarios(dists);
    const auto max_time_s{sim_info.get_max_time_in_seconds()};
    rand_fn = sim_info.make_random_function();
    reliability_schedule = rc.calc_reliability_schedule_by_component_tag(
        rand_fn, ds, max_time_s);
    scenario_schedules = calc_scenario_schedule(
        max_time_s, scenarios, ds, rand_fn);
    check_data();
    generate_failure_fragilities();
    // TODO: add the following function to use FragilityModes
    // fragility_schedule = calc_fragility_schedules(fragility_modes, scenario_schedules, failure_probs_by_comp_id_by_scenario_id);
    if constexpr (debug_level >= debug_level_high) {
      for (const auto& p : loads_by_id) {
        std::cout << p.first << ":\n";
        for (const auto& d : p.second) {
          std::cout << "- " << d << "\n";
        }
      }
    }
  }

  Main::Main(
      const SimulationInfo& sim_info_,
      const std::unordered_map<
        std::string,
        std::unique_ptr<Component>>& components_,
      const std::unordered_map<
        std::string,
        std::vector<::erin::network::Connection>>& networks_,
      const std::unordered_map<std::string, Scenario>& scenarios_,
      const std::unordered_map<std::string, std::vector<TimeState>>&
        reliability_schedule_,
      const std::unordered_map<std::string, std::vector<RealTimeType>>&
        scenario_schedules_
      ):
    sim_info{sim_info_},
    components{},
    networks{networks_},
    scenarios{scenarios_},
    failure_probs_by_comp_id_by_scenario_id{},
    reliability_schedule{reliability_schedule_},
    scenario_schedules{scenario_schedules_}
  {
    for (const auto& pair: components_) {
      components.insert(
          std::make_pair(
            pair.first,
            pair.second->clone()));
    }
    rand_fn = sim_info.make_random_function();
    check_data();
    generate_failure_fragilities();
  }

  void
  Main::generate_failure_fragilities()
  {
    for (const auto& s: scenarios) {
      const auto& intensities = s.second.get_intensities();
      std::unordered_map<std::string, std::vector<double>> m{};
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
              [](double a, double b){ return a > b; });
          if (probs.size() == 0) {
            // no need to add an empty vector of probabilities
            // note: probabilitie of 0.0 (i.e., never fail) are not added in
            // apply_intensities
            continue;
          }
          m[c_pair.first] = probs;
        }
      }
      failure_probs_by_comp_id_by_scenario_id[s.first] = m;
    }
  }

  ScenarioResults
  Main::run(const std::string& scenario_id, RealTimeType scenario_start_s)
  {
    // TODO: check input structure to ensure that keys are available in maps that
    //       should be there. If not, provide a good error message about what's
    //       wrong.
    // The Run Algorithm
    // 0. Check if we have a valid scenario_id
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "enter Main::run(\"" << scenario_id << "\")\n";
    }
    const auto it = scenarios.find(scenario_id);
    if (it == scenarios.end()) {
      std::ostringstream oss;
      oss << "scenario_id '" << scenario_id << "'" 
             " is not in available scenarios\n"; 
      oss << "avilable choices: ";
      for (const auto& item: scenarios) {
        oss << "'" << item.first << "', ";
      }
      oss << "\n";
      throw std::invalid_argument(oss.str());
    }
    // 1. Reference the relevant scenario
    const auto& the_scenario = it->second;
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "... the_scenario = " << the_scenario << "\n";
    }
    auto do_reliability = the_scenario.get_calc_reliability();
    std::unordered_map<std::string, std::vector<TimeState>>
      clipped_reliability_schedule{};
    const auto duration = the_scenario.get_duration();
    if (do_reliability) {
      clipped_reliability_schedule = clip_schedule_to<std::string>(
            reliability_schedule,
            scenario_start_s,
            scenario_start_s + duration);
      if constexpr (debug_level >= debug_level_high) {
        for (const auto& item : clipped_reliability_schedule) {
          std::cout << item.first << ":\n";
          for (const auto& x : item.second) {
            std::cout << x << " ";
          }
          std::cout << "\n";
        }
      }
    }
    // 2. Construct and Run Simulation
    // 2.1. Instantiate a devs network
    adevs::Digraph<FlowValueType, Time> network;
    // 2.2. Interconnect components based on the network definition
    const auto& network_id = the_scenario.get_network_id();
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "... network_id = " << network_id << "\n";
    }
    const auto& connections = networks[network_id];
    const auto& fpbc = failure_probs_by_comp_id_by_scenario_id.at(scenario_id);
    auto elements = erin::network::build(
        scenario_id, network, connections, components, fpbc, rand_fn, true,
        clipped_reliability_schedule);
    std::shared_ptr<FlowWriter> fw = std::make_shared<DefaultFlowWriter>();
    for (auto e_ptr: elements) {
      e_ptr->set_flow_writer(fw);
    }
    adevs::Simulator<PortValue, Time> sim;
    network.add(&sim);
    const int max_no_advance_factor{10000};
    int max_no_advance =
      static_cast<int>(elements.size()) * max_no_advance_factor;
    auto sim_good = run_devs(
        sim, duration, max_no_advance,
        "scenario_run:" + scenario_id +
        "(" + std::to_string(scenario_start_s) + ")");
    fw->finalize_at_time(duration);
    // 4. create a SimulationResults object from a FlowWriter
    // - stream-line the SimulationResults object to do less and be leaner
    // - alternatively, make the FlowWriter take over the SimulationResults role?
    auto results = fw->get_results();
    auto stream_ids = fw->get_stream_ids();
    auto comp_types = fw->get_component_types();
    auto port_roles = fw->get_port_roles();
    fw->clear();
    if (comp_types.size() != stream_ids.size()) {
      std::ostringstream oss{};
      oss << "comp_types.size() != stream_ids.size()\n"
          << "comp_types.size() = " << comp_types.size() << "\n"
          << "stream_ids.size() = " << stream_ids.size() << "\n";
      throw std::runtime_error(oss.str());
    }
    if ((!sim_good) || (debug_level >= debug_level_high)) {
      if (!sim_good) {
        std::cout << "ERROR DUMP for " << scenario_id << ":\n";
      }
      std::cout << "results:\n";
      using size_type = std::vector<Datum>::size_type;
      size_type num_results{0};
      bool is_first{true};
      for (const auto& p : results) {
        if (is_first) {
          is_first = false;
          std::cout << p.first << ":time,"
                    << p.first << ":requested,"
                    << p.first << ":achieved";
          num_results = p.second.size();
        }
        else {
          std::cout << ","
                    << p.first << ":time,"
                    << p.first << ":requested,"
                    << p.first << ":achieved";
          num_results = std::max(num_results, results.size());
        }
      }
      std::cout << "\n";
      for (size_type idx{0}; idx < num_results; ++idx) {
        is_first = true;
        for (const auto& p : results) {
          auto results_size = p.second.size();
          if (is_first) {
            if (idx < results_size) {
              std::cout <<        p.second[idx].time
                        << "," << p.second[idx].requested_value
                        << "," << p.second[idx].achieved_value;
            }
            else {
              std::cout << "--,--,--";
            }
            is_first = false;
          }
          else {
            if (idx < results_size) {
              std::cout << "," << p.second[idx].time
                        << "," << p.second[idx].requested_value
                        << "," << p.second[idx].achieved_value;
              }
            else {
              std::cout << ",--,--,--";
            }
          }
        }
        std::cout << "\n";
      }
      std::cout << "stream_ids:\n";
      for (const auto& p : stream_ids) {
        std::cout << "... " << p.first << ": " << p.second << "\n";
      }
      std::cout << "comp_types:\n";
      for (const auto& p : comp_types) {
        std::cout << "... " << p.first << ": " << component_type_to_tag(p.second) << "\n";
      }
    }
    return process_single_scenario_results(
        sim_good,
        duration,
        scenario_start_s,
        std::move(results),
        std::move(stream_ids),
        std::move(comp_types),
        std::move(port_roles));
  }

  AllResults
  Main::run_all()
  {
    std::unordered_map<std::string, std::vector<ScenarioResults>> out{};
    for (const auto& s: scenarios) {
      const auto& scenario_id = s.first;
      std::vector<ScenarioResults> results{};
      for (const auto& start_time : scenario_schedules[scenario_id]) {
        auto result = run(scenario_id, start_time);
        if (result.get_is_good()) {
          results.emplace_back(result);
        } else {
          std::ostringstream oss{};
          oss << "problem running scenario \"" << scenario_id << "\"\n";
          throw std::invalid_argument(oss.str());
        }
      }
      out[scenario_id] = results;
    }
    return AllResults{true, out};
  }

  RealTimeType
  Main::max_time_for_scenario(const std::string& scenario_id)
  {
    auto it = scenarios.find(scenario_id);
    if (it == scenarios.end()) {
      std::ostringstream oss;
      oss << "scenario_id \"" << scenario_id << "\" not found in scenarios";
      throw std::invalid_argument(oss.str());
    }
    return it->second.get_duration();
  }

  void
  Main::check_data() const
  {
    // TODO: implement this
  }

  Main
  make_main_from_string(const std::string& raw_toml)
  {
    std::stringstream ss{};
    ss << raw_toml;
    return Main{TomlInputReader{ss}};
  }


  ////////////////////////////////////////////////////////////
  // Scenario
  Scenario::Scenario(
      std::string name_,
      std::string network_id_,
      RealTimeType duration_,
      int max_occurrences_,
      ERIN::size_type occurrence_dist_id_,
      std::unordered_map<std::string, double> intensities_,
      bool calc_reliability_):
    name{std::move(name_)},
    network_id{std::move(network_id_)},
    duration{duration_},
    max_occurrences{max_occurrences_},
    occurrence_distribution_id{occurrence_dist_id_},
    intensities{std::move(intensities_)},
    num_occurrences{0},
    calc_reliability{calc_reliability_}
  {
  }

  std::ostream&
  operator<<(std::ostream& os, const Scenario& s)
  {
    os << "Scenario("
       << "name=\"" << s.name << "\", "
       << "network_id=\"" << s.network_id << "\", "
       << "duration=" << s.duration << ", "
       << "max_occurrences=" << s.max_occurrences << ", "
       << "occurrence_distribution_id=" << s.occurrence_distribution_id << ", "
       << "intensities=..., "
       << "num_occurrences=" << s.num_occurrences << ", "
       << "calc_reliability=" << (s.calc_reliability ? "true" : "false")
       << ")";
    return os;
  }

  bool
  operator==(const Scenario& a, const Scenario& b)
  {
    return (a.name == b.name) &&
           (a.network_id == b.network_id) &&
           (a.duration == b.duration) &&
           (a.max_occurrences == b.max_occurrences) &&
           (a.occurrence_distribution_id == b.occurrence_distribution_id) &&
           (a.intensities == b.intensities) &&
           (a.calc_reliability == b.calc_reliability);
  }

  bool
  operator!=(const Scenario& a, const Scenario& b)
  {
    return !(a == b);
  }


  ////////////////////////////////////////////////////////////
  // Helper Functions
  bool
  run_devs(
      adevs::Simulator<PortValue, Time>& sim,
      const RealTimeType max_time,
      const int max_no_advance,
      const std::string& run_id)
  {
    bool sim_good{true};
    int non_advance_count{0};
    for (
        auto t = sim.now(), t_next = sim.next_event_time();
        ((t_next < inf) && (t_next.real <= max_time));
        sim.exec_next_event(), t = t_next, t_next = sim.next_event_time()) {
      if (t_next.real == t.real) {
        ++non_advance_count;
      }
      else {
        non_advance_count = 0;
      }
      if (non_advance_count >= max_no_advance) {
        sim_good = false;
        std::cout << "ERROR: non_advance_count > max_no_advance:\n";
        std::cout << "run_id           : " << run_id << "\n";
        std::cout << "non_advance_count: " << non_advance_count << "\n";
        std::cout << "max_no_advance   : " << max_no_advance << "\n";
        std::cout << "time.real        : " << t.real << " seconds\n";
        std::cout << "time.logical     : " << t.logical << "\n";
        break;
      }
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "The current time is:\n";
        std::cout << "... real   : " << t.real << "\n";
        std::cout << "... logical: " << t.logical << "\n";
      }
    }
    return sim_good;
  }

  ScenarioResults
  process_single_scenario_results(
      bool sim_good,
      RealTimeType duration,
      RealTimeType scenario_start_s,
      std::unordered_map<std::string,std::vector<Datum>> results,
      std::unordered_map<std::string,std::string> stream_ids,
      std::unordered_map<std::string,ComponentType> comp_types,
      std::unordered_map<std::string,PortRole> port_roles)
  {
    if (!sim_good) {
      return ScenarioResults{
        sim_good,
        scenario_start_s,
        duration,
        std::move(results),
        std::move(stream_ids),
        std::move(comp_types),
        std::move(port_roles)
      };
    }
    return ScenarioResults{
      sim_good,
      scenario_start_s, 
      duration,
      std::move(results),
      std::move(stream_ids),
      std::move(comp_types),
      std::move(port_roles)};
  }

  double
  calc_energy_availability_from_stats(const ScenarioStats& ss)
  {
    auto numerator = static_cast<double>(ss.uptime);
    auto denominator = static_cast<double>(ss.uptime + ss.downtime);
    if ((ss.uptime + ss.downtime) == 0)
      return 0.0;
    return numerator / denominator;
  }

  std::unordered_map<std::string, FlowValueType>
  calc_energy_usage_by_stream(
      const std::vector<std::string>& comp_ids,
      const ComponentType ct,
      const std::unordered_map<std::string, ScenarioStats>& stats_by_comp,
      const std::unordered_map<std::string, std::string>& streams_by_comp,
      const std::unordered_map<std::string, ComponentType>& comp_type_by_comp)
  {
    std::unordered_map<std::string, FlowValueType> totals{};
    for (const auto& comp_id: comp_ids) {
      const auto& this_ct = comp_type_by_comp.at(comp_id);
      if (this_ct != ct) {
        continue;
      }
      const auto& stats = stats_by_comp.at(comp_id);
      const auto& stream_name = streams_by_comp.at(comp_id);
      auto it = totals.find(stream_name);
      if (it == totals.end()) {
        totals[stream_name] = stats.total_energy;
      }
      else {
        totals[stream_name] += stats.total_energy;
      }
    }
    return totals;
  }

  std::unordered_map<std::string, FlowValueType>
  calc_energy_usage_by_port_role(
      const std::vector<std::string>& port_ids,
      const PortRole role,
      const std::unordered_map<std::string, ScenarioStats>& stats_by_port_id,
      const std::unordered_map<std::string, std::string>& streams_by_port_id,
      const std::unordered_map<std::string, PortRole>& port_role_by_port_id)
  {
    std::unordered_map<std::string, FlowValueType> totals{};
    for (const auto& port_id: port_ids) {
      const auto& this_role = port_role_by_port_id.at(port_id);
      if (this_role != role) {
        continue;
      }
      const auto& stats = stats_by_port_id.at(port_id);
      const auto& stream_name = streams_by_port_id.at(port_id);
      auto it = totals.find(stream_name);
      if (it == totals.end()) {
        totals[stream_name] = stats.total_energy;
      }
      else {
        totals[stream_name] += stats.total_energy;
      }
    }
    return totals;
  }

  std::unordered_map<std::string,std::string>
  stream_types_to_stream_ids(
      const std::unordered_map<std::string,std::string>& stream_types)
  {
    std::unordered_map<std::string,std::string> stream_ids{};
    for (const auto& item: stream_types) {
      stream_ids[item.first] = item.second;
    }
    return stream_ids;
  }

  std::unordered_map<std::string,std::string>
  stream_types_to_stream_ids(
      std::unordered_map<std::string,std::string>&& stream_types)
  {
    std::unordered_map<std::string,std::string> stream_ids{};
    for (auto it = stream_types.cbegin(); it != stream_types.cend();) {
      const auto& s = it->second;
      stream_ids.emplace(std::make_pair(it->first, s));
      it = stream_types.erase(it);
    }
    return stream_ids;
  }

  template <class T>
  std::vector<T>
  unpack_results(
      const std::unordered_map<std::string, std::vector<Datum>>& results,
      const std::string& comp_id,
      const std::function<T(const Datum&)>& f)
  {
    auto it = results.find(comp_id);
    if (it == results.end()) {
      return std::vector<T>{};
    }
    const auto& data = results.at(comp_id);
    std::vector<T> output(data.size());
    std::transform(data.cbegin(), data.cend(), output.begin(), f);
    return output;
  }

  std::vector<RealTimeType>
  get_times_from_results_for_component(
      const std::unordered_map<std::string, std::vector<Datum>>& results,
      const std::string& comp_id)
  {
    return unpack_results<RealTimeType>(
        results, comp_id,
        [](const Datum& d) -> RealTimeType { return d.time; });
  }

  std::vector<FlowValueType>
  get_actual_flows_from_results_for_component(
      const std::unordered_map<std::string, std::vector<Datum>>& results,
      const std::string& comp_id)
  {
    return unpack_results<FlowValueType>(
        results, comp_id,
        [](const Datum& d) -> FlowValueType { return d.achieved_value; });
  }

  std::vector<FlowValueType>
  get_requested_flows_from_results_for_component(
      const std::unordered_map<std::string, std::vector<Datum>>& results,
      const std::string& comp_id)
  {
    return unpack_results<FlowValueType>(
        results, comp_id,
        [](const Datum& d) -> FlowValueType { return d.requested_value; });
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
}
