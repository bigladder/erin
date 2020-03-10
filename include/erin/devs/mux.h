/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_MUX_H
#define ERIN_DEVS_MUX_H
#include "erin/devs.h"
#include "erin/type.h"

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions

  ////////////////////////////////////////////////////////////
  // state
  struct MuxState
  {
    int num_inflows{0};
    int num_outflows{0};
    std::vector<Port> inflow_ports{};
    std::vector<Port> outflow_ports{};
  };

  MuxState make_mux_state(int num_inflows, int num_outflows);

  ////////////////////////////////////////////////////////////
  // time advance

  ////////////////////////////////////////////////////////////
  // internal transition

  ////////////////////////////////////////////////////////////
  // external transition

  ////////////////////////////////////////////////////////////
  // confluent transition

  ////////////////////////////////////////////////////////////
  // output function
}

#endif // ERIN_DEVS_MUX_H
