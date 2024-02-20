/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_utils.h"
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
		if (tag == "d" || tag == "ds" || tag == "day" || tag == "days")
		{
			return TimeUnit::Day;
		}
		if (tag == "week" || tag == "weeks")
		{
			return TimeUnit::Week;
		}
		if (tag == "yr" || tag == "y" || tag == "yrs" || tag == "ys"
			|| tag == "year" || tag == "years")
		{
			return TimeUnit::Year;
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
			case (TimeUnit::Day):
			{
				result = "d";
			} break;
			case (TimeUnit::Week):
			{
				result = "week";
			} break;
			case (TimeUnit::Year):
			{
				result = "yr";
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
			} break;
			case (TimeUnit::Day):
			{
				return t * 24.0 * 3600.0;
			} break;
			case (TimeUnit::Week):
			{
				return t * 7.0 * 24.0 * 3600.0;
			} break;
			case (TimeUnit::Year):
			{
				// NOTE: see note in erin_next_utils.h about year; we are
				// assuming 1 year is exactly 365 days vs 365.25.
				return t * 365.0 * 24.0 * 3600.0;
			} break;
		}
		std::ostringstream oss{};
		oss << "unhandled time unit '" << TimeUnitToTag(unit)
			<< "'" << std::endl;
		throw new std::invalid_argument{ oss.str() };
	}

	std::string
	SecondsToPrettyString(double time_s)
	{
		size_t years = static_cast<size_t>(time_s) / seconds_per_year;
		size_t hours =
			static_cast<size_t>(time_s - seconds_per_year * years)
			/ seconds_per_hour;
		size_t minutes =
			static_cast<size_t>(time_s - seconds_per_year * years
				- seconds_per_hour * hours)
			/ seconds_per_minute;
		size_t seconds =
			static_cast<size_t>(time_s - seconds_per_year * years
				- seconds_per_hour * hours
				- seconds_per_minute * minutes);
		std::ostringstream oss{};
		oss << years << " yr "
			<< hours << " h "
			<< minutes << " min "
			<< seconds << " s";
		return oss.str();
	}
}