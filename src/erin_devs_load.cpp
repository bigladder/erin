/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/load.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace erin::devs
{
  LoadState
  make_load_state(const std::vector<LoadItem>& loads)
  {
    return LoadState{
      0,
      static_cast<int>(loads.size()),
      loads,
      -1,
      Port{0, 0.0}};
  }

  RealTimeType
  load_current_time(const LoadState& state)
  {
    return state.time;
  }

  RealTimeType
  load_next_time(const LoadState& state)
  {
    auto next_idx = state.current_index + 1;
    if (next_idx < state.number_of_loads)
      return state.loads[next_idx].get_time();
    return infinity;
  }

  FlowValueType
  load_current_request(const LoadState& state)
  {
    return state.inflow_port.get_requested();
  }

  FlowValueType
  load_current_achieved(const LoadState& state)
  {
    return state.inflow_port.get_achieved();
  }

  RealTimeType
  load_time_advance(const LoadState& state)
  {
    if (state.current_index < 0)
      return 0;
    auto next_idx = state.current_index + 1;
    if (next_idx < state.number_of_loads) {
      auto next_time = load_next_time(state);
      return next_time - state.time;
    }
    return infinity;
  }

  LoadState
  load_internal_transition(const LoadState& state)
  {
    auto next_time = load_next_time(state);
    if (next_time == infinity)
      return state;
    auto next_idx = state.current_index + 1;
    return LoadState{
      next_time,
      state.number_of_loads,
      state.loads,
      next_idx,
      state.inflow_port.with_requested(
          state.loads[next_idx].get_value(),
          next_time)};
  }

  LoadState
  load_external_transition(
      const LoadState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs)
  {
    FlowValueType inflow_achieved{0.0};
    for (const auto& x : xs) {
      switch (x.port) {
        case inport_inflow_achieved:
          {
            inflow_achieved += x.value;
            break;
          }
        default:
          {
            std::ostringstream oss{};
            oss << "unhandled port! port = " << x.port << "\n";
            throw std::runtime_error(oss.str());
          }
      }
    }
    auto new_time = state.time + dt;
    return LoadState{
      new_time,
      state.number_of_loads,
      state.loads,
      state.current_index,
      state.inflow_port.with_achieved(inflow_achieved, new_time)};
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
    auto next_idx = state.current_index + 1;
    auto max_idx{state.number_of_loads - 1};
    if (next_idx < max_idx)
      ys.emplace_back(
          PortValue{outport_inflow_request, state.loads[next_idx].get_value()});
    else if (next_idx == max_idx)
      ys.emplace_back(PortValue{outport_inflow_request, 0.0});
  }
}
