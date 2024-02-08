/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_simulation.h"
#include <assert.h>

namespace erin_next
{
	size_t
	Simulation_RegisterFlow(Simulation& s, std::string const& flowTag)
	{
		size_t id = s.FlowTypeMap.Type.size();
		for (size_t i=0; i < id; ++i)
		{
			if (s.FlowTypeMap.Type[i] == flowTag)
			{
				return i;
			}
		}
		s.FlowTypeMap.Type.push_back(flowTag);
		return id;
	}

	size_t
	Simulation_RegisterScenario(Simulation& s, std::string const& scenarioTag)
	{
		size_t id = s.ScenarioMap.Tags.size();
		for (size_t i = 0; i < id; ++i)
		{
			if (s.ScenarioMap.Tags[i] == scenarioTag)
			{
				return i;
			}
		}
		s.ScenarioMap.Tags.push_back(scenarioTag);
		return id;
	}

	size_t
	Simulation_RegisterLoadSchedule(
		Simulation& s,
		std::string const& tag,
		std::vector<TimeAndLoad> const& loadSchedule)
	{
		size_t id = s.LoadMap.Tags.size();
		assert(s.LoadMap.Tags.size() == s.LoadMap.Loads.size());
		for (size_t i = 0; i < id; ++i)
		{
			if (s.LoadMap.Tags[i] == tag)
			{
				s.LoadMap.Loads[i].clear();
				s.LoadMap.Loads[i] = loadSchedule;
				return i;
			}
		}
		s.LoadMap.Tags.push_back(tag);
		s.LoadMap.Loads.push_back(loadSchedule);
		return id;
	}

	std::optional<size_t>
	Simulation_GetLoadIdByTag(Simulation const& s, std::string const& tag)
	{
		for (size_t i = 0; i < s.LoadMap.Tags.size(); ++i)
		{
			if (s.LoadMap.Tags[i] == tag)
			{
				return i;
			}
		}
		return {};
	}

	void
	Simulation_PrintComponents(Simulation const& s, Model const& m)
	{
		for (size_t i = 0; i < m.ComponentMap.CompType.size(); ++i)
		{
			size_t outflowTypeIdx = m.ComponentMap.OutflowType[i];
			size_t inflowTypeIdx = m.ComponentMap.InflowType[i];
			std::cout << i << ": " << ToString(m.ComponentMap.CompType[i]);
			if (!m.ComponentMap.Tag[i].empty())
			{
				std::cout << " -- " << m.ComponentMap.Tag[i] << std::endl;
			}
			else
			{
				std::cout << std::endl;
			}
			if (inflowTypeIdx < s.FlowTypeMap.Type.size()
				&& !s.FlowTypeMap.Type[inflowTypeIdx].empty())
			{
				std::cout << "- inflow: "
					<< s.FlowTypeMap.Type[inflowTypeIdx] << std::endl;
			}
			if (outflowTypeIdx < s.FlowTypeMap.Type.size()
				&& !s.FlowTypeMap.Type[outflowTypeIdx].empty())
			{
				std::cout << "- outflow: "
					<< s.FlowTypeMap.Type[outflowTypeIdx] << std::endl;
			}
		}
	}

	void
	Simulation_PrintScenarios(Simulation const& s)
	{
		for (size_t i = 0; i < s.ScenarioMap.Tags.size(); ++i)
		{
			std::cout << i << ": " << s.ScenarioMap.Tags[i] << std::endl;
		}
	}

}