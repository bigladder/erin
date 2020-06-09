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
    std::vector<std::string> tag{};
    std::vector<size_type> failure_cdf{};
    std::vector<size_type> repair_cdf{};
  };

  struct Cdf {
    std::vector<std::string> tag{};
    std::vector<size_type> subtype_id{};
    std::vector<CdfType> cdf_type{};
  };

  struct FailureMode_Component_Link {
    std::vector<size_type> failure_mode_id{};
    std::vector<size_type> component_id{};
  };

  struct Fixed_CDF
  {
    std::vector<std::int64_t> value{};
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
          std::int64_t value_in_seconds);

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
      calc_reliability_schedule(std::int64_t final_time) const;

    private:
      Fixed_CDF fixed_cdf;
      Cdf cdfs;
      FailureMode fms;
      FailureMode_Component_Link fm_comp_links;
      std::set<size_type> components;
      Component_meta comp_meta;
      size_type next_fixed_cdf_id;
      size_type next_cdf_id;
      size_type next_fm_id;
      size_type next_comp_id;

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
