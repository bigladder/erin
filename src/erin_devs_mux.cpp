/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/mux.h"
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
  void
  check_num_flows(const std::string& tag, int n)
  {
    if ((n < minimum_number_of_ports) || (n > maximum_number_of_ports)) {
      std::ostringstream oss{};
      oss << tag << " must be >= " << minimum_number_of_ports
          << " and <= " << maximum_number_of_ports << "\n"
          << tag << " = " << n << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  MuxState
  make_mux_state(int num_inflows, int num_outflows)
  {
    check_num_flows("num_inflows", num_inflows);
    check_num_flows("num_outflows", num_outflows);
    return MuxState{
      0,
      num_inflows,
      num_outflows,
      std::vector<Port>(num_inflows),
      std::vector<Port>(num_outflows)};
  }

  RealTimeType
  mux_current_time(const MuxState& state)
  {
    return state.time;
  }

  RealTimeType
  mux_time_advance(const MuxState& state)
  {
    bool report{false};
    const auto& t = state.time;
    for (const auto& ip : state.inflow_ports) {
      if (ip.should_propagate_request_at(t)) {
        report = true;
        break;
      }
    }
    if (!report) {
      for (const auto& op : state.outflow_ports) {
        if (op.should_propagate_achieved_at(t)) {
          report = true;
          break;
        }
      }
    }
    if (report)
      return 0;
    return infinity;
  }

  std::vector<Port>
  distribute_inflow_to_outflow_in_order(
      const std::vector<Port>& outflows,
      FlowValueType amount,
      RealTimeType time)
  {
    using size_type = std::vector<Port>::size_type;
    if (amount < 0.0) {
      std::ostringstream oss{};
      oss << "amount must be >= 0.0\n"
          << "amount = " << amount << "\n";
      throw std::invalid_argument(oss.str());
    }
    auto new_outflows{outflows};
    FlowValueType total_requested{0.0};
    FlowValueType total_supply{amount};
    for (size_type idx{0}; idx < outflows.size(); ++idx) {
      const auto& op = outflows[idx];
      auto request{op.get_requested()};
      total_requested += request;
      if (request >= amount) {
        new_outflows[idx] = op.with_achieved(amount, time);
        amount = 0.0;
      } else if (request < amount) {
        new_outflows[idx] = op.with_achieved(request, time);
        amount -= request;
      }
    }
    if (amount != 0.0) {
      std::ostringstream oss{};
      oss << "inflow amount was greater than total requested\n"
          << "total requested: " << total_requested << "\n"
          << "total supply:    " << total_supply << "\n";
      throw std::runtime_error(oss.str());
    }
    return new_outflows;
  }

  std::vector<Port>
  request_difference_from_next_highest_inflow_port(
      const std::vector<Port> inflow_ports,
      int idx_of_request,
      FlowValueType request,
      RealTimeType time)
  {
    auto num_inflows = static_cast<int>(inflow_ports.size());
    if ((idx_of_request < 0) || (idx_of_request >= num_inflows)) {
      std::ostringstream oss{};
      oss << "index of request must be >= 0 or < inflow_ports.size()\n"
          << "idx_of_request: " << idx_of_request << "\n"
          << "inflow_ports.size(): " << num_inflows << "\n";
      throw std::invalid_argument(oss.str());
    }
    auto new_inflows{inflow_ports};
    for (int idx{idx_of_request}; idx < num_inflows; ++idx) {
      if (idx == idx_of_request)
        new_inflows[idx] = new_inflows[idx].with_requested(request, time);
      else
        new_inflows[idx] = new_inflows[idx].with_requested(0.0, time);
    }
    return new_inflows;
  }

  std::vector<Port>
  rerequest_inflows_in_order(
      const std::vector<Port>& inflow_ports,
      FlowValueType total_outflow_request,
      RealTimeType time)
  {
    using size_type = std::vector<Port>::size_type;
    auto new_inflows{inflow_ports};
    new_inflows[0] = inflow_ports[0].with_requested(total_outflow_request, time);
    for (size_type idx{1}; idx < inflow_ports.size(); ++idx)
      new_inflows[idx] = inflow_ports[idx].with_requested(0.0, time);
    return new_inflows;
  }

  MuxState
  mux_external_transition(
      const MuxState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs)
  {
    auto time{state.time + dt};
    auto inflow_ports{state.inflow_ports};
    auto outflow_ports{state.outflow_ports};
    int highest_inflow_port_received{-1};
    for (const auto& x : xs) {
      int port = x.port;
      int port_n_ia = port - inport_inflow_achieved;
      int port_n_or = port - inport_outflow_request;
      int port_n{-1};
      if ((port_n_ia >= 0) && (port_n_ia < state.num_inflows)) {
        port_n = port_n_ia;
        if (port_n > highest_inflow_port_received)
          highest_inflow_port_received = port_n;
        inflow_ports[port_n] = inflow_ports[port_n].with_achieved(x.value, time);
      }
      else if ((port_n_or >= 0) && (port_n_or < state.num_outflows)) {
        port_n = port_n_or;
        outflow_ports[port_n] = outflow_ports[port_n].with_requested(x.value, time);
      }
      else {
        std::ostringstream oss;
        oss << "BadPortError: unhandled port: \"" << x.port << "\"";
        throw std::runtime_error(oss.str());
      }
    }
    FlowValueType total_inflow_achieved{0.0};
    for (const auto& ip : inflow_ports)
      total_inflow_achieved += ip.get_achieved();
    FlowValueType total_outflow_request{0.0};
    for (const auto& op : outflow_ports)
      total_outflow_request += op.get_requested();
    auto diff{total_inflow_achieved - total_outflow_request};
    if (diff > ERIN::flow_value_tolerance) {
      // oversupplying... need to re-request to inflows so they give
      // less. Restart requests from port zero.
      inflow_ports = rerequest_inflows_in_order(
          inflow_ports, total_outflow_request, time);
    } else if (diff < ERIN::neg_flow_value_tol) {
      // undersupplying... need to re-request to inflows for more unless
      // we've already heard from the highest port
      if (highest_inflow_port_received >= (state.num_inflows - 1)) {
        outflow_ports = distribute_inflow_to_outflow_in_order(
            outflow_ports, total_inflow_achieved, time);
      } else {
        inflow_ports = request_difference_from_next_highest_inflow_port(
            inflow_ports,
            highest_inflow_port_received + 1,
            (-1 * diff),
            time);
      }
    } else {
      // diff ~= 0.0, redistribute outflows just in case
      outflow_ports = distribute_inflow_to_outflow_in_order(
          outflow_ports, total_inflow_achieved, time);
    }
    return MuxState{
      time,
      state.num_inflows,
      state.num_outflows,
      std::move(inflow_ports),
      std::move(outflow_ports)};
  }

  std::vector<PortValue>
  mux_output_function(const MuxState& state)
  {
    std::vector<PortValue> ys{};
    mux_output_function_mutable(state, ys);
    return ys;
  }

  void
  mux_output_function_mutable(
      const MuxState& state,
      std::vector<PortValue>& ys)
  {
    using size_type = std::vector<PortValue>::size_type;
    for (size_type idx{0}; idx < state.inflow_ports.size(); ++idx) {
      const auto& ip = state.inflow_ports[idx];
      if (ip.should_propagate_request_at(state.time))
        ys.emplace_back(
            PortValue{
              outport_inflow_request + static_cast<int>(idx), 
              ip.get_requested()});
    }
    for (size_type idx{0}; idx < state.outflow_ports.size(); ++idx) {
      const auto& op = state.outflow_ports[idx];
      if (op.should_propagate_achieved_at(state.time))
        ys.emplace_back(
            PortValue{
              outport_outflow_achieved + static_cast<int>(idx),
              op.get_achieved()});
    }
  }
}
