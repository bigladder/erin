/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_ERIN_H
#define ERIN_ERIN_H
#include "../../vendor/bdevs/include/adevs.h"
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
#include <vector>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // ScenarioStats
  struct ScenarioStats
  {
    RealTimeType uptime;
    RealTimeType downtime;
    FlowValueType load_not_served;
    FlowValueType total_energy;
  };

  ////////////////////////////////////////////////////////////
  // ScenarioResults
  class ScenarioResults
  {
    public:
      ScenarioResults();
      ScenarioResults(
          bool is_good_,
          const std::unordered_map<std::string,std::vector<Datum>>& results,
          const std::unordered_map<std::string,StreamType>& stream_types,
          const std::unordered_map<std::string,ComponentType>& component_types);
      [[nodiscard]] bool get_is_good() const { return is_good; }
      [[nodiscard]] std::unordered_map<std::string, std::vector<Datum>>
        get_results() const { return results; }

      [[nodiscard]] std::string to_csv(const RealTimeType& max_time) const;
      [[nodiscard]] std::unordered_map<std::string,double>
        calc_energy_availability();
      [[nodiscard]] std::unordered_map<std::string,RealTimeType>
        calc_max_downtime();
      [[nodiscard]] std::unordered_map<std::string,FlowValueType>
        calc_load_not_served();
      [[nodiscard]] std::unordered_map<std::string,FlowValueType>
        calc_energy_usage_by_stream(ComponentType ct);
      [[nodiscard]] std::string to_stats_csv();

    private:
      bool is_good;
      std::unordered_map<std::string, std::vector<Datum>> results;
      std::unordered_map<std::string, StreamType> stream_types;
      std::unordered_map<std::string, ComponentType> component_types;
      std::unordered_map<std::string, ScenarioStats> statistics;
      std::vector<std::string> keys;
  };

  ScenarioStats calc_scenario_stats(const std::vector<Datum>& ds);

  ////////////////////////////////////////////////////////////
  // Scenario
  class Scenario : public adevs::Atomic<PortValue>
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
      [[nodiscard]] const std::unordered_map<std::string,double>
        get_intensities() const { return intensities; }

      bool operator==(const Scenario& other) const;
      bool operator!=(const Scenario& other) const {
        return !(operator==(other));
      }
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

      std::vector<ScenarioResults> get_results() const { return results; }
      void set_runner(const std::function<ScenarioResults(void)>& f) {
        runner = f;
      }

    private:
      std::string name;
      std::string network_id;
      RealTimeType duration;
      int max_occurrences;
      std::function<RealTimeType(void)> calc_time_to_next;
      std::unordered_map<std::string, double> intensities;
      RealTimeType t;
      int num_occurrences;
      std::vector<ScenarioResults> results;
      std::function<ScenarioResults(void)> runner;
  };

  ////////////////////////////////////////////////////////////
  // InputReader
  class InputReader
  {
    public:
      virtual StreamInfo read_stream_info() = 0;
      virtual std::unordered_map<std::string, StreamType>
        read_streams(const StreamInfo& si) = 0;
      virtual std::unordered_map<std::string, std::vector<LoadItem>>
        read_loads() = 0;
      virtual std::unordered_map<std::string, std::unique_ptr<Component>>
        read_components(
            const std::unordered_map<std::string, StreamType>& stm,
            const std::unordered_map<std::string, std::vector<LoadItem>>&
              loads_by_id) = 0;
      virtual std::unordered_map<
        std::string,
        std::vector<::erin::network::Connection>> read_networks() = 0;
      virtual std::unordered_map<std::string, Scenario> read_scenarios() = 0;
      virtual ~InputReader() = default;
  };

  ////////////////////////////////////////////////////////////
  // TomlInputReader
  class TomlInputReader : public InputReader
  {
    public:
      explicit TomlInputReader(toml::value  v);
      explicit TomlInputReader(const std::string& path);
      explicit TomlInputReader(std::istream& in);

      StreamInfo read_stream_info() override;
      std::unordered_map<std::string, StreamType>
        read_streams(const StreamInfo& si) override;
      std::unordered_map<std::string, std::vector<LoadItem>>
        read_loads() override;
      std::unordered_map<std::string, std::unique_ptr<Component>>
        read_components(
            const std::unordered_map<std::string, StreamType>& stm,
            const std::unordered_map<
              std::string, std::vector<LoadItem>>& loads_by_id) override;
      std::unordered_map<
        std::string, std::vector<::erin::network::Connection>>
        read_networks() override;
      std::unordered_map<std::string, Scenario> read_scenarios() override;

    private:
      toml::value data;

      std::vector<LoadItem>
        get_loads_from_array(const std::vector<toml::table>& load_array) const;
      std::vector<LoadItem>
        load_loads_from_csv(const std::string& file_path) const;
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

    private:
      bool is_good;
      std::unordered_map<std::string, std::vector<ScenarioResults>> results;
  };

  ////////////////////////////////////////////////////////////
  // Main Class
  class Main
  {
    public:
      explicit Main(const std::string& input_toml);
      Main(
          const StreamInfo& si,
          const std::unordered_map<std::string, StreamType>& streams,
          const std::unordered_map<
            std::string,
            std::unique_ptr<Component>>& comps,
          const std::unordered_map<
            std::string,
            std::vector<::erin::network::Connection>>& networks,
          const std::unordered_map<std::string, Scenario>& scenarios);
      ScenarioResults run(const std::string& scenario_id);
      AllResults run_all(RealTimeType sim_max_time);
      RealTimeType max_time_for_scenario(const std::string& scenario_id);

    private:
      StreamInfo stream_info;
      std::unordered_map<std::string, StreamType> stream_types_map;
      std::unordered_map<std::string, std::unique_ptr<Component>> components;
      std::unordered_map<
        std::string,
        std::vector<::erin::network::Connection>> networks;
      std::unordered_map<std::string, Scenario> scenarios;

      void check_data() const;
      bool run_devs(
          adevs::Simulator<PortValue>& sim,
          const RealTimeType max_time,
          const std::unordered_set<std::string>::size_type max_non_advance);
  };
}

#endif // ERIN_ERIN_H
