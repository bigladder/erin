/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_UTILS_H
#define ERIN_NEXT_UTILS_H
#include <string>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdint.h>

namespace erin_next
{
	// Clojure program to calculate the below:
	// > (def days-per-month [31 28 31 30 31 30 31 31 30 31 30 31])
	// > (count days-per-month) ;=> 12
	// > (reduce + days-per-month) ;=> 365
	// > (reductions + days-per-month) ;=>
	// ;;  [31 59 90 120 151 181 212 243 273 304 334 365]
	std::vector<uint32_t> const days_per_month{// January
											   31,
											   // February (non-leap year)
											   28,
											   // March
											   31,
											   // April
											   30,
											   // May
											   31,
											   // June
											   30,
											   // July
											   31,
											   // August
											   31,
											   // September
											   30,
											   // October
											   31,
											   // November
											   30,
											   // December
											   31
	};
	std::vector<uint32_t> const day_of_year_to_month{
		// January is doy <= 31 days
		31,
		// February (non-leap year) is doy <= 59
		59,
		// March is doy <= 90, etc.
		90,
		// April
		120,
		// May
		151,
		// June
		181,
		// July
		212,
		// August
		243,
		// September
		273,
		// October
		304,
		// November
		334,
		// December
		365
	};
	uint32_t const num_months{12};
	int const max_month_idx = 11;
	int const min_month_idx = 0;

	// Time Conversion Factors
	int constexpr seconds_per_minute{60};
	int constexpr minutes_per_hour{60};
	int constexpr seconds_per_hour{seconds_per_minute * minutes_per_hour};
	int constexpr hours_per_day{24};
	int constexpr seconds_per_day{seconds_per_hour * hours_per_day};
	int constexpr seconds_per_week{seconds_per_day * 7};
	// NOTE: there are actually 365.25 days per year but our time clock
	// doesn't acknowledge leap years so we use a slightly lower factor.
	// Hopefully, this won't bite us... For this simulation, one year is
	// always 365 days
	int constexpr days_per_year{365};
	int constexpr seconds_per_year{seconds_per_day * days_per_year};

	struct Months_days_elapsed
	{
		// months of time that have ELAPSED January 1 at 00:00:00
		uint32_t months;
		// days of the next month
		uint32_t days;
	};

	Months_days_elapsed
	DayOfYearToMonthsDaysElapsed(uint64_t day_of_year);

	std::string
	TimeToISO8601Period(uint64_t time_seconds);

	void
	WriteErrorMessage(std::string const& tag, std::string const& message);

	std::string
	WriteErrorToString(std::string const& tag, std::string const& msg);

}

#endif
