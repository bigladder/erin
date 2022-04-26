/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs.h"
#include "erin/devs/flow_meter.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // state
  bool
  operator==(const FlowMeterState& a, const FlowMeterState& b)
  {
      return
        (a.time == b.time) && (a.port == b.port)
        && (a.report_inflow_request == b.report_inflow_request)
        && (a.report_outflow_achieved == b.report_outflow_achieved);
  }

  bool
  operator!=(const FlowMeterState& a, const FlowMeterState& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const FlowMeterState& s)
  {
    return os << "{:t " << s.time
              << " "
              << ":p " << s.port
              << " "
              << ":report-ir? " << s.report_inflow_request
              << " "
              << ":report-oa? " << s.report_outflow_achieved
              << "}";
  }

  FlowMeterState
  flow_meter_make_state()
  {
      return FlowMeterState{0, Port3{}, false, false};
  }

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  flow_meter_time_advance(const FlowMeterState& state)
  {
    auto dt{infinity};
    if (state.report_outflow_achieved || state.report_inflow_request) {
      dt = 0;
    }
    return dt;
  }

  ////////////////////////////////////////////////////////////
  // internal transition
  FlowMeterState
  flow_meter_internal_transition(const FlowMeterState& state)
  {
    return FlowMeterState{state.time, state.port, false, false};
  }

  ////////////////////////////////////////////////////////////
  // external transition
  FlowMeterState
  flow_meter_external_transition(
      const FlowMeterState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs)
  {
    if (has_reset_token(xs)) {
      return FlowMeterState{
        state.time + elapsed_time,
        Port3{},
        false,
        false};
    }
    bool got_ia{false};
    bool got_or{false};
    FlowValueType inflow_achieved{0.0};
    FlowValueType outflow_request{0.0};
    for (const auto& x : xs) {
      switch (x.port) {
        case inport_outflow_request:
          {
            got_or = true;
            outflow_request += x.value;
            break;
          }
        case inport_inflow_achieved:
          {
            got_ia = true;
            inflow_achieved += x.value;
            break;
          }
        default:
          {
            std::ostringstream oss{};
            oss << "unhandled port " << x.port << "\n"
                << "- value " << x.value << "\n"
                << "- from flow_meter_external_transition\n"
                << "- s " << state << "\n"
                << "- xs " << ERIN::vec_to_string<PortValue>(xs) << "\n";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    auto p{state.port};
    PortUpdate3 update{p, false, false};
    if (got_ia && got_or) {
      update = p.with_requested_and_achieved(outflow_request, inflow_achieved);
    }
    else if (got_ia) {
      update = p.with_achieved(inflow_achieved);
    }
    else if (got_or) {
      update = p.with_requested(outflow_request);
    }
    return FlowMeterState{
      state.time + elapsed_time,
      update.port,
      update.send_request,
      update.send_achieved};
  }

  ////////////////////////////////////////////////////////////
  // confluent transition
  FlowMeterState
  flow_meter_confluent_transition(
      const FlowMeterState& state,
      const std::vector<PortValue>& xs)
  {
    return flow_meter_external_transition(
      flow_meter_internal_transition(state), 0, xs);
  }

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  flow_meter_output_function(const FlowMeterState& state)
  {
    std::vector<PortValue> ys{};
    flow_meter_output_function_mutable(state, ys);
    return ys;
  }

  void 
  flow_meter_output_function_mutable(
    const FlowMeterState& state,
    std::vector<PortValue>& ys)
  {
    if (state.report_inflow_request) {
      ys.emplace_back(PortValue{outport_inflow_request, state.port.get_requested()});
    }
    if (state.report_outflow_achieved) {
      ys.emplace_back(PortValue{outport_outflow_achieved, state.port.get_achieved()});
    }
    return;
  }
}