/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

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
  struct LoadData
  {
    int number_of_loads{0};
    std::vector<RealTimeType> times{};
    std::vector<FlowValueType> load_values{};
  };

  struct LoadState
  {
    RealTimeType time{0};
    int current_index{0};
    Port2 inflow_port{};
  };

  LoadData make_load_data(const std::vector<LoadItem>& loads);

  LoadState make_load_state();

  RealTimeType load_current_time(const LoadState& state);

  RealTimeType load_next_time(const LoadData& data, const LoadState& state);

  FlowValueType load_current_request(const LoadState& state);

  FlowValueType load_current_achieved(const LoadState& state);
  
  std::ostream& operator<<(std::ostream& os, const LoadState& s);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType load_time_advance(const LoadData& data, const LoadState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  LoadState load_internal_transition(const LoadData& data, const LoadState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  LoadState load_external_transition(
      const LoadState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // confluent transition
  LoadState load_confluent_transition(
      const LoadData& data,
      const LoadState& state,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue> load_output_function(
      const LoadData& data, const LoadState& state);

  void
  load_output_function_mutable(
      const LoadData& data, const LoadState& state, std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_LOAD_H
