/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs/on_off_switch.h"
#include "debug_utils.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <typeindex>

namespace erin::devs
{
  bool
  operator==(const OnOffSwitchData& a, const OnOffSwitchData& b)
  {
    return (
        (a.times == b.times)
        && (a.states == b.states));
  }

  bool
  operator!=(const OnOffSwitchData& a, const OnOffSwitchData& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const OnOffSwitchData& a)
  {
    namespace E = ERIN;
    return os << "OnOffSwitchData("
              << "times="
              << E::vec_to_string<RealTimeType>(a.times) << ", "
              << "states="
              << E::vec_to_string<bool>(a.states) << ")";
  }

  bool
  operator==(const OnOffSwitchState& a, const OnOffSwitchState& b)
  {
    return (
        (a.time == b.time)
        && (a.state == b.state)
        && (a.next_index == b.next_index)
        && (a.inflow_port == b.inflow_port)
        && (a.outflow_port == b.outflow_port)
        && (a.report_inflow_request == b.report_inflow_request)
        && (a.report_outflow_achieved == b.report_outflow_achieved));
  }

  bool
  operator!=(const OnOffSwitchState& a, const OnOffSwitchState& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const OnOffSwitchState& a)
  {
    namespace E = ERIN;
    return os << "OnOffSwitchState("
              << "time=" << a.time << ", "
              << "state=" << a.state << ", "
              << "next_index=" << a.next_index << ", "
              << "inflow_port=" << a.inflow_port << ", "
              << "outflow_port=" << a.outflow_port << ", " 
              << "report_inflow_request=" << a.report_inflow_request << ", "
              << "report_outflow_achieved=" << a.report_outflow_achieved << ")";
  }

