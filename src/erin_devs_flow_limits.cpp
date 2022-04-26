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
      std::ostringstream oss{};
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
    return os << "{"
              << ":lower-limit " << f.lower_limit
              << " :upper_limit " << f.upper_limit
              << "}";
  }

  FlowLimitsState
  make_flow_limits_state(
      FlowValueType lower_limit,
      FlowValueType upper_limit)
  {
    return make_flow_limits_state(
        0,
        Port3{lower_limit},
        Port3{lower_limit},
        lower_limit,
        upper_limit,
        false,
        false);
  }

  FlowLimitsState
  make_flow_limits_state(
      RealTimeType time,
      Port3 inflow_port,
      Port3 outflow_port,
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
    os << "{:t " << s.time << " "
       << ":inflow " << s.inflow_port << " "
       << ":outflow " << s.outflow_port << " "
       << ":limits " << s.limits << " "
       << ":report_ir? " << s.report_inflow_request << " "
       << ":report-oa? " << s.report_outflow_achieved << "}";
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
    if (has_reset_token(xs)) {
      return FlowLimitsState{
        state.time + elapsed_time,
        Port3{},
        Port3{},
        state.limits,
        false,
        false,
      };
    }
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
    auto ip = state.inflow_port;
    auto op = state.outflow_port;
    bool report_ir{state.report_inflow_request};
    bool report_oa{state.report_outflow_achieved};
    if (got_inflow_achieved) {
      auto update_ip = ip.with_achieved(inflow_achieved);
      report_ir = report_ir || update_ip.send_request;
      ip = update_ip.port;
      auto update_op = op.with_achieved(ip.get_achieved());
      report_oa = report_oa || update_op.send_achieved;
      op = update_op.port;
    }
    if (got_outflow_request) {
      auto update_op = op.with_requested(outflow_request);
      report_oa = report_oa || update_op.send_achieved;
      op = update_op.port;
      auto inflow_request = std::clamp(
        outflow_request,
        state.limits.get_lower_limit(),
        state.limits.get_upper_limit());
      auto update_ip = ip.with_requested(inflow_request);
      report_ir = report_ir || update_ip.send_request;
      ip = update_ip.port;
      update_op = op.with_achieved(ip.get_achieved());
      report_oa = report_oa || update_op.send_achieved;
      op = update_op.port;
    }
    return FlowLimitsState{
      state.time + elapsed_time,
      ip,
      op,
      state.limits,
      report_ir,
      report_oa,
    };
  }

  FlowLimitsState
  flow_limits_confluent_transition(
      const FlowLimitsState& state,
      const std::vector<PortValue>& xs)
  {
    return flow_limits_external_transition(
      flow_limits_internal_transition(state), 0, xs);
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
