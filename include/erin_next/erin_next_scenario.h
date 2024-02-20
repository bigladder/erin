/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_SCENARIO_H
#define ERIN_NEXT_SCENARIO_H

#include "../vendor/toml11/toml.hpp"
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_result.h"
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
		std::vector<TimeUnit> TimeUnits;
		std::vector<double> Durations;
		// NOTE: an entry of none means "no max occurrences"; will take as
		// many as fit in the max time of the simulation (see SimulationInfo)
		std::vector<std::optional<size_t>> MaxOccurrences;
	};

	std::optional<size_t>
	ScenarioDict_GetScenarioByTag(ScenarioDict& sd, std::string const& tag);

	size_t
	ScenarioDict_RegisterScenario(ScenarioDict& sd, std::string const& tag);

	size_t
	ScenarioDict_RegisterScenario(
		ScenarioDict& sd,
		std::string const& tag,
		size_t occurrenceDistId,
		double duration,
		TimeUnit timeUnit,
		std::optional<size_t> maxOccurrences);

	std::optional<size_t>
	ParseSingleScenario(
		ScenarioDict& sd,
		DistributionSystem const& ds,
		toml::table const& table,
		std::string const& fullName,
		std::string const& tag);

	Result
	ParseScenarios(
		ScenarioDict& sd,
		DistributionSystem const& ds,
		toml::table const& table);

	void
	Scenario_Print(ScenarioDict const& sd, DistributionSystem const& ds);
}

#endif