/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

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
    return os << "TimeState("
              << "time=" << ts.time << ", "
              << "state=" << ts.state << ")";
  }

  ReliabilityCoordinator::ReliabilityCoordinator():
    fms{},
    fm_comp_links{},
    comp_meta{}
  {
  }

  size_type
  ReliabilityCoordinator::add_failure_mode(
      const std::string& tag,
      const size_type& failure_cdf_id,
      const size_type& repair_cdf_id)
  {
    auto id{fms.tag.size()};
    fms.tag.emplace_back(tag);
    fms.failure_cdf.emplace_back(failure_cdf_id);
    fms.repair_cdf.emplace_back(repair_cdf_id);
    return id;
  }

  void
  ReliabilityCoordinator::link_component_with_failure_mode(
      const size_type& component_id,
      const size_type& failure_mode_id)
  {
    fm_comp_links.component_id.emplace_back(component_id);
    fm_comp_links.failure_mode_id.emplace_back(failure_mode_id);
    if (component_id >= comp_meta.tag.size()) {
      std::ostringstream oss{};
      oss << "Attempt to add unregistered component_id `"
          << component_id << "`";
      throw std::invalid_argument(oss.str());
    }
  }

  size_type
  ReliabilityCoordinator::register_component(const std::string& tag)
  {
    auto id{comp_meta.tag.size()};
    comp_meta.tag.emplace_back(tag);
    return id;
  }

  void
  ReliabilityCoordinator::calc_next_events(
      const std::function<double()>& rand_fn,
      const erin::distribution::CumulativeDistributionSystem& cds,
      std::unordered_map<size_type, RealTimeType>& comp_id_to_dt,
      bool is_failure) const
  {
    const auto num_fm_comp_links{fm_comp_links.failure_mode_id.size()};
    for (size_type i{0}; i < num_fm_comp_links; ++i) {
      const auto& comp_id = fm_comp_links.component_id.at(i);
      const auto& fm_id = fm_comp_links.failure_mode_id.at(i);
      size_type cdf_id{0};
      if (is_failure) {
        cdf_id = fms.failure_cdf.at(fm_id);
      }
      else { // is_repair
        cdf_id = fms.repair_cdf.at(fm_id);
      }
      auto dt = cds.next_time_advance(cdf_id, rand_fn());
      auto& dt_fm = comp_id_to_dt[comp_id]; 
      if ((dt_fm == -1) || (dt < dt_fm)) {
        dt_fm = dt;
      }
    }
  }

  size_type
  ReliabilityCoordinator::update_schedule(
      std::unordered_map<size_type, RealTimeType>& comp_id_to_time,
      std::unordered_map<size_type, RealTimeType>& comp_id_to_dt,
      std::unordered_map<size_type, std::vector<TimeState>>&
        comp_id_to_reliability_schedule,
      RealTimeType final_time,
      bool next_state) const
  {
    size_type num_past_final_time{0};
    auto num_components{comp_meta.tag.size()};
    for (size_type comp_id{0}; comp_id < num_components; ++comp_id) {
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
      const std::function<double()>& rand_fn,
      const erin::distribution::CumulativeDistributionSystem& cds,
      RealTimeType final_time) const
  {
    const auto num_components{comp_meta.tag.size()};
    std::unordered_map<size_type, RealTimeType> comp_id_to_time{};
    std::unordered_map<size_type, RealTimeType> comp_id_to_dt{};
    std::unordered_map<size_type, std::vector<TimeState>>
      comp_id_to_reliability_schedule{};
    for (size_type comp_id{0}; comp_id < num_components; ++comp_id) {
      comp_id_to_time[comp_id] = 0;
      comp_id_to_dt[comp_id] = -1;
      comp_id_to_reliability_schedule[comp_id] =
        std::vector<TimeState>{TimeState{0, true}};
    }
    size_type count{0};
    while (true) {
      calc_next_events(rand_fn, cds, comp_id_to_dt, true);
      count = update_schedule(
          comp_id_to_time,
          comp_id_to_dt,
          comp_id_to_reliability_schedule,
          final_time,
          false);
      if (count == num_components) {
        break;
      }
      calc_next_events(rand_fn, cds, comp_id_to_dt, false);
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

  std::unordered_map<std::string, std::vector<TimeState>>
  ReliabilityCoordinator::calc_reliability_schedule_by_component_tag(
      const std::function<double()>& rand_fn,
      const erin::distribution::CumulativeDistributionSystem& cds,
      RealTimeType final_time) const
  {
    auto sch = calc_reliability_schedule(rand_fn, cds, final_time);
    std::unordered_map<std::string, std::vector<TimeState>> out{};
    for (size_type comp_id{0}; comp_id < comp_meta.tag.size(); ++comp_id) {
      const auto& tag = comp_meta.tag[comp_id];
      out[tag] = std::move(sch[comp_id]);
    }
    return out;
  }
}
