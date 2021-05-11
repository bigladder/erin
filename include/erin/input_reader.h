/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_INPUT_READER_H
#define ERIN_INPUT_READER_H

#include "erin/type.h"
#include "erin/stream.h"
#include "erin/component.h"
#include "erin/network.h"
#include "erin/scenario.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include <optional>
#include <string>
#include <unordered_map>
#include <iostream>

namespace ERIN
{
  std::unordered_map<std::string, std::vector<RealTimeType>>
  calc_scenario_schedule(
      const RealTimeType max_time_s,
      const std::unordered_map<std::string, Scenario>& scenarios,
      const erin::distribution::DistributionSystem& ds,
      const std::function<double()>& rand_fn);

  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<erin::fragility::FailureProbAndRepair>>>
  generate_failure_fragilities(
    const std::unordered_map<std::string, Scenario>& scenarios,
    const std::unordered_map<std::string, std::unique_ptr<Component>>& components);

  class TomlInputReader;

  class InputReader
  {
    public:
      explicit InputReader(const std::string& path);
      explicit InputReader(std::istream& in);

      [[nodiscard]]
      SimulationInfo
      get_simulation_info() const {
        return sim_info;
      };
      [[nodiscard]]
      std::unordered_map<std::string, std::unique_ptr<Component>>
      get_components() const;
      [[nodiscard]]
      std::unordered_map<std::string, std::vector<erin::network::Connection>> 
      get_networks() const {
        return networks;
      };
      [[nodiscard]]
      std::unordered_map<std::string, Scenario>
      get_scenarios() const {
        return scenarios;
      };
      [[nodiscard]]
      std::unordered_map<std::string, std::vector<TimeState>>
      get_reliability_schedule() const {
        return reliability_schedule;
      };
      [[nodiscard]]
      std::unordered_map<std::string, std::vector<RealTimeType>>
      get_scenario_schedules() const {
        return scenario_schedules;
      };
      [[nodiscard]]
      std::unordered_map<std::string, std::vector<std::unordered_map<std::string, erin::fragility::FragilityInfo>>>
      get_fragility_info_by_comp_by_inst_by_scenario() const {
        return fragility_info_by_comp_tag_by_instance_by_scenario_tag;
      };
    
    private:
      SimulationInfo sim_info;
      std::unordered_map<std::string, std::unique_ptr<Component>> components;
      std::unordered_map<std::string, std::vector<erin::network::Connection>> networks;
      std::unordered_map<std::string, Scenario> scenarios;
      std::unordered_map<std::string, std::vector<TimeState>> reliability_schedule;
      std::unordered_map<std::string, std::vector<RealTimeType>> scenario_schedules;
      std::unordered_map<std::string, std::vector<std::unordered_map<std::string, erin::fragility::FragilityInfo>>>
        fragility_info_by_comp_tag_by_instance_by_scenario_tag;
      void initialize(TomlInputReader& reader);
  };

  struct StreamIDs
  {
    std::string input_stream_id;
    std::string output_stream_id;
    std::string lossflow_stream_id;
  };

  ////////////////////////////////////////////////////////////
  // TomlInputReader
  class TomlInputReader
  {
    public:
      explicit TomlInputReader(toml::value  v);
      explicit TomlInputReader(const std::string& path);
      explicit TomlInputReader(std::istream& in);

      SimulationInfo read_simulation_info();
      std::unordered_map<std::string, std::string>
        read_streams(const SimulationInfo& si);
      std::unordered_map<std::string, std::vector<LoadItem>>
        read_loads();
      std::unordered_map<std::string, std::unique_ptr<Component>>
        read_components(
            const std::unordered_map<
              std::string, std::vector<LoadItem>>& loads_by_id,
            const std::unordered_map<
              std::string, erin::fragility::FragilityCurve>& fragility_curves,
            const std::unordered_map<
              std::string, erin::fragility::FragilityMode>& fragility_modes,
            const std::unordered_map<
              std::string, size_type>& failure_modes,
            ReliabilityCoordinator& rc);
      std::unordered_map<std::string, std::unique_ptr<Component>>
        read_components(
            const std::unordered_map<std::string, std::vector<LoadItem>>&
              loads_by_id);
      std::unordered_map<
        std::string, std::vector<erin::network::Connection>>
        read_networks();
      std::unordered_map<std::string, Scenario> read_scenarios(
          const std::unordered_map<std::string, ERIN::size_type>& dists
          );
      std::unordered_map<std::string, erin::fragility::FragilityCurve>
        read_fragility_curve_data();
      std::unordered_map<std::string, size_type>
        read_distributions(erin::distribution::DistributionSystem& cds);
      std::unordered_map<std::string, size_type>
        read_failure_modes(
            const std::unordered_map<std::string, size_type>& dist_ids,
            ReliabilityCoordinator& rc);
      std::unordered_map<std::string, erin::fragility::FragilityMode>
        read_fragility_modes(
          const std::unordered_map<std::string, size_type>& dist_ids,
          const std::unordered_map<std::string, erin::fragility::FragilityCurve>& fragility_curves);

    private:
      toml::value data;

      [[nodiscard]] std::vector<LoadItem>
        get_loads_from_array(
            const std::vector<toml::value>& load_array,
            TimeUnits time_units,
            RateUnits rate_units,
            const std::string& load_id) const;
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
      void read_uncontrolled_source_component(
          const toml::table& tt,
          const std::string& id,
          const std::string& outflow,
          const std::unordered_map<
            std::string, std::vector<LoadItem>>& profiles_by_id,
          std::unordered_map<
            std::string, std::unique_ptr<Component>>& components,
          fragility_map&& frags) const;
      void read_mover_component(
          const toml::table& tt,
          const std::string& id,
          const std::string& outflow,
          std::unordered_map<
            std::string, std::unique_ptr<Component>>& components,
          fragility_map&& frags) const;
      [[nodiscard]] double read_number(const toml::value& v) const;
      [[nodiscard]] double read_number(const std::string& v) const;
      [[nodiscard]] std::optional<double>
        read_optional_number(const toml::table& tt, const std::string& key);
      [[nodiscard]] double
        read_number_at(const toml::table& tt, const std::string& key);
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
      [[nodiscard]] erin::distribution::DistType
        read_dist_type(
            const toml::table& tt,
            const std::string& dist_id) const;
      [[nodiscard]] StreamIDs read_stream_ids(
          const toml::table& tt,
          const std::string& comp_id) const;
      [[nodiscard]] fragility_map read_component_fragilities(
          const toml::table& tt,
          const std::string& comp_id,
          const std::unordered_map<
            std::string,
            erin::fragility::FragilityCurve>& fragility_curves,
          const std::unordered_map<
            std::string,
            erin::fragility::FragilityMode>& fragility_modes) const;
      void check_top_level_entries() const;
  };

  std::string parse_component_id(const std::string& tag);

  erin::port::Type parse_component_port(const std::string& tag);

  int parse_component_port_num(const std::string& tag);
}

#endif // ERIN_INPUT_READER_H
