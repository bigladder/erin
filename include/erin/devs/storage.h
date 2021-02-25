/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_DEVS_STORAGE_H
#define ERIN_DEVS_STORAGE_H
#include "erin/devs.h"
#include "erin/type.h"
#include <iostream>
#include <string>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions
  bool storage_is_full(double soc);

  bool storage_is_empty(double soc);

  double calc_time_to_fill(double soc, double capacity, double inflow);

  double calc_time_to_drain(double soc, double capacity, double outflow);

  template <class N>
  void assert_positive(N number, const std::string& msg)
  {
    if (number <= 0) {
      std::ostringstream oss{};
      oss << "number must be > 0\n"
          << "number = " << number << "\n"
          << "message: " << msg << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  template <class N>
  void assert_non_negative(N number, const std::string& msg)
  {
    if (number < 0) {
      std::ostringstream oss{};
      oss << "number must be >= 0\n"
          << "number = " << number << "\n"
          << "message: " << msg << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  template <class N>
  void assert_fraction(N number, const std::string& msg)
  {
    if ((number < 0) || (number > 1)) {
      std::ostringstream oss{};
      oss << "number must be >= 0 and <= 1\n"
          << "number = " << number << "\n"
          << "message: " << msg << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  double update_soc(
      FlowValueType inflow_achieved,
      FlowValueType outflow_achieved,
      RealTimeType dt,
      FlowValueType capacity);

  ////////////////////////////////////////////////////////////
  // state and data
  /**
   * StorageData: immutable (constant) reference data that doesn't change
   * over a simulation
   */
  struct StorageData
  {
    FlowValueType capacity{1.0};
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

  StorageState storage_external_transition_on_outflow_request(
      const StorageData& data,
      const StorageState& state,
      FlowValueType outflow_request,
      RealTimeType dt,
      RealTimeType time);

  StorageState storage_external_transition_on_inflow_achieved(
      const StorageData& data,
      const StorageState& state,
      FlowValueType inflow_achieved,
      RealTimeType dt,
      RealTimeType time);

  StorageState storage_external_transition_on_in_out_flow(
      const StorageData& data,
      const StorageState& state,
      FlowValueType outflow_request,
      FlowValueType inflow_achieved,
      RealTimeType dt,
      RealTimeType time);

  ////////////////////////////////////////////////////////////
  // confluent transition
  StorageState storage_confluent_transition(
      const StorageData& data,
      const StorageState& state,
      const std::vector<PortValue>& xs);

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
