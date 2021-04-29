/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_SCENARIO_STATS_H
#define ERIN_SCENARIO_STATS_H
#include "erin/type.h"
#include <iostream>

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
}

#endif // ERIN_SCENARIO_STATS_H