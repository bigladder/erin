
/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/mover.h"
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
  MoverData
  make_mover_data(FlowValueType COP)
  {
    if (COP <= 0.0) {
      std::ostringstream oss{};
      oss << "COP must be > 0.0 but is " << COP << ".\n";
      throw std::runtime_error(oss.str());
    }
    return MoverData{COP};
  }

  bool
  operator==(const MoverData& a, const MoverData& b)
  {
    return (a.COP == b.COP);
  }

  bool
  operator!=(const MoverData& a, const MoverData& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const MoverData& a)
  {
    return os << "MoverData(COP = " << a.COP << ")";
  }

  MoverState
  make_mover_state()
  {
    return MoverState{};
  }

  bool
  operator==(const MoverState& a, const MoverState& b)
  {
    return (a.time == b.time) &&
           (a.inflow0_port == b.inflow0_port) &&
           (a.inflow1_port == b.inflow1_port) &&
           (a.outflow_port == b.outflow_port) &&
           (a.report_inflow0_request == b.report_inflow0_request) &&
           (a.report_inflow1_request == b.report_inflow1_request) &&
           (a.report_outflow_achieved == b.report_outflow_achieved);
  }

  bool
  operator!=(const MoverState& a, const MoverState& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const MoverState& a)
  {
    return os << "MoverState("
              << "time = " << a.time << ", "
              << "inflow0_port = " << a.inflow0_port << ", "
              << "inflow1_port = " << a.inflow1_port << ", "
              << "outflow_port = " << a.outflow_port << ", "
              << "report_inflow0_request = " << a.report_inflow0_request << ", "
              << "report_inflow1_request = " << a.report_inflow1_request << ", "
              << "report_outflow_achieved = "
              << a.report_outflow_achieved << ")";
  }

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  mover_time_advance(
      const MoverData& /* data */,
      const MoverState& state)
  {
    RealTimeType dt{0};
    if (state.report_inflow0_request ||
        state.report_inflow1_request ||
        state.report_outflow_achieved) {
      dt = 0;
    }
    else {
      dt = infinity;
    }
    return dt;
  }

  ////////////////////////////////////////////////////////////
  // internal transition
  MoverState
  mover_internal_transition(
      const MoverData& /* data */,
      const MoverState& state)
  {
    return MoverState{
      state.time, // internal transitions alsways take 0 time
      state.inflow0_port,
      state.inflow1_port,
      state.outflow_port,
      false,
      false,
      false,
    };
  }

  ////////////////////////////////////////////////////////////
  // external transition
  MoverState
  mover_external_transition(
      const MoverData& data,
      const MoverState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs)
  {
    bool got_outflow_request{false};
    bool got_inflow0_achieved{false};
    bool got_inflow1_achieved{false};
    FlowValueType outflow_request{0.0};
    FlowValueType inflow0_achieved{0.0};
    FlowValueType inflow1_achieved{0.0};
    constexpr int inport0_inflow_achieved{inport_inflow_achieved};
    constexpr int inport1_inflow_achieved{inport_inflow_achieved + 1};
    for (const auto& x: xs) {
      switch (x.port) {
        case inport_outflow_request:
          {
            got_outflow_request = true;
            outflow_request += x.value;
            break;
          }
        case inport0_inflow_achieved:
          {
            got_inflow0_achieved = true;
            inflow0_achieved += x.value;
            break;
          }
        case inport1_inflow_achieved:
          {
            got_inflow1_achieved = true;
            inflow1_achieved += x.value;
            break;
          }
        default:
          {
            std::ostringstream oss{};
            oss << "unhandled port " << x.port
                << " in mover_external_transition(...)";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    auto new_time = state.time + elapsed_time;
    auto p_in0 = state.inflow0_port;
    auto p_in1 = state.inflow1_port;
    auto p_out = state.outflow_port;
    if (got_outflow_request) {
      p_out = p_out.with_requested(outflow_request, new_time);
      auto inflow0 = outflow_request / (1.0 + (1.0 / data.COP));
      auto inflow1 = outflow_request / (1.0 + data.COP);
      p_in0 = p_in0.with_requested(inflow0, new_time);
      p_in1 = p_in1.with_requested(inflow1, new_time);
    }
    else if (got_inflow0_achieved && got_inflow1_achieved) {
      p_in0 = p_in0.with_achieved(inflow0_achieved, new_time);
      p_in1 = p_in1.with_achieved(inflow1_achieved, new_time);
      auto calc_of_inflow0 = inflow1_achieved * data.COP;
      if (calc_of_inflow0 > inflow0_achieved) {
        p_in1 = p_in1.with_achieved(
            inflow0_achieved * (1.0 / data.COP), new_time);
      }
      else if (calc_of_inflow0 < inflow0_achieved) {
        p_in0 = p_in0.with_achieved(calc_of_inflow0, new_time);
      }
      p_out = p_out.with_achieved(
          p_in1.get_achieved() * (1.0 + data.COP), new_time);
    }
    else if (got_inflow0_achieved) {
      p_in0 = p_in0.with_achieved(inflow0_achieved, new_time);
      p_in1 = p_in1.with_achieved(
          inflow0_achieved * (1.0 / data.COP), new_time);
      p_out = p_out.with_achieved(
          p_in0.get_achieved() * (1.0 + (1.0 / data.COP)), new_time);
    }
    else if (got_inflow1_achieved) {
      p_in0 = p_in0.with_achieved(inflow1_achieved * data.COP, new_time);
      p_in1 = p_in1.with_achieved(inflow1_achieved, new_time);
      p_out = p_out.with_achieved(
          inflow1_achieved * (1.0 + data.COP), new_time);
    }
    return MoverState{
      new_time,
      p_in0,
      p_in1,
      p_out,
      p_in0.should_propagate_request_at(new_time),
      p_in1.should_propagate_request_at(new_time),
      p_out.should_propagate_achieved_at(new_time)
    };
  }

  ////////////////////////////////////////////////////////////
  // confluent transition
  MoverState
  mover_confluent_transition(
      const MoverData& data,
      const MoverState& state,
      const std::vector<PortValue>& xs)
  {
    return mover_external_transition(
        data, mover_internal_transition(data, state), 0, xs);
  }

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  mover_output_function(const MoverData& d, const MoverState& s)
  {
    std::vector<PortValue> ys{};
    mover_output_function_mutable(d, s, ys);
    return ys;
  }

  void
  mover_output_function_mutable(
      const MoverData& /* data */,
      const MoverState& state,
      std::vector<PortValue>& ys)
  {
    if (state.report_inflow0_request) {
      ys.emplace_back(
          PortValue{
            outport_inflow_request,
            state.inflow0_port.get_requested()});
    }
    if (state.report_inflow1_request) {
      ys.emplace_back(
          PortValue{
            outport_inflow_request + 1,
            state.inflow1_port.get_requested()});
    }
    if (state.report_outflow_achieved) {
      ys.emplace_back(
          PortValue{
            outport_outflow_achieved,
            state.outflow_port.get_achieved()});
    }
  }
}
