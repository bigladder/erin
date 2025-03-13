// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_TIME_AND_AMOUNT_H
#define ERIN_TIME_AND_AMOUNT_H

#include <stdint.h>
#include <ostream>

#include "erin/const.h"

namespace erin
{

struct TimeAndAmount
{
    double time_s = 0.0;
    flow_t amount_W = 0;
};

std::ostream& operator<<(std::ostream& os, TimeAndAmount const& time_and_amount);

} // namespace erin

#endif
