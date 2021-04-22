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
    UncontrolledSourceState uss{state};
    auto next_index = static_cast<size_type>(state.index) + 1;
    if (uss.report_outflow_at_current_index) {
      uss.report_outflow_at_current_index = false;
    }
    else if (next_index < data.num_items) {
      auto next_time = data.times[next_index];
      auto next_supply = data.supply[next_index];
      auto inflow = state.inflow_port.with_requested(next_supply, next_time);
      auto out = state.outflow_port;
      auto spill = state.spill_port.with_requested(next_supply, next_time);
      auto out_req = out.get_requested();
      if (next_supply >= out_req) {
        out = out.with_achieved(out_req, next_time);
        spill = spill.with_achieved(next_supply - out_req, next_time);
      }
      else {
        out = out.with_achieved(next_supply, next_time);
        spill = spill.with_achieved(0.0, next_time);
      }
      uss.time = next_time;
      uss.index = static_cast<int>(next_index);
      uss.report_outflow_at_current_index =
        out.should_propagate_achieved_at(next_time);
      uss.inflow_port = inflow;
      uss.outflow_port = out;
      uss.spill_port = spill;
    }
    return uss;
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
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "- outflow_request = " << outflow_request << "\n";
    }
    auto next_time = state.time + dt;
    auto index = static_cast<size_type>(state.index);
    auto supply = (index < data.num_items) ?  data.supply[index] : 0.0;
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
    bool report{out.should_propagate_achieved_at(next_time)};
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "- new value of report* = " << report << "\n";
    }
    return UncontrolledSourceState{
      next_time,
      state.index,
      state.inflow_port,
      out,
      spill,
      report
    };
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
      const UncontrolledSourceData& /* data */,
      const UncontrolledSourceState& state,
      std::vector<PortValue>& ys)
  {
    if (state.report_outflow_at_current_index) {
      ys.emplace_back(
          PortValue{outport_outflow_achieved,
                    state.outflow_port.get_achieved()});
    }
  }
}
