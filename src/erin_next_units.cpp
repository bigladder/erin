/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_units.h"
#include <exception>
#include <sstream>
#include <exception>

namespace erin_next
{
	std::optional<TimeUnit>
	TagToTimeUnit(std::string const& tag)
	{
		
		if (tag == "s" || tag == "sec" || tag == "secs"
			|| tag == "second" || tag == "seconds")
		{
			return TimeUnit::Second;
		}
		if (tag == "min" || tag == "mins"
			|| tag == "minute" || tag == "minutes")
		{
			return TimeUnit::Minute;
		}
		if (tag == "h" || tag == "hr" || tag == "hrs"
			|| tag == "hour" || tag == "hours")
		{
			return TimeUnit::Hour;
		}
		return {};
	}

	std::string
	TimeUnitToTag(TimeUnit unit)
	{
		std::string result;
		switch (unit)
		{
			case (TimeUnit::Second):
			{
				result = "s";
			} break;
			case (TimeUnit::Minute):
			{
				result = "min";
			} break;
			case (TimeUnit::Hour):
			{
				result = "h";
			} break;
			default:
			{
				std::ostringstream oss{};
				oss << "unhandled TimeType '" << unit << "'" << std::endl;
				throw new std::runtime_error{ oss.str() };
			} break;
		}
		return result;
	}

	double
	Time_ToSeconds(double t, TimeUnit unit)
	{
		switch (unit)
		{
			case (TimeUnit::Second):
			{
				return t;
			} break;
			case (TimeUnit::Minute):
			{
				return t * 60.0;
			} break;
			case (TimeUnit::Hour):
			{
				return t * 3600.0;
			}
		}
		std::ostringstream oss{};
		oss << "unhandled time unit '" << TimeUnitToTag(unit)
			<< "'" << std::endl;
		throw new std::invalid_argument{ oss.str() };
	}
}