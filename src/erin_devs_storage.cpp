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

  StorageData
  storage_make_data(FlowValueType capacity)
  {
    if (capacity < 0.0) {
      std::ostringstream oss{};
      oss << "capacity must be >= 0.0\n"
          << "capacity = " << capacity << "\n";
      throw std::invalid_argument(oss.str());
    }
    return StorageData{capacity};
  }

  StorageState
  storage_make_state(
      double soc)
  {
    storage_check_soc(soc);
    return StorageState{
      0,
      soc,
      Port{0, 0.0, 0.0},
      Port{0, 0.0, 0.0},
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
}
