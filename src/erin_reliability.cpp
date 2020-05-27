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
      const auto& fm_dist_type_id = distribution_id_to_distribution_type.at(failure_mode_distribution_id);
      const auto& repair_dist_type_id = distribution_id_to_distribution_type.at(repair_mode_distribution_id);
      auto it_fm_type = failure_mode_types_to_component_ids.find(fm_dist_type_id);
      if (it_fm_type == failure_mode_types_to_component_ids.end()) {
        failure_mode_types_to_component_ids[fm_dist_type_id] = {component_id};
        failure_mode_types_to_dist_ids[fm_dist_type_id] = {failure_mode_distribution_id};
      }
      else {
        failure_mode_types_to_component_ids[fm_dist_type_id].emplace_back(component_id);
        failure_mode_types_to_dist_ids[fm_dist_type_id].emplace_back(failure_mode_distribution_id);
      }
      auto it_rm_type = repair_types_to_component_ids.find(repair_dist_type_id);
      if (it_rm_type == repair_types_to_component_ids.end()) {
        repair_types_to_component_ids[repair_dist_type_id] = {component_id};
        repair_types_to_dist_ids[repair_dist_type_id] = {repair_mode_distribution_id};
      }
      else {
        repair_types_to_component_ids[repair_dist_type_id].emplace_back(component_id);
        repair_types_to_dist_ids[repair_dist_type_id].emplace_back(repair_mode_distribution_id);
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
}
