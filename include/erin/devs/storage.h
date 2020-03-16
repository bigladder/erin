/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_STORAGE_H
#define ERIN_DEVS_STORAGE_H
#include "erin/devs.h"
#include "erin/type.h"

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
  };

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

  StorageData storage_make_data(FlowValueType capacity);

  StorageState storage_make_state(double soc=1.0);

  RealTimeType storage_current_time(const StorageState& state);

  double storage_current_soc(const StorageState& state);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType storage_time_advance(
      const StorageData& data,
      const StorageState& state);

  ////////////////////////////////////////////////////////////
  // internal transition

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
}


#endif // ERIN_DEVS_STORAGE_H
