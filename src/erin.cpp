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
    Main(InputReader{input_file_path})
  {
  }

  Main::Main(const InputReader& reader):
    Main(
      reader.get_simulation_info(),
      reader.get_components(),
      reader.get_networks(),
      reader.get_scenarios(),
      reader.get_scenario_schedules(),
      reader.get_reliability_schedule(),
      reader.get_fragility_info_by_comp_by_inst_by_scenario()
    )
  {
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
      const std::unordered_map<std::string, std::vector<RealTimeType>>&
        scenario_schedules_,
      const std::unordered_map<std::string, std::vector<TimeState>>&
        reliability_schedule_,
      const std::unordered_map<std::string, std::vector<std::unordered_map<std::string, erin::fragility::FragilityInfo>>>&
        fi_by_comp_by_inst_by_scenario_
      ):
    sim_info{sim_info_},
    components{},
    networks{networks_},
    scenarios{scenarios_},
    reliability_schedule{reliability_schedule_},
    scenario_schedules{scenario_schedules_},
    fragility_info_by_comp_tag_by_instance_by_scenario_tag{fi_by_comp_by_inst_by_scenario_}
  {
    for (const auto& pair: components_) {
      components.insert(
          std::make_pair(
            pair.first,
            pair.second->clone()));
    }
    check_data();
  }

  ScenarioResults
  Main::run(
    const std::string& scenario_id,
    RealTimeType scenario_start_s,
    int instance_num)
  {
    // TODO: check input structure to ensure that keys are available in maps that
    //       should be there. If not, provide a good error message about what's
    //       wrong.
    namespace EF = erin::fragility;
    std::unordered_map<std::string, EF::FragilityInfo> fibc{};
    const auto it_frag = fragility_info_by_comp_tag_by_instance_by_scenario_tag.find(scenario_id);
    if (it_frag != fragility_info_by_comp_tag_by_instance_by_scenario_tag.end()) {
      auto inst_num = static_cast<std::size_t>(instance_num);
      if (inst_num < it_frag->second.size()) {
        fibc = it_frag->second[inst_num];
      }
    }
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
    auto elements = erin::network::build_v2(
        scenario_id, network, connections, components, fibc, duration, true,
        clipped_reliability_schedule);
    std::shared_ptr<FlowWriter> fw = std::make_shared<DefaultFlowWriter>();
    for (auto e_ptr: elements) {
      e_ptr->set_flow_writer(fw);
    }
    adevs::Simulator<PortValue, Time> sim;
    network.add(&sim);
    const int max_no_advance_factor{10'000};
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
      int instance_num{0};
      for (const auto& start_time : scenario_schedules[scenario_id]) {
        auto result = run(scenario_id, start_time, instance_num);
        ++instance_num;
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
    return Main{InputReader{ss}};
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
}
