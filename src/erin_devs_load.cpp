/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs/load.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace erin::devs
{
  void
  check_loads(const std::vector<LoadItem>& loads)
  {
    using size_type = std::vector<LoadItem>::size_type;
    auto N{loads.size()};
    if (N < 1) {
      std::ostringstream oss{};
      oss << "loads must have at least one LoadItems but "
             "only " << N << " provided\n";
      throw std::invalid_argument(oss.str());
    }
    RealTimeType t_last{-1};
    for (size_type idx{0}; idx < loads.size(); ++idx) {
      const auto& x = loads[idx];
      auto t = x.time;
      auto v = x.value;
      if (v < 0.0) {
        std::ostringstream oss{};
        oss << "LoadItem[" << idx << "] has a negative load value. Negative flows are not allowed.\n";
        throw std::invalid_argument(oss.str());
      }
      if ((t < 0) || (t <= t_last)) {
        std::ostringstream oss{};
        oss << "LoadItems must have time points that are everywhere "
            << "increasing and positive but it doesn't...\n"
            << "t[" << idx << "] = " << t << "\n"
            << "t[" << (idx - 1) << "] = " << t_last << "\n";
        throw std::invalid_argument(oss.str());
      }
      t_last = t;
    }
  }

  LoadData
  make_load_data(const std::vector<LoadItem>& loads)
  {
    check_loads(loads);
    auto num_loads = loads.size();
    std::vector<RealTimeType> times(num_loads, 0);
    std::vector<FlowValueType> load_values(num_loads, 0.0);
    for (decltype(num_loads) i{0}; i < num_loads; ++i) {
      times[i] = loads[i].time;
      load_values[i] = loads[i].value;
    }
    return LoadData{
      static_cast<int>(num_loads),
      std::move(times),
      std::move(load_values)};
  }

  LoadState
  make_load_state()
  {
    return LoadState{0, -1, Port3{}, false};
  }

  RealTimeType
  load_current_time(const LoadState& state)
  {
    return state.time;
  }

  RealTimeType
  load_next_time(const LoadData& data, const LoadState& state)
  {
    auto next_idx = state.current_index + 1;
    if (next_idx < data.number_of_loads) {
      return data.times[next_idx];
    }
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
  
  std::ostream&
  operator<<(std::ostream& os, const LoadState& s)
  {
    return os << "{"
              << ":t " << s.time
              << " "
              << ":idx " << s.current_index
              << " "
              << ":inflow-port " << s.inflow_port
              << " "
              << ":resend-request? " << s.resend_request
              << "}";
  }

  RealTimeType
  load_time_advance(const LoadData& data, const LoadState& state)
  {
    if (state.resend_request) {
      return 0;
    }
    auto next_time = load_next_time(data, state);
    if (next_time == infinity) {
      return infinity;
    }
    return next_time - state.time;
  }

  LoadState
  load_internal_transition(const LoadData& data, const LoadState& state)
  {
    if (state.resend_request) {
      return LoadState{
        state.time,
        state.current_index,
        state.inflow_port,
        false,
      };
    }
    auto max_idx = data.number_of_loads - 1;
    if (state.current_index >= max_idx) {
      return state;
    }
    auto next_idx = state.current_index + 1;
    auto next_time = data.times[next_idx];
    auto next_load = data.load_values[next_idx];
    auto update = state.inflow_port.with_requested(next_load);
    return LoadState{
      next_time,
      next_idx,
      update.port,
      false,
    };
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
    auto update = state.inflow_port.with_achieved(inflow_achieved);
    return LoadState{
      new_time,
      state.current_index,
      update.port,
      update.send_request,
    };
  }

  LoadState
  load_confluent_transition(
      const LoadData& data,
      const LoadState& state,
      const std::vector<PortValue>& xs)
  {
    auto dt = load_time_advance(data, state);
    return load_internal_transition(
        data,
        load_external_transition(state, dt, xs));
  }

  std::vector<PortValue>
  load_output_function(const LoadData& data, const LoadState& state)
  {
    std::vector<PortValue> ys{};
    load_output_function_mutable(data, state, ys);
    return ys;
  }

  void
  load_output_function_mutable(
      const LoadData& data,
      const LoadState& state,
      std::vector<PortValue>& ys)
  {
    if (state.resend_request) {
      ys.emplace_back(
          PortValue{
            outport_inflow_request,
            state.inflow_port.get_requested()});
    }
    else {
      auto next_state = load_internal_transition(data, state);
      auto max_idx{data.number_of_loads - 1};
      if (next_state.current_index <= max_idx) {
        if (next_state.inflow_port.should_send_request(state.inflow_port)) {
          ys.emplace_back(
              PortValue{
                outport_inflow_request,
                next_state.inflow_port.get_requested()});
        }
      }
    }
  }
}
