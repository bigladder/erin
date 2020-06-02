/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_RELIABILITY_H
#define ERIN_RELIABILITY_H
#include "erin/type.h"
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace ERIN
{
  using size_type = std::vector<std::int64_t>::size_type;
  constexpr size_type DISTRIBUTION_TYPE_FIXED{0};

  struct TimeState
  {
    std::int64_t time{0};
    FlowValueType state{1.0};
  };

  enum class CdfType
  {
    Fixed = 0//,
    //Normal,
    //Weibull
  };

  // move to hidden interface
  struct FailureMode
  {
    std::vector<size_type> component_id{};
    std::vector<std::string> name{};
    std::vector<size_type> failure_cdf{};
    std::vector<CdfType> failure_cdf_type{};
    std::vector<size_type> repair_cdf{};
    std::vector<CdfType> repair_cdf_type{};
  };

  // move to hidden interface
  struct Fixed_CDF
  {
    std::vector<std::int64_t> value{};
    std::vector<std::int64_t> time_multiplier{}; // to convert from given units to seconds
  };

  bool operator==(const TimeState& a, const TimeState& b);
  bool operator!=(const TimeState& a, const TimeState& b);
  std::ostream& operator<<(std::ostream& os, const TimeState& ts);

  class ReliabilityCoordinator
  {
    public:
      ReliabilityCoordinator();

      size_type add_fixed_cdf(
          std::int64_t value,
          TimeUnits units = TimeUnits::Hours);

      size_type add_failure_mode(
          const size_type& comp_id,
          const std::string& name,
          const size_type& failure_cdf_id,
          const CdfType& failure_cdf_type,
          const size_type& repair_cdf_id,
          const CdfType& repair_cdf_type);

      std::unordered_map<size_type, std::vector<TimeState>>
      calc_reliability_schedule(std::int64_t final_time) const;

    private:
      Fixed_CDF fixed_cdf;
      FailureMode fms;
      size_type next_fixed_cdf_id;
      size_type next_fm_id;
      std::set<size_type> components;

      void calc_next_events(
          std::unordered_map<size_type, std::int64_t>& comp_id_to_dt,
          bool is_failure) const;

      size_type
      update_schedule(
          std::unordered_map<size_type, std::int64_t>& comp_id_to_time,
          std::unordered_map<size_type, std::int64_t>& comp_id_to_dt,
          std::unordered_map<size_type, std::vector<TimeState>>&
            comp_id_to_reliability_schedule,
          std::int64_t final_time,
          FlowValueType next_state) const;
  };
}

#endif // ERIN_RELIABILITY_H
