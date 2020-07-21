/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_UNCONTROLLED_SOURCE_H
#define ERIN_DEVS_UNCONTROLLED_SOURCE_H
#include "erin/devs.h"
#include "erin/type.h"
#include <iostream>
#include <string>
#include <vector>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions
  using LoadItem = ERIN::LoadItem;
  using size_type = std::vector<RealTimeType>::size_type;

  ////////////////////////////////////////////////////////////
  // state
  struct UncontrolledSourceData
  {
    std::vector<RealTimeType> times{};
    std::vector<FlowValueType> supply{};
    size_type num_items{0};
  };

  bool operator==(
      const UncontrolledSourceData& a,
      const UncontrolledSourceData& b);
  bool operator!=(
      const UncontrolledSourceData& a,
      const UncontrolledSourceData& b);
  std::ostream& operator<<(
      std::ostream& os,
      const UncontrolledSourceData& a);

  UncontrolledSourceData make_uncontrolled_source_data(
      const std::vector<LoadItem>& loads);

  struct UncontrolledSourceState
  {
    RealTimeType time{0};
    size_type index{0};
    Port outflow_port{}; // normal supply outflow
    Port spill_port{};   // unused supply goes out here
  };

  bool operator==(const UncontrolledSourceState& a, const UncontrolledSourceState& b);
  bool operator!=(const UncontrolledSourceState& a, const UncontrolledSourceState& b);
  std::ostream& operator<<(std::ostream& os, const UncontrolledSourceState& a);

  UncontrolledSourceState make_uncontrolled_source_state();
  
  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  uncontrolled_src_time_advance(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  UncontrolledSourceState
  uncontrolled_src_internal_transition(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  UncontrolledSourceState
  uncontrolled_src_external_transition(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // confluent transition
  UncontrolledSourceState
  uncontrolled_src_confluent_transition(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  uncontrolled_src_output_function(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state);

  void 
  uncontrolled_src_output_function_mutable(
      const UncontrolledSourceData& data,
      const UncontrolledSourceState& state,
      std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_UNCONTROLLED_SOURCE_H
