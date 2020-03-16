/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/storage.h"
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>

namespace erin::devs
{
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
    if (capacity < 0.0) {
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
    if (net_inflow == 0.0)
      return infinity;
    if (net_inflow > 0.0) {
      if (state.soc == 1.0) {
        return 0;
      }
      auto available_cap{(1.0 - state.soc) * data.capacity};
      auto time_to_fill{available_cap / net_inflow};
      return static_cast<RealTimeType>(std::floor(time_to_fill));
    }
    // net_inflow < 0.0
    if (state.soc == 0.0)
      return 0;
    auto available_store{state.soc * data.capacity};
    auto time_to_drain{available_store / (-1.0 * net_inflow)};
    return static_cast<RealTimeType>(std::floor(time_to_drain));
  }

  StorageState
  storage_internal_transition(
      const StorageData& data,
      const StorageState& state)
  {
    auto dt{storage_time_advance(data, state)};
    auto time{state.time + dt};
    auto net_inflow{
      state.inflow_port.get_achieved()
      - state.outflow_port.get_achieved()};
    auto capacity_increase{dt * net_inflow};
    auto soc_increase{capacity_increase / data.capacity};
    auto soc{state.soc + soc_increase};
    auto ip{state.inflow_port};
    auto op{state.outflow_port};
    if (soc == 1.0) {
      ip = ip.with_requested(0.0, time);
    }
    if (soc == 0.0) {
      op = op.with_achieved(0.0, time);
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
    auto ip{state.inflow_port};
    auto op{state.outflow_port};
    if (got_outflow_request)
      op = op.with_requested(outflow_request, time);
    if (got_inflow_achieved)
      ip = op.with_achieved(inflow_achieved, time);
    auto net_inflow{
      state.inflow_port.get_achieved()
      - state.outflow_port.get_achieved()};
    auto cap_change{net_inflow * elapsed_time};
    auto soc_change{cap_change / data.capacity};
    return StorageState{
      state.time + elapsed_time,
      state.soc + soc_change,
      ip,
      op,
      ip.should_propagate_request_at(time),
      op.should_propagate_achieved_at(time)};
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
    } else if (dt > 0) {
      auto ip{state.inflow_port};
      auto op{state.outflow_port};
      auto net_inflow{
        state.inflow_port.get_achieved()
        - state.outflow_port.get_achieved()};
      if (net_inflow > 0.0) {
        ip = ip.with_requested(0.0, time);
      } else if (net_inflow < 0.0) {
        op = op.with_achieved(0.0, time);
      }
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
    }
  }
}
