/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_ERIN_H
#define ERIN_ERIN_H
#include "adevs.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include "erin/component.h"
#include "erin/distribution.h"
#include "erin/element.h"
#include "erin/fragility.h"
#include "erin/input_reader.h"
#include "erin/network.h"
#include "erin/port.h"
#include "erin/reliability.h"
#include "erin/scenario.h"
#include "erin/scenario_stats.h"
#include "erin/scenario_results.h"
#include "erin/stream.h"
#include "erin/type.h"
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // AllScenarioStats
  struct AllScenarioStats
  {
    std::vector<ScenarioResults>::size_type num_occurrences;
    RealTimeType time_in_scenario_s;
    std::unordered_map<std::string, RealTimeType> max_downtime_by_comp_id_s;
    std::unordered_map<std::string, std::string> stream_types_by_comp_id;
    std::unordered_map<std::string, ComponentType> component_types_by_comp_id;
    std::unordered_map<std::string, PortRole> port_roles_by_port_id;
    std::unordered_map<std::string, double> energy_availability_by_comp_id;
    std::unordered_map<std::string, double> load_not_served_by_comp_id_kW;
    std::unordered_map<std::string, double> total_energy_by_comp_id_kJ;
    std::unordered_map<std::string, double> totals_by_stream_id_for_source_kJ;
    std::unordered_map<std::string, double> totals_by_stream_id_for_load_kJ;
    std::unordered_map<std::string, double> totals_by_stream_id_for_storage_kJ;
    std::unordered_map<std::string, double> totals_by_stream_id_for_waste_kJ;
  };

  ////////////////////////////////////////////////////////////
  // AllResults
  class AllResults
  {
    public:
      AllResults();
      AllResults(bool is_good_);
      AllResults(
          bool is_good,
          const std::unordered_map<std::string, std::vector<ScenarioResults>>&
            results_);
      [[nodiscard]] bool get_is_good() const { return is_good; }
      [[nodiscard]] std::unordered_map<
        std::string, std::vector<ScenarioResults>> get_results() const {
          return results;
        }
      [[nodiscard]] std::string to_csv() const;
      [[nodiscard]] std::string to_stats_csv() const;
      [[nodiscard]] std::unordered_map<std::string, AllScenarioStats>
        get_stats() const;
      [[nodiscard]] std::vector<std::string>::size_type
        number_of_scenarios() const { return scenario_ids.size(); }
      [[nodiscard]] const std::vector<std::string>& get_scenario_ids() const {
        return scenario_ids;
      }
      [[nodiscard]] std::unordered_map<
        std::string, std::vector<ScenarioResults>::size_type>
          get_num_results() const;
      [[nodiscard]] std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::vector<double>>>
          get_total_energy_availabilities() const;
      [[nodiscard]] const std::vector<std::string>& get_comp_ids() const {
        return comp_ids;
      }
      [[nodiscard]] AllResults with_is_good_as(bool is_good) const;

      friend bool operator==(const AllResults& a, const AllResults& b);

    private:
      bool is_good;
      std::unordered_map<std::string, std::vector<ScenarioResults>> results;
      // generated
      std::set<std::string> scenario_id_set;
      std::set<std::string> comp_id_set;
      std::set<std::string> stream_key_set;
      std::vector<std::string> scenario_ids;
      std::vector<std::string> comp_ids;
      std::vector<std::string> stream_keys;
      // map from a pair of scenario occurrence time and scenario id to the
      // scenario results (can only be one for a given pair).
      std::map<
        std::pair<RealTimeType,std::string>,
        std::reference_wrapper<const ScenarioResults>> outputs;

      void write_total_line_for_stats_csv(
          std::ostream& oss,
          const std::string& scenario_id,
          const AllScenarioStats& all_ss,
          const std::unordered_map<std::string, double>& totals_by_stream,
          const std::string& label) const;
      void write_energy_balance_line_for_stats_csv(
          std::ostream& oss,
          const std::string& scenario_id,
          const AllScenarioStats& all_ss,
          const FlowValueType& balance) const;
      void write_component_line_for_stats_csv(
          std::ostream& oss,
          const AllScenarioStats& all_ss,
          const std::string& comp_id,
          const std::string& scenario_id) const;
      void write_header_for_stats_csv(std::ostream& oss) const;
  };

  bool operator==(const AllResults& a, const AllResults& b);
  bool operator!=(const AllResults& a, const AllResults& b);

  ////////////////////////////////////////////////////////////
  // Main Class
  class Main
  {
    public:
      explicit Main(const std::string& input_toml);
      explicit Main(const InputReader& reader);
      Main(
          const SimulationInfo& si,
          const std::unordered_map<
            std::string,
            std::unique_ptr<Component>>& comps,
          const std::unordered_map<
            std::string,
            std::vector<::erin::network::Connection>>& networks,
          const std::unordered_map<std::string, Scenario>& scenarios,
          const std::unordered_map<std::string, std::vector<RealTimeType>>&
            scenario_schedules,
          const std::unordered_map<std::string, std::vector<TimeState>>&
            reliability_schedule = {},
          const std::unordered_map<std::string, std::vector<std::unordered_map<std::string, erin::fragility::FragilityInfo>>>&
            fi_by_comp_by_inst_by_scenario = {}
          );
      ScenarioResults run(
          const std::string& scenario_id,
          RealTimeType scenario_start_s = 0,
          int instance_num = 0);
      AllResults run_all();
      RealTimeType max_time_for_scenario(const std::string& scenario_id);
      [[nodiscard]] const SimulationInfo& get_sim_info() const {
        return sim_info;
      }
      [[nodiscard]] const std::unordered_map<
        std::string, std::unique_ptr<Component>>& get_components() const {
          return components;
        }
      [[nodiscard]] const std::unordered_map<
        std::string, std::vector<::erin::network::Connection>>& get_networks() const {
          return networks;
        }
      [[nodiscard]] std::unordered_map<std::string, std::vector<TimeState>>
        get_reliability_schedule() const {
          return reliability_schedule;
        }
      [[nodiscard]] RealTimeType time_of_first_occurrence_of_scenario(
        const std::string& scenario_tag);

    private:
      SimulationInfo sim_info;
      std::unordered_map<std::string, std::unique_ptr<Component>> components;
      std::unordered_map<
        std::string,
        std::vector<::erin::network::Connection>> networks;
      std::unordered_map<std::string, Scenario> scenarios;
      std::unordered_map<std::string, std::vector<TimeState>>
        reliability_schedule;
      std::unordered_map<std::string, std::vector<RealTimeType>>
        scenario_schedules;
      std::unordered_map<std::string,
        std::vector<
          std::unordered_map<std::string, erin::fragility::FragilityInfo>>>
        fragility_info_by_comp_tag_by_instance_by_scenario_tag;

      void check_data() const;
  };

  Main make_main_from_string(const std::string& raw_toml);

  ////////////////////////////////////////////////////////////
  // Helper Functions
  bool run_devs(
      adevs::Simulator<PortValue, Time>& sim,
      const RealTimeType max_time,
      const int max_no_advance,
      const std::string& run_id);

  bool run_devs_v2(
      adevs::Simulator<PortValue, Time>& sim,
      const RealTimeType max_time,
      const int max_no_advance,
      const std::string& run_id,
      std::vector<FlowElement*>& elements);

  ScenarioResults process_single_scenario_results(
      bool sim_good,
      RealTimeType duration,
      RealTimeType scenario_start_time_s,
      std::unordered_map<std::string,std::vector<Datum>> results,
      std::unordered_map<std::string,std::string> stream_ids,
      std::unordered_map<std::string,ComponentType> comp_types,
      std::unordered_map<std::string,PortRole> port_roles);

  std::unordered_map<std::string,std::string>
  stream_types_to_stream_ids(
      const std::unordered_map<std::string,std::string>& stm);
  std::unordered_map<std::string,std::string>
  stream_types_to_stream_ids(
      std::unordered_map<std::string,std::string>&& stm);

  std::vector<RealTimeType>
    get_times_from_results_for_component(
        const std::unordered_map<std::string, std::vector<Datum>>& results,
        const std::string& comp_id);

  std::vector<FlowValueType>
    get_actual_flows_from_results_for_component(
        const std::unordered_map<std::string, std::vector<Datum>>& results,
        const std::string& comp_id);

  std::vector<FlowValueType>
    get_requested_flows_from_results_for_component(
        const std::unordered_map<std::string, std::vector<Datum>>& results,
        const std::string& comp_id);
}

#endif // ERIN_ERIN_H
