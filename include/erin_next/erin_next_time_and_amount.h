/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_TIME_AND_AMOUNT_H
#define ERIN_TIME_AND_AMOUNT_H
#include <stdint.h>
#include <ostream>

namespace erin
{
    struct TimeAndAmount
    {
        double Time_s = 0.0;
        uint32_t Amount_W = 0;
    };

    std::ostream&
    operator<<(std::ostream& os, TimeAndAmount const& timeAndAmount);
} // namespace erin

#endif
