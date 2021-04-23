/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_DEVS_FLOW_METER_H
#define ERIN_DEVS_FLOW_METER_H
#include "erin/devs.h"
#include "erin/type.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions

  ////////////////////////////////////////////////////////////
  // state
  struct FlowMeterState
  {
    RealTimeType time{0};
    Port3 port{};
    bool report_inflow_request{false};
    bool report_outflow_achieved{false};
  };

  bool operator==(const FlowMeterState& a, const FlowMeterState& b);
  bool operator!=(const FlowMeterState& a, const FlowMeterState& b);
  std::ostream& operator<<(std::ostream& os, const FlowMeterState& a);

  FlowMeterState flow_meter_make_state();

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  flow_meter_time_advance(const FlowMeterState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  FlowMeterState
  flow_meter_internal_transition(const FlowMeterState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  FlowMeterState
  flow_meter_external_transition(
      const FlowMeterState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // confluent transition
  FlowMeterState
  flow_meter_confluent_transition(
      const FlowMeterState& state,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  flow_meter_output_function(const FlowMeterState& state);

  void 
  flow_meter_output_function_mutable(
      const FlowMeterState& state,
      std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_FLOW_METER_H