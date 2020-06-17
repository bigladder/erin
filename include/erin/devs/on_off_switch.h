/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_ON_OFF_SWITCH_H
#define ERIN_DEVS_ON_OFF_SWITCH_H
#include "erin/devs.h"
#include "erin/type.h"
#include "erin/reliability.h"
#include <functional>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

namespace erin::devs
{
  struct OnOffSwitchData
  {
    std::vector<RealTimeType> schedule_times{};
    std::vector<FlowValueType> schedule_values{};
  };

  bool operator==(const OnOffSwitchData& a, const OnOffSwitchData& b);
  bool operator!=(const OnOffSwitchData& a, const OnOffSwitchData& b);
  std::ostream& operator<<(std::ostream& os, const OnOffSwitchData& a);

  struct OnOffSwitchState
  {
    RealTimeType time{0};
    FlowValueType state{1.0};
    Port inflow_port{0, 0.0, 0.0};
    Port outflow_port{0, 0.0, 0.0};
  };

  bool operator==(const OnOffSwitchState& a, const OnOffSwitchState& b);
  bool operator!=(const OnOffSwitchState& a, const OnOffSwitchState& b);
  std::ostream& operator<<(std::ostream& os, const OnOffSwitchState& a);

  OnOffSwitchData
  make_on_off_switch_data(const std::vector<ERIN::TimeState>& schedule);

  OnOffSwitchState
  make_on_off_switch_state(const OnOffSwitchData& data);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  on_off_switch_time_advance(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  //OnOffSwitchState
  //on_off_switch_internal_transition(const OnOffSwitchState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  OnOffSwitchState
  on_off_switch_external_transition(
      const OnOffSwitchState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs); 

  /*
  ////////////////////////////////////////////////////////////
  // confluent transition
  OnOffSwitchState
  on_off_switch_confluent_transition(
      const OnOffSwitchState& state,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  on_off_switch_output_function(const OnOffSwitchState& state);

  void 
  on_off_switch_output_function_mutable(
      const OnOffSwitchState& state,
      std::vector<PortValue>& ys);
  */
}

#endif // ERIN_DEVS_ON_OFF_SWITCH_H
