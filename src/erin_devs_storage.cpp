/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/storage.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace erin::devs
{
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
    if (storage_is_full(next_soc) || (next_soc > 1.0))
      return 1.0;
    if (storage_is_empty(next_soc) || (next_soc < 0.0))
      return 0.0;
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
    auto ip{state.inflow_port};
    auto op = state.outflow_port.with_requested(outflow_request, time);
    auto soc = update_soc(
        state.soc,
        state.inflow_port.get_achieved(),
        state.outflow_port.get_achieved(),
        dt,
        data.capacity);
    if (soc == 1.0)
      ip = ip.with_requested(
          std::clamp(outflow_request, 0.0, data.max_charge_rate),
          time);
    else
      ip = ip.with_requested(data.max_charge_rate, time);
    if (soc == 0.0)
      op = op.with_achieved(
          std::clamp(
            outflow_request,
            0.0,
            data.max_charge_rate),
          time);
    return StorageState{
      time,
      soc,
      ip,
      op,
      ip.should_propagate_request_at(time),
      op.should_propagate_achieved_at(time)};
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
    ip = ip.with_achieved(inflow_achieved, time);
    auto soc = update_soc(
        state.soc,
        state.inflow_port.get_achieved(),
        state.outflow_port.get_achieved(),
        dt,
        data.capacity);
    auto ip_ach{ip.get_achieved()};
    auto op_ach{op.get_achieved()};
    if ((soc == 1.0) && (ip_ach > op_ach))
      ip = ip.with_requested(op_ach, time);
    if ((soc == 0.0) && (op_ach > ip_ach))
      op = op.with_achieved(
          std::clamp(ip_ach, 0.0, op.get_requested()),
          time);
    return StorageState{
      time,
      soc,
      ip,
      op,
      ip.should_propagate_request_at(time),
      op.should_propagate_achieved_at(time)};
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
    op = op.with_requested(outflow_request, time);
    ip = ip.with_achieved(inflow_achieved, time);
    auto net_inflow{inflow_achieved - outflow_request};
    auto soc = update_soc(
        state.soc,
        state.inflow_port.get_achieved(),
        state.outflow_port.get_achieved(),
        dt,
        data.capacity);
    auto flow = std::clamp(outflow_request, 0.0, data.max_charge_rate);
    if ((soc == 1.0) && (net_inflow > 0.0))
      ip = ip.with_requested(flow, time);
    if ((soc == 0.0) && (net_inflow < 0.0)) {
      op = op.with_achieved(flow, time);
      ip = ip.with_requested(flow, time);
    }
    return StorageState{
      time,
      soc,
      ip,
      op,
      ip.should_propagate_request_at(time),
      op.should_propagate_achieved_at(time)};
  }

  std::ostream&
  operator<<(std::ostream& os, const StorageData& data)
  {
    return (os << "StorageData{"
               << "capacity=" << data.capacity << ", "
               << "max_charge_rate=" << data.max_charge_rate << "}");
  }

  std::ostream&
  operator<<(std::ostream& os, const StorageState& state)
  {
    bool rir{state.report_inflow_request};
    bool roa{state.report_outflow_achieved};
    return (os << "StorageState{"
               << "time=" << state.time << ", "
               << "soc=" << state.soc << ", "
               << "inflow_port=" << state.inflow_port << ", "
               << "outflow_port=" << state.outflow_port << ", "
               << "report_inflow_request=" << rir << ", "
               << "report_outflow_achieved=" << roa << "}");
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
      const StorageData& data,
      double soc)
  {
    storage_check_soc(soc);
    FlowValueType requested_charge{0.0}; 
    if (soc < 1.0)
      requested_charge = data.max_charge_rate;
    return StorageState{
      0,
      soc,
      Port{
        0,
        requested_charge,
        requested_charge,
        requested_charge != 0.0,
        false},
      Port{0, 0.0, 0.0},
      requested_charge != 0.0,
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
    if (state.report_inflow_request || state.report_outflow_achieved)
      return 0;
    storage_check_soc(state.soc);
    auto net_inflow{
      state.inflow_port.get_achieved()
      - state.outflow_port.get_achieved()};
    auto high_soc{1.0 - ERIN::flow_value_tolerance};
    if (std::abs(net_inflow) <= ERIN::flow_value_tolerance)
      return infinity;
    if (net_inflow > ERIN::flow_value_tolerance) {
      if (state.soc >= high_soc)
        return 0;
      auto time_to_fill{
        calc_time_to_fill(state.soc, data.capacity, net_inflow)};
      if (time_to_fill < 1.0)
        return 1;
      return static_cast<RealTimeType>(std::floor(time_to_fill));
    }
    // net_inflow < 0.0
    if (state.soc <= ERIN::flow_value_tolerance)
      return 0;
    auto inflow_request{state.inflow_port.get_requested()};
    if ((state.soc < high_soc) && (inflow_request < data.max_charge_rate))
      return 0;
    auto max_inflow_at_full_charge{
      std::clamp(-1 * net_inflow, 0.0, data.max_charge_rate)};
    if ((state.soc < high_soc) && (inflow_request < max_inflow_at_full_charge))
      return 0;
    auto time_to_drain{
      calc_time_to_drain(state.soc, data.capacity, (-1.0 * net_inflow))};
    if (time_to_drain < 1.0)
      return 1;
    return static_cast<RealTimeType>(std::floor(time_to_drain));
  }

  StorageState
  storage_internal_transition(
      const StorageData& data,
      const StorageState& state)
  {
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
    auto ip{state.inflow_port};
    auto op{state.outflow_port};
    if (soc == 1.0) {
      ip = ip.with_requested(
          std::clamp(op.get_requested(), 0.0, data.max_charge_rate),
          time);
    }
    else {
      ip = ip.with_requested(data.max_charge_rate, time);
    }
    if (soc == 0.0) {
      op = op.with_achieved(
          std::clamp(op.get_requested(), 0.0, ip.get_achieved()),
          time);
    }
    if ((soc > 0.0) && (soc < 1.0)) {
      auto inflow_achieved{state.inflow_port.get_achieved()};
      auto outflow_achieved{state.outflow_port.get_achieved()};
      auto net_inflow{inflow_achieved - outflow_achieved};
      if (net_inflow > 0.0) {
        auto time_to_fill{
          calc_time_to_fill(state.soc, data.capacity, net_inflow)};
        if (time_to_fill < 1.0) {
          auto remaining_capacity{(1.0 - state.soc) * data.capacity};
          auto new_net_inflow{(remaining_capacity / 1.0)};
          ip = ip.with_requested(
              new_net_inflow + outflow_achieved,
              time);
        }
      } else if (net_inflow < 0.0) {
        auto time_to_drain{
          calc_time_to_drain(state.soc, data.capacity, (-1 * net_inflow))};
        if (time_to_drain < 1.0) {
          auto remaining_capacity{state.soc * data.capacity};
          auto new_net_outflow{(remaining_capacity / 1.0)};
          op = op.with_achieved(
              new_net_outflow + inflow_achieved,
              time);
        }
      }
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
                << " in storage_external_transition(...)";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    auto time{state.time + elapsed_time};
    if (got_outflow_request && (!got_inflow_achieved))
      return storage_external_transition_on_outflow_request(
          data, state, outflow_request, elapsed_time, time);
    if (got_inflow_achieved && (!got_outflow_request))
      return storage_external_transition_on_inflow_achieved(
          data, state, inflow_achieved, elapsed_time, time);
    if (got_inflow_achieved && got_outflow_request)
      return storage_external_transition_on_in_out_flow(
          data, state, outflow_request, inflow_achieved, elapsed_time, time);
    std::ostringstream oss{};
    oss << "unhandled situation: external transition with neither inflow or outflow\n"
        << "got_inflow_achieved = " << got_inflow_achieved << "\n"
        << "got_outflow_request = " << got_outflow_request << "\n";
    throw std::runtime_error(oss.str());
  }

  StorageState
  storage_confluent_transition(
      const StorageData& data,
      const StorageState& state,
      const std::vector<PortValue>& xs)
  {
    auto next_state = storage_internal_transition(data, state);
    return storage_external_transition(data, next_state, 0, xs);
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
    auto dt{storage_time_advance(data, state)};
    auto time{state.time + dt};
    if (dt == 0) {
      if (state.inflow_port.should_propagate_request_at(time))
        ys.emplace_back(
            PortValue{
              outport_inflow_request,
              state.inflow_port.get_requested()});
      if (state.outflow_port.should_propagate_achieved_at(time))
        ys.emplace_back(
            PortValue{
              outport_outflow_achieved,
              state.outflow_port.get_achieved()});
      return;
    }
    if (dt > 0) {
      auto ip{state.inflow_port};
      auto op{state.outflow_port};
      auto net_inflow{
        state.inflow_port.get_achieved()
        - state.outflow_port.get_achieved()};
      if (net_inflow > 0.0)
        ip = ip.with_requested(0.0, time);
      if (net_inflow < 0.0)
        op = op.with_achieved(
            std::clamp(
              op.get_requested(),
              0.0,
              ip.get_achieved()),
            time);
      if (ip.should_propagate_request_at(time))
        ys.emplace_back(
            PortValue{
              outport_inflow_request,
              ip.get_requested()});
      if (op.should_propagate_achieved_at(time))
        ys.emplace_back(
            PortValue{
              outport_outflow_achieved,
              op.get_achieved()});
      return;
    }
    std::ostringstream oss{};
    oss << "dt < 0\n"
        << "dt = " << dt << "\n";
    throw std::runtime_error(oss.str());
  }
}
