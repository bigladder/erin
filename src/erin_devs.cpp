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

  bool
  operator==(const FlowLimitsState& a, const FlowLimitsState& b)
  {
    return (a.time == b.time)
      && (a.inflow_port == b.inflow_port)
      && (a.outflow_port == b.outflow_port)
      && (a.lower_limit == b.lower_limit)
      && (a.upper_limit == b.upper_limit)
      && (a.report_inflow_request == b.report_inflow_request)
      && (a.report_outflow_achieved == b.report_outflow_achieved);
  }

  bool
  operator!=(const FlowLimitsState& a, const FlowLimitsState& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const FlowLimitsState& s)
  {
    os << "FlowLimitsState(time=" << s.time << ", "
       << "inflow_port=" << s.inflow_port << ", "
       << "outflow_port=" << s.outflow_port << ", "
       << "lower_limit=" << s.lower_limit << ", "
       << "upper_limit=" << s.upper_limit << ", "
       << "report_inflow_request=" << s.report_inflow_request << ", "
       << "report_outflow_achieved=" << s.report_outflow_achieved << ")";
    return os;
  }

  RealTimeType
  flow_limits_time_advance(const FlowLimitsState& state)
  {
    if (state.report_inflow_request || state.report_outflow_achieved) {
      return 0;
    }
    return infinity;
  }

  FlowLimitsState
  flow_limits_external_transition(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs)
  {
    bool got_outflow_request{false};
    bool got_inflow_achieved{false};
    FlowValueType outflow_request{0.0};
    FlowValueType inflow_achieved{0.0};
    for (const auto& x: xs) {
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
                << " in flow_limits_external_transition(...)";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    if (got_outflow_request) {
      // if we get an outflow request, even if we get a simultaneous inflow
      // achieved, we should just process the outflow request as the request takes
      // precedence.
      return flow_limits_external_transition_on_outflow_request(
          state, elapsed_time, outflow_request);
    }
    if (got_inflow_achieved) {
      return flow_limits_external_transition_on_inflow_achieved(
          state, elapsed_time, inflow_achieved);
    }
    return state;
  }

  FlowLimitsState
  flow_limits_external_transition_on_outflow_request(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      FlowValueType outflow_request)
  {
    const auto& ip = state.inflow_port;
    const auto& op = state.outflow_port;
    auto t = state.time + elapsed_time;
    auto inflow_request = std::clamp(outflow_request, state.lower_limit, state.upper_limit);
    auto new_ip = ip.with_requested_and_achieved(
        inflow_request,
        inflow_request,
        t);
    auto new_op = op.with_requested_and_achieved(
        outflow_request,
        inflow_request,
        t);
    return FlowLimitsState{
      t,
      new_ip,
      new_op,
      state.lower_limit,
      state.upper_limit,
      new_ip.should_propagate_request_at(t),
      new_op.should_propagate_achieved_at(t)
    };
  }

  FlowLimitsState
  flow_limits_external_transition_on_inflow_achieved(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      FlowValueType inflow_achieved)
  {
    const auto& ip = state.inflow_port;
    const auto& op = state.outflow_port;
    auto t = state.time + elapsed_time;
    auto new_ip = ip.with_achieved(inflow_achieved, t);
    auto new_op = op.with_achieved(inflow_achieved, t);
    return FlowLimitsState{
      t,
      new_ip,
      new_op,
      state.lower_limit,
      state.upper_limit,
      false,
      new_op.should_propagate_achieved_at(t)
    };
  }

  FlowLimitsState
  flow_limits_internal_transition(const FlowLimitsState& state)
  {
    return FlowLimitsState{
      state.time,
      state.inflow_port,
      state.outflow_port,
      state.lower_limit,
      state.upper_limit,
      false,
      false
    };
  }

  std::vector<PortValue>
  flow_limits_output_function(const FlowLimitsState& state)
  {
    std::vector<PortValue> out{};
    const auto& ip = state.inflow_port;
    const auto& op = state.outflow_port;
    if (state.report_inflow_request) {
      out.emplace_back(PortValue{outport_inflow_request, ip.get_requested()});
    }
    if (state.report_outflow_achieved) {
      out.emplace_back(PortValue{outport_outflow_achieved, op.get_achieved()});
    }
    return out;
  }
}
