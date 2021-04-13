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
    return should_send_achieved_internal(
        requested, achieved,
        previous.requested, previous.achieved);
  }

  bool
  Port2::should_send_achieved_internal(
          FlowValueType /* r */, FlowValueType a,
          FlowValueType /* prev_r */, FlowValueType prev_a) const
  {
    return (prev_a != a);
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

  ////////////////////////////////////////////////////////////
  // Port3
  Port3::Port3():
    Port3(0.0, 0.0)
  {
  }

  Port3::Port3(FlowValueType r):
    Port3(r, 0.0)
  {
  }

  Port3::Port3(
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
  }

  PortUpdate3
  Port3::with_requested(FlowValueType r) const
  {
    return PortUpdate3{
      Port3{r, achieved},
      r != requested,       // send_request
      false,                // send_achieved
    };
  }

  PortUpdate3
  Port3::with_achieved(FlowValueType a) const
  {
    return PortUpdate3{
      Port3{requested, a},
      a > requested,        // send_request
      a != achieved,        // send_achieved
    };
  }

  PortUpdate3
  Port3::with_requested_and_available(
      FlowValueType r,
      FlowValueType available) const
  {
    return with_requested_and_achieved(r, std::min(r, available));
  }

  PortUpdate3
  Port3::with_requested_and_achieved(
          FlowValueType r,
          FlowValueType a) const
  {
    auto update_r = with_requested(r);
    auto update_a = update_r.port.with_achieved(a);
    return PortUpdate3{
      update_a.port,
      update_r.send_request || update_a.send_request, // send_request
      update_a.send_achieved,                         // send_achieved
    };
  }

  bool
  operator==(const Port3& a, const Port3& b)
  {
    return (a.achieved == b.achieved) && (a.requested == b.requested);
  }

  bool
  operator!=(const Port3& a, const Port3& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const Port3& p)
  {
    return os << "{:r " << p.requested << ", :a " << p.achieved << "}";
  }

  bool
  operator==(const PortUpdate3& a, const PortUpdate3& b)
  {
    return (a.port == b.port)
      && (a.send_request == b.send_request)
      && (a.send_achieved == b.send_achieved);
  }

  bool
  operator!=(const PortUpdate3& a, const PortUpdate3& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const PortUpdate3& p)
  {
    return os << "{"
              << ":p " << p.port
              << " "
              << ":send-request? " << p.send_request
              << " "
              << ":send-achieved? " << p.send_achieved
              << "}";
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
