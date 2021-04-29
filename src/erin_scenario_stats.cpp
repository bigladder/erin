/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/scenario_stats.h"
#include <algorithm>

namespace ERIN
{
  ScenarioStats&
  ScenarioStats::operator+=(const ScenarioStats& other)
  {
    uptime += other.uptime;
    downtime += other.downtime;
    max_downtime = std::max(max_downtime, other.max_downtime);
    load_not_served += other.load_not_served;
    total_energy += other.total_energy;
    return *this;
  }

  ScenarioStats
  operator+(const ScenarioStats& a, const ScenarioStats& b)
  {
    return ScenarioStats{
      a.uptime + b.uptime,
      a.downtime + b.downtime,
      std::max(a.max_downtime, b.max_downtime),
      a.load_not_served + b.load_not_served,
      a.total_energy + b.total_energy};
  }

  bool
  operator==(const ScenarioStats& a, const ScenarioStats& b)
  {
    if ((a.uptime == b.uptime)
        && (a.downtime == b.downtime)
        && (a.max_downtime == b.max_downtime)) {
      const auto diff_lns = std::abs(a.load_not_served - b.load_not_served);
      const auto diff_te = std::abs(a.total_energy - b.total_energy);
      const auto& fvt = flow_value_tolerance;
      return (diff_lns < fvt) && (diff_te < fvt);
    }
    return false;
  }

  bool
  operator!=(const ScenarioStats& a, const ScenarioStats& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const ScenarioStats& s)
  {
    os << "ScenarioStats(uptime=" << s.uptime << ", "
       << "downtime=" << s.downtime << ", "
       << "max_downtime=" << s.max_downtime << ", "
       << "load_not_served=" << s.load_not_served << ", "
       << "total_energy=" << s.total_energy << ")";
    return os;
  }
}