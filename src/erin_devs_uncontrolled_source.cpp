/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/uncontrolled_source.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace erin::devs
{
  bool operator==(const UncontrolledSourceData& a, const UncontrolledSourceData& b)
  {
    return (a.num_items == b.num_items) &&
           (a.times == b.times) &&
           (a.supply == b.supply);
  }

  bool operator!=(const UncontrolledSourceData& a, const UncontrolledSourceData& b)
  {
    return !(a == b);
  }

  std::ostream& operator<<(std::ostream& os, const UncontrolledSourceData& a)
  {
    return os << "UncontrolledSourceData("
              << "times = " << ERIN::vec_to_string<RealTimeType>(a.times)
              << ", supply = " << ERIN::vec_to_string<FlowValueType>(a.supply)
              << ", num_items = " << a.num_items << ")";
  }

  UncontrolledSourceData
  make_uncontrolled_source_data(const std::vector<LoadItem>& loads)
  {
    std::vector<RealTimeType> times{};
    std::vector<FlowValueType> supply{};
    size_type num_items{0};
    for (const auto& li : loads) {
      times.emplace_back(li.get_time());
      if (li.get_is_end()) {
        supply.emplace_back(0.0);
      }
      else {
        supply.emplace_back(li.get_value());
      }
      ++num_items;
    }
    return UncontrolledSourceData{
      std::move(times), std::move(supply), num_items};
  }

  bool
  operator==(
      const UncontrolledSourceState& a,
      const UncontrolledSourceState& b)
  {
    return (a.time == b.time) &&
           (a.index == b.index) &&
           (a.outflow_port == b.outflow_port) &&
           (a.spill_port == b.spill_port);
  }

  bool operator!=(
      const UncontrolledSourceState& a,
      const UncontrolledSourceState& b)
  {
    return !(a == b);
  }

  std::ostream& operator<<(std::ostream& os, const UncontrolledSourceState& a)
  {
    return os << "UncontrolledSourceState("
              << "time = " << a.time << ", "
              << "index = " << a.index << ", "
              << "outflow_port = " << a.outflow_port << ", "
              << "spill_port = " << a.spill_port << ")";
  }

  UncontrolledSourceState
  make_uncontrolled_source_state()
  {
    return UncontrolledSourceState{};
  }

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  uncontrolled_src_time_advance(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state)
  {
    auto next_index = state.index + 1;
    auto ta = infinity;
    if (next_index < data.num_items) {
      ta = data.times[next_index] - state.time;
    }
    return ta;
  }

  ////////////////////////////////////////////////////////////
  // internal transition
  UncontrolledSourceState
  uncontrolled_src_internal_transition(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state)
  {
    auto next_index = state.index + 1;
    if (next_index >= data.num_items) {
      return state;
    }
    auto next_time = data.times[next_index];
    auto next_supply = data.supply[next_index];
    auto out = state.outflow_port;
    auto spill = state.spill_port;
    auto out_req = out.get_requested();
    if (next_supply >= out_req) {
      out = out.with_achieved(out_req, next_time);
      spill = spill.with_achieved(next_supply - out_req, next_time);
    }
    else {
      out = out.with_achieved(next_supply, next_time);
      spill = spill.with_achieved(0.0, next_time);
    }
    return UncontrolledSourceState{
      next_time,
      next_index,
      std::move(out),
      std::move(spill)
    };
  }

  ////////////////////////////////////////////////////////////
  // external transition
  UncontrolledSourceState
  uncontrolled_src_external_transition(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs)
  {
    FlowValueType outflow_request{0.0};
    for (const auto& x : xs) {
      switch (x.port) {
        case inport_outflow_request:
          {
            outflow_request += x.value;
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
    auto next_time = state.time + dt;
    auto supply = (state.index < data.num_items) ?
      data.supply[state.index]
      : 0.0;
    auto out = state.outflow_port.with_requested(outflow_request, next_time);
    auto spill = state.spill_port.with_requested(supply, next_time);
    if (supply >= outflow_request) {
      out = out.with_achieved(outflow_request, next_time);
      spill = spill.with_achieved(supply - outflow_request, next_time);
    }
    else {
      out = out.with_achieved(supply, next_time);
      spill = spill.with_achieved(0.0, next_time);
    }
    return UncontrolledSourceState{
      next_time, state.index, std::move(out), std::move(spill)};
  }

  ////////////////////////////////////////////////////////////
  // confluent transition
  UncontrolledSourceState
  uncontrolled_src_confluent_transition(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state,
      const std::vector<PortValue>& xs)
  {
    return uncontrolled_src_external_transition(
        data,
        uncontrolled_src_internal_transition(data, state),
        0,
        xs);
  }

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  uncontrolled_src_output_function(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state)
  {
    std::vector<PortValue> ys{};
    uncontrolled_src_output_function_mutable(data, state, ys);
    return ys;
  }

  void 
  uncontrolled_src_output_function_mutable(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state,
      std::vector<PortValue>& ys)
  {
    auto next_index = state.index + 1;
    if (next_index < data.num_items) {
      auto next_time = data.times[next_index];
      auto supply = data.supply[next_index];
      auto out_req = state.outflow_port.get_requested();
      auto out = state.outflow_port;
      if (supply >= out_req) {
        out = out.with_achieved(out_req, next_time);
      }
      else {
        out = out.with_achieved(supply, next_time);
      }
      if (out.should_propagate_achieved_at(next_time)) {
        ys.emplace_back(
            PortValue{outport_outflow_achieved, out.get_achieved()});
      }
    }
  }
}
