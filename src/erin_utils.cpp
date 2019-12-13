/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/utils.h"
#include "erin/type.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace erin::utils
{
  std::string
  time_to_iso_8601_period(::ERIN::RealTimeType time_seconds)
  {
    namespace E = ::ERIN;
    if (time_seconds < 0) {
      return "";
    }
    const auto seconds_per_minute =
      static_cast<E::RealTimeType>(E::seconds_per_minute);
    //const auto seconds_per_hour =
    //  static_cast<RealTimeType>(E::seconds_per_hour);
    //const auto seconds_per_hour =
    //  static_cast<RealTimeType>(E::seconds_per_hour);
    const auto seconds = time_seconds % seconds_per_minute;
    const auto minutes = std::div(time_seconds, seconds_per_minute);
    const auto hours = time_seconds * 0;
    const auto days = time_seconds * 0;
    const auto months = time_seconds * 0;
    const auto years = time_seconds * 0;
    std::ostringstream oss;
    oss << "P"
        << std::right << std::setfill('0') << std::setw(4)
        << years
        << "-"
        << std::right << std::setfill('0') << std::setw(2)
        << months
        << "-"
        << std::right << std::setfill('0') << std::setw(2)
        << days
        << "T"
        << std::right << std::setfill('0') << std::setw(2)
        << hours
        << ":"
        << std::right << std::setfill('0') << std::setw(2)
        << minutes.quot
        << ":"
        << std::right << std::setfill('0') << std::setw(2)
        << seconds;
    return oss.str();
  }
}
