/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_VALIDATION_H
#define ERIN_NEXT_VALIDATION_H
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace erin_next
{
	std::unordered_set<std::string> const ValidTimeUnits{
		// TODO: rework these to be singular; use abbreviations? h, min, s?
		"hours", "minutes", "seconds",
	};

	std::unordered_set<std::string> const ValidRateUnits{
		"W", "kW", "MW",
	};

	std::unordered_set<std::string> const ValidQuantityUnits{
		"J", "kJ", "MJ", "Wh", "kWh", "MWh",
	};
}

#endif