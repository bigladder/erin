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

  // Data Structs
  struct TimeState
  {
    std::int64_t time{0};
    FlowValueType state{1.0};
  };

  bool operator==(const TimeState& a, const TimeState& b);
  bool operator!=(const TimeState& a, const TimeState& b);
  std::ostream& operator<<(std::ostream& os, const TimeState& ts);

  enum class CdfType
  {
    Fixed = 0//,
    //Normal,
    //Weibull
  };

  std::string cdf_type_to_tag(CdfType cdf_type);
  CdfType tag_to_cdf_type(const std::string& tag);

  struct FailureMode
  {
    // TODO: remove component_id
    std::vector<size_type> component_id{};
    std::vector<std::string> name{};
    std::vector<size_type> failure_cdf{};
    // TODO: remove failure_cdf_type
    std::vector<CdfType> failure_cdf_type{};
    std::vector<size_type> repair_cdf{};
    // TODO: remove repair_cdf_type
    std::vector<CdfType> repair_cdf_type{};
  };

  /*
  struct Cdf {
    std::vector<size_type> subtype_id{};
    std::vector<CdfType> cdf_type{};
  };

  struct FailureMode_Component_Link {
    std::vector<size_type> failure_mode_id{};
    std::vector<size_type> component_id{};
  };
  */

  struct Fixed_CDF
  {
    std::vector<std::int64_t> value{};
    // TODO: remove time_multiplier, just make value always be seconds
    std::vector<std::int64_t> time_multiplier{}; // to convert from given units to seconds
  };

  // Main Class to do Reliability Schedule Creation
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
      /*
      size_type add_fixed_cdf(
          std::int64_t value_in_seconds);

      size_type add_failure_mode(
          const std::string& name,
          const size_type& failure_cdf_id,
          const size_type& repair_cdf_id
          );

      void link_component_with_failure_mode(
          const size_type& comp_id,
          const size_type& fm_id);

      */

      std::unordered_map<size_type, std::vector<TimeState>>
      calc_reliability_schedule(std::int64_t final_time) const;

    private:
      Fixed_CDF fixed_cdf;
      /*
      Cdf cdfs;
      size_type next_cdf_id;
      FailureMode_Component_Link fm_comp_links;
      size_type next_comp_fm_link_id;
      */
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
