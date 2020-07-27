/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_DEVS_MOVER_H
#define ERIN_DEVS_MOVER_H
#include "erin/devs.h"
#include "erin/type.h"
#include <iostream>
#include <string>
#include <vector>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions

  ////////////////////////////////////////////////////////////
  // state
  struct MoverData
  {
    FlowValueType COP{1.0};
  };

  MoverData make_mover_data(FlowValueType COP);

  bool operator==(const MoverData& a, const MoverData& b);
  bool operator!=(const MoverData& a, const MoverData& b);
  std::ostream& operator<<(std::ostream& os, const MoverData& a);

  struct MoverState
  {
    RealTimeType time{0};
    Port inflow0_port{0, 0.0, 0.0};
    Port inflow1_port{0, 0.0, 0.0};
    Port outflow_port{0, 0.0, 0.0};
    bool report_inflow0_request{false};
    bool report_inflow1_request{false};
    bool report_outflow_achieved{false};
  };

  MoverState make_mover_state();

  bool operator==(const MoverState& a, const MoverState& b);
  bool operator!=(const MoverState& a, const MoverState& b);
  std::ostream& operator<<(std::ostream& os, const MoverState& a);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  mover_time_advance(
      const MoverData& data,
      const MoverState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  MoverState
  mover_internal_transition(
      const MoverData& data,
      const MoverState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  MoverState
  mover_external_transition(
      const MoverData& data,
      const MoverState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // confluent transition
  MoverState
  mover_confluent_transition(
      const MoverData& data,
      const MoverState& state,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  mover_output_function(const MoverData& d, const MoverState& s);

  void
  mover_output_function_mutable(
      const MoverData& d,
      const MoverState& s,
      std::vector<PortValue>& ys);

}

#endif // ERIN_DEVS_MOVER_H
