/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include "debug_utils.h"
#include "erin/erin.h"
#include "erin_csv.h"
#include "erin/fragility.h"
#include "erin/port.h"
#include "erin/utils.h"
#include "erin_generics.h"
#include "toml_helper.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace ERIN
{
  ScenarioStats&
  ScenarioStats::operator+=(const ScenarioStats& other)
  {
    uptime += other.uptime;
    downtime += other.downtime;
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
      a.load_not_served + b.load_not_served,
      a.total_energy + b.total_energy};
  }

  bool
  operator==(const ScenarioStats& a, const ScenarioStats& b)
  {
    if ((a.uptime == b.uptime) && (a.downtime == b.downtime)) {
      const auto diff_lns = std::abs(a.load_not_served - b.load_not_served);
      const auto diff_te = std::abs(a.total_energy - b.total_energy);
      const auto& fvt = flow_value_tolerance;
      return (diff_lns < fvt) && (diff_te < fvt);
    }
    return false;
  }

  ////////////////////////////////////////////////////////////
  // TomlInputReader
  TomlInputReader::TomlInputReader(toml::value data_):
    InputReader(),
    data{std::move(data_)}
  {
  }

  TomlInputReader::TomlInputReader(const std::string& path):
    InputReader(),
    data{}
  {
    data = toml::parse(path);
  }

  TomlInputReader::TomlInputReader(std::istream& in):
    InputReader(),
    data{}
  {
    data = toml::parse(in, "input_from_string.toml");
  }

  SimulationInfo
  TomlInputReader::read_simulation_info()
  {
    const auto sim_info = toml::find(data, "simulation_info");
    const std::string rate_unit =
      toml::find_or(sim_info, "rate_unit", "kW");
    const std::string quantity_unit =
      toml::find_or(sim_info, "quantity_unit", "kJ");
    const std::string time_tag =
      toml::find_or(sim_info, "time_unit", "years");
    const TimeUnits time_units = tag_to_time_units(time_tag);
    const RealTimeType max_time =
      static_cast<RealTimeType>(toml::find_or(sim_info, "max_time", 1000));
    return SimulationInfo{rate_unit, quantity_unit, time_units, max_time};
  }

  std::unordered_map<std::string, StreamType>
  TomlInputReader::read_streams(const SimulationInfo&)
  {
    const auto toml_streams = toml::find<toml::table>(data, "streams");
    std::unordered_map<std::string, StreamType> stream_types_map;
    for (const auto& s: toml_streams) {
      toml::value t = s.second;
      toml::table tt = toml::get<toml::table>(t);
      auto other_rate_units = std::unordered_map<std::string,FlowValueType>();
      auto other_quantity_units =
        std::unordered_map<std::string,FlowValueType>();
      auto it1 = tt.find("other_rate_units");
      if (it1 != tt.end()) {
        const auto oru = toml::get<toml::table>(it1->second);
        for (const auto& p: oru)
          other_rate_units.insert(std::pair<std::string,FlowValueType>(
                p.first,
                toml::get<FlowValueType>(p.second)));
      }
      auto it2  = tt.find("other_quantity_units");
      if (it2 != tt.end()) {
        const auto oqu = toml::get<toml::table>(it2->second);
        for (const auto& p: oqu)
          other_quantity_units.insert(std::pair<std::string,FlowValueType>(
                p.first,
                toml::get<FlowValueType>(p.second)));
      }
      const std::string stream_type{toml::find<std::string>(t, "type")};
      stream_types_map.insert(std::make_pair(
            s.first,
            StreamType(
              stream_type,
              "kW",
              "kJ",
              1.0,
              other_rate_units,
              other_quantity_units)));
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
    double out{0.0};
    try {
      out = toml::get<double>(v);
    }
    catch (toml::exception&) {
      out = static_cast<double>(toml::get<int>(v));
    }
    return out;
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
      if ((size < 1) || (size > 2)) {
        std::ostringstream oss;
        oss << "time_rate_pairs must be 1 or 2 elements in length; "
            << "size = " << size << "\n";
        throw std::runtime_error(oss.str());
      }
      the_time = time_to_seconds(read_number(xs[0]), time_units);
      if (size == 2) {
        the_value = static_cast<FlowValueType>(read_number(xs[1]));
        the_loads.emplace_back(LoadItem{the_time, the_value});
      } else {
        the_loads.emplace_back(LoadItem{the_time});
      }
    }
    return the_loads;
  }

  std::vector<LoadItem>
  TomlInputReader::load_loads_from_csv(const std::string& csv_path) const
  {
    std::vector<LoadItem> the_loads{};
    TimeUnits time_units{TimeUnits::Hours};
    RateUnits rate_units{RateUnits::KiloWatts};
    RealTimeType t;
    FlowValueType v;
    std::ifstream ifs{csv_path};
    if (!ifs.is_open()) {
      std::ostringstream oss;
      oss << "input file stream on \"" << csv_path
          << "\" failed to open for reading\n";
      throw std::runtime_error(oss.str());
    }
    bool in_header{true};
    for (int row{0}; ifs.good(); ++row) {
      auto cells = ::erin_csv::read_row(ifs);
      if (cells.size() == 0) {
        break;
      }
      if (in_header) {
        in_header = false;
        if (cells.size() < 2) {
          std::ostringstream oss;
          oss << "badly formatted file \"" << csv_path << "\"\n";
          oss << "row: " << row << "\n";
          ::erin_csv::stream_out(oss, cells);
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
          ::erin_csv::stream_out(oss, cells);
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
          ::erin_csv::stream_out(oss, cells);
          oss << "original error: " << e.what() << "\n";
          ifs.close();
          throw std::runtime_error(oss.str());
        }
        if (rate_units != RateUnits::KiloWatts) {
          std::ostringstream oss;
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
        ::erin_csv::stream_out(oss, cells);
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
        ::erin_csv::stream_out(oss, cells);
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
    auto& back = the_loads.back();
    back = LoadItem{back.get_time()};
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "loads read in:\n";
      for (const auto ld: the_loads) {
        std::cout << "  {t: " << ld.get_time();
        if (ld.get_is_end()) {
          std::cout << "}\n";
        }
        else {
          std::cout << ", v: " << ld.get_value() << "}\n";
        }
      }
    }
    ifs.close();
    return the_loads;
  }

  std::unordered_map<std::string, std::unique_ptr<Component>>
  TomlInputReader::read_components(
      const std::unordered_map<std::string, StreamType>& stream_types_map,
      const std::unordered_map<std::string, std::vector<LoadItem>>& loads_by_id,
      const std::unordered_map<std::string, ::erin::fragility::FragilityCurve>&
        fragilities)
  {
    namespace ef = ::erin::fragility;
    const auto toml_comps = toml::find<toml::table>(data, "components");
    if constexpr (debug_level >= debug_level_high) {
      std::cout << toml_comps.size() << " components found\n";
    }
    std::unordered_map<std::string, std::unique_ptr<Component>> components{};
    std::string field_read;
    for (const auto& c: toml_comps) {
      const auto& comp_id = c.first;
      toml::value t = c.second;
      const toml::table tt = toml::get<toml::table>(t);
      std::string comp_type_tag;
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
      std::string input_stream_id;
      try {
        input_stream_id =
          toml_helper::read_required_table_field<std::string>(
              tt, {"input_stream", "stream", "output_stream"}, field_read);
      }
      catch (std::out_of_range& e) {
        std::ostringstream oss;
        oss << "original error: " << e.what() << "\n";
        oss << "failed to find 'input_stream', 'stream', "
            << "or 'output_stream' for component \"" << comp_id << "\"\n";
        throw std::runtime_error(oss.str());
      }
      auto output_stream_id = input_stream_id;
      if (field_read != "stream") {
        output_stream_id = toml_helper::read_optional_table_field<std::string>(
            tt, {"output_stream"}, input_stream_id, field_read);
      }
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
        std::cout << "comp: " << comp_id << ".input_stream_id  = "
                  << input_stream_id << "\n";
        std::cout << "comp: " << comp_id << ".output_stream_id = "
                  << output_stream_id << "\n";
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
      switch (component_type) {
        case ComponentType::Source:
          {
            auto stream_type_it = stream_types_map.find(output_stream_id);
            if (stream_type_it == stream_types_map.end()) {
              std::ostringstream oss;
              oss << "output stream '" << output_stream_id << "' not "
                  << "found in stream_types_map for component '"
                  << comp_id << "'";
              throw std::runtime_error(oss.str());
            }
            const auto& stream_type = (*stream_type_it).second;
            read_source_component(
                tt,
                comp_id,
                stream_type,
                components,
                std::move(frags));
            break;
          }
        case ComponentType::Load:
          read_load_component(
              tt,
              comp_id,
              stream_types_map.at(input_stream_id),
              loads_by_id,
              components,
              std::move(frags));
          break;
        case ComponentType::Muxer:
          read_muxer_component(
              tt,
              comp_id,
              stream_types_map.at(input_stream_id),
              components,
              std::move(frags));
          break;
        case ComponentType::Converter:
          {
            std::ostringstream oss;
            oss << "unimplemented component type\n";
            oss << "type = \"" << comp_type_tag << "\" "
                << "(" << static_cast<int>(component_type) << ")\n";
            throw std::runtime_error(oss.str());
            break;
          }
        default:
          {
            std::ostringstream oss;
            oss << "unhandled component type\n";
            oss << "type = \"" << comp_type_tag << "\" "
              << "(" << static_cast<int>(component_type) << ")\n";
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

  std::unordered_map<std::string, ::erin::fragility::FragilityCurve>
  TomlInputReader::read_fragility_data()
  {
    namespace ef = erin::fragility;
    std::unordered_map<std::string, ef::FragilityCurve> out;
    std::string field_read;
    const auto& tt = toml_helper::read_optional_table_field<toml::table>(
        toml::get<toml::table>(data), {"fragility"}, toml::table{}, field_read);
    if (tt.empty()) {
      return out;
    }
    for (const auto& pair : tt) {
      const auto& curve_id = pair.first;
      const auto& data_table = toml::get<toml::table>(pair.second);
      std::string curve_type_tag;
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
            double lower_bound;
            double upper_bound;
            try {
              lower_bound = toml_helper::read_required_table_field<double>(
                  data_table, {"lower_bound"}, field_read);
              upper_bound = toml_helper::read_required_table_field<double>(
                  data_table, {"upper_bound"}, field_read);
            }
            catch (std::out_of_range& e) {
              std::ostringstream oss;
              oss << "lower_bound and/or upper_bound not read correctly "
                << "for '" << curve_id << "'; "
                << "be sure to specify numbers with decimal points "
                << "for TOML to parse.\n";
              oss << "actual error: " << e.what() << "\n";
              throw std::runtime_error(oss.str());
            }
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
      const StreamType& stream,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    std::string field_read;
    const auto max_outflow = toml_helper::read_optional_table_field<FlowValueType>(
        tt, {"max_outflow", "max_output"}, 0.0, field_read);
    bool is_limited{field_read != ""};
    const auto min_outflow = toml_helper::read_optional_table_field<FlowValueType>(
        tt, {"min_outflow", "min_output"}, 0.0, field_read);
    is_limited = is_limited || (field_read != "");
    Limits lim{};
    if (is_limited) {
      lim = Limits{min_outflow, max_outflow};
    }
    std::unique_ptr<Component> source_comp =
      std::make_unique<SourceComponent>(id, stream, std::move(frags), lim);
    components.insert(std::make_pair(id, std::move(source_comp)));
  }

  void
  TomlInputReader::read_load_component(
      const toml::table& tt,
      const std::string& id,
      const StreamType& stream,
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
            std::cout << "(" << li.get_time();
            if (li.get_is_end()) {
              std::cout << ")";
            }
            else {
              std::cout << ", " << li.get_value() << "), ";
            }
          }
          std::cout << "]\n";
        }
      }
      std::unique_ptr<Component> load_comp =
        std::make_unique<LoadComponent>(id, stream, loads_by_scenario, std::move(frags));
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
      const StreamType& stream,
      std::unordered_map<
        std::string, std::unique_ptr<Component>>& components,
      fragility_map&& frags) const
  {
    std::string field_read;
    auto num_inflows = toml_helper::read_optional_table_field<int>(
        tt, {"num_inflows", "num_inputs"}, 1, field_read);
    auto num_outflows = toml_helper::read_optional_table_field<int>(
        tt, {"num_outflows", "num_outputs"}, 1, field_read);
    auto mds_tag = toml_helper::read_optional_table_field<std::string>(
        tt, {"dispatch_strategy"}, "in_order", field_read);
    auto mds = tag_to_muxer_dispatch_strategy(mds_tag);
    std::unique_ptr<Component> mux_comp =
      std::make_unique<MuxerComponent>(
          id, stream, num_inflows, num_outflows, std::move(frags), mds);
    components.insert(
        std::make_pair(id, std::move(mux_comp)));
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
    if constexpr (debug_level >= debug_level_high) {
      std::cout << toml_nets.size() << " networks found\n";
    }
    for (const auto& n: toml_nets) {
      std::vector<enw::Connection> nw_list;
      std::vector<std::vector<std::string>> raw_connects;
      auto nested_nw_table = toml::get<toml::table>(n.second);
      auto nested_nw_it = nested_nw_table.find("connections");
      if (nested_nw_it != nested_nw_table.end()) {
        raw_connects = toml::get<std::vector<std::vector<std::string>>>(
            nested_nw_it->second);
      }
      int item_num{0};
      for (const auto& connect: raw_connects) {
        std::string comp_01_id;
        ::erin::port::Type comp_01_port;
        int comp_01_port_num;
        std::string comp_02_id;
        ::erin::port::Type comp_02_port;
        int comp_02_port_num;
        auto num_items = connect.size();
        const int infer_outflow0_to_inflow0{2};
        const int infer_port_numbers{4};
        const int explicit_connection{6};
        const int inferred_port_number{0};
        if (num_items == infer_outflow0_to_inflow0) {
          const int idx_comp_01_id{0};
          const int idx_comp_02_id{1};
          comp_01_id = connect.at(idx_comp_01_id);
          comp_01_port = ep::Type::Outflow;
          comp_01_port_num = inferred_port_number;
          comp_02_id = connect.at(idx_comp_02_id);
          comp_02_port = ep::Type::Inflow;
          comp_02_port_num = inferred_port_number;
        }
        else if (num_items == infer_port_numbers) {
          const int idx_comp_01_id{0};
          const int idx_comp_01_port_type{1};
          const int idx_comp_02_id{2};
          const int idx_comp_02_port_type{3};
          comp_01_id = connect.at(idx_comp_01_id);
          comp_01_port = ::erin::port::tag_to_type(connect.at(idx_comp_01_port_type));
          comp_01_port_num = inferred_port_number;
          comp_02_id = connect.at(idx_comp_02_id);
          comp_02_port = ::erin::port::tag_to_type(connect.at(idx_comp_02_port_type));
          comp_02_port_num = inferred_port_number;
        }
        else if (num_items == explicit_connection) {
          const int idx_comp_01_id{0};
          const int idx_comp_01_port_type{1};
          const int idx_comp_01_port_num{2};
          const int idx_comp_02_id{3};
          const int idx_comp_02_port_type{4};
          const int idx_comp_02_port_num{5};
          comp_01_id = connect.at(idx_comp_01_id);
          comp_01_port = ::erin::port::tag_to_type(connect.at(idx_comp_01_port_type));
          comp_01_port_num = std::stoi(connect.at(idx_comp_01_port_num));
          comp_02_id = connect.at(idx_comp_02_id);
          comp_02_port = ::erin::port::tag_to_type(connect.at(idx_comp_02_port_type));
          comp_02_port_num = std::stoi(connect.at(idx_comp_02_port_num));
        }
        else {
          std::ostringstream oss;
          oss << "network " << nested_nw_it->first << " "
              << "connection[" << item_num << "] "
              << "doesn't have " << infer_outflow0_to_inflow0
              << ", " << infer_port_numbers
              << ", or " << explicit_connection << " items\n"
              << "num_items: " << num_items << "\n";
          throw std::invalid_argument(oss.str());
        }
        nw_list.emplace_back(
            enw::Connection{
              enw::ComponentAndPort{
                comp_01_id, comp_01_port, comp_01_port_num},
              enw::ComponentAndPort{
                comp_02_id, comp_02_port, comp_02_port_num}});
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
  TomlInputReader::read_scenarios()
  {
    const std::string default_time_units{"hours"};
    std::unordered_map<std::string, Scenario> scenarios;
    const auto toml_scenarios = toml::find<toml::table>(data, "scenarios");
    if constexpr (debug_level >= debug_level_high) {
      std::cout << toml_scenarios.size() << " scenarios found\n";
    }
    for (const auto& s: toml_scenarios) {
      const auto dist = toml::find<toml::table>(
          s.second, "occurrence_distribution");
      const auto next_occurrence_dist =
        ::erin_generics::read_toml_distribution<RealTimeType>(dist);
      const auto time_unit_str = toml::find_or(
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
            std::ostringstream oss;
            oss << "unhandled time units '" << static_cast<int>(time_units)
                << "'";
            throw std::runtime_error(oss.str());
          }
      }
      const auto duration = static_cast<::ERIN::RealTimeType>(
          toml::find<int>(s.second, "duration") * time_multiplier);
      const auto max_occurrences = toml::find<int>(s.second, "max_occurrences");
      const auto network_id = toml::find<std::string>(s.second, "network");
      std::unordered_map<std::string,double> intensity{};
      auto intensity_tmp =
        toml::find_or(
            s.second, "intensity",
            std::unordered_map<std::string,toml::value>{});
      for (const auto& pair: intensity_tmp) {
        double v{0.0};
        try {
          v = pair.second.as_floating();
        }
        catch (const toml::exception&) {
          v = static_cast<double>(pair.second.as_integer());
        }
        intensity.insert(std::make_pair(pair.first, v));
      }
      scenarios.insert(
          std::make_pair(
            s.first,
            Scenario{
              s.first,
              network_id,
              duration,
              max_occurrences,
              next_occurrence_dist,
              intensity}));
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

  ////////////////////////////////////////////////////////////
  // ScenarioResults
  ScenarioResults::ScenarioResults():
    ScenarioResults(false,0,0,{},{},{})
  {
  }

  ScenarioResults::ScenarioResults(
      bool is_good_,
      RealTimeType scenario_start_time_,
      RealTimeType scenario_duration_,
      const std::unordered_map<std::string, std::vector<Datum>>& results_,
      const std::unordered_map<std::string, StreamType>& stream_types_,
      const std::unordered_map<std::string, ComponentType>& component_types_):
    is_good{is_good_},
    scenario_start_time{scenario_start_time_},
    scenario_duration{scenario_duration_},
    results{results_},
    stream_types{stream_types_},
    component_types{component_types_},
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
    if (!::erin::utils::is_superset(comp_ids, keys)) {
      std::ostringstream oss;
      oss << "comp_ids is not a superset of the keys defined in "
             "this ScenarioResults";
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
        times_set.emplace(d.time);
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
    for (std::vector<RealTimeType>::size_type i{0}; i < times.size(); ++i) {
      std::ostringstream oss;
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
    std::unordered_map<std::string, double> out;
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
      auto st_id = stream_types.at(k).get_type();
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
    auto eubs_src = calc_energy_usage_by_stream(ComponentType::Source);
    auto eubs_load = calc_energy_usage_by_stream(ComponentType::Load);
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
      ::erin_generics::print_unordered_map("ea", ea);
      ::erin_generics::print_unordered_map("md", md);
      ::erin_generics::print_unordered_map("lns", lns);
      ::erin_generics::print_unordered_map("eubs_src", eubs_src);
      ::erin_generics::print_unordered_map("eubs_load", eubs_load);
      std::cout << "metrics done...\n";
    }
    std::ostringstream oss;
    oss << "component id,type,stream,energy availability,max downtime ("
        << time_units_to_tag(time_units) << "),load not served (kJ)";
    std::set<std::string> stream_key_set{};
    for (const auto& s: stream_types) {
      stream_key_set.emplace(s.second.get_type());
    }
    std::vector<std::string> stream_keys{
      stream_key_set.begin(), stream_key_set.end()};
    // should not need. set is ordered/sorted.
    //std::sort(stream_keys.begin(), stream_keys.end());
    for (const auto& sk: stream_keys) {
      oss << "," << sk << " energy used (kJ)";
    }
    oss << "\n";
    for (const auto& k: keys) {
      oss << k
          << "," << component_type_to_tag(component_types.at(k))
          << "," << stream_types.at(k).get_type()
          << "," << ea.at(k)
          << "," << convert_time_in_seconds_to(md.at(k), time_units)
          << "," << lns.at(k);
      auto stats = statistics.at(k);
      auto st = stream_types.at(k);
      for (const auto& sk: stream_keys) {
        if (st.get_type() != sk) {
          oss << ",0.0";
          continue;
        }
        oss << "," << stats.total_energy;
      }
      oss << "\n";
    }
    oss << "TOTAL (source),,,,,";
    for (const auto& sk: stream_keys) {
      auto it = eubs_src.find(sk);
      if (it == eubs_src.end()) {
        oss << ",0.0";
        continue;
      }
      oss << "," << (it->second);
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
    }
    oss << "\n";
    return oss.str();
  }

  std::unordered_map<std::string, double>
  ScenarioResults::total_loads_by_stream_helper(
      const std::function<
        FlowValueType(const std::vector<Datum>& f)>& sum_fn) const
  {
    std::unordered_map<std::string, double> out;
    if (is_good) {
      for (const auto& comp_id : keys) {
        const auto& comp_type = component_types.at(comp_id);
        if (comp_type != ComponentType::Load) {
          continue;
        }
        const auto& stream = stream_types.at(comp_id);
        const auto& stream_name = stream.get_type();
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

  std::unordered_map<std::string, double>
  ScenarioResults::total_requested_loads_by_stream() const
  {
    return total_loads_by_stream_helper(sum_requested_load);
  }

  std::unordered_map<std::string, double>
  ScenarioResults::total_achieved_loads_by_stream() const
  {
    return total_loads_by_stream_helper(sum_achieved_load);
  }

  ////////////////////////////////////////////////////////////
  // ScenarioStats
  ScenarioStats
  calc_scenario_stats(const std::vector<Datum>& ds)
  {
    RealTimeType uptime{0};
    RealTimeType downtime{0};
    RealTimeType t0{0};
    FlowValueType req{0};
    FlowValueType ach{0};
    FlowValueType ach_energy{0};
    FlowValueType load_not_served{0.0};
    for (const auto d: ds) {
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
      }
      else {
        uptime += dt;
      }
      load_not_served += dt * gap;
      req = d.requested_value;
      ach_energy += ach * dt;
      ach = d.achieved_value;
    }
    return ScenarioStats{uptime, downtime, load_not_served, ach_energy};
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
            const auto& stream_types{scenario_results.get_stream_types()};
            for (const auto& s: stream_types) {
              stream_key_set.emplace(s.second.get_type());
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
    if (is_good) {
      std::ostringstream oss;
      oss << "scenario id"
             ",number of occurrences"
             ",total time in scenario (hours)"
             ",component id"
             ",type"
             ",stream"
             ",energy availability"
             ",max downtime (hours)"
             ",load not served (kJ)";
      for (const auto& stream_id: stream_keys) {
        oss << "," << stream_id << " energy used (kJ)";
      }
      oss << "\n";
      for (const auto& scenario_id: scenario_ids) {
        const auto& results_for_scenario = results.at(scenario_id);
        const auto num_occurrences{results_for_scenario.size()};
        if (num_occurrences == 0) {
          continue;
        }
        const auto& stream_types = results_for_scenario[0].get_stream_types();
        const auto& comp_types = results_for_scenario[0].get_component_types();
        RealTimeType time_in_scenario{0};
        std::unordered_map<std::string, ScenarioStats> stats_by_comp;
        std::unordered_map<std::string, double> totals_by_stream_source;
        std::unordered_map<std::string, double> totals_by_stream_load;
        for (const auto& scenario_results: results_for_scenario) {
          time_in_scenario += scenario_results.get_duration_in_seconds();
          const auto& stats_by_comp_temp = scenario_results.get_statistics();
          const auto& the_comp_ids = scenario_results.get_component_ids();
          const auto totals_by_stream_source_temp = calc_energy_usage_by_stream(
              the_comp_ids,
              ComponentType::Source,
              stats_by_comp_temp,
              stream_types,
              comp_types);
          const auto totals_by_stream_load_temp = calc_energy_usage_by_stream(
              the_comp_ids,
              ComponentType::Load,
              stats_by_comp_temp,
              stream_types,
              comp_types);
          for (const auto& cid_stats_pair: stats_by_comp_temp) {
            const auto& comp_id = cid_stats_pair.first;
            const auto& stats = cid_stats_pair.second;
            auto it = stats_by_comp.find(comp_id);
            if (it == stats_by_comp.end()) {
              stats_by_comp[comp_id] = stats;
            }
            else {
              it->second += stats;
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
        }
        auto stats_by_comp_end = stats_by_comp.end();
        for (const auto& comp_id: comp_ids) {
          auto it_comp_id = stats_by_comp.find(comp_id);
          if (it_comp_id == stats_by_comp_end) {
            continue;
          }
          const auto& stats = stats_by_comp.at(comp_id);
          std::string stream_name;
          auto it_st = stream_types.find(comp_id);
          if (it_st == stream_types.end()) {
            stream_name = "--";
          }
          else {
            stream_name = it_st->second.get_type();
          }
          std::string comp_type;
          auto it_ct = comp_types.find(comp_id);
          if (it_ct == comp_types.end()) {
            comp_type = "--";
          }
          else {
            const auto& ct = comp_types.at(comp_id);
            comp_type = component_type_to_tag(ct);
          }
          const auto ea = calc_energy_availability_from_stats(stats);
          const auto md = stats.downtime;
          const auto lns = stats.load_not_served;
          oss << scenario_id
              << "," << num_occurrences
              << "," <<
              convert_time_in_seconds_to(time_in_scenario, TimeUnits::Hours)
              << "," << comp_id
              << "," << comp_type
              << "," << stream_name
              << "," << ea
              << "," << convert_time_in_seconds_to(md, TimeUnits::Hours)
              << "," << lns;
          for (const auto& s: stream_keys) {
            if (s != stream_name) {
              oss << ",0.0";
              continue;
            }
            oss << "," << stats.total_energy;
          }
          oss  << "\n";
        }
        auto tbss_end = totals_by_stream_source.end();
        oss << scenario_id
            << "," << num_occurrences
            << "," <<
            convert_time_in_seconds_to(time_in_scenario, TimeUnits::Hours)
            << "," << "TOTAL (source),,,,,";
        for (const auto& s: stream_keys) {
          auto it = totals_by_stream_source.find(s);
          if (it == tbss_end) {
            oss << ",0.0";
            continue;
          }
          oss << "," << it->second;
        }
        oss  << "\n";
        auto tbsl_end = totals_by_stream_load.end();
        oss << scenario_id
            << "," << num_occurrences
            << "," <<
            convert_time_in_seconds_to(time_in_scenario, TimeUnits::Hours)
            << "," << "TOTAL (load),,,,,";
        for (const auto& s: stream_keys) {
          auto it = totals_by_stream_load.find(s);
          if (it == tbsl_end) {
            oss << ",0.0";
            continue;
          }
          oss << "," << it->second;
        }
        oss  << "\n";
      }
      return oss.str();
    }
    return "";
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
    //using size_type = std::vector<ScenarioResults>::size_type;
    using T_scenario_id = std::string;
    using T_stream_name = std::string;
    using T_total_energy_availability = double;
    using T_total_energy_availability_return =
      std::unordered_map<
        T_scenario_id,
        std::unordered_map<
          T_stream_name,
          std::vector<T_total_energy_availability>>>;
    T_total_energy_availability_return out;
    /*
    for (const auto& scenario_id: scenario_ids) {
      const auto& srs = results.at(scenario_id);
      const auto num_srs = srs.size();
      std::vector<double> vs(srs.size(), 0.0);
      for (size_type i{0}; i < num_srs; ++i) {
        const auto& sr = srs.at(i);
        const auto& stats = srs.get_statistics();
        //vs[i] = sr.calc_energy_availability();
      }
      out[scenario_id] = vs;
    }
    */
    return out;
  }

  //////////////////////////////////////////////////////////// 
  // Main
  // main class that runs the simulation from file
  Main::Main(const std::string& input_file_path):
    failure_probs_by_comp_id_by_scenario_id{}
  {
    auto reader = TomlInputReader{input_file_path};
    // Read data into private class fields
    sim_info = reader.read_simulation_info();
    stream_types_map = reader.read_streams(sim_info);
    auto loads_by_id = reader.read_loads();
    auto fragilities = reader.read_fragility_data();
    components = reader.read_components(
        stream_types_map, loads_by_id, fragilities);
    networks = reader.read_networks();
    scenarios = reader.read_scenarios();
    check_data();
    generate_failure_fragilities();
  }

  Main::Main(
      const SimulationInfo& sim_info_,
      const std::unordered_map<std::string, StreamType>& streams_,
      const std::unordered_map<
        std::string,
        std::unique_ptr<Component>>& components_,
      const std::unordered_map<
        std::string,
        std::vector<::erin::network::Connection>>& networks_,
      const std::unordered_map<std::string, Scenario>& scenarios_):
    sim_info{sim_info_},
    stream_types_map{streams_},
    components{},
    networks{networks_},
    scenarios{scenarios_},
    failure_probs_by_comp_id_by_scenario_id{}
  {
    for (const auto& pair: components_) {
      components.insert(
          std::make_pair(
            pair.first,
            pair.second->clone()));
    }
    check_data();
    generate_failure_fragilities();
  }

  void
  Main::generate_failure_fragilities()
  {
    for (const auto& s: scenarios) {
      const auto& intensities = s.second.get_intensities();
      std::unordered_map<std::string, std::vector<double>> m;
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
    auto elements = ::erin::network::build(
        scenario_id, network, connections, components, fpbc);
    adevs::Simulator<PortValue, Time> sim;
    network.add(&sim);
    const auto duration = the_scenario.get_duration();
    const int max_no_advance_factor{10};
    int max_no_advance =
      static_cast<int>(elements.size()) * max_no_advance_factor;
    auto sim_good = run_devs(sim, duration, max_no_advance);
    return process_single_scenario_results(
        sim_good, elements, duration, scenario_start_s);
  }

  AllResults
  Main::run_all()
  {
    RealTimeType sim_max_time = sim_info.get_max_time_in_seconds();
    std::unordered_map<std::string, std::vector<ScenarioResults>> out{};
    // 1. create the network and simulator
    adevs::Simulator<PortValue, Time> sim{};
    // 2. add all scenarios
    std::vector<Scenario*> copies{};
    for (const auto& s: scenarios) {
      auto p = new Scenario{s.second};
      auto scenario_id = p->get_name();
      p->set_runner(
          // ss = scenario start (seconds)
          [this, scenario_id](RealTimeType ss) -> ScenarioResults {
            return this->run(scenario_id, ss);
          });
      sim.add(p);
      // NOTE: sim will delete all pointers passed to it upon deletion.
      // Therefore, we don't need to worry about deleting Scenario pointers
      // (sim does it). Pointers in `copies` are there for reference.
      copies.emplace_back(p);
    }
    // 3. run the simulation
    const int max_no_advance{10};
    const auto sim_good = run_devs(sim, sim_max_time, max_no_advance);
    // 4. pull results into one map
    for (const auto s: copies) {
      out[s->get_name()] = s->get_results();
    }
    return AllResults{sim_good, out};
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
    std::stringstream ss;
    ss << raw_toml;
    TomlInputReader tir{ss};
    const auto si = tir.read_simulation_info();
    const auto streams = tir.read_streams(si);
    const auto loads = tir.read_loads();
    const auto fragilities = tir.read_fragility_data();
    const auto comps = tir.read_components(streams, loads, fragilities);
    const auto networks = tir.read_networks();
    const auto scenarios = tir.read_scenarios();
    return Main{si, streams, comps, networks, scenarios};
  }


  ////////////////////////////////////////////////////////////
  // Scenario
  Scenario::Scenario(
      std::string name_,
      std::string network_id_,
      RealTimeType duration_,
      int max_occurrences_,
      std::function<RealTimeType(void)> calc_time_to_next_,
      std::unordered_map<std::string, double> intensities_):
    adevs::Atomic<PortValue, Time>(),
    name{std::move(name_)},
    network_id{std::move(network_id_)},
    duration{duration_},
    max_occurrences{max_occurrences_},
    calc_time_to_next{std::move(calc_time_to_next_)},
    intensities{std::move(intensities_)},
    t{0},
    num_occurrences{0},
    results{},
    runner{nullptr}
  {
  }

  // TODO: revisit this. Not sure this is the right way to do equality for
  // scenarios. Do we need equality?
  bool
  Scenario::operator==(const Scenario& other) const
  {
    if (this == &other) return true;
    return (name == other.name) &&
           (network_id == other.network_id) &&
           (duration == other.duration) &&
           (max_occurrences == other.max_occurrences) &&
           (intensities == other.intensities);
  }

  void
  Scenario::delta_int()
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Scenario::delta_int();name=" << name << "\n";
    }
    ++num_occurrences;
    if (runner != nullptr) {
      auto result = runner(t);
      results.emplace_back(result);
    }
  }

  void
  Scenario::delta_ext(Time e, std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Scenario::delta_ext("
                << "{" << e.real << "," << e.logical << "}, ";
      auto first{true};
      for (const auto x: xs) {
        if (first) {
          std::cout << "{";
        }
        else {
          std::cout << ", ";
        }
        std::cout << "{port=" << x.port << ", value=" << x.value << "}";
      }
      std::cout << "});name=" << name << "\n";
    }
    // Nothing to do here. Should be no external transitions at this time.
  }

  void
  Scenario::delta_conf(std::vector<PortValue>& xs)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "Scenario::delta_conf();name=" << name << "\n";
    }
    auto e = Time{0,0};
    delta_int();
    delta_ext(e, xs);
  }

  Time
  Scenario::ta()
  {
    if (calc_time_to_next == nullptr) {
      return inf;
    }
    if ((max_occurrences >= 0) && (num_occurrences >= max_occurrences)) {
      return inf;
    }
    auto dt = calc_time_to_next();
    t += dt;
    return Time{dt, 0};
  }

  void
  Scenario::output_func(std::vector<PortValue>&)
  {
    // nothing to do here as well. Scenarios are autonomous and don't interact
    // with each other. Although we *could* have scenarios send their results
    // out to an aggregator on each transition, probably best just for each
    // scenario to track its local state and report back after simulation
    // finishes.
  }

  std::ostream&
  operator<<(std::ostream& os, const Scenario& s)
  {
    os << "Scenario("
       << "name=\"" << s.name << "\", "
       << "network_id=\"" << s.network_id << "\", "
       << "duration=" << s.duration << ", "
       << "max_occurrences=" << s.max_occurrences << ", "
       << "calc_time_next=..., "
       << "intensities=..., "
       << "t=" << s.t << ", "
       << "num_occurrences=" << s.num_occurrences << ", "
       << "results=..., "
       << "runner=...)";
    return os;
  }

  ////////////////////////////////////////////////////////////
  // Helper Functions
  bool
  run_devs(
      adevs::Simulator<PortValue, Time>& sim,
      const RealTimeType max_time,
      const int max_no_advance)
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
      const std::vector<FlowElement*>& elements,
      RealTimeType duration,
      RealTimeType scenario_start_s)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "entering process_single_scenario_results(...)\n";
      std::cout << "... sim_good = " << (sim_good ? "true" : "false") << "\n";
      std::cout << "... elements = " << vec_to_string<FlowElement*>(elements)
                << "\n";
      std::cout << "... duration = " << duration << "\n";
    }
    std::unordered_map<std::string,std::vector<Datum>> results;
    std::unordered_map<std::string,ComponentType> comp_types;
    std::unordered_map<std::string,StreamType> stream_types;
    if (!sim_good) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "sim_good = false; return\n";
      }
      return ScenarioResults{
        sim_good,
        scenario_start_s,
        duration,
        results,
        stream_types,
        comp_types};
    }
    int element_number{0};
    for (const auto& e: elements) {
      auto vals = e->get_results(duration);
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "... .............................\n";
        std::cout << "... element_number: " << element_number << "\n";
        std::cout << "... e: " << e << "\n";
        std::cout << "... e.id: " << e->get_id() << "\n";
        std::cout << "... e.element_type: "
                  << element_type_to_tag(e->get_element_type()) << "\n";
        std::cout << "... e.inflow_type : " << e->get_inflow_type() << "\n";
        std::cout << "... e.outflow_type: " << e->get_outflow_type() << "\n";
        auto the_ct = e->get_component_type();
        std::cout << "... the_ct = " << static_cast<int>(the_ct) << "\n";
        std::cout << "... e.component_type: "
                  << component_type_to_tag(the_ct) << "\n";
        std::cout << "... vals: " << vec_to_string<Datum>(vals) << "\n";
      }
      if (!vals.empty()) {
        auto id{e->get_id()};
        auto in_st{e->get_inflow_type()};
        auto out_st{e->get_outflow_type()};
        auto comp_type{e->get_component_type()};
        if (in_st != out_st) {
          std::ostringstream oss;
          oss << "MixedStreamsError:\n";
          oss << "input stream != output_stream but it should\n";
          oss << "input stream: " << in_st.get_type() << "\n";
          oss << "output stream: " << out_st.get_type() << "\n";
          throw std::runtime_error(oss.str());
        }
        comp_types.emplace(std::make_pair(id, comp_type));
        stream_types.emplace(std::make_pair(id, in_st));
        results.emplace(std::make_pair(id, vals));
      }
      ++element_number;
    }
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "return\n";
    }
    return ScenarioResults{
      sim_good,
      scenario_start_s, 
      duration,
      results,
      stream_types,
      comp_types};
  }

  double
  calc_energy_availability_from_stats(const ScenarioStats& ss)
  {
    auto numerator = static_cast<double>(ss.uptime);
    auto denominator = static_cast<double>(ss.uptime + ss.downtime);
    if ((ss.uptime + ss.downtime) == 0) {
      return 0.0;
    }
    return numerator / denominator;
  }

  std::unordered_map<std::string, FlowValueType>
  calc_energy_usage_by_stream(
      const std::vector<std::string>& comp_ids,
      const ComponentType ct,
      const std::unordered_map<std::string, ScenarioStats>& stats_by_comp,
      const std::unordered_map<std::string, StreamType>& streams_by_comp,
      const std::unordered_map<std::string, ComponentType>& comp_type_by_comp)
  {
    std::unordered_map<std::string, FlowValueType> totals{};
    for (const auto& comp_id: comp_ids) {
      const auto& this_ct = comp_type_by_comp.at(comp_id);
      if (this_ct != ct) {
        continue;
      }
      const auto& stats = stats_by_comp.at(comp_id);
      const auto& stream_name = streams_by_comp.at(comp_id).get_type();
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
}
