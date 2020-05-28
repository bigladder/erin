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

  /*
  std::unordered_map<size_type, std::vector<TimeState>>
  calc_reliability_schedule(
      const std::vector<size_type>& component_id_to_failure_mode_distribution_id,
      const std::vector<size_type>& component_id_to_repair_mode_distribution_id,
      const std::vector<size_type>& distribution_id_to_distribution_type,
      const std::vector<size_type>& distribution_id_to_distribution_type_id,
      const std::vector<std::int64_t>& fixed_distribution_attr_value
      )
  {
    const auto num_components = component_id_to_failure_mode_distribution_id.size();
    std::unordered_map<size_type, std::vector<size_type>> failure_mode_types_to_component_ids{};
    std::unordered_map<size_type, std::vector<size_type>> failure_mode_types_to_dist_ids{};
    std::unordered_map<size_type, std::vector<size_type>> repair_types_to_component_ids{};
    std::unordered_map<size_type, std::vector<size_type>> repair_types_to_dist_ids{};
    std::vector<std::int64_t> comp_id_to_time(num_components, 0);
    std::unordered_map<size_type, std::vector<TimeState>> comp_id_to_reliability_schedule{};
    for (size_type component_id{0}; component_id < num_components; ++component_id) {
      comp_id_to_reliability_schedule.emplace(
          std::pair<size_type, std::vector<TimeState>>(
            component_id,
            std::vector<TimeState>{TimeState{0, true}}));
      const auto& failure_mode_distribution_id = component_id_to_failure_mode_distribution_id.at(component_id);
      const auto& repair_mode_distribution_id = component_id_to_repair_mode_distribution_id.at(component_id);
      const auto& fm_dist_type_id = distribution_id_to_distribution_type_id.at(failure_mode_distribution_id);
      const auto& repair_dist_type_id = distribution_id_to_distribution_type_id.at(repair_mode_distribution_id);
      const auto& fm_dist_type = distribution_id_to_distribution_type.at(failure_mode_distribution_id);
      const auto& repair_dist_type = distribution_id_to_distribution_type.at(repair_mode_distribution_id);
      auto it_fm_type = failure_mode_types_to_component_ids.find(fm_dist_type);
      if (it_fm_type == failure_mode_types_to_component_ids.end()) {
        failure_mode_types_to_component_ids[fm_dist_type] = {component_id};
        failure_mode_types_to_dist_ids[fm_dist_type] = {failure_mode_distribution_id};
      }
      else {
        failure_mode_types_to_component_ids[fm_dist_type].emplace_back(component_id);
        failure_mode_types_to_dist_ids[fm_dist_type].emplace_back(failure_mode_distribution_id);
      }
      auto it_rm_type = repair_types_to_component_ids.find(repair_dist_type);
      if (it_rm_type == repair_types_to_component_ids.end()) {
        repair_types_to_component_ids[repair_dist_type] = {component_id};
        repair_types_to_dist_ids[repair_dist_type] = {repair_mode_distribution_id};
      }
      else {
        repair_types_to_component_ids[repair_dist_type].emplace_back(component_id);
        repair_types_to_dist_ids[repair_dist_type].emplace_back(repair_mode_distribution_id);
      }
    }
    const std::int64_t final_time{10};
    size_type count{0};
    while (true) {
      count = 0;
      for (const auto& fm_comp_pair : failure_mode_types_to_component_ids) {
        const auto& fm_type = fm_comp_pair.first;
        for (size_type i{0}; i < fm_comp_pair.second.size(); ++i) {
          const auto& comp_id = fm_comp_pair.second.at(i);
          const auto& dist_id = failure_mode_types_to_dist_ids.at(fm_type).at(i);
          const auto& time = comp_id_to_time.at(comp_id);
          if (time >= final_time) {
            ++count;
            continue;
          }
          std::int64_t dt{0};
          switch (fm_type) {
            case DISTRIBUTION_TYPE_FIXED:
              {
                dt = fixed_distribution_attr_value.at(dist_id);
                auto next_time = time + dt;
                comp_id_to_reliability_schedule[comp_id].emplace_back(
                    TimeState{next_time, false});
                comp_id_to_time[comp_id] = next_time;
              }
            default:
              std::cout << "shouldn't get here...\n";
          }
        }
      }
      if (count == num_components) {
        break;
      }
      count = 0;
      for (const auto& rm_comp_pair : repair_types_to_component_ids) {
        const auto& rm_type = rm_comp_pair.first;
        for (size_type i{0}; i < rm_comp_pair.second.size(); ++i) {
          const auto& comp_id = rm_comp_pair.second.at(i);
          const auto& dist_id = repair_types_to_dist_ids.at(rm_type).at(i);
          const auto& time = comp_id_to_time.at(comp_id);
          if (time >= final_time) {
            ++count;
            continue;
          }
          std::int64_t dt{0};
          switch (rm_type) {
            case DISTRIBUTION_TYPE_FIXED:
              {
                dt = fixed_distribution_attr_value.at(dist_id);
                auto next_time = time + dt;
                comp_id_to_reliability_schedule[comp_id].emplace_back(
                    TimeState{next_time, true});
                comp_id_to_time[comp_id] = next_time;
              }
            default:
              std::cout << "shouldn't get here...\n";
          }
        }
      }
    }
    return comp_id_to_reliability_schedule;
  }
  */

  ReliabilityCoordinator::ReliabilityCoordinator():
    fixed_cdf{},
    fms{},
    next_fixed_cdf_id{0},
    next_fm_id{0},
    components{}
  {
  }

  size_type
  ReliabilityCoordinator::add_fixed_cdf(std::int64_t value)
  {
    auto id{next_fixed_cdf_id};
    ++next_fixed_cdf_id;
    fixed_cdf.value.emplace_back(value);
    fixed_cdf.time_multiplier.emplace_back(3600);
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
      for (size_type i{0}; i < fms.component_id.size(); ++i) {
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
        if (t > final_time) {
          ++count;
          continue;
        }
        comp_id_to_reliability_schedule[comp_id].emplace_back(TimeState{t, false});
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
        const auto& t = comp_id_to_time[comp_id];
        const auto& dt = comp_id_to_dt[comp_id];
        auto next_t = t + dt;
        comp_id_to_time[comp_id] = next_t;
        if (next_t >= final_time) {
          ++count;
        }
        comp_id_to_reliability_schedule[comp_id].emplace_back(TimeState{next_t, true});
      }
      if (count == num_components) {
        break;
      }
    }
    return comp_id_to_reliability_schedule;
  }
}
