/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/on_off_switch.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <typeindex>

namespace erin::devs
{
  bool
  operator==(const OnOffSwitchData& a, const OnOffSwitchData& b)
  {
    return (
        (a.schedule_times == b.schedule_times)
        && (a.schedule_values == b.schedule_values));
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
              << "schedule_times="
              << E::vec_to_string<RealTimeType>(a.schedule_times) << ", "
              << "schedule_values="
              << E::vec_to_string<FlowValueType>(a.schedule_values) << ")";
  }

  bool
  operator==(const OnOffSwitchState& a, const OnOffSwitchState& b)
  {
    return (
        (a.time == b.time)
        && (a.state == b.state)
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
              << "inflow_port=" << a.inflow_port << ", "
              << "outflow_port=" << a.outflow_port << ", " 
              << "report_inflow_request=" << a.report_inflow_request << ", "
              << "report_outflow_achieved=" << a.report_outflow_achieved << ")";
  }

  OnOffSwitchData
  make_on_off_switch_data(
      const std::vector<ERIN::TimeState>& schedule)
  {
    FlowValueType last_state{-1.0};
    std::vector<RealTimeType> times{};
    std::vector<FlowValueType> states{};
    for (const auto& item : schedule) {
      if ((last_state == -1.0) || (item.state != last_state)) {
        times.emplace_back(item.time);
        states.emplace_back(item.state);
      }
      last_state = item.state;
    }
    return OnOffSwitchData{
      times,        // schedule_times
      states        // schedule_values
    };
  }

  OnOffSwitchState
  make_on_off_switch_state(const OnOffSwitchData& data)
  {
    using stype = std::vector<RealTimeType>::size_type; 
    FlowValueType init_state{1.0};
    for (stype i{0}; i < data.schedule_times.size(); ++i) {
      if (data.schedule_times[i] <= 0) {
        init_state = data.schedule_values.at(i);
      }
      else {
        break;
      }
    }
    return OnOffSwitchState{
      0,            // time
      init_state,   // state
      Port{0, 0.0}, // inflow_port
      Port{0, 0.0}, // outflow_port
      false,
      false
    };
  }

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  on_off_switch_time_advance(
      const OnOffSwitchData& data,
      const OnOffSwitchState& state)
  {
    auto now = state.time;
    if (state.report_inflow_request || state.report_outflow_achieved) {
      return 0;
    }
    for (const auto& t : data.schedule_times) {
      if (t > now) {
        return (t - now);
      }
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
    using st = std::vector<RealTimeType>::size_type;
    auto dt = on_off_switch_time_advance(data, state);
    auto next_time = state.time + dt;
    auto next_state = state.state;
    for (st i{0}; i < data.schedule_times.size(); ++i) {
      const auto& t = data.schedule_times[i];
      if (t == next_time) {
        next_state = data.schedule_values[i];
        break;
      }
      if (t > next_time) {
        break;
      }
    }
    return OnOffSwitchState{
      next_time,
      next_state,
      state.inflow_port,
      state.outflow_port,
      false,
      false
    };
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
            std::ostringstream oss;
            oss << "unhandled port " << x.port
                << " in on_off_switch_external_transition(...)";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    const auto& t = state.time;
    auto new_time = t + elapsed_time;
    auto ip = state.inflow_port;
    auto op = state.outflow_port;
    if (state.state == 1.0) {
      if (got_inflow_achieved) {
        ip = ip.with_achieved(inflow_achieved, new_time);
        op = op.with_achieved(inflow_achieved, new_time);
      }
      if (got_outflow_request) {
        op = op.with_requested(outflow_request, new_time);
        ip = ip.with_requested(outflow_request, new_time);
      }
    }
    else if (state.state == 0.0) {
      ip = ip.with_requested(0.0, new_time);
      op = op.with_achieved(0.0, new_time);
    }
    else {
      std::ostringstream oss{};
      oss << "unsupported state value for on/off switch " << state.state
          << "; state should only be 0.0 or 1.0";
      throw std::invalid_argument(oss.str());
    }
    return OnOffSwitchState{
      new_time,
      state.state,
      ip,
      op,
      ip.should_propagate_request_at(new_time),
      op.should_propagate_achieved_at(new_time)
    };
  }

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  on_off_switch_output_function(const OnOffSwitchState& state)
  {
    std::vector<PortValue> ys{};
    on_off_switch_output_function_mutable(state, ys);
    return ys;
  }

  void 
  on_off_switch_output_function_mutable(
      const OnOffSwitchState& state,
      std::vector<PortValue>& ys)
  {
    if (state.report_inflow_request) {
      ys.emplace_back(
          PortValue{
            outport_inflow_request,
            state.inflow_port.get_requested()});
    }
    if (state.report_outflow_achieved) {
      ys.emplace_back(
          PortValue{
            outport_outflow_achieved,
            state.outflow_port.get_achieved()});
    }
  }
}