  OnOffSwitchData
  make_on_off_switch_data(
      const std::vector<ERIN::TimeState>& schedule)
  {
    bool last_state{false};
    bool first_item{true};
    RealTimeType last_time{-1};
    std::vector<RealTimeType> times{};
    std::vector<bool> states{};
    for (const auto& item : schedule) {
      if (item.time <= last_time) {
        std::ostringstream oss{};
        oss << "times are not increasing for schedule:\n"
            << "item.time = " << item.time << "\n"
            << "last_time = " << last_time << "\n"
            << ERIN::vec_to_string<ERIN::TimeState>(schedule) << "\n";
        throw std::invalid_argument(oss.str());
      }
      if (first_item || (item.state != last_state)) {
        times.emplace_back(item.time);
        states.emplace_back(item.state);
        first_item = false;
      }
      last_state = item.state;
      last_time = item.time;
    }
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "Making on/off switch. Processed schedule is:\n";
      for (std::vector<RealTimeType>::size_type i{0}; i < times.size(); ++i) {
        std::cout << "(time=" << times[i] << ", state=" << states.at(i) << ")\n";
      }
    }
    return OnOffSwitchData{
      times,        // schedule_times
      states,       // schedule_values
      times.size()
    };
  }

  OnOffSwitchState
  make_on_off_switch_state(const OnOffSwitchData& data)
  {
    using stype = std::vector<RealTimeType>::size_type; 
    bool init_state{true};
    stype next_index{0};
    if ((data.num_items > 0) && (data.times[0] == 0)) {
      next_index = 1;
    }
    for (stype i{0}; i < data.num_items; ++i) {
      if (data.times[i] == 0) {
        init_state = data.states[i];
      }
      else {
        break;
      }
    }
    return OnOffSwitchState{
      0,            // time
      init_state,   // state
      next_index,   // next_index
      Port2{0.0},   // inflow_port
      Port2{0.0},   // outflow_port
      false,
      false,
    };
  }

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  on_off_switch_time_advance(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state)
  {
    if (state.report_inflow_request || state.report_outflow_achieved) {
      return 0;
    }
    if (state.next_index < data.num_items) {
      return data.times[state.next_index] - state.time;
    }
    return infinity;
  }

  ////////////////////////////////////////////////////////////
  // internal transition
  OnOffSwitchState
  on_off_switch_internal_transition(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state)
  {
    if (state.report_inflow_request || state.report_outflow_achieved) {
      return OnOffSwitchState{
        state.time,
        state.state,
        state.next_index,
        state.inflow_port,
        state.outflow_port,
        false,
        false};
    }
    if (state.next_index < data.num_items) {
      auto next_time = data.times[state.next_index];
      auto next_state = data.states[state.next_index];
      auto ip{state.inflow_port};
      auto op{state.outflow_port};
      if (next_state) {
        ip = ip.with_requested(op.get_requested()).port;
        op = op.with_achieved(ip.get_achieved()).port;
      }
      else {
        ip = ip.with_requested(0.0).port;
        op = op.with_achieved(0.0).port;
      }
      return OnOffSwitchState{
        next_time,
        next_state,
        state.next_index + 1,
        ip,
        op,
        false,
        false
      };
    }
    throw std::runtime_error("invalid internal transition");
  }

  ////////////////////////////////////////////////////////////
  // external transition
  OnOffSwitchState
  on_off_switch_external_transition(
      const OnOffSwitchState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs)
  {
    bool got_outflow_request{false};
    bool got_inflow_achieved{false};
    FlowValueType outflow_request{0.0};
    FlowValueType inflow_achieved{0.0};
    for (const auto& x : xs) {
      switch (x.port) {
        case inport_outflow_request:
          {
            got_outflow_request = true;
            outflow_request += x.value;
            break;
          }
        case inport_inflow_achieved:
          {
            got_inflow_achieved = true;
            inflow_achieved += x.value;
            break;
          }
        default:
          {
            std::ostringstream oss{};
            oss << "unhandled port " << x.port
                << " in on_off_switch_external_transition(...)";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    const auto& t = state.time;
    auto new_time = t + elapsed_time;
    auto ip{state.inflow_port};
    auto op{state.outflow_port};
    bool report_ir{false};
    bool report_oa{false};
    if (state.state) {
      if (got_inflow_achieved) {
        auto update_ip = ip.with_achieved(inflow_achieved);
        auto update_op = op.with_achieved(update_ip.port.get_achieved());
        ip = update_ip.port;
        op = update_op.port;
        report_ir = false;
        report_oa = update_op.send_update;
      }
      if (got_outflow_request) {
        auto update_op = op.with_requested(outflow_request);
        auto update_ip = ip.with_requested(outflow_request);
        op = update_op.port;
        ip = update_ip.port;
        report_ir = update_ip.send_update;
        report_oa =
            (state.outflow_port.get_achieved() != op.get_achieved())
            && (op.get_requested() != op.get_achieved());
      }
    }
    else {
      auto update_ip = ip.with_requested(0.0);
      ip = update_ip.port;
      report_ir = update_ip.send_update;  
      if (got_outflow_request) {
        auto update_op_r = op.with_requested(outflow_request);
        auto update_op_a = update_op_r.port.with_achieved(0.0);
        op = update_op_a.port;
        report_oa = update_op_a.send_update;
      }
      else {
        auto update_op = op.with_achieved(0.0);
        op = update_op.port;
        report_oa = update_op.send_update;
      }
    }
    return OnOffSwitchState{
      new_time,
      state.state,
      state.next_index,
      ip,
      op,
      report_ir,
      report_oa,
    };
  }

  ////////////////////////////////////////////////////////////
  // confluent transition
  OnOffSwitchState
  on_off_switch_confluent_transition(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state,
      const std::vector<PortValue>& xs)
  {
    return on_off_switch_external_transition(
        on_off_switch_internal_transition(data, state), 0, xs);
  }

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  on_off_switch_output_function(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state)
  {
    std::vector<PortValue> ys{};
    on_off_switch_output_function_mutable(data, state, ys);
    return ys;
  }

  void 
  on_off_switch_output_function_mutable(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state,
      std::vector<PortValue>& ys)
  {
    if (state.report_inflow_request || state.report_outflow_achieved) {
      if (state.report_inflow_request) {
        // This would only happen after an external transition
        // We need to pass the request/achieved value through to outport
        if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
          std::cout << "switch:I(0) <- " << state.inflow_port.get_requested() << "\n";
        }
        ys.emplace_back(
            PortValue{
              outport_inflow_request,
              state.inflow_port.get_requested()});
      }
      if (state.report_outflow_achieved) {
        if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
          std::cout << "switch:O(0) -> " << state.outflow_port.get_achieved() << "\n";
        }
        ys.emplace_back(
            PortValue{
              outport_outflow_achieved,
              state.outflow_port.get_achieved()});
      }
    }
    else {
      auto dt = on_off_switch_time_advance(data, state);
      auto next_time = state.time + dt;
      // outputs fire BEFORE internal transition
      // if state is currently true, then next state will be false
      // therefore, we need to turn off any flows
      ERIN::FlowValueType flow{state.state ? 0.0 : state.outflow_port.get_requested()};
      auto ip = state.inflow_port.with_requested(flow).port;
      auto op = state.outflow_port.with_achieved(flow).port;
      if (ip.should_send_request(state.inflow_port)) {
        ys.emplace_back(
            PortValue{
            outport_inflow_request,
            flow});
      }
      if (op.should_send_achieved(state.outflow_port)) {
        ys.emplace_back(
            PortValue{
            outport_outflow_achieved,
            flow});
      }
    }
  }
}
