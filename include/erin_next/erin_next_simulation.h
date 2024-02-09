/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_SIMULATION_H
#define ERIN_NEXT_SIMULATION_H
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_simulation_info.h"
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next_scenario.h"
#include "erin_next/erin_next_result.h"
#include "../vendor/toml11/toml.hpp"
#include <string>
#include <vector>
#include <optional>
#include <cstdlib>

namespace erin_next
{
	struct Simulation
	{
		FlowType FlowTypeMap;
		ScenarioDict ScenarioMap;
		LoadDict LoadMap;
		SimulationInfo Info;
	};

	void
	Simulation_Init(Simulation& s);

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
	Simulation_RegisterAllLoads(
		Simulation& s,
		std::vector<Load> const& loads);

	void
	Simulation_PrintComponents(
		Simulation const& s, Model const& m);

	void
	Simulation_PrintScenarios(Simulation const& s);

	void
	Simulation_PrintLoads(Simulation const& s);

	size_t
	Simulation_ScenarioCount(Simulation const& s);

	Result
	Simulation_ParseSimulationInfo(Simulation& s, toml::value const& v);
}

#endif