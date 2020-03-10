/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_LOAD_H
#define ERIN_DEVS_LOAD_H
#include "erin/devs.h"
#include "erin/type.h"
#include <vector>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions
  using LoadItem = ERIN::LoadItem;
  void check_loads(const std::vector<LoadItem>& loads);

  ////////////////////////////////////////////////////////////
  // state

  struct LoadState
  {
    RealTimeType time{0};
    int number_of_loads{0};
    std::vector<LoadItem> loads{};
    int current_index{0};
    Port inflow_port{};
  };

  LoadState make_load_state(const std::vector<LoadItem>& loads);

  RealTimeType load_current_time(const LoadState& state);

  RealTimeType load_next_time(const LoadState& state);

  FlowValueType load_current_request(const LoadState& state);

  FlowValueType load_current_achieved(const LoadState& state);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType load_time_advance(const LoadState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  LoadState load_internal_transition(const LoadState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  LoadState load_external_transition(
      const LoadState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // confluent transition
  LoadState load_confluent_transition(
      const LoadState& state,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue> load_output_function(const LoadState& state);

  void
  load_output_function_mutable(
      const LoadState& state, std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_CONVERTER_H
