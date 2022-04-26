/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_SCENARIO_RESULTS_H
#define ERIN_SCENARIO_RESULTS_H
#include "erin/type.h"
#include "erin/scenario_stats.h"
#include "erin/stream.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // ScenarioResults
  class ScenarioResults
  {
    public:
      ScenarioResults();
      ScenarioResults(
          bool is_good_,
          RealTimeType scenario_start,
          RealTimeType duration,
          std::unordered_map<std::string,std::vector<Datum>> results,
          std::unordered_map<std::string,std::string> stream_ids,
          std::unordered_map<std::string,ComponentType> component_types,
          std::unordered_map<std::string,PortRole> port_roles);
      [[nodiscard]] bool get_is_good() const { return is_good; }
      [[nodiscard]] std::unordered_map<std::string, std::vector<Datum>>
        get_results() const { return results; }
      [[nodiscard]] std::unordered_map<std::string, std::string>
        get_stream_ids() const { return stream_ids; }
      [[nodiscard]] std::unordered_map<std::string, ComponentType>
        get_component_types() const { return component_types; }
      [[nodiscard]] std::unordered_map<std::string, ScenarioStats>
        get_statistics() const { return statistics; }
      [[nodiscard]] std::vector<std::string> get_component_ids() const {
        return keys;
      }
      [[nodiscard]] std::unordered_map<std::string, PortRole>
        get_port_roles_by_port_id() const { return port_roles_by_port_id; }

      [[nodiscard]] std::string to_csv(
          TimeUnits time_units = TimeUnits::Hours) const;
      [[nodiscard]] std::vector<std::string> to_csv_lines(
          const std::vector<std::string>& comp_ids,
          bool make_header = true,
          TimeUnits time_units = TimeUnits::Hours) const;
      [[nodiscard]] std::unordered_map<std::string,double>
        calc_energy_availability() const;
      [[nodiscard]] std::unordered_map<std::string,RealTimeType>
        calc_max_downtime();
      [[nodiscard]] std::unordered_map<std::string,FlowValueType>
        calc_load_not_served();
      [[nodiscard]] std::unordered_map<std::string,FlowValueType>
        calc_energy_usage_by_stream(ComponentType ct);
      [[nodiscard]] std::unordered_map<std::string,FlowValueType>
        calc_energy_usage_by_role(PortRole role);
      [[nodiscard]] std::string to_stats_csv(TimeUnits time_units = TimeUnits::Hours);
      [[nodiscard]] RealTimeType get_start_time_in_seconds() const {
        return scenario_start_time;
      }
      [[nodiscard]] RealTimeType get_duration_in_seconds() const {
        return scenario_duration;
      }
      [[nodiscard]] std::unordered_map<std::string, FlowValueType>
        total_requested_loads_by_stream() const;
      [[nodiscard]] std::unordered_map<std::string, FlowValueType>
        total_achieved_loads_by_stream() const;
      [[nodiscard]] std::unordered_map<std::string, FlowValueType>
        total_energy_availability_by_stream() const;

      friend bool operator==(const ScenarioResults& a, const ScenarioResults& b);

    private:
      bool is_good;
      RealTimeType scenario_start_time;
      RealTimeType scenario_duration;
      std::unordered_map<std::string, std::vector<Datum>> results;
      std::unordered_map<std::string, std::string> stream_ids;
      std::unordered_map<std::string, ComponentType> component_types;
      std::unordered_map<std::string, PortRole> port_roles_by_port_id;
      std::unordered_map<std::string, ScenarioStats> statistics;
      std::vector<std::string> keys;

      [[nodiscard]] std::unordered_map<std::string, FlowValueType>
        total_loads_by_stream_helper(
            const std::function<
              FlowValueType(const std::vector<Datum>& f)>& f) const;
  };

  bool operator==(const ScenarioResults& a, const ScenarioResults& b);
  bool operator!=(const ScenarioResults& a, const ScenarioResults& b);

  ScenarioStats calc_scenario_stats(const std::vector<Datum>& ds);

  double calc_energy_availability_from_stats(const ScenarioStats& ss);

  std::unordered_map<std::string, FlowValueType> calc_energy_usage_by_stream(
      const std::vector<std::string>& comp_ids,
      const ComponentType ct,
      const std::unordered_map<std::string, ScenarioStats>& stats_by_comp,
      const std::unordered_map<std::string, std::string>& streams_by_comp,
      const std::unordered_map<std::string, ComponentType>& comp_type_by_comp);

  std::unordered_map<std::string, FlowValueType> calc_energy_usage_by_port_role(
      const std::vector<std::string>& port_ids,
      const PortRole role,
      const std::unordered_map<std::string, ScenarioStats>& stats_by_port_id,
      const std::unordered_map<std::string, std::string>& streams_by_port_id,
      const std::unordered_map<std::string, PortRole>& port_role_by_port_id);
}

#endif // ERIN_SCENARIO_RESULTS_H