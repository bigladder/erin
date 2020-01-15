/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_UTILS_H
#define ERIN_UTILS_H
#include "erin/type.h"
#include <iostream>

namespace erin::utils
{
  using RealTimeType = ERIN::RealTimeType;
  /**
   * Months_days_elapsed is a duration of time in months and days.
   * Months_days_elapsed should only take on values from {0,0} (no time forward
   * in months or days) to {11,30} 11 months and 30 days HAVE ELAPSED.  The
   * assumption is that the "clock" always starts from January 1 at 00:00:00 so
   * therefore a value of {11,30} would be some time on or after midnight on
   * December 31.
   */
  class Months_days_elapsed
  {
    public:
      Months_days_elapsed(
          RealTimeType month,
          RealTimeType days);

      [[nodiscard]] RealTimeType get_elapsed_months() const {
        return months;
      }
      [[nodiscard]] RealTimeType get_elapsed_days_of_month() const {
        return days;
      }

      friend bool operator==(
          const Months_days_elapsed& a, const Months_days_elapsed& b);
      friend std::ostream& operator<<(std::ostream& os, const Months_days_elapsed& mdd); 

    private:
      RealTimeType months; // months of time that have ELAPSED January 1 at 00:00:00
      RealTimeType days; // days of the next month
  };

  bool operator==(const Months_days_elapsed& a, const Months_days_elapsed& b);

  std::ostream& operator<<(std::ostream& os, const Months_days_elapsed& mdd); 

  std::string time_to_iso_8601_period(RealTimeType time_seconds);

  Months_days_elapsed day_of_year_to_months_days_elapsed(RealTimeType day_of_year);

  bool is_superset(
      const std::vector<std::string>& superset,
      const std::vector<std::string>& compared_to);
}

#endif // ERIN_UTILS_H
