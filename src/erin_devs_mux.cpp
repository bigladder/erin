/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs/mux.h"
#include "debug_utils.h"
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
  void
  mux_check_num_flows(const std::string& tag, int n)
  {
    if ((n < minimum_number_of_ports) || (n > maximum_number_of_ports)) {
      std::ostringstream oss{};
      oss << tag << " must be >= " << minimum_number_of_ports
          << " and <= " << maximum_number_of_ports << "\n"
          << tag << " = " << n << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  bool
  mux_should_report(
      RealTimeType time,
      const std::vector<Port>& inflow_ports,
      const std::vector<Port>& outflow_ports)
  {
    for (const auto& ip : inflow_ports) {
      if (ip.should_propagate_request_at(time)) {
        return true;
      }
    }
    for (const auto& op : outflow_ports) {
      if (op.should_propagate_achieved_at(time)) {
        return true;
      }
    }
    return false;
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
    FlowValueType remaining_supply{amount};
    for (size_type idx{0}; idx < outflows.size(); ++idx) {
      const auto& op = outflows[idx];
      auto request{op.get_requested()};
      total_requested += request;
      if (request >= remaining_supply) {
        new_outflows[idx] = op.with_achieved(remaining_supply, time);
        remaining_supply = 0.0;
      } else {
        new_outflows[idx] = op.with_achieved(request, time);
        remaining_supply -= request;
      }
    }
    if (remaining_supply < 0.0) {
      remaining_supply = 0.0;
    }
    if ((remaining_supply != 0.0) && (remaining_supply > ERIN::flow_value_tolerance)) {
      std::ostringstream oss{};
      oss << "inflow amount was greater than total requested\n"
          << "total requested : " << total_requested << "\n"
          << "total supply    : " << amount << "\n"
          << "remaining supply: " << remaining_supply << "\n";
      throw std::runtime_error(oss.str());
    }
    return new_outflows;
  }

  std::vector<Port>
  distribute_inflow_to_outflow_evenly(
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
    auto num_outflows{outflows.size()};
    auto num_live{num_outflows};
    FlowValueType even_flow{0.0};
    std::vector<FlowValueType> outflow_supplies(num_live, 0.0);
    std::vector<FlowValueType> outflow_requests(num_live, 0.0);
    std::vector<Port> new_outflows{};
    FlowValueType total_requested{0.0};
    for (size_type idx{0}; idx < num_outflows; ++idx) {
      auto req = outflows[idx].get_requested();
      total_requested += req;
      outflow_requests[idx] = req;
    }
    if (amount > total_requested) {
      std::ostringstream oss{};
      oss << "amount delivered is greater than total requested outflow!\n"
          << "amount delivered: " << amount << "\n"
          << "total_requested : " << total_requested << "\n";
      throw std::invalid_argument(oss.str());
    }
    FlowValueType amount_remaining{amount};
    int n{0};
    constexpr int max_iter{100};
    while ((num_live > 0) && (amount_remaining > 0.0)) {
      n += 1;
      if (n > max_iter) {
        std::ostringstream oss{};
        oss << "breaking out of infinite loop in "
            << "distribute_inflow_to_outflow_evenly!\n";
        throw std::invalid_argument(oss.str());
      }
      even_flow = amount_remaining / num_live;
      num_live = 0;
      for (size_type i{0}; i < num_outflows; ++i) {
        auto s = outflow_supplies[i] + even_flow;
        auto r = outflow_requests[i];
        if (s > r) {
          outflow_supplies[i] = r;
          amount_remaining += ((s - r) - even_flow);
        }
        else if (s == r) {
          outflow_supplies[i] = s;
          amount_remaining -= even_flow;
        }
        else if (s < r) {
          outflow_supplies[i] = s;
          amount_remaining -= even_flow;
          num_live += 1;
        }
      }
    }
    for (size_type idx{0}; idx < num_outflows; ++idx) {
      const auto& of = outflows[idx];
      new_outflows.emplace_back(of.with_achieved(outflow_supplies[idx], time));
    }
    return new_outflows;
  }

  std::vector<Port>
  distribute_inflow_to_outflow(
      MuxerDispatchStrategy outflow_strategy,
      const std::vector<Port>& outflows,
      FlowValueType amount,
      RealTimeType time)
  {
    const auto& outflow_ports{outflows};
    if (outflow_strategy == MuxerDispatchStrategy::InOrder) {
      return distribute_inflow_to_outflow_in_order(
          outflow_ports, amount, time);
    }
    else if (outflow_strategy == MuxerDispatchStrategy::Distribute) {
      return distribute_inflow_to_outflow_evenly(
          outflow_ports, amount, time);
    }
    std::ostringstream oss{};
    oss << "unhandled muxer dispatch strategy "
        << static_cast<int>(outflow_strategy) << "\n";
    throw std::invalid_argument(oss.str());
  }

  std::vector<Port>
  request_difference_from_next_highest_inflow_port(
      const std::vector<Port>& inflow_ports,
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
    int tgt_idx = idx_of_request;
    FlowValueType remaining_request{request};
    for (int idx{idx_of_request}; idx < num_inflows; ++idx) {
      if (idx == tgt_idx) {
        new_inflows[idx] = new_inflows[idx].with_requested(
            remaining_request, time);
        auto achieved{new_inflows[idx].get_achieved()};
        auto diff{std::abs(achieved - remaining_request)};
        if (diff > ERIN::flow_value_tolerance) {
          // This can happen if we're re-requesting the same amount we already
          // asked for previously. In that case, the achieved value is already
          // known without asking upstream again and the new achieved is
          // already set. We must check if we're deficient and propagate the
          // remaining request upstream until we find a port that will satisfy
          // it.
          ++tgt_idx;
        }
        remaining_request -= achieved;
        if (remaining_request <= ERIN::flow_value_tolerance) {
          remaining_request = 0.0;
        }
      }
      else {
        new_inflows[idx] = new_inflows[idx].with_requested(0.0, time);
      }
    }
    return new_inflows;
  }

  std::vector<Port>
  request_inflows_intelligently(
      const std::vector<Port>& inflow_ports,
      FlowValueType& remaining_request,
      RealTimeType time)
  {
    using size_type = std::vector<Port>::size_type;
    auto new_inflows{inflow_ports};
    for (size_type idx{0}; idx < new_inflows.size(); ++idx) {
      auto achieved = new_inflows[idx].get_achieved();
      auto requested = new_inflows[idx].get_requested();
      auto tolc = new_inflows[idx].get_time_of_last_change();
      if ((tolc == time)
          && (achieved < remaining_request)
          && (achieved < requested)) {
        new_inflows[idx] = new_inflows[idx].with_requested_and_achieved(
            remaining_request, achieved, time);
      }
      else {
        new_inflows[idx] = new_inflows[idx].with_requested(
            remaining_request, time);
      }
      remaining_request -= new_inflows[idx].get_achieved();
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
    for (size_type idx{1}; idx < inflow_ports.size(); ++idx) {
      new_inflows[idx] = inflow_ports[idx].with_requested(0.0, time);
    }
    return new_inflows;
  }

  MuxerDispatchStrategy
  tag_to_muxer_dispatch_strategy(const std::string& tag)
  {
    if (tag == "in_order") {
      return MuxerDispatchStrategy::InOrder;
    }
    else if (tag == "distribute") {
      return MuxerDispatchStrategy::Distribute;
    }
    std::ostringstream oss;
    oss << "unhandled tag '" << tag << "' for Muxer_dispatch_strategy\n";
    throw std::runtime_error(oss.str());
  }

  std::string
  muxer_dispatch_strategy_to_string(MuxerDispatchStrategy mds)
  {
    switch (mds) {
      case MuxerDispatchStrategy::InOrder:
        return std::string{"in_order"};
      case MuxerDispatchStrategy::Distribute:
        return std::string{"distribute"};
    }
    std::ostringstream oss;
    oss << "unhandled Muxer_dispatch_strategy '"
      << static_cast<int>(mds) << "'\n";
    throw std::runtime_error(oss.str());
  }

  MuxState
  make_mux_state(
      int num_inflows,
      int num_outflows,
      MuxerDispatchStrategy strategy)
  {
    mux_check_num_flows("num_inflows", num_inflows);
    mux_check_num_flows("num_outflows", num_outflows);
    return MuxState{
      0,
      num_inflows,
      num_outflows,
      std::vector<Port>(num_inflows),
      std::vector<Port>(num_outflows),
      false,
      strategy};
  }

  RealTimeType
  mux_current_time(const MuxState& state)
  {
    return state.time;
  }

  FlowValueType
  mux_get_inflow_request(const MuxState& state)
  {
    FlowValueType sum{0.0};
    for (const auto& p : state.inflow_ports) {
      sum += p.get_requested();
    }
    return sum;
  }

  FlowValueType
  mux_get_outflow_request(const MuxState& state)
  {
    FlowValueType sum{0.0};
    for (const auto& p : state.outflow_ports) {
      sum += p.get_requested();
    }
    return sum;
  }

  FlowValueType
  mux_get_inflow_achieved(const MuxState& state)
  {
    FlowValueType sum{0.0};
    for (const auto& p : state.inflow_ports) {
      sum += p.get_achieved();
    }
    return sum;
  }

  FlowValueType
  mux_get_outflow_achieved(const MuxState& state)
  {
    FlowValueType sum{0.0};
    for (const auto& p : state.outflow_ports) {
      sum += p.get_achieved();
    }
    return sum;
  }

  RealTimeType
  mux_time_advance(const MuxState& state)
  {
    if (state.do_report)
      return 0;
    return infinity;
  }

  MuxState
  mux_internal_transition(const MuxState& state)
  {
    return MuxState{
      state.time,
      state.num_inflows,
      state.num_outflows,
      state.inflow_ports,
      state.outflow_ports,
      false,
      state.outflow_strategy};
  }

  MuxState
  mux_external_transition(
      const MuxState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs)
  {
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "mux_external_transition(\n"
                << "  state=...,\n"
                << "  dt=" << dt << ",\n"
                << "  xs=" << ERIN::vec_to_string<PortValue>(xs) << ")\n";
    }
    auto time{state.time + dt};
    auto inflow_ports{state.inflow_ports};
    auto outflow_ports{state.outflow_ports};
    bool got_outflow{false};
    int highest_inflow_port_received{-1};
    for (const auto& x : xs) {
      int port = x.port;
      int port_n_ia = port - inport_inflow_achieved;
      int port_n_or = port - inport_outflow_request;
      int port_n{-1};
      if ((port_n_ia >= 0) && (port_n_ia < state.num_inflows)) {
        port_n = port_n_ia;
        if (port_n > highest_inflow_port_received) {
          highest_inflow_port_received = port_n;
        }
        inflow_ports[port_n] = inflow_ports[port_n].with_achieved(x.value, time);
      }
      else if ((port_n_or >= 0) && (port_n_or < state.num_outflows)) {
        port_n = port_n_or;
        outflow_ports[port_n] = outflow_ports[port_n].with_requested(x.value, time);
        got_outflow = true;
      }
      else {
        std::ostringstream oss{};
        oss << "BadPortError: unhandled port: \"" << x.port << "\"";
        throw std::runtime_error(oss.str());
      }
    }
    FlowValueType total_inflow_achieved{0.0};
    for (const auto& ip : inflow_ports) {
      total_inflow_achieved += ip.get_achieved();
    }
    FlowValueType total_outflow_request{0.0};
    for (const auto& op : outflow_ports) {
      total_outflow_request += op.get_requested();
    }
    auto diff{total_inflow_achieved - total_outflow_request};
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
      std::cout << "... total_inflow_achieved: "
                << total_inflow_achieved << "\n"
                << "... total_outflow_request: "
                << total_outflow_request << "\n"
                << "... diff                 : " << diff << "\n"
                << "... got_outflow          : "
                << (got_outflow ? "true" : "false") << "\n"
                << "... highest_inflow_port_received: "
                << highest_inflow_port_received << "\n";
    }
    if (diff > ERIN::flow_value_tolerance) {
      // oversupplying... need to re-request to inflows so they give
      // less. Restart requests from port zero.
      if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
        std::cout << "...oversupplying\n";
      }
      inflow_ports = rerequest_inflows_in_order(
          inflow_ports, total_outflow_request, time);
      outflow_ports = distribute_inflow_to_outflow(
          state.outflow_strategy, outflow_ports, total_outflow_request, time);
    } else if (diff < ERIN::neg_flow_value_tol) {
      if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
        std::cout << "...undersupplying\n";
      }
      if (got_outflow) {
        // undersupplying... got a new requested outflow, rerequest inflows
        inflow_ports = rerequest_inflows_in_order(
            inflow_ports, total_outflow_request, time);
        outflow_ports = distribute_inflow_to_outflow(
            state.outflow_strategy, outflow_ports, total_outflow_request, time);
      } else {
        // undersupplying... need to re-request to inflows for more unless
        // we've already heard from the highest port
        if (highest_inflow_port_received >= (state.num_inflows - 1)) {
          if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
            std::cout << "...distributing inflow to outflow\n";
          }
          outflow_ports = distribute_inflow_to_outflow(
              state.outflow_strategy, outflow_ports, total_inflow_achieved, time);
        } else {
          if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
            std::cout << "...requesting difference from "
                      << "... next highest inflow port\n"
                      << "... next highest inflow port: "
                      << (highest_inflow_port_received + 1) << "\n"
                      << "... requested amount: " << (-1 * diff) << "\n";
          }
          inflow_ports = request_difference_from_next_highest_inflow_port(
              inflow_ports,
              highest_inflow_port_received + 1,
              (-1 * diff),
              time);
          total_inflow_achieved = 0.0;
          for (const auto& ip : inflow_ports) {
            total_inflow_achieved += ip.get_achieved();
          }
          if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
            std::cout << "... updated total_inflow_achieved: "
                      << total_inflow_achieved << "\n";
          }
          outflow_ports = distribute_inflow_to_outflow(
              state.outflow_strategy, outflow_ports, total_inflow_achieved, time);
        }
      }
    } else {
      if constexpr (ERIN::debug_level >= ERIN::debug_level_high) {
        std::cout << "...inflows equal outflows\n";
      }
      // diff ~= 0.0, redistribute outflows just in case
      outflow_ports = distribute_inflow_to_outflow(
          state.outflow_strategy, outflow_ports, total_outflow_request, time);
    }
    bool do_report = mux_should_report(time, inflow_ports, outflow_ports);
    return MuxState{
      time,
      state.num_inflows,
      state.num_outflows,
      std::move(inflow_ports),
      std::move(outflow_ports),
      do_report,
      state.outflow_strategy};
  }

  MuxState
  mux_confluent_transition(
      const MuxState& state,
      const std::vector<PortValue>& xs)
  {
    auto dt = mux_time_advance(state);
    auto s0 = mux_external_transition(state, dt, xs);
    auto s1 = mux_internal_transition(s0);
    bool do_report = mux_should_report(s1.time, s1.inflow_ports, s1.outflow_ports);
    return MuxState{
      s1.time,
      s1.num_inflows,
      s1.num_outflows,
      std::move(s1.inflow_ports),
      std::move(s1.outflow_ports),
      do_report,
      state.outflow_strategy};
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
      if (ip.should_propagate_request_at(state.time)) {
        ys.emplace_back(
            PortValue{
              outport_inflow_request + static_cast<int>(idx), 
              ip.get_requested()});
      }
    }
    for (size_type idx{0}; idx < state.outflow_ports.size(); ++idx) {
      const auto& op = state.outflow_ports[idx];
      if (op.should_propagate_achieved_at(state.time)) {
        ys.emplace_back(
            PortValue{
              outport_outflow_achieved + static_cast<int>(idx),
              op.get_achieved()});
      }
    }
  }
}
