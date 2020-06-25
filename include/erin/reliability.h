/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_RELIABILITY_H
#define ERIN_RELIABILITY_H
#include "erin/type.h"
#include "erin/distribution.h"
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace ERIN
{
  // Data Structs
  struct TimeState
  {
    RealTimeType time{0};
    bool state{true};
  };

  bool operator==(const TimeState& a, const TimeState& b);
  bool operator!=(const TimeState& a, const TimeState& b);
  std::ostream& operator<<(std::ostream& os, const TimeState& ts);

  struct FailureMode
  {
    std::vector<std::string> tag{};
    std::vector<size_type> failure_cdf{};
    std::vector<size_type> repair_cdf{};
  };

  struct FailureMode_Component_Link {
    std::vector<size_type> failure_mode_id{};
    std::vector<size_type> component_id{};
  };

  struct Component_meta
  {
    std::vector<std::string> tag{};
  };

  // Main Class to do Reliability Schedule Creation
  class ReliabilityCoordinator
  {
    public:
      ReliabilityCoordinator();

      size_type add_fixed_cdf(
          const std::string& tag,
          RealTimeType value_in_seconds);

      size_type add_failure_mode(
          const std::string& tag,
          const size_type& failure_cdf_id,
          const size_type& repair_cdf_id
          );

      void link_component_with_failure_mode(
          const size_type& comp_id,
          const size_type& fm_id);

      size_type register_component(const std::string& tag);

      [[nodiscard]] size_type lookup_cdf_by_tag(const std::string& tag) const;

      std::unordered_map<size_type, std::vector<TimeState>>
      calc_reliability_schedule(RealTimeType final_time);

      std::unordered_map<std::string, std::vector<TimeState>>
      calc_reliability_schedule_by_component_tag(RealTimeType final_time);

    private:
      erin::distribution::CumulativeDistributionSystem cds;
      FailureMode fms;
      FailureMode_Component_Link fm_comp_links;
      Component_meta comp_meta;

      void calc_next_events(
          std::unordered_map<size_type, RealTimeType>& comp_id_to_dt,
          bool is_failure);

      size_type
      update_schedule(
          std::unordered_map<size_type, RealTimeType>& comp_id_to_time,
          std::unordered_map<size_type, RealTimeType>& comp_id_to_dt,
          std::unordered_map<size_type, std::vector<TimeState>>&
            comp_id_to_reliability_schedule,
          RealTimeType final_time,
          bool next_state) const;
  };

  template <class T>
  std::unordered_map<T, std::vector<TimeState>>
  clip_schedule_to(
      const std::unordered_map<T, std::vector<TimeState>>& schedule,
      RealTimeType start_time,
      RealTimeType end_time)
  {
    std::unordered_map<T, std::vector<TimeState>> new_sch{};
    for (const auto& item : schedule) {
      std::vector<TimeState> tss{};
      bool state{true};
      for (const auto& ts : item.second) {
        if (ts.time < start_time) {
          state = ts.state;
          continue;
        }
        else if (ts.time == start_time) {
            tss.emplace_back(ts);
        }
        else if ((ts.time > start_time) && (ts.time <= end_time)) {
          if (tss.size() == 0) {
            tss.emplace_back(TimeState{0, state});
          }
          tss.emplace_back(TimeState{ts.time - start_time, ts.state});
        }
        else if (ts.time > end_time) {
          break;
        }
      }
      new_sch[item.first] = std::move(tss);
    }
    return new_sch;
  }

  template <class T>
  std::unordered_map<T, std::vector<TimeState>>
  rezero_times(
      const std::unordered_map<T, std::vector<TimeState>>& schedule,
      RealTimeType start_time)
  {
    std::unordered_map<T, std::vector<TimeState>> new_sch{};
    for (const auto& item : schedule) {
      std::vector<TimeState> tss{};
      for (const auto& ts : item.second) {
        tss.emplace_back(TimeState{ts.time - start_time, ts.state});
      }
      new_sch[item.first] = std::move(tss);
    }
    return new_sch;
  }
}

#endif // ERIN_RELIABILITY_H
