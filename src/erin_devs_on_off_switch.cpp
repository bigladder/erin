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
  operator==(const OnOffSwitchState& a, const OnOffSwitchState& b)
  {
    return (
        (a.time == b.time)
        && (a.state == b.state)
        && (a.inflow_port == b.inflow_port)
        && (a.outflow_port == b.outflow_port)
        && (a.schedule_times == b.schedule_times)
        && (a.schedule_values == b.schedule_values));
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
              << "schedule_times=" 
              << E::vec_to_string<RealTimeType>(a.schedule_times) << ", "
              << "schedule_values="
              << E::vec_to_string<FlowValueType>(a.schedule_values) << ")";
  }

  OnOffSwitchState
  make_on_off_switch_state(
      const std::vector<ERIN::TimeState>& schedule)
  {
    FlowValueType last_state{-1.0};
    std::vector<RealTimeType> times{};
    std::vector<FlowValueType> states{};
    FlowValueType init_state{1.0};
    for (const auto& item : schedule) {
      if (item.time <= 0) {
        init_state = item.state;
      }
      if ((last_state == -1.0) || (item.state != last_state)) {
        times.emplace_back(item.time);
        states.emplace_back(item.state);
      }
      last_state = item.state;
    }
    return OnOffSwitchState{
      0,            // time
      init_state,   // state
      Port{0, 0.0}, // inflow_port
      Port{0, 0.0}, // outflow_port
      times,        // schedule_times
      states        // schedule_values
    };
  }

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  on_off_switch_time_advance(const OnOffSwitchState& state)
  {
    auto now = state.time;
    for (const auto& t : state.schedule_times) {
      if (t > now) {
        return (t - now);
      }
    }
    return infinity;
  }

}
