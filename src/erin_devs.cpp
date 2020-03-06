/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs.h"
#include <algorithm>
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
    if (achieved > requested) {
      std::ostringstream oss;
      oss << "achieved more than requested error!\n"
          << "time_of_last_change : " << time_of_last_change << "\n"
          << "requested           : " << requested << "\n"
          << "achieved            : " << achieved << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  bool
  Port::should_propagate_request_at(RealTimeType time) const
  {
    return time == time_of_last_change;
  }

  bool
  Port::should_propagate_achieved_at(RealTimeType time) const
  {
    return (time == time_of_last_change) && (achieved != requested);
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
  Port::with_requested_and_achieved(
      FlowValueType new_request,
      FlowValueType new_achieved,
      RealTimeType time) const
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
      ((new_request == requested) && (new_request == new_achieved)) ? time_of_last_change : time,
      new_request,
      new_achieved};
  }

  Port
  Port::with_achieved(FlowValueType new_achieved, RealTimeType time) const
  {
    // when we set an achieved flow, we do not touch the request; we are still
    // requesting what we request regardless of what is achieved.
    auto t{time_of_last_change};
    if (achieved != new_achieved)
      t = time;
    return Port{t, requested, new_achieved};
  }

  bool
  operator==(const Port& a, const Port& b)
  {
    return (a.time_of_last_change == b.time_of_last_change)
        && (a.requested == b.requested)
        && (a.achieved == b.achieved);
  }

  bool
  operator!=(const Port& a, const Port& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const Port& p)
  {
    os << "Port("
       << "time_of_last_change=" << p.time_of_last_change << ", "
       << "requested=" << p.requested << ", "
       << "achieved=" << p.achieved << ")";
    return os;
  }
}
