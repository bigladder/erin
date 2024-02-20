/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_UNITS_H
#define ERIN_NEXT_UNITS_H
#include <string>
#include <optional>

namespace erin_next
{
	enum TimeUnit
	{
		Second,
		Minute,
		Hour,
		Day,
		Week,
		Year,
	};

	std::optional<TimeUnit>
	TagToTimeUnit(std::string const& tag);

	std::string
	TimeUnitToTag(TimeUnit unit);

	double
	Time_ToSeconds(double t, TimeUnit unit);

	std::string
	SecondsToPrettyString(double time_s);
}

#endif