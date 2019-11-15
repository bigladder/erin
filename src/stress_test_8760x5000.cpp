/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include <iostream>
#include <string>
#include "erin/erin.h"

int
main() {
  std::string scenario_id{"blue_sky"};
  std::string stream_id{"electricity"};
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
  ::ERIN::StreamType elec{
    std::string{"electricity_medium_voltage"},
      si.get_rate_unit(),
      si.get_quantity_unit(),
      si.get_seconds_per_time_unit(),
      {}, {}};
  std::unordered_map<std::string, ::ERIN::StreamType> streams{
    std::make_pair(stream_id, elec)};
  std::unordered_map<std::string,
    std::shared_ptr<::ERIN::Component>> components;
  std::unordered_map<std::string, std::vector<std::string>> nw;
  std::unordered_map<std::string, ::ERIN::Scenario> scenarios{
    {scenario_id, ::ERIN::Scenario{scenario_id, net_id, N}}};
  const int M{5000};
  std::string src_prefix{"source_"};
  std::string load_prefix{"load_"};
  for (int j{0}; j < M; ++j) {
    std::string source_id =  src_prefix + std::to_string(j);
    std::string load_id = load_prefix + std::to_string(j);
    components[source_id] = std::make_shared<::ERIN::SourceComponent>(
        source_id,
        elec);
    components[load_id] = std::make_shared<::ERIN::LoadComponent>(
            load_id,
            elec,
            loads_by_scenario);
    nw[source_id] = std::vector<std::string>{load_id};
  }
  std::unordered_map<std::string,
    std::unordered_map<std::string, std::vector<std::string>>>
      networks{{net_id, nw}};
  std::cout << "construction completed!\n";
  ::ERIN::Main m{si, streams, components, networks, scenarios};
  std::cout << "running!\n";
  auto out = m.run(scenario_id);
  std::cout << "done!\n";
  if (out.get_is_good()) {
    std::cout << "success!\n";
  }
  else {
    std::cout << "failure!\n";
  }
  return 0;
}
