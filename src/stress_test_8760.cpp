/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */
#include <iostream>
#include "erin/erin.h"

int
main() {
  std::string scenario_id{"blue_sky"};
  std::string stream_id{"electricity"};
  std::string source_id{"electric_utility"};
  std::string load_id{"cluster_01_electric"};
  std::string net_id{"normal_operations"};
  const int N{8760};
  std::vector<::ERIN::LoadItem> loads;
  for (int i{0}; i < N; ++i) {
    loads.emplace_back(::ERIN::LoadItem{i, 1.0});
  }
  loads.emplace_back(::ERIN::LoadItem{N});
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>>
    loads_by_scenario{{scenario_id, loads}};
  ::ERIN::StreamInfo si{"kW", "kJ", 1.0};
  std::unordered_map<std::string, ::ERIN::StreamType> streams{
    std::make_pair(
        stream_id,
        ::ERIN::StreamType(
          std::string{"electricity_medium_voltage"},
          si.get_rate_unit(),
          si.get_quantity_unit(),
          si.get_seconds_per_time_unit(),
          {}, {}))};
  std::unordered_map<std::string, std::shared_ptr<::ERIN::Component>> components{
    std::make_pair(
        source_id,
        std::make_shared<::ERIN::SourceComponent>(
          source_id,
          streams[stream_id])),
    std::make_pair(
        load_id,
        std::make_shared<::ERIN::LoadComponent>(
          load_id,
          streams[stream_id],
          loads_by_scenario))};
  std::unordered_map<
    std::string, std::unordered_map<std::string, std::vector<std::string>>>
    networks{{net_id, {{source_id, {load_id}}}}};
  std::unordered_map<std::string, ::ERIN::Scenario> scenarios{};
  scenarios.emplace(
      std::make_pair(
        scenario_id, 
        ::ERIN::Scenario(
          scenario_id,
          net_id,
          N,
          -1,
          nullptr,
          {})));
  auto m = ::ERIN::Main(si, streams, components, networks, scenarios);
  auto out = m.run(scenario_id);
  if (out.get_is_good()) {
    std::cout << "success!\n";
  }
  else {
    std::cout << "failure!\n";
  }
  return 0;
}
