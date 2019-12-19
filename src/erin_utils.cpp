/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/utils.h"
#include "erin/type.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace erin::utils
{
  // Clojure program to calculate the below:
  // > (def days-per-month [31 28 31 30 31 30 31 31 30 31 30 31])
  // > (count days-per-month) ;=> 12
  // > (reduce + days-per-month) ;=> 365
  // > (reductions + days-per-month) ;=> [31 59 90 120 151 181 212 243 273 304 334 365]
  const std::vector<ERIN::RealTimeType> days_per_month{
    31,   // January
    28,   // February (non-leap year)
    31,   // March
    30,   // April
    31,   // May
    30,   // June
    31,   // July
    31,   // August
    30,   // September
    31,   // October
    30,   // November
    31};  // December
  const std::vector<ERIN::RealTimeType> day_of_year_to_month{
    31,   // January is doy <= 31 days
    59,   // February (non-leap year) is doy <= 59
    90,   // March is doy <= 90, etc.
    120,  // April
    151,  // May
    181,  // June
    212,  // July
    243,  // August
    273,  // September
    304,  // October
    334,  // November
    365}; // December
  const auto days_per_year{*day_of_year_to_month.rbegin()};
  const auto num_months{days_per_month.size()};
  const int max_month_idx = 11;
  const int min_month_idx = 0;

  Months_days_elapsed::Months_days_elapsed(
      ERIN::RealTimeType m_,
      ERIN::RealTimeType d_):
    months{m_},
    days{d_}
  {
    if ((months < min_month_idx) || (months > max_month_idx)) {
      std::ostringstream oss;
      oss << "Months_days_elapsed: invalid month argument\n"
          << "months = " << months << " but this is "
          << "NUMBER OF MONTHS ELAPSED; therefore 0 <= months <= 11\n"
          << "if you have months > 11 then that means 12 (or more) "
          << "months have passed which is a year\n"
          << "THIS CLASS DOES NOT HOLD MORE THAN A YEAR!\n";
      throw std::invalid_argument(oss.str());
    }
    using size_type = std::vector<ERIN::RealTimeType>::size_type;
    auto idx = static_cast<size_type>(months);
    const auto max_days = days_per_month[idx];
    if ((days < 0) || (days >= max_days)) {
      std::ostringstream oss;
      oss << "Months_days_elapsed: invalid days argument\n"
          << "days = " << days << " but this class counts DAYS ELAPSED; "
          << "therefore 0 <= days < " << max_days
          << " (the month you're in is " << (months + 1) << ")\n"
          << "if you have " << max_days << " or more days ELAPSED, you'd be "
          << "in the next month.";
      throw std::invalid_argument(oss.str());
    }
  }

  bool
  operator==(const Months_days_elapsed& a, const Months_days_elapsed& b)
  {
    return (a.months == b.months) && (a.days == b.days);
  }

  std::ostream&
  operator<<(std::ostream& os, const Months_days_elapsed& mdd)
  {
    os << "Months_days_elapsed("
       << "months=" << mdd.months << ","
       << "days=" << mdd.days << ")";
    return os;
  }

  Months_days_elapsed
  day_of_year_to_months_days_elapsed(ERIN::RealTimeType day_of_year)
  {
    if (day_of_year < 0) {
      const auto neg_doy = std::abs(day_of_year % days_per_year);
      return day_of_year_to_months_days_elapsed(days_per_year - neg_doy);
    }
    const auto doy = day_of_year % days_per_year;
    using size_type = std::vector<ERIN::RealTimeType>::size_type;
    for (size_type i{0}; i < num_months; ++i) {
      if (doy < day_of_year_to_month[i]) {
        // elapsed_months_of_days = the number of days (since year start)
        // represented by the elapsed months
        ERIN::RealTimeType elapsed_months_of_days{0}; 
        if (i > 0) {
          elapsed_months_of_days = day_of_year_to_month[i - 1];
        }
        return Months_days_elapsed{
          static_cast<ERIN::RealTimeType>(i),
          doy - elapsed_months_of_days};
      }
    }
    std::ostringstream oss;
    oss << "impossible condition error: should not get here\n"
        << "doy: " << doy << "\n"
        << "day_of_year: " << day_of_year << "\n"
        << "days_per_year: " << days_per_year << "\n";
    throw std::runtime_error(oss.str());
  }

  std::string
  time_to_iso_8601_period(::ERIN::RealTimeType time_seconds)
  {
    namespace E = ::ERIN;
    if (time_seconds < 0) {
      return "";
    }
    const auto seconds = time_seconds % E::rtt_seconds_per_minute;
    const auto minute_div = std::div(time_seconds, E::rtt_seconds_per_minute);
    const auto minutes = minute_div.quot % E::rtt_minutes_per_hour;
    const auto hour_div = std::div(time_seconds, E::rtt_seconds_per_hour);
    const auto hours = hour_div.quot % E::rtt_hours_per_day;
    const auto day_of_year_div = std::div(time_seconds, E::rtt_seconds_per_day);
    const auto day_of_year = day_of_year_div.quot % days_per_year;
    const auto month_days = day_of_year_to_months_days_elapsed(day_of_year);
    const auto days = month_days.get_elapsed_days_of_month();
    const auto months = month_days.get_elapsed_months();
    const auto year_div = std::div(time_seconds, E::rtt_seconds_per_year);
    const auto years = year_div.quot;
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
        << minutes
        << ":"
        << std::right << std::setfill('0') << std::setw(2)
        << seconds;
    return oss.str();
  }

  bool
  is_superset(const std::vector<std::string>& superset, const std::vector<std::string>& compared_to)
  {
    for (const auto& ct_k: compared_to) {
      bool found_it{false};
      for (const auto& ss_k: superset) {
        if (ss_k == ct_k) {
          found_it = true;
          break;
        }
      }
      if (!found_it) {
        return false;
      }
    }
    return true;
  }
}
