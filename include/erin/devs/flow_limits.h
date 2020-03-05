/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_FLOW_LIMITS_H
#define ERIN_DEVS_FLOW_LIMITS_H
#include "erin/type.h"
#include "erin/devs.h"
#include <iostream>
#include <string>
#include <vector>

namespace erin::devs
{
  class FlowLimits
  {
    public:
      FlowLimits(
          FlowValueType lower_limit,
          FlowValueType upper_limit);

      [[nodiscard]] FlowValueType get_lower_limit() const { return lower_limit; }
      [[nodiscard]] FlowValueType get_upper_limit() const { return upper_limit; }

      friend bool operator==(const FlowLimits& a, const FlowLimits& b);
      friend std::ostream& operator<<(std::ostream& os, const FlowLimits& f);

    private:
      FlowValueType lower_limit;
      FlowValueType upper_limit;
  };

  bool operator==(const FlowLimits& a, const FlowLimits& b);
  bool operator!=(const FlowLimits& a, const FlowLimits& b);
  std::ostream& operator<<(std::ostream& os, const FlowLimits& f);

  const RealTimeType default_start_time{0};
  const FlowValueType default_upper_flow_limit{1e12};
  const FlowValueType default_lower_flow_limit{0.0};

  struct FlowLimitsState
  {
    RealTimeType time{default_start_time};
    Port inflow_port{
      default_start_time,
      default_lower_flow_limit,
      default_lower_flow_limit};
    Port outflow_port{
      default_start_time,
      default_lower_flow_limit,
      default_lower_flow_limit};
    FlowLimits limits{
      default_lower_flow_limit,
      default_upper_flow_limit};
    bool report_inflow_request{false};
    bool report_outflow_achieved{false};
  };

  bool operator==(const FlowLimitsState& a, const FlowLimitsState& b);
  bool operator!=(const FlowLimitsState& a, const FlowLimitsState& b);
  std::ostream& operator<<(std::ostream& os, const FlowLimitsState& s);

  FlowLimitsState make_flow_limits_state(
      FlowValueType lower_limit,
      FlowValueType upper_limit);

  FlowLimitsState make_flow_limits_state(
      RealTimeType time,
      Port inflow_port,
      Port outflow_port,
      FlowValueType lower_limit,
      FlowValueType upper_limit,
      bool report_inflow_request,
      bool report_outflow_achieved);

  RealTimeType
  flow_limits_time_advance(const FlowLimitsState& state);

  FlowLimitsState
  flow_limits_external_transition(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs);

  FlowLimitsState
  flow_limits_external_transition_on_outflow_request(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      FlowValueType outflow_request);

  FlowLimitsState
  flow_limits_external_transition_on_inflow_achieved(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      FlowValueType inflow_achieved);

  FlowLimitsState
  flow_limits_internal_transition(const FlowLimitsState& state);

  std::vector<PortValue>
  flow_limits_output_function(const FlowLimitsState& state);

  void 
  flow_limits_output_function_mutable(
      const FlowLimitsState& state,
      std::vector<PortValue>& ys);

}
#endif // ERIN_DEVS_FLOW_LIMITS_H
