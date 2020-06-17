/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/on_off_switch.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <typeindex>

namespace erin::devs
{
  OnOffSwitchState
  make_on_off_switch_state(
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
    return OnOffSwitchState{
      0,            // time
      Port{0, 0.0}, // inflow_port
      Port{0, 0.0}, // outflow_port
      times,        // schedule_times
      states        // schedule_values
    };
  }
}
