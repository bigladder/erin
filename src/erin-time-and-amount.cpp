// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include "erin/time_and_amount.h"

namespace erin
{
std::ostream& operator<<(std::ostream& os, TimeAndAmount const& timeAndLoad)
{
    os << "TimeAndAmount{"
       << "Time_s=" << timeAndLoad.time_s << "; "
       << "Amount_W=" << timeAndLoad.amount_W << "}";
    return os;
}
} // namespace erin
