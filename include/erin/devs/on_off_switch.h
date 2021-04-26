/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

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
    std::vector<RealTimeType> times{};
    std::vector<bool> states{};
    std::vector<RealTimeType>::size_type num_items;
  };

  bool operator==(const OnOffSwitchData& a, const OnOffSwitchData& b);
  bool operator!=(const OnOffSwitchData& a, const OnOffSwitchData& b);
  std::ostream& operator<<(std::ostream& os, const OnOffSwitchData& a);

  struct OnOffSwitchState
  {
    RealTimeType time{0};
    bool state{true};
    std::vector<RealTimeType>::size_type next_index{0};
    Port3 inflow_port{};
    Port3 outflow_port{};
    bool report_inflow_request{false};
    bool report_outflow_achieved{false};
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
  OnOffSwitchState
  on_off_switch_internal_transition(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  OnOffSwitchState
  on_off_switch_external_transition(
      const OnOffSwitchState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // confluent transition
  OnOffSwitchState
  on_off_switch_confluent_transition(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  on_off_switch_output_function(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state);

  void 
  on_off_switch_output_function_mutable(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state,
      std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_ON_OFF_SWITCH_H
