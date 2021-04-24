/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

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
    return os << "{:cop " << a.COP << "}";
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
    return os << "{"
              << ":t " << a.time << " "
              << ":ip0 " << a.inflow0_port << " "
              << ":ip1 " << a.inflow1_port << " "
              << ":op " << a.outflow_port << " "
              << ":send-ir0? " << a.report_inflow0_request << " "
              << ":send-ir1? " << a.report_inflow1_request << " "
              << ":send-oa? " << a.report_outflow_achieved << "}";
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
    bool got_or{false};
    bool got_ia0{false};
    bool got_ia1{false};
    FlowValueType outflow_request{0.0};
    FlowValueType inflow0_achieved{0.0};
    FlowValueType inflow1_achieved{0.0};
    constexpr int inport0_inflow_achieved{inport_inflow_achieved};
    constexpr int inport1_inflow_achieved{inport_inflow_achieved + 1};
    for (const auto& x: xs) {
      switch (x.port) {
        case inport_outflow_request:
          {
            got_or = true;
            outflow_request += x.value;
            break;
          }
        case inport0_inflow_achieved:
          {
            got_ia0 = true;
            inflow0_achieved += x.value;
            break;
          }
        case inport1_inflow_achieved:
          {
            got_ia1 = true;
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
    auto ip0{state.inflow0_port};
    auto ip1{state.inflow1_port};
    auto op{state.outflow_port};
    bool send_ir0{false};
    bool send_ir1{false};
    bool send_oa{false};
    if (got_ia0) {
      auto update_ip0 = ip0.with_achieved(inflow0_achieved);
      ip0 = update_ip0.port;
      send_ir0 = send_ir0 || update_ip0.send_request;
    }
    if (got_ia1) {
      auto update_ip1 = ip1.with_achieved(inflow1_achieved);
      ip1 = update_ip1.port;
      send_ir1 = send_ir1 || update_ip1.send_request;
    }
    if (got_ia0 || got_ia1) {
      auto inflow1_by_ip0{ip0.get_achieved() * (1.0 / data.COP)};
      auto outflow_by_ip0{(1.0 + (1.0 / data.COP)) * ip0.get_achieved()};
      auto inflow0_by_ip1{ip1.get_achieved() * data.COP};
      auto outflow_by_ip1{(1.0 + data.COP) * ip1.get_achieved()};
      auto dominant_outflow{0.0};
      if (outflow_by_ip0 < outflow_by_ip1) {
        dominant_outflow = outflow_by_ip0;
        auto update_ip1 = ip1.with_requested(inflow1_by_ip0);
        ip1 = update_ip1.port;
        send_ir1 = send_ir1 || update_ip1.send_request;
      }
      else {
        dominant_outflow = outflow_by_ip1;
        auto update_ip0 = ip0.with_requested(inflow0_by_ip1);
        ip0 = update_ip0.port;
        send_ir0 = send_ir0 || update_ip0.send_request;
      }
      auto update_op = op.with_achieved(dominant_outflow);
      op = update_op.port;
      send_oa = send_oa || update_op.send_achieved;
    }
    if (got_or) {
      auto update_op = op.with_requested(outflow_request);
      op = update_op.port;
      send_oa = send_oa || update_op.send_achieved;
      auto inflow0 = outflow_request / (1.0 + (1.0 / data.COP));
      auto inflow1 = outflow_request / (1.0 + data.COP);
      auto update_ip0 = ip0.with_requested(inflow0);
      ip0 = update_ip0.port;
      send_ir0 = send_ir0 || update_ip0.send_request;
      auto update_ip1 = ip1.with_requested(inflow1);
      ip1 = update_ip1.port;
      send_ir1 = send_ir1 || update_ip1.send_request;
    }
    return MoverState{
      new_time,
      ip0,
      ip1,
      op,
      send_ir0 || ip0.should_send_request(state.inflow0_port),
      send_ir1 || ip1.should_send_request(state.inflow1_port),
      send_oa || op.should_send_achieved(state.outflow_port)};
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
