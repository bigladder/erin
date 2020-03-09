/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/load.h"

namespace erin::devs
{
  LoadState
  make_load_state(const std::vector<DurationLoad>& duration_loads)
  {
    return LoadState{
      static_cast<int>(duration_loads.size()),
      duration_loads,
      -1};
  }

  RealTimeType
  load_time_advance(const LoadState& state)
  {
    if (state.current_index == -1)
      return 0;
    if (state.current_index < state.number_of_loads)
      return state.duration_loads.at(state.current_index).duration;
    return infinity;
  }

  LoadState
  load_internal_transition(const LoadState& state)
  {
    return LoadState{
      state.number_of_loads,
      state.duration_loads,
      state.current_index + 1};
  }

  std::vector<PortValue>
  load_output_function(const LoadState& state)
  {
    std::vector<PortValue> ys{};
    load_output_function_mutable(state, ys);
    return ys;
  }

  void
  load_output_function_mutable(const LoadState& state, std::vector<PortValue>& ys)
  {
    auto idx = state.current_index + 1;
    if (idx < state.number_of_loads)
      ys.emplace_back(
          PortValue{outport_inflow_request, state.duration_loads.at(idx).load});
  }
}
