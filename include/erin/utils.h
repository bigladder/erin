/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_UTILS_H
#define ERIN_UTILS_H
#include "erin/type.h"

namespace erin::utils
{
  std::string time_to_iso_8601_period(::ERIN::RealTimeType time_seconds);
}

#endif // ERIN_UTILS_H
