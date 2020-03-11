/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_MUX_H
#define ERIN_DEVS_MUX_H
#include "erin/devs.h"
#include "erin/type.h"
#include <vector>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions

  ////////////////////////////////////////////////////////////
  // state
  const int minimum_number_of_ports{1};
  const int maximum_number_of_ports{max_port_numbers};

  struct MuxState
  {
    RealTimeType time{0};
    int num_inflows{0};
    int num_outflows{0};
    std::vector<Port> inflow_ports{};
    std::vector<Port> outflow_ports{};
  };

  MuxState make_mux_state(int num_inflows, int num_outflows);

  RealTimeType mux_current_time(const MuxState& state);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType mux_time_advance(const MuxState& state);

  ////////////////////////////////////////////////////////////
  // internal transition

  ////////////////////////////////////////////////////////////
  // external transition
  MuxState mux_external_transition(
      const MuxState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // confluent transition

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue> mux_output_function(const MuxState& state);

  void mux_output_function_mutable(
      const MuxState& state,
      std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_MUX_H
