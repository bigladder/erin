/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs.h"
#include "erin/devs/flow_limits.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
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
  operator==(const FlowLimits& a, const FlowLimits& b)
  {
    return (a.lower_limit == b.lower_limit) && (a.upper_limit == b.upper_limit);
  }

  bool operator!=(const FlowLimits& a, const FlowLimits& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const FlowLimits& f)
  {
    return os << "FlowLimits("
              << "lower_limit=" << f.lower_limit
              << ", upper_limit=" << f.upper_limit
              << ")";
  }

  FlowLimitsState
  make_flow_limits_state(
      FlowValueType lower_limit,
      FlowValueType upper_limit)
  {
    return make_flow_limits_state(
        0,
        Port2{lower_limit, lower_limit},
        Port2{lower_limit, lower_limit},
        lower_limit,
        upper_limit,
        false,
        false);
  }

  FlowLimitsState
  make_flow_limits_state(
      RealTimeType time,
      Port2 inflow_port,
      Port2 outflow_port,
      FlowValueType lower_limit,
      FlowValueType upper_limit,
      bool report_inflow_request,
      bool report_outflow_achieved)
  {
    if (time < 0) {
      throw std::invalid_argument("time must be >= 0");
    }
    return FlowLimitsState{
      time,
      inflow_port,
      outflow_port,
      FlowLimits{lower_limit, upper_limit},
      report_inflow_request,
      report_outflow_achieved
    };
  }

  bool
  operator==(const FlowLimitsState& a, const FlowLimitsState& b)
  {
    return (a.time == b.time)
      && (a.inflow_port == b.inflow_port)
      && (a.outflow_port == b.outflow_port)
      && (a.limits == b.limits)
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
       << "limits=" << s.limits << ", "
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
  flow_limits_internal_transition(const FlowLimitsState& state)
  {
    return FlowLimitsState{
      state.time,
      state.inflow_port,
      state.outflow_port,
      state.limits,
      false,
      false
    };
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
            std::ostringstream oss{};
            oss << "unhandled port " << x.port
                << " in flow_limits_external_transition(...)";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    auto new_state{state};
    if (got_inflow_achieved) {
      new_state = flow_limits_external_transition_on_inflow_achieved(
          new_state, elapsed_time, inflow_achieved);
    }
    if (got_outflow_request) {
      new_state = flow_limits_external_transition_on_outflow_request(
          new_state, elapsed_time, outflow_request);
    }
    return new_state;
  }

  FlowLimitsState
  flow_limits_external_transition_on_outflow_request(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      FlowValueType outflow_request)
  {
    auto inflow_request = std::clamp(
        outflow_request,
        state.limits.get_lower_limit(),
        state.limits.get_upper_limit());
    auto ip_update = state.inflow_port.with_requested(inflow_request);
    auto op_update_r = state.outflow_port.with_requested(outflow_request);
    auto op_update_a = op_update_r.port.with_achieved(ip_update.port.get_achieved());
    return FlowLimitsState{
      state.time + elapsed_time,
      ip_update.port,
      op_update_a.port,
      state.limits,
      ip_update.send_update,
      op_update_a.send_update,
    };
  }

  FlowLimitsState
  flow_limits_external_transition_on_inflow_achieved(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      FlowValueType inflow_achieved)
  {
    auto ip_update = state.inflow_port.with_achieved(inflow_achieved);
    auto op_update = state.outflow_port.with_achieved(inflow_achieved);
    return FlowLimitsState{
      state.time + elapsed_time,
      ip_update.port,
      op_update.port,
      state.limits,
      false,
      op_update.send_update,
    };
  }

  FlowLimitsState
  flow_limits_confluent_transition(
      const FlowLimitsState& state,
      const std::vector<PortValue>& xs)
  {
    auto next_state = flow_limits_internal_transition(state);
    if (state.report_inflow_request && got_inflow_achieved(xs) &&
        (total_inflow_achieved(xs) > state.inflow_port.get_requested()))
    {
      return next_state;
    }
    return flow_limits_external_transition(next_state, 0, xs);
  }

  std::vector<PortValue>
  flow_limits_output_function(const FlowLimitsState& state)
  {
    std::vector<PortValue> ys{};
    flow_limits_output_function_mutable(state, ys);
    return ys;
  }

  void
  flow_limits_output_function_mutable(const FlowLimitsState& state, std::vector<PortValue>& ys)
  {
    if (state.report_inflow_request) {
      ys.emplace_back(
          PortValue{
              outport_inflow_request,
              state.inflow_port.get_requested(),
          });
    }
    if (state.report_outflow_achieved) {
      ys.emplace_back(
        PortValue{
          outport_outflow_achieved,
          state.outflow_port.get_achieved()});
    }
  }
}
