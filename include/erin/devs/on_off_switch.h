/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_ON_OFF_SWITCH_H
#define ERIN_DEVS_ON_OFF_SWITCH_H
#include "erin/devs.h"
#include "erin/type.h"
#include "erin/reliability.h"
#include <functional>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

namespace erin::devs
{
  struct OnOffSwitchState
  {
    RealTimeType time{0};
    Port inflow_port{0, 0.0, 0.0};
    Port outflow_port{0, 0.0, 0.0};
    std::vector<RealTimeType> schedule_times{};
    std::vector<FlowValueType> schedule_values{};
  };

  //bool operator==(const OnOffSwitchState& a, const OnOffSwitchState& b);
  //bool operator!=(const OnOffSwitchState& a, const OnOffSwitchState& b);
  //std::ostream& operator<<(std::ostream& os, const OnOffSwitchState& a);

  OnOffSwitchState
  make_on_off_switch_state(const std::vector<ERIN::TimeState>& schedule);


}

#endif // ERIN_DEVS_ON_OFF_SWITCH_H
