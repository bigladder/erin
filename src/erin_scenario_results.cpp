/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/scenario_results.h"
#include "erin/utils.h"
#include "debug_utils.h"
#include "erin_generics.h"
#include <iomanip>

namespace ERIN
{
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

}
