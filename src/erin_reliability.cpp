/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/reliability.h"
#include <iostream>
#include <utility>
//#include <sstream>

namespace ERIN
{
  bool
  operator==(const TimeState& a, const TimeState& b)
  {
    return (a.time == b.time)
      && (a.state == b.state);
  }

  bool
  operator!=(const TimeState& a, const TimeState& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const TimeState& ts)
  {
    return os << "TimeState(time="
              << ts.time << ", "
              << (ts.state ? "true" : "false")
              << ")";
  }

  ReliabilityCoordinator::ReliabilityCoordinator():
    fixed_cdf{},
    fms{},
    next_fixed_cdf_id{0},
    next_fm_id{0},
    components{}
  {
  }

  size_type
  ReliabilityCoordinator::add_fixed_cdf(
      std::int64_t value,
      TimeUnits units)
  {
    auto id{next_fixed_cdf_id};
    ++next_fixed_cdf_id;
    fixed_cdf.value.emplace_back(value);
    fixed_cdf.time_multiplier.emplace_back(time_to_seconds(1.0, units));
    return id;
  }

  size_type
  ReliabilityCoordinator::add_failure_mode(
      const size_type& comp_id,
      const std::string& name,
      const size_type& failure_cdf_id,
      const CdfType& failure_cdf_type,
      const size_type& repair_cdf_id,
      const CdfType& repair_cdf_type)
  {
    auto id{next_fm_id};
    fms.component_id.emplace_back(comp_id);
    fms.name.emplace_back(name);
    fms.failure_cdf.emplace_back(failure_cdf_id);
    fms.failure_cdf_type.emplace_back(failure_cdf_type);
    fms.repair_cdf.emplace_back(repair_cdf_id);
    fms.repair_cdf_type.emplace_back(repair_cdf_type);
    ++next_fm_id;
    components.emplace(comp_id);
    return id;
  }

  std::unordered_map<size_type, std::vector<TimeState>>
  ReliabilityCoordinator::calc_reliability_schedule(
      std::int64_t final_time) const
  {
    const auto num_components{components.size()};
    const auto num_fms{fms.component_id.size()};
    std::unordered_map<size_type, std::int64_t> comp_id_to_time{};
    std::unordered_map<size_type, std::int64_t> comp_id_to_dt{};
    std::unordered_map<size_type, std::vector<TimeState>> comp_id_to_reliability_schedule{};
    for (const auto& comp_id : components) {
      comp_id_to_time[comp_id] = 0;
      comp_id_to_dt[comp_id] = -1;
      comp_id_to_reliability_schedule[comp_id] = std::vector<TimeState>{TimeState{0, true}};
    }
    size_type count{0};
    while (true) {
      for (size_type i{0}; i < num_fms; ++i) {
        const auto& comp_id = fms.component_id.at(i);
        const auto& failure_cdf_id = fms.failure_cdf.at(i);
        const auto& failure_cdf_type = fms.failure_cdf_type.at(i);
        switch (failure_cdf_type) {
          case CdfType::Fixed:
            {
              auto v = fixed_cdf.value.at(failure_cdf_id);
              auto m = fixed_cdf.time_multiplier.at(failure_cdf_id);
              auto dt = v * m;
              auto& dt_f = comp_id_to_dt[comp_id]; 
              if ((dt_f == -1) || (dt < dt_f)) {
                dt_f = dt;
              }
            }
          default:
            {
              std::cout << "shouldn't be here...\n";
            }
        }
      }
      count = 0;
      for (const auto& comp_id : components) {
        auto& t = comp_id_to_time[comp_id];
        if (t > final_time) {
          ++count;
          continue;
        }
        auto& dt = comp_id_to_dt[comp_id];
        t = t + dt;
        dt = -1;
        comp_id_to_reliability_schedule[comp_id].emplace_back(TimeState{t, false});
        if (t > final_time) {
          ++count;
          continue;
        }
      }
      if (count == num_components) {
        break;
      }
      for (size_type i{0}; i < fms.component_id.size(); ++i) {
        const auto& comp_id = fms.component_id.at(i);
        const auto& repair_cdf_id = fms.repair_cdf.at(i);
        const auto& repair_cdf_type = fms.repair_cdf_type.at(i);
        switch (repair_cdf_type) {
          case CdfType::Fixed:
            {
              auto v = fixed_cdf.value.at(repair_cdf_id);
              auto m = fixed_cdf.time_multiplier.at(repair_cdf_id);
              auto dt = v * m;
              auto& dt_r = comp_id_to_dt[comp_id]; 
              if ((dt_r == -1) || (dt < dt_r)) {
                dt_r = dt;
              }
            }
          default:
            {
              std::cout << "shouldn't be here...\n";
            }
        }
      }
      count = 0;
      for (const auto& comp_id : components) {
        auto& t = comp_id_to_time[comp_id];
        if (t > final_time) {
          ++count;
          continue;
        }
        auto& dt = comp_id_to_dt[comp_id];
        t = t + dt;
        dt = -1;
        comp_id_to_reliability_schedule[comp_id].emplace_back(TimeState{t, true});
        if (t >= final_time) {
          ++count;
          continue;
        }
      }
      if (count == num_components) {
        break;
      }
    }
    return comp_id_to_reliability_schedule;
  }
}
