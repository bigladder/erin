/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_SIMULATION_INFO_H
#define ERIN_NEXT_SIMULATION_INFO_H
#include <ostream>
#include <string>
#include <optional>
#include <unordered_map>
#include "../vendor/toml11/toml.hpp"

namespace erin_next
{
	struct SimulationInfo
	{
		std::string RateUnit;
		std::string QuantityUnit;
		std::string TimeUnit;
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