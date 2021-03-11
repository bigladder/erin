/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs.h"
#include "debug_utils.h"
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <numeric>

namespace erin::devs
{
  Port::Port():
    Port(-1, 0.0, 0.0, false, false)
  {}

  Port::Port(RealTimeType t, FlowValueType r):
    Port(t, r, r, false, false)
  {
  }

  Port::Port(RealTimeType t, FlowValueType r, FlowValueType a):
    Port(t, r, a, false, false)
  {
  }

  Port::Port(
      RealTimeType t,
      FlowValueType r,
      FlowValueType a,
      bool r_prop,
      bool a_prop):
    time_of_last_change{t},
    requested{r},
    achieved{a},
    propagate_request{r_prop},
    propagate_achieved{a_prop}
  {
    if (achieved > requested) {
      std::ostringstream oss;
      oss << "achieved more than requested error!\n"
          << "time_of_last_change : " << time_of_last_change << "\n"
          << "requested           : " << requested << "\n"
          << "achieved            : " << achieved << "\n"
          << "propagate_request   : "
          << (propagate_request ? "true" : "false") << "\n"
          << "propagate_achieved  : "
          << (propagate_achieved ? "true" : "false") << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  bool
  Port::should_propagate_request_at(RealTimeType time) const
  {
    bool result = (time == time_of_last_change) && propagate_request;
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "Port::should_propagate_request_at(time=" << time << ") = ";
      std::cout << (result ? "true" : "false") << "\n";
    }
    return result;
  }

  bool
  Port::should_propagate_achieved_at(RealTimeType time) const
  {
    bool result = (time == time_of_last_change) && propagate_achieved;
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "Port::should_propagate_achieved_at(time=" << time << ") = ";
      std::cout << (result ? "true" : "false") << "\n";
    }
    return result;
  }

