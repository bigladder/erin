/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_SIMULATION_H
#define ERIN_NEXT_SIMULATION_H
#include "erin_next/erin_next.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdlib>

namespace erin_next
{
	struct Simulation
	{
		FlowType FlowTypeMap;
		ScenarioType ScenarioMap;
		LoadType LoadMap;
	};

	size_t
	Simulation_RegisterFlow(Simulation& s, std::string const& flowTag);

	size_t
	Simulation_RegisterScenario(Simulation& s, std::string const& scenarioTag);

	size_t
	Simulation_RegisterLoadSchedule(
		Simulation& s,
		std::string const& tag,
		std::vector<TimeAndLoad> const& loadSchedule);

	std::optional<size_t>
	Simulation_GetLoadIdByTag(Simulation const& s, std::string const& tag);

	void
	Simulation_PrintComponents(
		Simulation const& s, Model const& m);

	void
	Simulation_PrintScenarios(Simulation const& s);
}

#endif