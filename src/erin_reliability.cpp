/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/reliability.h"
#include <iostream>
#include <utility>
#include <stdexcept>

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

  void
  ReliabilityCoordinator::calc_next_events(
      std::unordered_map<size_type, std::int64_t>& comp_id_to_dt,
      bool is_failure
      ) const
  {
    const auto num_fms{fms.component_id.size()};
    for (size_type i{0}; i < num_fms; ++i) {
      const auto& comp_id = fms.component_id.at(i);
      size_type cdf_id{0};
      if (is_failure) {
        cdf_id = fms.failure_cdf.at(i);
      }
      else {
        cdf_id = fms.repair_cdf.at(i);
      }
      auto cdf_type = CdfType::Fixed;
      if (is_failure) {
        cdf_type = fms.failure_cdf_type.at(i);
      }
      else {
        cdf_type = fms.repair_cdf_type.at(i);
      }
      switch (cdf_type) {
        case CdfType::Fixed:
          {
            auto v = fixed_cdf.value.at(cdf_id);
            auto m = fixed_cdf.time_multiplier.at(cdf_id);
            auto dt = v * m;
            auto& dt_fm = comp_id_to_dt[comp_id]; 
            if ((dt_fm == -1) || (dt < dt_fm)) {
              dt_fm = dt;
            }
          }
        default:
          {
            throw std::runtime_error("unhandled Cumulative Density Function");
          }
      }
    }
  }

  size_type
  ReliabilityCoordinator::update_schedule(
      std::unordered_map<size_type, std::int64_t>& comp_id_to_time,
      std::unordered_map<size_type, std::int64_t>& comp_id_to_dt,
      std::unordered_map<size_type, std::vector<TimeState>>&
        comp_id_to_reliability_schedule,
      std::int64_t final_time,
      bool next_state
      ) const
  {
    size_type num_past_final_time{0};
    for (const auto& comp_id : components) {
      auto& t = comp_id_to_time[comp_id];
      if (t > final_time) {
        ++num_past_final_time;
        continue;
      }
      auto& dt = comp_id_to_dt[comp_id];
      t = t + dt;
      dt = -1;
      comp_id_to_reliability_schedule[comp_id].emplace_back(
          TimeState{t, next_state});
      if (t > final_time) {
        ++num_past_final_time;
        continue;
      }
    }
    return num_past_final_time;
  }

  std::unordered_map<size_type, std::vector<TimeState>>
  ReliabilityCoordinator::calc_reliability_schedule(
      std::int64_t final_time) const
  {
    const auto num_components{components.size()};
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
      calc_next_events(comp_id_to_dt, true);
      count = update_schedule(
          comp_id_to_time,
          comp_id_to_dt,
          comp_id_to_reliability_schedule,
          final_time,
          false);
      if (count == num_components) {
        break;
      }
      calc_next_events(comp_id_to_dt, false);
      count = update_schedule(
          comp_id_to_time,
          comp_id_to_dt,
          comp_id_to_reliability_schedule,
          final_time,
          true);
      if (count == num_components) {
        break;
      }
    }
    return comp_id_to_reliability_schedule;
  }
}
