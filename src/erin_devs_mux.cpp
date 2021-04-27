/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs/mux.h"
#include "debug_utils.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
  void
  print_ports(
      const std::vector<Port3>::size_type idx,
      const std::string& tag,
      const RealTimeType time,
      const Port3& flow,
      const Port3& new_flow)
  {
    std::cout << tag << "(" << idx << ") @ t = " << time
              << " (" << flow.get_requested()
              << ", " << flow.get_achieved()
              << ") -->"
              << " (" << new_flow.get_requested()
              << ", " << new_flow.get_achieved()
              << ")\n";
  }

  void
  print_flows(
      const std::string& tag,
      const RealTimeType time,
      const std::vector<Port3>& flows,
      const std::vector<Port3>& new_flows)
  {
    if (flows.size() != new_flows.size()) {
      std::cout << "FLOW SIZES DON'T MATCH!!!\n"
                << "flows.size() = " << flows.size() << "\n"
                << "new_flows.size() = " << new_flows.size() << "\n";
      return;
    }
    using st = std::vector<Port3>::size_type;
    for (st idx{0}; idx < flows.size(); ++idx) {
      print_ports(idx, tag, time, flows.at(idx), new_flows.at(idx));
    }
  }

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
      const std::vector<bool>& report_irs,
      const std::vector<bool>& report_oas)
  {
    for (const auto& flag : report_irs) {
      if (flag) {
        return true;
      }
    }
    for (const auto& flag : report_oas) {
      if (flag) {
        return true;
      }
    }
    return false;
  }
  
  void
  assert_gte_zero(FlowValueType amount)
  {
    if (amount < 0.0) {
      std::ostringstream oss{};
      oss << "amount must be >= 0.0\n"
          << "amount = " << amount << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  std::vector<PortUpdate3>
  distribute_inflow_to_outflow_in_order(
      const std::vector<Port3>& outflows,
      FlowValueType amount)
  {
    using size_type = std::vector<Port>::size_type;
    assert_gte_zero(amount);
    std::vector<PortUpdate3> updates{};
    FlowValueType total_requested{0.0};
    FlowValueType remaining_supply{amount};
    for (size_type idx{0}; idx < outflows.size(); ++idx) {
      const auto& op = outflows[idx];
      auto request{op.get_requested()};
      total_requested += request;
      decltype(request) this_request{0.0};
      if (request >= remaining_supply) {
        this_request = remaining_supply;
        remaining_supply = 0.0;
      } else {
        this_request = request;
        remaining_supply -= request;
      }
      updates.emplace_back(op.with_achieved(this_request));
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
    return updates;
  }

  std::vector<PortUpdate3>
  distribute_inflow_to_outflow_evenly(
      const std::vector<Port3>& outflows,
      FlowValueType amount)
  {
    using size_type = std::vector<Port>::size_type;
    assert_gte_zero(amount);
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
    constexpr int max_iter{1000};
    while ((num_live > 0) && (amount_remaining > ERIN::flow_value_tolerance)) {
      n += 1;
      if (n > max_iter) {
        std::ostringstream oss{};
        oss << "breaking out of infinite loop in "
            << "distribute_inflow_to_outflow_evenly!\n"
            << "n = " << n << "\n"
            << "num_live = " << num_live << "\n"
            << "amount_remaining = " << amount_remaining << "\n";
        throw std::invalid_argument(oss.str());
      }
      even_flow = amount_remaining / static_cast<FlowValueType>(num_live);
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
    std::vector<PortUpdate3> updates{};
    for (size_type idx{0}; idx < num_outflows; ++idx) {
      const auto& of = outflows[idx];
      auto supply{outflow_supplies[idx]};
      updates.emplace_back(of.with_achieved(supply));
    }
    return updates;
  }

  bool
  achieved_values_changed(const Port& outflow, const Port& new_outflow)
  {
    const auto diff{
      std::abs(outflow.get_achieved() - new_outflow.get_achieved())};
    return diff > ERIN::flow_value_tolerance;
  }

  bool
  iteration_detected(
      const std::vector<Port>& outflows,
      const std::vector<Port>& new_outflows,
      FlowValueType amount,
      RealTimeType time)
  {
    using st = std::vector<Port>::size_type;
    bool any_changed_again_this_time{false};
    FlowValueType total_outflow_request{0.0};
    for (st idx{0}; idx < outflows.size(); ++idx) {
      const auto& of = outflows[idx];
      if (achieved_values_changed(of, new_outflows[idx])) {
        any_changed_again_this_time = any_changed_again_this_time
          || (time == of.get_time_of_last_change());
      }
      total_outflow_request += of.get_requested();
    }
    bool same_total_request{
      std::abs(amount - total_outflow_request) < ERIN::flow_value_tolerance};
    return any_changed_again_this_time && same_total_request;
  }

  std::vector<PortUpdate3>
  distribute_inflow_to_outflow(
      MuxerDispatchStrategy outflow_strategy,
      const std::vector<Port3>& outflows,
      FlowValueType amount)
  {
    std::vector<Port> new_outflows{};
    if (outflow_strategy == MuxerDispatchStrategy::InOrder) {
      return distribute_inflow_to_outflow_in_order(outflows, amount);
    }
    else if (outflow_strategy == MuxerDispatchStrategy::Distribute) {
      return distribute_inflow_to_outflow_evenly(outflows, amount);
    }
    std::ostringstream oss{};
    oss << "unhandled muxer dispatch strategy "
        << static_cast<int>(outflow_strategy) << "\n";
    throw std::invalid_argument(oss.str());
  }
  
  std::vector<PortUpdate3>
  request_inflows_intelligently(
      const std::vector<Port3>& inflows,
      FlowValueType remaining_request)
  {
    std::vector<PortUpdate3> updates{};
    for (std::size_t idx{0}; idx < inflows.size(); ++idx) {
      auto update = inflows[idx].with_requested(remaining_request);
      remaining_request -= update.port.get_achieved();
      updates.emplace_back(update);
      if (remaining_request < ERIN::flow_value_tolerance) {
        remaining_request = 0.0;
      }
    }
    return updates;
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

  // TODO: change name to muxer_dispatch_strategy_to_tag
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
      std::vector<Port3>(num_inflows),
      std::vector<Port3>(num_outflows),
      std::vector<bool>(num_inflows, false),
      std::vector<bool>(num_outflows, false),
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
    return std::accumulate(
        state.inflow_ports.begin(), state.inflow_ports.end(), 0.0,
        [](const auto& s, const auto& p) { return s + p.get_requested(); });
  }

  FlowValueType
  mux_get_outflow_request(const MuxState& state)
  {
    return std::accumulate(
        state.outflow_ports.begin(), state.outflow_ports.end(), 0.0,
        [](const auto& s, const auto& p) { return s + p.get_requested(); });
  }

  FlowValueType
  mux_get_inflow_achieved(const MuxState& state)
  {
    return std::accumulate(
        state.inflow_ports.begin(), state.inflow_ports.end(), 0.0,
        [](const auto& s, const auto& p) { return s + p.get_achieved(); });
  }

  FlowValueType
  mux_get_outflow_achieved(const MuxState& state)
  {
    return std::accumulate(
        state.outflow_ports.begin(), state.outflow_ports.end(), 0.0,
        [](const auto& s, const auto& p) { return s + p.get_achieved(); });
  }
  
  std::ostream&
  operator<<(std::ostream& os, const MuxState& s)
  {
    return os << "{:t " << s.time
              << " :num-inflows " << s.num_inflows
              << " :num-outflows " << s.num_outflows 
              << " :inflow_ports " << ERIN::vec_to_string<Port3>(s.inflow_ports)
              << " :outflow_ports " << ERIN::vec_to_string<Port3>(s.outflow_ports)
              << " :report_irs " << ERIN::vec_to_string<bool>(s.report_irs)
              << " :report_oas " << ERIN::vec_to_string<bool>(s.report_oas)
              << " :strategy " << muxer_dispatch_strategy_to_string(s.outflow_strategy)
              << "}";
  }

  RealTimeType
  mux_time_advance(const MuxState& state)
  {
    if (mux_should_report(state.report_irs, state.report_oas)) {
      return 0;
    }
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
      std::vector<bool>(state.num_inflows, false),
      std::vector<bool>(state.num_outflows, false),
      state.outflow_strategy};
  }

  MuxState
  mux_external_transition(
      const MuxState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs)
  {
    using st = std::vector<bool>::size_type;
    auto time{state.time + dt};
    const FlowValueType none_value{-1.0};
    std::vector<FlowValueType> inflows(state.num_inflows, none_value);
    std::vector<FlowValueType> outflows(state.num_outflows, none_value);
    for (const auto& x : xs) {
      int port = x.port;
      int port_n_ia = port - inport_inflow_achieved;
      int port_n_or = port - inport_outflow_request;
      int port_n{-1};
      if ((port_n_ia >= 0) && (port_n_ia < state.num_inflows)) {
        port_n = port_n_ia;
        if (inflows[port_n] == none_value) {
          inflows[port_n] = x.value;
        }
        else {
          inflows[port_n] += x.value;
        }
      }
      else if ((port_n_or >= 0) && (port_n_or < state.num_outflows)) {
        port_n = port_n_or;
        if (outflows[port_n] == none_value) {
          outflows[port_n] = x.value;
        }
        else {
          outflows[port_n] += x.value;
        }
      }
      else {
        std::ostringstream oss{};
        oss << "BadPortError: unhandled port: \"" << x.port << "\"";
        throw std::runtime_error(oss.str());
      }
    }
    std::vector<bool> report_irs(state.num_inflows, false);
    std::vector<bool> report_oas(state.num_outflows, false);
    auto new_ips{state.inflow_ports};
    for (st idx{0}; idx < inflows.size(); idx++) {
      if (inflows[idx] != none_value) {
        auto update = new_ips[idx].with_achieved(inflows[idx]);
        report_irs[idx] = report_irs[idx] || update.send_request;
        new_ips[idx] = update.port;
        inflows[idx] = update.port.get_achieved();
      }
    }
    auto new_ops{state.outflow_ports};
    for (st idx{0}; idx < outflows.size(); idx++) {
      if (outflows[idx] != none_value) {
        auto update = new_ops[idx].with_requested(outflows[idx]);
        report_oas[idx] = report_oas[idx] || update.send_achieved;
        new_ops[idx] = update.port;
      }
    }
    FlowValueType total_outflow_request = std::accumulate(
        new_ops.begin(), new_ops.end(), 0.0,
        [](const auto& s, const auto& p) { return s + p.get_requested(); }
    );
    auto ip_updates = request_inflows_intelligently(
        new_ips, total_outflow_request);
    for (st idx{0}; idx < static_cast<st>(state.num_inflows); idx++) {
      new_ips[idx] = ip_updates[idx].port;
      report_irs[idx] = report_irs[idx] || ip_updates[idx].send_request;
    }
    FlowValueType the_total_inflow_achieved = std::accumulate(
        new_ips.begin(), new_ips.end(), 0.0,
        [](const auto& s, const auto& p) { return s + p.get_achieved(); });
    auto op_updates = distribute_inflow_to_outflow(
        state.outflow_strategy,
        new_ops,
        the_total_inflow_achieved);
    for (st idx{0}; idx < static_cast<st>(state.num_outflows); idx++) {
      new_ops[idx] = op_updates[idx].port;
      auto achieved_changed{
        state.outflow_ports[idx].get_achieved() != new_ops[idx].get_achieved()};
      report_oas[idx] =
        op_updates[idx].send_achieved
        || achieved_changed;
    }
    return MuxState{
      time,
      state.num_inflows,
      state.num_outflows,
      std::move(new_ips),
      std::move(new_ops),
      std::move(report_irs),
      std::move(report_oas),
      state.outflow_strategy};
  }

  MuxState
  mux_confluent_transition(
      const MuxState& state,
      const std::vector<PortValue>& xs)
  {
    return mux_external_transition(
        mux_internal_transition(state), 0, xs);
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
      const auto& flag = state.report_irs[idx];
      if (flag) {
        ys.emplace_back(
            PortValue{
              outport_inflow_request + static_cast<int>(idx), 
              ip.get_requested()});
      }
    }
    for (size_type idx{0}; idx < state.outflow_ports.size(); ++idx) {
      const auto& op = state.outflow_ports[idx];
      const auto& flag = state.report_oas[idx];
      if (flag) {
        ys.emplace_back(
            PortValue{
              outport_outflow_achieved + static_cast<int>(idx),
              op.get_achieved()});
      }
    }
  }
}
