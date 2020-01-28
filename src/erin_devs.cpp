/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs.h"
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
  Port::Port():
    Port(-1, 0.0, 0.0)
  {}

  Port::Port(RealTimeType t, FlowValueType r):
    Port(t, r, r)
  {
  }

  Port::Port(RealTimeType t, FlowValueType r, FlowValueType a):
    time_of_last_change{t},
    requested{r},
    achieved{a}
  {
  }

  bool
  Port::should_propagate_request_at(RealTimeType time) const
  {
    return time == time_of_last_change;
  }

  bool
  Port::should_propagate_achieved_at(RealTimeType /* time */) const
  {
    return false;
  }

  Port
  Port::with_requested(FlowValueType new_request, RealTimeType time) const
  {
    // when we set a new request, we assume achieved is met until we hear
    // otherwise
    if (time < time_of_last_change) {
      std::ostringstream oss;
      oss << "invalid time argument: time flowing backwards...\n"
          << "time = " << time << "\n"
          << "time_of_last_change = " << time_of_last_change << "\n";
      throw std::invalid_argument(oss.str());
    }
    return Port{
      new_request == requested ? time_of_last_change : time,
      new_request};
  }

  Port
  Port::with_achieved(FlowValueType new_achieved, RealTimeType /* time */) const
  {
    // when we set an achieved flow, we do not touch the request; we are still
    // requesting what we request regardless of what is achieved.
    // if achieved is more than requested, that is an error.
    //if (new_achieved > requested) {
    //  std::ostringstream oss;
    //  oss << "achieved more than requested error!\n"
    //      << "new_achieved: " << new_achieved << "\n"
    //      << "requested   : " << requested << "\n"
    //      << "achieved    : " << achieved << "\n";
    //  throw std::invalid_argument(oss.str());
    //}
    return Port{time_of_last_change, requested, new_achieved};
  }

  ////////////////////////////////////////////////////////////
  // FlowLimits
  FlowLimits::FlowLimits(
      FlowValueType lower_limit_,
      FlowValueType upper_limit_):
    lower_limit{lower_limit_},
    upper_limit{upper_limit_}
  {
    if (lower_limit > upper_limit) {
      std::ostringstream oss;
      oss << "FlowLimits error: lower_limit (" << lower_limit
          << ") > upper_limit (" << upper_limit << ")";
      throw std::invalid_argument(oss.str());
    }
  }
}
