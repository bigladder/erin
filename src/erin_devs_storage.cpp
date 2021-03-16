/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs/storage.h"
#include "debug_utils.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace erin::devs
{
  RealTimeType
  time_to_next_soc_event(
      FlowValueType net_inflow,
      FlowValueType capacity,
      FlowValueType current_soc)
  {
    if (std::abs(net_inflow) < ERIN::flow_value_tolerance) {
      return infinity;
    }
    if (net_inflow > ERIN::flow_value_tolerance) {
      return static_cast<RealTimeType>(
          std::floor(calc_time_to_fill(current_soc, capacity, net_inflow)));
    }
    // net_inflow < (-ERIN::flow_value_tolerance)
    return static_cast<RealTimeType>(
        std::floor(
          calc_time_to_drain(
            current_soc, capacity, -1.0 * net_inflow)));
  }

  FlowValueType
  max_single_step_net_inflow(double soc, double capacity)
  {
    return capacity * (1.0 - soc); // divided by 1.0 second
  }

  FlowValueType
  max_single_step_net_outflow(double soc, double capacity)
  {
    return capacity * soc; // divided by 1.0 second
  }

  bool
  storage_is_full(double soc)
  {
    return (std::abs(1.0 - soc) <= ERIN::flow_value_tolerance);
  }

  bool
  storage_is_empty(double soc)
  {
    return (std::abs(soc) <= ERIN::flow_value_tolerance);
  }

  double
  calc_time_to_fill(double soc, double capacity, double inflow)
  {
    assert_positive<double>(
        inflow,
        "inflow in calc_time_to_fill must be positive");
    auto available_cap{(1.0 - soc) * capacity};
    return (available_cap / inflow);
  }

  double calc_time_to_drain(
      double soc,
      double capacity,
      double outflow)
  {
    assert_positive<double>(
        outflow, "outflow in calc_time_to_drain must be > 0");
    auto available_store{soc * capacity};
    return (available_store / outflow);
  }

  void
  storage_check_soc(const FlowValueType& soc)
  {
    if ((soc > 1.0) || (soc < 0.0)) {
      std::ostringstream oss{};
      oss << "soc must be >= 0.0 and <= 1.0\n"
          << "soc = " << soc << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  storage_check_flow(FlowValueType flow)
  {
    if (flow < 0.0) {
      std::ostringstream oss{};
      oss << "invalid flow: flow must be >= 0.0\n"
          << "flow = " << flow << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  storage_check_elapsed_time(RealTimeType dt)
  {
    if (dt < 0) {
      std::ostringstream oss{};
      oss << "dt must by >= 0\n"
          << "dt = " << dt << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  double
  update_soc(
      double soc,
      FlowValueType inflow_achieved,
      FlowValueType outflow_achieved,
      RealTimeType dt,
      FlowValueType capacity)
  {
    assert_fraction<double>(
        soc, "soc in update_soc");
    assert_non_negative<FlowValueType>(
        inflow_achieved, "inflow_achieved in update_soc");
    assert_non_negative<FlowValueType>(
        outflow_achieved, "outflow_achieved in update_soc");
    assert_non_negative<RealTimeType>(
        dt, "dt in update_soc");
    assert_positive<FlowValueType>(
        capacity, "capacity in update_soc");
    auto net_inflow{inflow_achieved - outflow_achieved};
    auto cap_change{net_inflow * dt};
    auto soc_change{cap_change / capacity};
    auto next_soc{soc + soc_change};
    if (storage_is_full(next_soc) || (next_soc > 1.0)) {
      return 1.0;
    }
    if (storage_is_empty(next_soc) || (next_soc < 0.0)) {
      return 0.0;
    }
    return next_soc;
  }

  StorageState
  storage_external_transition_on_outflow_request(
      const StorageData& data,
      const StorageState& state,
      FlowValueType outflow_request,
      RealTimeType dt,
      RealTimeType time)
  {
    storage_check_flow(outflow_request);
    storage_check_elapsed_time(dt);
    bool report_ir{false};
    bool report_oa{false};
    auto ip{state.inflow_port};
    auto op = state.outflow_port.with_requested(outflow_request).port;
    auto soc = update_soc(
        state.soc,
        state.inflow_port.get_achieved(),
        state.outflow_port.get_achieved(),
        dt,
        data.capacity);
    if (soc == 1.0) {
      auto update_ip = ip.with_requested(
          std::clamp(outflow_request, 0.0, data.max_charge_rate));
      report_ir = update_ip.send_update;
      ip = update_ip.port;
    }
    else {
      auto update_ip = ip.with_requested(data.max_charge_rate);
      report_ir = update_ip.send_update;
      ip = update_ip.port;
    }
    if (soc == 0.0) {
      auto update_op = op.with_achieved(
          std::clamp(
            outflow_request,
            0.0,
            ip.get_achieved()));
      report_oa = update_op.send_update;
      op = update_op.port;
    }
    return StorageState{
      time,
      soc,
      ip,
      op,
      report_ir,
      report_oa};
  }

  StorageState
  storage_external_transition_on_inflow_achieved(
      const StorageData& data,
      const StorageState& state,
      FlowValueType inflow_achieved,
      RealTimeType dt,
      RealTimeType time)
  {
    storage_check_flow(inflow_achieved);
    storage_check_elapsed_time(dt);
    auto ip{state.inflow_port};
    auto op{state.outflow_port};
    bool report_ir{false};
    auto inflow_requested{state.inflow_port.get_requested()};
    if (inflow_achieved > inflow_requested) {
      inflow_achieved = inflow_requested;
      report_ir = true;
    }
    ip = ip.with_achieved(inflow_achieved).port;
    auto soc = update_soc(
        state.soc,
        state.inflow_port.get_achieved(),
        state.outflow_port.get_achieved(),
        dt,
        data.capacity);
    auto ip_ach{ip.get_achieved()};
    auto op_ach{op.get_achieved()};
    if ((soc == 1.0) && (ip_ach > op_ach)) {
      ip = ip.with_requested(op_ach).port;
    }
    if ((soc == 0.0) && (op_ach > ip_ach)) {
      op = op.with_achieved(
          std::clamp(ip_ach, 0.0, op.get_requested())).port;
    }
    return StorageState{
      time,
      soc,
      ip,
      op,
      report_ir || ip.should_send_request(state.inflow_port),
      op.should_send_achieved(state.outflow_port)};
  }

  StorageState
  storage_external_transition_on_in_out_flow(
      const StorageData& data,
      const StorageState& state,
      FlowValueType outflow_request,
      FlowValueType inflow_achieved,
      RealTimeType dt,
      RealTimeType time)
  {
    auto ip{state.inflow_port};
    auto op{state.outflow_port};
    bool report_ir{false};
    auto inflow_request{state.inflow_port.get_requested()};
    if (inflow_achieved > inflow_request) {
      report_ir = true;
      inflow_achieved = inflow_request;
    }
    op = op.with_requested(outflow_request).port;
    ip = ip.with_achieved(inflow_achieved).port;
    auto net_inflow{inflow_achieved - outflow_request};
    auto soc = update_soc(
        state.soc,
        state.inflow_port.get_achieved(),
        state.outflow_port.get_achieved(),
        dt,
        data.capacity);
    auto flow = std::clamp(outflow_request, 0.0, data.max_charge_rate);
    if ((soc == 1.0) && (net_inflow > 0.0)) {
      ip = ip.with_requested(flow).port;
    }
    if ((soc == 0.0) && (net_inflow < 0.0)) {
      op = op.with_achieved(flow).port;
      ip = ip.with_requested(flow).port;
    }
    return StorageState{
      time,
      soc,
      ip,
      op,
      report_ir || ip.should_send_request(state.inflow_port),
      op.should_send_achieved(state.outflow_port)};
  }

  std::ostream&
  operator<<(std::ostream& os, const StorageData& data)
  {
    return (os << "{:capacity " << data.capacity << " "
               << ":max-charge-rate " << data.max_charge_rate << "}");
  }

  std::ostream&
  operator<<(std::ostream& os, const StorageState& state)
  {
    bool rir{state.report_inflow_request};
    bool roa{state.report_outflow_achieved};
    return os << "{"
              << ":t " << state.time << " "
              << ":soc " << state.soc << " "
              << ":inflow-port " << state.inflow_port << " "
              << ":outflow-port " << state.outflow_port << " "
              << ":report-ir? " << rir << " "
              << ":report-oa? " << roa << "}";
  }

  StorageData
  storage_make_data(
      FlowValueType capacity,
      FlowValueType max_charge_rate)
  {
    if (capacity <= 0.0) {
      std::ostringstream oss{};
      oss << "capacity must be >= 0.0\n"
          << "capacity = " << capacity << "\n";
      throw std::invalid_argument(oss.str());
    }
    if (max_charge_rate <= 0.0) {
      std::ostringstream oss{};
      oss << "max_charge_rate must be > 0.0\n"
          << "max_charge_rate = " << max_charge_rate << "\n";
      throw std::invalid_argument(oss.str());
    }
    return StorageData{capacity, max_charge_rate};
  }

  StorageState
  storage_make_state(
      const StorageData& /* data */,
      double soc)
  {
    storage_check_soc(soc);
    return StorageState{
      0,
      soc,
      Port2{0.0},
      Port2{0.0},
      false,
      false};
  }

  RealTimeType
  storage_current_time(const StorageState& state)
  {
    return state.time;
  }

  double 
  storage_current_soc(const StorageState& state)
  {
    return state.soc;
  }

  RealTimeType
  storage_time_advance(
      const StorageData& data,
      const StorageState& state)
  {
    if (state.report_inflow_request || state.report_outflow_achieved) {
      return 0;
    }
    storage_check_soc(state.soc);
    if ((state.soc < 1.0) && (state.inflow_port.get_requested() != data.max_charge_rate)) {
      return 0;
    }
    return time_to_next_soc_event(
        state.inflow_port.get_achieved() - state.outflow_port.get_achieved(),
        data.capacity,
        state.soc);
  }

  StorageState
  storage_internal_transition(
      const StorageData& data,
      const StorageState& state)
  {
    namespace E = ERIN;
    if constexpr (E::debug_level >= E::debug_level_high) {
      std::cout << "storage_internal_transition(...)\n";
    }
    auto dt{storage_time_advance(data, state)};
    if ((dt == infinity) || (dt < 0)) {
      std::ostringstream oss{};
      oss << "internal transition after an infinite or negative time advance...\n"
          << "dt = " << dt << "\n";
      throw std::runtime_error(oss.str());
    }
    auto time{state.time + dt};
    auto soc = update_soc(
        state.soc,
        state.inflow_port.get_achieved(),
        state.outflow_port.get_achieved(),
        dt,
        data.capacity);
    if constexpr (E::debug_level >= E::debug_level_high) {
      std::cout << "state.time = " << state.time <<"\n";
      std::cout << "time = " << time <<"\n";
      std::cout << "dt = " << dt <<"\n";
      std::cout << "state.soc = " << state.soc <<"\n";
      std::cout << "soc = " << soc <<"\n";
    }
    auto ip{state.inflow_port};
    auto op{state.outflow_port};
    if (soc == 1.0) {
      ip = ip.with_requested(
          std::clamp(op.get_requested(), 0.0, data.max_charge_rate)).port;
    }
    else {
      ip = ip.with_requested(data.max_charge_rate).port;
    }
    if (soc == 0.0) {
      op = op.with_achieved(
          std::clamp(op.get_requested(), 0.0, ip.get_achieved())).port;
    }
    if ((soc > 0.0) && (soc < 1.0)) {
      auto inflow_achieved{ip.get_achieved()};
      auto outflow_achieved{op.get_achieved()};
      auto net_inflow{inflow_achieved - outflow_achieved};
      if (net_inflow > ERIN::flow_value_tolerance) {
        auto max_net_inflow{max_single_step_net_inflow(soc, data.capacity)};
        // net-inflow = inflow - outflow; inflow|max = net-inflow|max + outflow
        auto max_inflow{max_net_inflow + outflow_achieved};
        if (inflow_achieved > max_inflow) {
          ip = ip.with_requested(max_inflow).port;
        }
      }
      else if (net_inflow < ERIN::neg_flow_value_tol) {
        auto max_net_outflow{max_single_step_net_outflow(soc, data.capacity)};
        // net-outflow = outflow - inflow; outflow|max = net-outflow|max + inflow
        auto max_outflow{max_net_outflow + inflow_achieved};
        if (outflow_achieved > max_outflow) {
          op = op.with_achieved(max_outflow).port;
        }
      }
    }
    if constexpr (E::debug_level >= E::debug_level_high) {
      std::cout << "time = " << time <<"\n";
      std::cout << "soc = " << soc <<"\n";
      std::cout << "ip = " << ip <<"\n";
      std::cout << "op = " << op <<"\n";
    }
    return StorageState{
      time,
      soc,
      ip,
      op,
      false,
      false};
  }

  StorageState
  storage_external_transition(
      const StorageData& data,
      const StorageState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs)
  {
    bool got_outflow_request{false};
    bool got_the_inflow_achieved{false};
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
            got_the_inflow_achieved = true;
            inflow_achieved += x.value;
            break;
          }
        default:
          {
            std::ostringstream oss;
            oss << "unhandled port " << x.port
                << " in storage_external_transition(...)";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    auto time{state.time + elapsed_time};
    if (got_outflow_request && (!got_the_inflow_achieved)) {
      return storage_external_transition_on_outflow_request(
          data, state, outflow_request, elapsed_time, time);
    }
    if (got_the_inflow_achieved && (!got_outflow_request)) {
      return storage_external_transition_on_inflow_achieved(
          data, state, inflow_achieved, elapsed_time, time);
    }
    if (got_the_inflow_achieved && got_outflow_request) {
      return storage_external_transition_on_in_out_flow(
          data, state, outflow_request, inflow_achieved, elapsed_time, time);
    }
    std::ostringstream oss{};
    oss << "unhandled situation: external transition with neither inflow or outflow\n"
        << "got_the_inflow_achieved = " << got_the_inflow_achieved << "\n"
        << "got_outflow_request = " << got_outflow_request << "\n";
    throw std::runtime_error(oss.str());
  }

  StorageState
  storage_confluent_transition(
      const StorageData& data,
      const StorageState& state,
      const std::vector<PortValue>& xs)
  {
    return storage_external_transition(
        data,
        storage_internal_transition(data, state),
        0,
        xs);
  }

  std::vector<PortValue>
  storage_output_function(
      const StorageData& data,
      const StorageState& state)
  {
    std::vector<PortValue> ys{};
    storage_output_function_mutable(data, state, ys);
    return ys;
  }

  void storage_output_function_mutable(
      const StorageData& data,
      const StorageState& state,
      std::vector<PortValue>& ys)
  {
    namespace E = ERIN;
    if constexpr (E::debug_level >= E::debug_level_high) {
      std::cout << "storage_output_function_mutable(...)\n";
    }
    auto dt{storage_time_advance(data, state)};
    if ((dt == infinity) || (dt < 0)) {
      throw std::runtime_error(
        "time-advance within the storage component is infinity!");
    }
    auto next_state = storage_internal_transition(data, state);
    if (state.report_inflow_request || next_state.inflow_port.should_send_request(state.inflow_port)) {
      ys.emplace_back(
          PortValue{
            outport_inflow_request,
            next_state.inflow_port.get_requested()});
    }
    if (state.report_outflow_achieved || next_state.outflow_port.should_send_achieved(state.outflow_port)) {
      ys.emplace_back(
          PortValue{
            outport_outflow_achieved,
            next_state.outflow_port.get_achieved()});
    }
    return;
  }

  FlowValueType
  storage_storeflow_achieved(const StorageState& s)
  {
    return s.inflow_port.get_achieved() - s.outflow_port.get_achieved();
  }
}
