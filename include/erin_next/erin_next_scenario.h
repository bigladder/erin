/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_SCENARIO_H
#define ERIN_NEXT_SCENARIO_H

#include "../vendor/toml11/toml.hpp"
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_units.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <optional>

namespace erin_next
{
	struct ScenarioDict
	{
		std::vector<std::string> Tags;
		std::vector<size_t> OccurrenceDistributionIds;
		// TODO: we can probably make time into an enum
		std::vector<TimeUnit> TimeUnits;
		// TODO: if we stay with multiple networks... which I don't recommend.
		std::vector<size_t> NetworkIds;
		std::vector<double> Durations;
		// NOTE: an entry of none means "no max occurrences"; will take as
		// many as fit in the max time of the simulation (see SimulationInfo)
		std::vector<std::optional<size_t>> MaxOccurrences;
	};

	std::optional<size_t>
	ParseSingleScenario(
		ScenarioDict& sd,
		DistributionSystem const& ds,
		toml::table const& table,
		std::string const& tag);

	std::vector<size_t>
	ParseScenarios(
		ScenarioDict& sd,
		DistributionSystem const& ds,
		toml::table const& table);

	void
	Scenario_Print(ScenarioDict const& sd, DistributionSystem const& ds);
}

#endif // ERIN_NEXT_SCENARIO_H