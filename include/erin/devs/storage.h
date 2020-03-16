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
  struct StorageState
  {
    FlowValueType capacity{0.0};
    double soc{0.0}; // soc = state of charge (0 <= soc <= 1)
    Port inflow_port{0, 0.0, 0.0};
    Port outflow_port{0, 0.0, 0.0};
    bool report_inflow_request{false};
    bool report_outflow_request{false};
  };

  StorageState storage_make_state(FlowValueType capacity, double soc=1.0);

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


#endif // ERIN_DEVS_STORAGE_H
