/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_DEVS_SUPPLY_H
#define ERIN_DEVS_SUPPLY_H
#include "erin/devs.h"
#include "erin/type.h"
#include <vector>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper functions and data
  const FlowValueType supply_unlimited_value{-1.0};

  ////////////////////////////////////////////////////////////
  // state
  struct SupplyData
  {
    FlowValueType maximum_outflow{supply_unlimited_value};
  };

  std::ostream& operator<<(std::ostream& os, const SupplyData& d);

  struct SupplyState
  {
    RealTimeType time{0};
    Port3 outflow_port{};
    bool send_achieved{false};
  };

  SupplyData make_supply_data(const FlowValueType& maximum_outflow);

  SupplyState make_supply_state();

  RealTimeType supply_current_time(const SupplyState& state);

  FlowValueType supply_current_request(const SupplyState& state);

  FlowValueType supply_current_achieved(const SupplyState& state);
  
  std::ostream& operator<<(std::ostream& os, const SupplyState& s);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType supply_time_advance(const SupplyState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  SupplyState supply_internal_transition(
    const SupplyState& state,
    bool verbose = true);

  ////////////////////////////////////////////////////////////
  // external transition
  SupplyState supply_external_transition(
      const SupplyData& data,
      const SupplyState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs,
      bool verbose = true);

  ////////////////////////////////////////////////////////////
  // confluent transition
  SupplyState supply_confluent_transition(
      const SupplyData& data,
      const SupplyState& state,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue> supply_output_function(const SupplyState& state);

  void
  supply_output_function_mutable(
      const SupplyState& state, std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_SUPPLY_H
