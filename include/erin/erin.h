/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

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
#include "erin/network.h"
#include "erin/port.h"
#include "erin/stream.h"
#include "erin/type.h"
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // ScenarioStats
  struct ScenarioStats
  {
    RealTimeType uptime;
    RealTimeType downtime;
    RealTimeType max_downtime;
    FlowValueType load_not_served;
    FlowValueType total_energy;

    ScenarioStats& operator+=(const ScenarioStats& other);
  };

  ScenarioStats operator+(const ScenarioStats& a, const ScenarioStats& b);
  bool operator==(const ScenarioStats& a, const ScenarioStats& b);
  bool operator!=(const ScenarioStats& a, const ScenarioStats& b);
  std::ostream& operator<<(std::ostream& os, const ScenarioStats& s);

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

  ScenarioStats calc_scenario_stats(const std::vector<Datum>& ds);
  bool operator==(const ScenarioResults& a, const ScenarioResults& b);
  bool operator!=(const ScenarioResults& a, const ScenarioResults& b);

  ////////////////////////////////////////////////////////////
  // Scenario
  class Scenario : public adevs::Atomic<PortValue, Time>
  {
    public:
      Scenario(
          std::string name,
          std::string network_id,
          RealTimeType duration,
          int max_occurrences,
          std::function<RealTimeType(void)> calc_time_to_next,
          std::unordered_map<std::string, double> intensities);

      [[nodiscard]] const std::string& get_name() const { return name; }
      [[nodiscard]] const std::string& get_network_id() const {
        return network_id;
      }
      [[nodiscard]] RealTimeType get_duration() const { return duration; }
      [[nodiscard]] int get_max_occurrences() const { return max_occurrences; }
      [[nodiscard]] int get_number_of_occurrences() const {
        return num_occurrences;
      }
      [[nodiscard]] const std::unordered_map<std::string,double>&
        get_intensities() const { return intensities; }

      bool operator==(const Scenario& other) const;
      bool operator!=(const Scenario& other) const {
        return !(operator==(other));
      }
      void delta_int() override;
      void delta_ext(Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      // the runner function must be:
      // RealTimeType scenario_start_time (seconds) -> ScenarioResults
      void set_runner(const std::function<void(RealTimeType)>& f) {
        runner = f;
      }

      friend std::ostream& operator<<(std::ostream& os, const Scenario& s);

    private:
      std::string name;
      std::string network_id;
      RealTimeType duration;
      int max_occurrences;
      std::function<RealTimeType(void)> calc_time_to_next;
      std::unordered_map<std::string, double> intensities;
      RealTimeType t;
      int num_occurrences;
      std::function<void(RealTimeType)> runner;
  };

  std::ostream& operator<<(std::ostream& os, const Scenario& s);

  ////////////////////////////////////////////////////////////
  // InputReader
  class InputReader
  {
    public:
      InputReader() = default;
      InputReader(const InputReader&) = delete;
      InputReader& operator=(const InputReader&) = delete;
      InputReader(InputReader&&) = delete;
      InputReader& operator=(InputReader&&) = delete;
      virtual ~InputReader() = default;

      virtual SimulationInfo read_simulation_info() = 0;
      virtual std::unordered_map<std::string, std::string>
        read_streams(const SimulationInfo& si) = 0;
      virtual std::unordered_map<std::string, std::vector<LoadItem>>
        read_loads() = 0;
      virtual std::unordered_map<std::string, std::unique_ptr<Component>>
        read_components(
            const std::unordered_map<std::string, std::vector<LoadItem>>&
              loads_by_id,
            const std::unordered_map<
              std::string, ::erin::fragility::FragilityCurve>& fragilities) = 0;
      virtual std::unordered_map<
        std::string,
        std::vector<::erin::network::Connection>> read_networks() = 0;
      virtual std::unordered_map<std::string, Scenario> read_scenarios() = 0;
      virtual std::unordered_map<std::string,::erin::fragility::FragilityCurve>
        read_fragility_data() = 0;
  };

  struct StreamIDs
  {
    std::string input_stream_id;
    std::string output_stream_id;
    std::string lossflow_stream_id;
  };

  ////////////////////////////////////////////////////////////
  // TomlInputReader
  class TomlInputReader : public InputReader
  {
    public:
      explicit TomlInputReader(toml::value  v);
      explicit TomlInputReader(const std::string& path);
      explicit TomlInputReader(std::istream& in);

      SimulationInfo read_simulation_info() override;
      std::unordered_map<std::string, std::string>
        read_streams(const SimulationInfo& si) override;
      std::unordered_map<std::string, std::vector<LoadItem>>
        read_loads() override;
      std::unordered_map<std::string, std::unique_ptr<Component>>
        read_components(
            const std::unordered_map<
              std::string, std::vector<LoadItem>>& loads_by_id,
            const std::unordered_map<
              std::string, ::erin::fragility::FragilityCurve>& fragilities)
        override;
      std::unordered_map<std::string, std::unique_ptr<Component>>
      read_components(
          const std::unordered_map<std::string, std::vector<LoadItem>>&
            loads_by_id) {
        return read_components(loads_by_id, {});
      }
      std::unordered_map<
        std::string, std::vector<::erin::network::Connection>>
        read_networks() override;
      std::unordered_map<std::string, Scenario> read_scenarios() override;
      std::unordered_map<std::string,::erin::fragility::FragilityCurve>
        read_fragility_data() override;

    private:
      toml::value data;

      [[nodiscard]] std::vector<LoadItem>
        get_loads_from_array(
            const std::vector<toml::value>& load_array,
            TimeUnits time_units,
            RateUnits rate_units) const;
      [[nodiscard]] std::vector<LoadItem>
        load_loads_from_csv(const std::string& file_path) const;
      void read_source_component(
          const toml::table& tt,
          const std::string& id,
          const std::string& stream,
          std::unordered_map<
            std::string, std::unique_ptr<Component>>& comps,
          fragility_map&& frags) const;
      void read_load_component(
          const toml::table& tt,
          const std::string& id,
          const std::string& stream,
          const std::unordered_map<
          std::string, std::vector<LoadItem>>& loads_by_id,
          std::unordered_map<
            std::string, std::unique_ptr<Component>>& components,
          fragility_map&& frags) const;
      void read_muxer_component(
          const toml::table& tt,
          const std::string& id,
          const std::string& stream,
          std::unordered_map<
            std::string, std::unique_ptr<Component>>& components,
          fragility_map&& frags) const;
      void read_converter_component(
          const toml::table& tt,
          const std::string& id,
          const std::string& input_stream,
          const std::string& output_stream,
          const std::string& lossflow_stream,
          std::unordered_map<
            std::string, std::unique_ptr<Component>>& components,
          fragility_map&& frags) const;
      void read_passthrough_component(
          const toml::table& tt,
          const std::string& id,
          const std::string& stream,
          std::unordered_map<
            std::string, std::unique_ptr<Component>>& components,
          fragility_map&& frags) const;
      void read_storage_component(
          const toml::table& tt,
          const std::string& id,
          const std::string& stream,
          std::unordered_map<
            std::string, std::unique_ptr<Component>>& components,
          fragility_map&& frags) const;
      [[nodiscard]] double read_number(const toml::value& v) const;
      [[nodiscard]] double read_number(const std::string& v) const;
      double read_fixed_random_for_sim_info(
          const toml::table& tt, bool& found_it) const;
      std::vector<double> read_fixed_series_for_sim_info(
          const toml::table& tt, bool& found_it) const;
      unsigned int read_random_seed_for_sim_info(
          const toml::table& tt, bool& found_it) const;
      [[nodiscard]] ComponentType
        read_component_type(
            const toml::table& tt,
            const std::string& comp_id) const;
      [[nodiscard]] StreamIDs read_stream_ids(
          const toml::table& tt,
          const std::string& comp_id) const;
      [[nodiscard]] fragility_map read_component_fragilities(
          const toml::table& tt,
          const std::string& comp_id,
          const std::unordered_map<
            std::string,
            ::erin::fragility::FragilityCurve>& fragilities) const;
  };

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
      Main(
          const SimulationInfo& si,
          const std::unordered_map<
            std::string,
            std::unique_ptr<Component>>& comps,
          const std::unordered_map<
            std::string,
            std::vector<::erin::network::Connection>>& networks,
          const std::unordered_map<std::string, Scenario>& scenarios);
      ScenarioResults run(
          const std::string& scenario_id, RealTimeType scenario_start_s = 0);
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

    private:
      SimulationInfo sim_info;
      std::unordered_map<std::string, std::unique_ptr<Component>> components;
      std::unordered_map<
        std::string,
        std::vector<::erin::network::Connection>> networks;
      std::unordered_map<std::string, Scenario> scenarios;
      std::unordered_map<
        std::string,
        std::unordered_map<
          std::string,
          std::vector<double>>> failure_probs_by_comp_id_by_scenario_id;
      std::function<double()> rand_fn;

      void check_data() const;
      void generate_failure_fragilities();
  };

  Main make_main_from_string(const std::string& raw_toml);

  ////////////////////////////////////////////////////////////
  // Helper Functions
  bool run_devs(
      adevs::Simulator<PortValue, Time>& sim,
      const RealTimeType max_time,
      const int max_no_advance,
      const std::string& run_id);

  ScenarioResults process_single_scenario_results(
      bool sim_good,
      RealTimeType duration,
      RealTimeType scenario_start_time_s,
      std::unordered_map<std::string,std::vector<Datum>> results,
      std::unordered_map<std::string,std::string> stream_ids,
      std::unordered_map<std::string,ComponentType> comp_types,
      std::unordered_map<std::string,PortRole> port_roles);

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

  std::string parse_component_id(const std::string& tag);

  erin::port::Type parse_component_port(const std::string& tag);

  int parse_component_port_num(const std::string& tag);
}

#endif // ERIN_ERIN_H
