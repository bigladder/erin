/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/erin.h"
#include "erin/network.h"
#include <iostream>
#include <string>

void
doit()
{
  namespace ep = erin::port;
  std::string scenario_id{"blue_sky"};
  std::string stream_id{"electricity"};
  std::string source_id{"electric_utility"};
  std::string load_id{"cluster_01_electric"};
  std::string net_id{"normal_operations"};
  const ERIN::RealTimeType N{8760}; // hours
  std::vector<ERIN::LoadItem> loads;
  for (ERIN::RealTimeType i{0}; i < N; ++i) {
    loads.emplace_back(ERIN::LoadItem{i, 1.0});
  }
  std::unordered_map<std::string, std::vector<ERIN::LoadItem>>
    loads_by_scenario{{scenario_id, loads}};
  ERIN::SimulationInfo si{ERIN::TimeUnits::Hours, N};
  std::unordered_map<std::string, std::string> streams{
    std::make_pair(stream_id, stream_id)};
  std::unordered_map<
    std::string,
    std::unique_ptr<ERIN::Component>> components;
  components.insert(
      std::make_pair(
        source_id,
        std::make_unique<ERIN::SourceComponent>(
          source_id,
          stream_id)));
  components.insert(
      std::make_pair(
        load_id,
        std::make_unique<ERIN::LoadComponent>(
          load_id,
          stream_id,
          loads_by_scenario)));
  std::unordered_map<
    std::string, std::vector<::erin::network::Connection>> networks{
      { net_id,
        {
          { { source_id, ep::Type::Outflow, 0},
            { load_id, ep::Type::Inflow, 0},
            stream_id}}}};
  std::unordered_map<std::string, ERIN::Scenario> scenarios{};
  scenarios.emplace(
      std::make_pair(
        scenario_id, 
        ERIN::Scenario{
          scenario_id,
          net_id,
          ERIN::time_to_seconds(N, ERIN::TimeUnits::Hours),
          -1,
          0,
          {},
          false}));
  std::unordered_map<std::string, std::vector<ERIN::TimeState>>
    reliability_schedule{};
  std::unordered_map<std::string, std::vector<ERIN::RealTimeType>>
    scenario_schedules{{scenario_id, {0}}};
  ERIN::Main m{
    si, components, networks, scenarios,
    scenario_schedules, reliability_schedule};
  auto out = m.run(scenario_id);
  if (out.get_is_good()) {
    std::cout << "success!\n";
  }
  else {
    std::cout << "failure!\n";
  }
}

int
main()
{
  try {
    doit();
  }
  catch (const std::exception& e) {
    std::cerr << "Unhandled exception\n"
              << "Exception: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
