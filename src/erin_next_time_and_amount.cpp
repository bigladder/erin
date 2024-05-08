/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_time_and_amount.h"

namespace erin
{
    std::ostream&
    operator<<(std::ostream& os, TimeAndAmount const& timeAndLoad)
    {
        os << "TimeAndAmount{"
           << "Time_s=" << timeAndLoad.Time_s << "; "
           << "Amount_W=" << timeAndLoad.Amount_W << "}";
        return os;
    }
} // namespace erin
