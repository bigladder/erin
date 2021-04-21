/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/devs/supply.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace erin::devs
{
  const bool supply_debug{false};

  std::ostream&
  operator<<(std::ostream& os, const SupplyData& d)
  {
    return os << "{:maximum-outflow " << d.maximum_outflow << "}";
  }

  SupplyData
  make_supply_data(const FlowValueType& maximum_outflow)
  {
    return SupplyData{maximum_outflow};
  }

  SupplyState
  make_supply_state()
  {
    return SupplyState{0, Port3{0.0, 0.0}, false};
  }

  RealTimeType
  supply_current_time(const SupplyState& state)
  {
    return state.time;
  }

  FlowValueType
  supply_current_request(const SupplyState& state)
  {
    return state.outflow_port.get_requested();
  }

  FlowValueType
  supply_current_achieved(const SupplyState& state)
  {
    return state.outflow_port.get_achieved();
  }
  
  std::ostream&
  operator<<(std::ostream& os, const SupplyState& s)
  {
    return os << "{"
              << ":t " << s.time
              << " "
              << ":outflow_port " << s.outflow_port
              << " "
              << ":send-achieved? " << s.send_achieved
              << "}";
  }

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  supply_time_advance(const SupplyState& state)
  {
    if (supply_debug) {
      std::cout << "supply_time_advance\n"
                << "- s:  " << state << "\n"
                << "- ta: ";
    }
    if (state.send_achieved) {
      if (supply_debug) {
        std::cout << "0\n";
      }
      return 0;
    }
    if (supply_debug) {
      std::cout << "infinity\n";
    }
    return infinity;
  }

  ////////////////////////////////////////////////////////////
  // internal transition
  SupplyState
  supply_internal_transition(const SupplyState& state, bool verbose)
  {
    auto next_state = SupplyState{
      state.time,
      state.outflow_port,
      false,
    };
    if (supply_debug && verbose) {
      std::cout << "supply_internal_transition\n"
                << "s:  " << state << "\n"
                << "s*: " << next_state << "\n";
    }
    return next_state;
  }

  ////////////////////////////////////////////////////////////
  // external transition
  SupplyState
  supply_external_transition(
      const SupplyData& data,
      const SupplyState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs,
      bool verbose)
  {
    auto inflow_request{0.0};
    for (const auto& x : xs) {
      switch (x.port)
      {
        case inport_outflow_request:
          inflow_request += x.value;
          break;
        default:
          std::ostringstream oss{};
          oss << "invalid port: " << x.port << " in supply_external_transition";
          throw std::invalid_argument(oss.str());
      }
    }
    auto update = state.outflow_port.with_requested_and_available(
        inflow_request,
        (data.maximum_outflow == supply_unlimited_value)
        ? inflow_request
        : data.maximum_outflow);
    auto next_state = SupplyState{
      state.time + dt,
      update.port,
      update.send_achieved};
    if (supply_debug && verbose) {
      std::cout << "supply_external_transition\n"
                << "- d:  " << data << "\n"
                << "- s:  " << state << "\n"
                << "- dt: " << dt << "\n"
                << "- xs: " << ERIN::vec_to_string<PortValue>(xs) << "\n"
                << "- s*: " << next_state << "\n";
    }
    return next_state;
  }

  ////////////////////////////////////////////////////////////
  // confluent transition
  SupplyState supply_confluent_transition(
      const SupplyData& data,
      const SupplyState& state,
      const std::vector<PortValue>& xs)
  {
    auto next_state = supply_external_transition(
        data,
        supply_internal_transition(state, false),
        0,
        xs,
        false);
    if (supply_debug) {
      std::cout << "supply_confluent_transition\n"
                << "- d:  " << data << "\n"
                << "- s:  " << state << "\n"
                << "- xs: " << ERIN::vec_to_string<PortValue>(xs) << "\n"
                << "- s*: " << next_state << "\n";
    }
    return next_state;
  }

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  supply_output_function(const SupplyState& state)
  {
    std::vector<PortValue> ys{};
    supply_output_function_mutable(state, ys);
    return ys;
  }

  void
  supply_output_function_mutable(
      const SupplyState& state,
      std::vector<PortValue>& ys)
  {
    if (state.send_achieved) {
      ys.emplace_back(
          PortValue{
            outport_outflow_achieved,
            supply_current_achieved(state)});
    }
    if (supply_debug) {
      std::cout << "supply_output_function_mutable"
                << "- s:  " << state << "\n"
                << "- ys: " << ERIN::vec_to_string<PortValue>(ys) << "\n";
    }
  }
}
