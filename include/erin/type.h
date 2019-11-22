/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_TYPE_H
#define ERIN_TYPE_H
#include "../../vendor/bdevs/include/adevs.h"

namespace ERIN
{
  using FlowValueType = double;
  using RealTimeType = int;
  using LogicalTimeType = int;
  using PortValue = adevs::port_value<FlowValueType>;
}

#endif // ERIN_ERIN_H
