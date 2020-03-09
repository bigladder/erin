/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_LOAD_H
#define ERIN_DEVS_LOAD_H
#include "erin/devs.h"
#include <vector>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions
  struct DurationLoad
  {
    RealTimeType duration{0};
    FlowValueType load{0.0};
  };

  ////////////////////////////////////////////////////////////
  // state

  struct LoadState
  {
    int number_of_loads{0};
    std::vector<DurationLoad> duration_loads{};
    int current_index{0};
  };

  LoadState make_load_state(const std::vector<DurationLoad>& duration_loads);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType load_time_advance(const LoadState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  LoadState load_internal_transition(const LoadState& state);

  ////////////////////////////////////////////////////////////
  // external transition

  ////////////////////////////////////////////////////////////
  // confluent transition

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue> load_output_function(const LoadState& state);

  void
  load_output_function_mutable(
      const LoadState& state, std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_CONVERTER_H
