/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs/uncontrolled_source.h"
#include "debug_utils.h"
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
    return os << "{"
              << ":ts " << ERIN::vec_to_string<RealTimeType>(a.times)
              << " :supply " << ERIN::vec_to_string<FlowValueType>(a.supply)
              << " :count = " << a.num_items << "}";
  }

  UncontrolledSourceData
  make_uncontrolled_source_data(const std::vector<LoadItem>& loads)
  {
    std::vector<RealTimeType> times{};
    std::vector<FlowValueType> supply{};
    size_type num_items{0};
    for (const auto& li : loads) {
      times.emplace_back(li.time);
      supply.emplace_back(li.value);
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
           (a.inflow_port == b.inflow_port) &&
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
    return os << "{"
              << ":t " << a.time << " "
              << " :idx " << a.index << " "
              << " :ip " << a.inflow_port << " "
              << " :op " << a.outflow_port << ", "
              << " :spill " << a.spill_port << " "
              << ":report_oa? "
              << a.report_outflow_at_current_index << "}";
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
    auto next_index = static_cast<size_type>(state.index) + 1;
    RealTimeType ta{0};
    if (state.report_outflow_at_current_index) {
      ta = 0;
    }
    else if (next_index < data.num_items) {
      ta = data.times[next_index] - state.time;
    }
    else {
      ta = infinity;
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
    if (state.report_outflow_at_current_index) {
      return UncontrolledSourceState{
        state.time,
        state.index,
        state.inflow_port,
        state.outflow_port,
        state.spill_port,
        false};
    }
    auto next_index = static_cast<size_type>(state.index) + 1;
    if (next_index < data.num_items) {
      auto supply = data.supply[next_index];
      auto ip{state.inflow_port};
      auto op{state.outflow_port};
      auto sp{state.spill_port};
      ip = ip.with_requested_and_achieved(supply, supply).port;
      auto oversupply{supply > op.get_requested()};
      auto update_op =
        op.with_achieved(oversupply ? op.get_requested() : supply);
      op = update_op.port;
      auto spilled{oversupply ? (supply - op.get_requested()) : 0.0};
      sp = sp.with_requested_and_achieved(spilled, spilled).port;
      return UncontrolledSourceState{
        data.times[next_index],
        static_cast<int>(next_index),
        ip,
        op,
        sp,
        update_op.send_achieved};
    }
    std::ostringstream oss{};
    oss << "invalid sequencing\n"
        << "in an uncontrolled source internal transition"
        << " but no more transition indices...\n"
        << "state: " << state << "\n";
    throw std::runtime_error(oss.str());
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
    auto idx = static_cast<size_type>(state.index);
    auto supply = (idx < data.num_items) ?  data.supply[idx] : 0.0;
    auto ip{state.inflow_port};
    auto update_op = state.outflow_port.with_requested(outflow_request);
    auto op{update_op.port};
    auto sp{state.spill_port};
    ip = ip.with_requested_and_achieved(supply, supply).port;
    auto oversupply{supply > op.get_requested()};
    update_op = op.with_achieved(oversupply ? op.get_requested() : supply);
    op = update_op.port;
    auto spilled{oversupply ? (supply - op.get_requested()) : 0.0};
    sp = sp.with_requested_and_achieved(spilled, spilled).port;
    return UncontrolledSourceState{
      next_time,
      state.index,
      ip,
      op,
      sp,
      update_op.send_achieved};
  }

  ////////////////////////////////////////////////////////////
  // confluent transition
  UncontrolledSourceState
  uncontrolled_src_confluent_transition(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state,
      const std::vector<PortValue>& xs)
  {
    auto next_state = uncontrolled_src_external_transition(
        data,
        uncontrolled_src_internal_transition(data, state),
        0,
        xs);
    next_state.report_outflow_at_current_index =
        next_state.report_outflow_at_current_index
        || next_state.outflow_port.should_send_achieved(state.outflow_port);
    return next_state;
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
      const UncontrolledSourceData& /* data */,
      const UncontrolledSourceState& state,
      std::vector<PortValue>& ys)
  {
    if (state.report_outflow_at_current_index) {
      ys.emplace_back(
          PortValue{
            outport_outflow_achieved,
            state.outflow_port.get_achieved()});
    }
  }
}
