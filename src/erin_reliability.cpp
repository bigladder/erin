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

  std::string
  cdf_type_to_tag(CdfType cdf_type)
  {
    switch (cdf_type) {
      case CdfType::Fixed:
        return std::string{"fixed"};
    }
    std::ostringstream oss{};
    oss << "unhandled cdf_type `" << static_cast<int>(cdf_type) << "`";
    throw std::invalid_argument(oss.str());
  }

  CdfType
  tag_to_cdf_type(const std::string& tag)
  {
    if (tag == "fixed") {
      return CdfType::Fixed;
    }
    std::ostringstream oss{};
    oss << "unhandled tag `" << tag << "` in tag_to_cdf_type";
    throw std::invalid_argument(oss.str());
  }

  ReliabilityCoordinator::ReliabilityCoordinator():
    fixed_cdf{},
    cdfs{},
    fms{},
    fm_comp_links{},
    components{},
    next_fixed_cdf_id{0},
    next_cdf_id{0},
    next_fm_id{0}
  {
  }

  size_type
  ReliabilityCoordinator::add_fixed_cdf(std::int64_t value_in_seconds)
  {
    auto id{next_fixed_cdf_id};
    ++next_fixed_cdf_id;
    fixed_cdf.value.emplace_back(value_in_seconds);
    cdfs.subtype_id.emplace_back(id);
    cdfs.cdf_type.emplace_back(CdfType::Fixed);
    return id;
  }

  size_type
  ReliabilityCoordinator::add_failure_mode(
      const std::string& name,
      const size_type& failure_cdf_id,
      const size_type& repair_cdf_id)
  {
    auto id{next_fm_id};
    fms.name.emplace_back(name);
    fms.failure_cdf.emplace_back(failure_cdf_id);
    fms.repair_cdf.emplace_back(repair_cdf_id);
    ++next_fm_id;
    return id;
  }

  void
  ReliabilityCoordinator::link_component_with_failure_mode(
      const size_type& component_id,
      const size_type& failure_mode_id)
  {
    fm_comp_links.component_id.emplace_back(component_id);
    fm_comp_links.failure_mode_id.emplace_back(failure_mode_id);
    components.emplace(component_id);
  }

  void
  ReliabilityCoordinator::calc_next_events(
      std::unordered_map<size_type, std::int64_t>& comp_id_to_dt,
      bool is_failure
      ) const
  {
    const auto num_fm_comp_links{fm_comp_links.failure_mode_id.size()};
    for (size_type i{0}; i < num_fm_comp_links; ++i) {
      const auto& comp_id = fm_comp_links.component_id.at(i);
      const auto& fm_id = fm_comp_links.failure_mode_id.at(i);
      size_type cdf_id{0};
      CdfType cdf_type{};
      size_type cdf_subtype_id{0};
      if (is_failure) {
        cdf_id = fms.failure_cdf.at(fm_id);
        cdf_type = cdfs.cdf_type.at(cdf_id);
        cdf_subtype_id = cdfs.subtype_id.at(cdf_id);
      }
      else { // is_repair
        cdf_id = fms.repair_cdf.at(i);
        cdf_type = cdfs.cdf_type.at(cdf_id);
        cdf_subtype_id = cdfs.subtype_id.at(cdf_id);
      }
      switch (cdf_type) {
        case CdfType::Fixed:
          {
            auto dt = fixed_cdf.value.at(cdf_id);
            auto& dt_fm = comp_id_to_dt[comp_id]; 
            if ((dt_fm == -1) || (dt < dt_fm)) {
              dt_fm = dt;
            }
            break;
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
      FlowValueType next_state
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
          0.0);
      if (count == num_components) {
        break;
      }
      calc_next_events(comp_id_to_dt, false);
      count = update_schedule(
          comp_id_to_time,
          comp_id_to_dt,
          comp_id_to_reliability_schedule,
          final_time,
          1.0);
      if (count == num_components) {
        break;
      }
    }
    return comp_id_to_reliability_schedule;
  }
}
