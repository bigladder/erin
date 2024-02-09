/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_units.h"
#include <exception>
#include <sstream>

namespace erin_next
{
	// TODO: make units into an enum
	double
	Time_ToSeconds(double t, std::string const& unit)
	{
		if (unit == "seconds")
		{
			return t;
		}
		if (unit == "minutes")
		{
			return t * 60.0;
		}
		if (unit == "hours")
		{
			return t * 3600.0;
		}
		std::ostringstream oss{};
		oss << "unhandled time unit '" << unit << "'" << std::endl;
		throw new std::invalid_argument{ oss.str() };
	}
}