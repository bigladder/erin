/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_SIMULATION_INFO_H
#define ERIN_NEXT_SIMULATION_INFO_H
#include "erin_next/erin_next_units.h"
#include "../vendor/toml11/toml.hpp"
#include <ostream>
#include <string>
#include <optional>
#include <unordered_map>

namespace erin_next
{
	// TODO: consider what we're asking for in SimulationInfo. I think we
	// should do the following: get rid of rate and quantity unit. TimeUnit
	// is needed as it corresponds with max_time. Otherwise, more thought is
	// needed for the rate units. Do we want to get into unit conversion for
	// some large number of potential rate units? If so, these should be
	// display units by flow type with, perhaps, an overall default. Units
	// specified elsewhere would just be for conversion on reading in. Again,
	// more thought is needed as we might need to consider ranges when using
	// uint32_t...
	struct SimulationInfo
	{
		std::string RateUnit;
		std::string QuantityUnit;
		TimeUnit TheTimeUnit;
		double MaxTime;
	};

	std::optional<SimulationInfo>
	ParseSimulationInfo(
		std::unordered_map<toml::key, toml::value> const& table);

	bool
	operator==(SimulationInfo const& a, SimulationInfo const& b);

	bool
	operator!=(SimulationInfo const& a, SimulationInfo const& b);

	std::ostream&
	operator<<(std::ostream& os, SimulationInfo const& s);
}

#endif // ERIN_NEXT_SIMULATION_INFO_H