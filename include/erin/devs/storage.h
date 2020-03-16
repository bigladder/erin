/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_STORAGE_H
#define ERIN_DEVS_STORAGE_H
#include "erin/devs.h"
#include "erin/type.h"
#include <iostream>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions

  ////////////////////////////////////////////////////////////
  // state
  /**
   * StorageData: immutable (constant) reference data that doesn't change
   * over a simulation
   */
  struct StorageData
  {
    FlowValueType capacity{0.0};
    FlowValueType max_charge_rate{1.0};
  };

  std::ostream& operator<<(std::ostream& os, const StorageData& data);

  /**
   * StorageState: state that may change between time-steps
   */
  struct StorageState
  {
    RealTimeType time{0};
    double soc{0.0}; // soc = state of charge (0 <= soc <= 1)
    Port inflow_port{0, 0.0, 0.0};
    Port outflow_port{0, 0.0, 0.0};
    bool report_inflow_request{false};
    bool report_outflow_achieved{false};
  };

  std::ostream& operator<<(std::ostream& os, const StorageState& state);

  StorageData storage_make_data(
      FlowValueType capacity,
      FlowValueType max_charge_rate);

  StorageState storage_make_state(const StorageData& data, double soc=1.0);

  RealTimeType storage_current_time(const StorageState& state);

  double storage_current_soc(const StorageState& state);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType storage_time_advance(
      const StorageData& data,
      const StorageState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  StorageState storage_internal_transition(
      const StorageData& data,
      const StorageState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  StorageState storage_external_transition(
      const StorageData& data,
      const StorageState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // confluent transition

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue> storage_output_function(
      const StorageData& data,
      const StorageState& state);

  void storage_output_function_mutable(
      const StorageData& data,
      const StorageState& state,
      std::vector<PortValue>& ys);
}


#endif // ERIN_DEVS_STORAGE_H