  Port
  Port::with_requested(FlowValueType new_request, RealTimeType time) const
  {
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "Port::with_requested("
                << "new_request=" << new_request << ", "
                << "time=" << time << ")\n";
    }
    // when we set a new request, we assume achieved is met until we hear
    // otherwise
    if (time < time_of_last_change) {
      std::ostringstream oss;
      oss << "invalid time argument: time flowing backwards...\n"
          << "time = " << time << "\n"
          << "time_of_last_change = " << time_of_last_change << "\n";
      throw std::invalid_argument(oss.str());
    }
    bool request_changed{std::abs(new_request - requested) > ERIN::flow_value_tolerance};
    return Port{
      request_changed ? time : time_of_last_change,
      new_request,
      request_changed? new_request : achieved,
      request_changed ? true : propagate_request,
      false};
  }

  Port
  Port::with_requested_and_achieved(
      FlowValueType new_request,
      FlowValueType new_achieved,
      RealTimeType time) const
  {
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "Port::with_requested_and_achieved("
                << "new_requested=" << new_request << ", "
                << "new_achieved=" << new_achieved << ", "
                << "time=" << time << ")\n";
    }
    if (time < time_of_last_change) {
      std::ostringstream oss;
      oss << "invalid time argument: time flowing backwards...\n"
          << "time = " << time << "\n"
          << "time_of_last_change = " << time_of_last_change << "\n";
      throw std::invalid_argument(oss.str());
    }
    bool is_same{
      (std::abs(new_request - requested) < ERIN::flow_value_tolerance)
      && (std::abs(new_achieved - achieved) < ERIN::flow_value_tolerance)};
    return Port{
      is_same ? time_of_last_change : time,
      new_request,
      new_achieved,
      is_same ? propagate_request : (new_request == new_achieved),
      is_same ? propagate_achieved : (new_request != new_achieved)};
  }

  Port
  Port::with_achieved(FlowValueType new_achieved, RealTimeType time) const
  {
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "Port::with_achieved("
                << "new_achieved=" << new_achieved << ", "
                << "time=" << time << ")\n";
    }
    // when we set an achieved flow, we do not touch the request; we are still
    // requesting what we request regardless of what is achieved.
    auto is_same{std::abs(achieved - new_achieved) < ERIN::flow_value_tolerance};
    return Port{
      is_same ? time_of_last_change : time,
      requested,
      new_achieved,
      false,
      is_same ? propagate_achieved : true};
  }

  bool
  operator==(const Port& a, const Port& b)
  {
    return (a.time_of_last_change == b.time_of_last_change)
        && (std::abs(a.requested - b.requested) < ERIN::flow_value_tolerance)
        && (std::abs(a.achieved - b.achieved) < ERIN::flow_value_tolerance);
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
       << "tL=" << p.time_of_last_change << ", "
       << "r=" << p.requested << ", "
       << "a=" << p.achieved << ", "
       << "prop-r?=" << p.propagate_request << ", "
       << "prop-a?=" << p.propagate_achieved << ")";
    return os;
  }
  
  // Port2
  Port2::Port2():
    Port2(0.0, 0.0)
  {
  }
  
  Port2::Port2(FlowValueType r):
    Port2(r, r)
  {
  }
  
  Port2::Port2(
    FlowValueType r,
    FlowValueType a
  ):
    requested{r},
    achieved{a}
  {
    if (requested < 0.0) {
      std::ostringstream oss{};
      oss << "requested flow is negative! Negative flows are not allowed\n"
          << "requested: " << requested << "\n"
          << "achieved : " << achieved << "\n";
      throw std::invalid_argument(oss.str());
    }
    if (achieved < 0.0) {
      std::ostringstream oss{};
      oss << "achieved flow is negative! Negative flows are not allowed\n"
          << "requested: " << requested << "\n"
          << "achieved : " << achieved << "\n";
      throw std::invalid_argument(oss.str());
    }
    if (achieved > requested) {
      if (std::abs(achieved - requested) < ERIN::flow_value_tolerance) {
        // precision issue; reset the achieved value
        achieved = requested;
      }
      else {
        std::ostringstream oss{};
        oss << "achieved more than requested error!\n"
            << "requested: " << requested << "\n"
            << "achieved : " << achieved << "\n";
        throw std::invalid_argument(oss.str());
      }
    }
  }

  PortUpdate
  Port2::with_requested(FlowValueType r) const
  {
    FlowValueType new_achieved = (achieved_is_limited() && (r > achieved))
        ? achieved
        : r;
    return {
      should_send_request_internal(r, requested),
      Port2{r, new_achieved},
    };
  }

  PortUpdate
  Port2::with_achieved(FlowValueType a) const
  {
    if (a > requested) {
      std::ostringstream oss{};
      oss << "achieved more than requested error!\n"
          << "requested: " << requested << "\n"
          << "achieved : " << achieved << "\n"
          << "new ach  : " << a << "\n";
      throw std::invalid_argument(oss.str());
    }
    return {
      should_send_achieved_internal(requested, a, requested, achieved),
      Port2{requested, a}
    };
  }
  
  bool
  Port2::should_send_request(const Port2& previous) const
  {
    return should_send_request_internal(requested, previous.requested);
  }
  
  bool
  Port2::should_send_achieved(const Port2& previous) const
  {
    /*
     * We send the achieved information if it hasn't previously been sent.
     * That will happen when
     * - the achieved value is different from previous
     * - AND request hasn't changed
     * 
     * If request has changed, we would only propagate achieved if it's changed
     * 
     * action     | c1-outflow | send achieved? 
     * ===========+============+===============
     * init       | R=0; A=0   | NA
     * IR=10      | R=10; A=10 | NA
     * OA=5       | R=10; A=5  | true1
     * IR=5+OA=6  | R=5; A=5   | false
     * IR=6+OA=4  | R=6; A=4   | true1
     * IR=4       | R=4; A=4   | NA
     * IR=4+OA=2  | R=4; A=2   | true1
     * IR=3+OA=2  | R=3; A=2   | false
     * IR=2+OA=2  | R=2; A=2   | false
     * IR=9+OA=2  | R=9; A=2   | true2 *
     * IR=7       | R=7; A=7   | NA
     * IR=6+OA=4  | R=6; A=4   | true1
     * IR=4       | R=4; A=4   | NA
     * IR=8+OA=4  | R=8; A=4   | true
     * IR=9       | R=9; A=4   | NA
     * IR=7       | R=7; A=4   | NA
     * OA=6       | R=7; A=6   | true1
     * OA=7       | R=7; A=7   | true1
     * IR=6       | R=6; A=6   | NA
     * OA=4       | R=6; A=4   | true1
     * IR=4       | R=4; A=4   | NA
     * OA=3       | R=4; A=3   | true1
     * IR=8+OA=4  | R=8; A=4   | true1
     * IR=5+OA=5  | R=5; A=5   | true1
     * IR=8       | R=8; A=8   | NA
     * OA=4       | R=8; A=4   | true1
     * OA=6       | R=8; A=6   | true1
     * IR=7+OA=7  | R=7; A=7   | true1 *
     * OA=6       | R=7; A=6   | true1
     * IR=6       | R=6; A=6   | NA
     * IR=3+OA=3  | R=3; A=3   | false
     * IR=8       | R=8; A=3   | NA
     * OA=6       | R=8; A=6   | true1
     * IR=5+OA=5  | R=5; A=5   | false
     * IR=8+OA=6  | R=8; A=6   | true
     * OA=8       | R=8; A=8   | true
     * IR=7+OA=7  | R=7; A=7   | false
     * IR=4+OA=4  | R=4; A=4   | false
    
    Propagate when achieved changed but requested didn't
    OR
    request changed but we now have a limit where we did not before
    */
    return should_send_achieved_internal(
        requested, achieved,
        previous.requested, previous.achieved);
  }

  bool
  Port2::should_send_achieved_internal(
          FlowValueType r, FlowValueType a,
          FlowValueType prev_r, FlowValueType prev_a) const
  {
    return ((prev_r == r) && (prev_a != a))
      || ((prev_r != r) && (r != a) && (prev_r == prev_a))
      || ((prev_r != r) && (r == a) && (a > prev_a))
      || ((prev_r != r) && (prev_r != prev_a) && (a < prev_a));
  }

  bool
  Port2::should_send_request_internal(
      FlowValueType r, FlowValueType prev_r) const
  {
    return (prev_r != r);
  }
  
  bool
  operator==(const Port2& a, const Port2& b)
  {
    return ((a.achieved == b.achieved) && (a.requested == b.requested));
  }

  bool operator!=(const Port2& a, const Port2& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const Port2& p)
  {
    return os << "Port2{r=" << p.requested << ",a=" << p.achieved << "}";
  }
  
  bool
  operator==(const PortUpdate& a, const PortUpdate& b)
  {
    return (a.send_update == b.send_update) && (a.port == b.port);
  }

  bool
  operator!=(const PortUpdate& a, const PortUpdate& b)
  {
    return !(a == b);
  }

  std::ostream& operator<<(std::ostream& os, const PortUpdate& p)
  {
    return os << "PortUpdate{update?=" << p.send_update
              << ", port=" << p.port << "}";
  }

  // Helper Functions
  // TODO: rename got_inflow_achieved as it shadows many local variables in external_transitions
  bool
  got_inflow_achieved(const std::vector<PortValue>& xs)
  {
    return std::any_of(xs.begin(), xs.end(),
        [](const auto& x) { return x.port == inport_inflow_achieved; });
  }

  FlowValueType
  total_inflow_achieved(const std::vector<PortValue>& xs)
  {
    return std::accumulate(xs.begin(), xs.end(), 0.0,
        [](const FlowValueType& sum, const auto& x) { 
          return sum + x.value;
        }
    );
  }
}
