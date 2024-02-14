/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_simulation.h"
#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_utils.h"
#include <assert.h>
#include <fstream>

namespace erin_next
{
	void
	Simulation_Init(Simulation& s)
	{
		// NOTE: we register a 'null' flow. This allows users to 'opt-out'
		// of flow specification by passing empty strings. Effectively, this
		// allows any connections to occur which is nice for simple examples.
		Simulation_RegisterFlow(s, "");
		s.Model.RandFn = []() { return 0.4; };
	}

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
		std::vector<TimeAndAmount> const& loadSchedule)
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
	Simulation_RegisterAllLoads(
		Simulation& s,
		std::vector<Load> const& loads)
	{
		s.LoadMap.Tags.clear();
		s.LoadMap.RateUnits.clear();
		s.LoadMap.Loads.clear();
		s.LoadMap.TimeUnits.clear();
		auto numLoads = loads.size();
		s.LoadMap.Tags.reserve(numLoads);
		s.LoadMap.RateUnits.reserve(numLoads);
		s.LoadMap.Loads.reserve(numLoads);
		s.LoadMap.TimeUnits.reserve(numLoads);
		for (size_t i = 0; i < numLoads; ++i)
		{
			s.LoadMap.Tags.push_back(loads[i].Tag);
			s.LoadMap.RateUnits.push_back(loads[i].RateUnit);
			s.LoadMap.Loads.push_back(loads[i].TimeAndLoads);
			s.LoadMap.TimeUnits.push_back(loads[i].TimeUnit);
		}
	}

	void
	Simulation_PrintComponents(Simulation const& s, Model const& m)
	{
		for (size_t i = 0; i < m.ComponentMap.CompType.size(); ++i)
		{
			std::vector<size_t> const& outflowTypes =
				m.ComponentMap.OutflowType[i];
			std::vector<size_t> inflowTypes =
				m.ComponentMap.InflowType[i];
			std::cout << i << ": " << ToString(m.ComponentMap.CompType[i]);
			if (!m.ComponentMap.Tag[i].empty())
			{
				std::cout << " -- " << m.ComponentMap.Tag[i] << std::endl;
			}
			else
			{
				std::cout << std::endl;
			}
			for (size_t inport = 0;
				inport < inflowTypes.size();
				++inport)
			{
				size_t inflowType = inflowTypes[inport];
				if (inflowType < s.FlowTypeMap.Type.size()
					&& !s.FlowTypeMap.Type[inflowType].empty())
				{
					std::cout << "- inport " << inport << ": "
						<< s.FlowTypeMap.Type[inflowType] << std::endl;
				}
			}
			for (size_t outport = 0;
				outport < outflowTypes.size();
				++outport)
			{
				size_t outflowType = outflowTypes[outport];
				if (outflowType < s.FlowTypeMap.Type.size()
					&& !s.FlowTypeMap.Type[outflowType].empty())
				{
					std::cout << "- outport " << outport << ": "
						<< s.FlowTypeMap.Type[outflowType] << std::endl;
				}
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

	void
	Simulation_PrintLoads(Simulation const& s)
	{
		for (size_t i = 0; i < s.LoadMap.Tags.size(); ++i)
		{
			std::cout << i << ": " << s.LoadMap.Tags[i] << std::endl;
			std::cout << "- load entries: "
				<< s.LoadMap.Loads[i].size() << std::endl;
			if (s.LoadMap.Loads[i].size() > 0)
			{
				// TODO: add time units
				std::cout << "- initial time: "
					<< s.LoadMap.Loads[i][0].Time << std::endl;
				// TODO: add time units
				std::cout << "- final time  : "
					<< s.LoadMap.Loads[i][s.LoadMap.Loads[i].size() - 1].Time
					<< std::endl;
				// TODO: add max rate
				// TODO: add min rate
				// TODO: add average rate
			}
		}
		/*
		std::cout << "Loads:" << std::endl;
		for (size_t i = 0; i < loads.size(); ++i)
		{
			std::cout << i << ": " << loads[i] << std::endl;
		}
		*/
	}

	size_t
	Simulation_ScenarioCount(Simulation const& s)
	{
		return s.ScenarioMap.Tags.size();
	}

	Result
	Simulation_ParseSimulationInfo(Simulation& s, toml::value const& v)
	{
		if (!v.contains("simulation_info"))
		{
			std::cout << "Required section [simulation_info] not found"
				<< std::endl;
			return Result::Failure;
		}
		toml::value const& simInfoTable = v.at("simulation_info");
		auto maybeSimInfo = ParseSimulationInfo(simInfoTable.as_table());
		if (!maybeSimInfo.has_value())
		{
			return Result::Failure;
		}
		s.Info = std::move(maybeSimInfo.value());
		return Result::Success;
	}

	Result
	Simulation_ParseLoads(Simulation& s, toml::value const& v)
	{
		toml::value const& loadTable = v.at("loads");
		auto maybeLoads = ParseLoads(loadTable.as_table());
		if (!maybeLoads.has_value())
		{
			return Result::Failure;
		}
		std::vector<Load> loads = std::move(maybeLoads.value());
		Simulation_RegisterAllLoads(s, loads);
		return Result::Success;
	}

	Result
	Simulation_ParseComponents(Simulation& s, toml::value const& v)
	{
		if (v.contains("components") && v.at("components").is_table())
		{
			return ParseComponents(s, s.Model, v.at("components").as_table());
		}
		std::cout << "required field 'components' not found" << std::endl;
		return Result::Failure;
	}

	Result
	Simulation_ParseDistributions(Simulation& s, toml::value const& v)
	{
		if (v.contains("dist") && v.at("dist").is_table())
		{
			// TODO: have ParseDistributions return a Result
			ParseDistributions(s.Model.DistSys, v.at("dist").as_table());
			return Result::Success;
		}
		std::cout << "required field 'dist' not found" << std::endl;
		return Result::Failure;
	}

	Result
	Simulation_ParseNetwork(Simulation& s, toml::value const& v)
	{
		std::string const n = "network";
		if (v.contains(n) && v.at(n).is_table())
		{
			return ParseNetwork(
				s.FlowTypeMap, s.Model, v.at(n).as_table());
		}
		else
		{
			std::cout << "required field '" << n << "' not found" << std::endl;
			return Result::Failure;
		}
	}

	Result
	Simulation_ParseScenarios(Simulation& s, toml::value const& v)
	{
		if (v.contains("scenarios") && v.at("scenarios").is_table())
		{
			return ParseScenarios(
				s.ScenarioMap, s.Model.DistSys, v.at("scenarios").as_table());
		}
		std::cout << "required field 'scenarios' not found or not a table"
			<< std::endl;
		return Result::Failure;
	}
	
	std::optional<Simulation>
	Simulation_ReadFromToml(toml::value const& v)
	{
		Simulation s = {};
		Simulation_Init(s);
		if (Simulation_ParseSimulationInfo(s, v) == Result::Failure)
		{
			return {};
		}
		if (Simulation_ParseLoads(s, v) == Result::Failure)
		{
			return {};
		}
		// Components
		if (Simulation_ParseComponents(s, v) == Result::Failure)
		{
			return {};
		}
		// Distributions
		if (Simulation_ParseDistributions(s, v) == Result::Failure)
		{
			return {};
		}
		// Network
		if (Simulation_ParseNetwork(s, v) == Result::Failure)
		{
			return {};
		}
		// Scenarios
		if (Simulation_ParseScenarios(s, v) == Result::Failure)
		{
			return {};
		}
		return s;
	}

	void
	Simulation_Print(Simulation const& s)
	{
		std::cout << "-----------------" << std::endl;
		std::cout << s.Info << std::endl;
		std::cout << "\nLoads:" << std::endl;
		Simulation_PrintLoads(s);
		std::cout << "\nComponents:" << std::endl;
		Simulation_PrintComponents(s, s.Model);
		std::cout << "\nDistributions:" << std::endl;
		s.Model.DistSys.print_distributions();
		std::cout << "\nConnections:" << std::endl;
		Model_PrintConnections(s.Model, s.FlowTypeMap);
		std::cout << "\nScenarios:" << std::endl;
		Scenario_Print(s.ScenarioMap, s.Model.DistSys);
	}

	void
	Simulation_Run(Simulation& s)
	{
		// TODO: expose proper options
		bool option_verbose = true;
		std::string eventFilePath{ "out.csv" };
		// TODO: check the components and network:
		// -- that all components are hooked up to something
		// -- that no port is double linked
		// -- that all connections have the correct flows
		// -- that required ports are linked
		// -- check that we have a proper acyclic graph?
		// NOW, we want to do a simulation for each scenario
		// TODO: generate reliability information from time = 0 to final time
		// ... from sim info. Those schedules will be clipped to the times of
		// ... each scenario's instance.
		// TODO: generate a data structure to hold all results.
		// TODO: set random function for Model based on SimInfo
		std::ofstream out;
		out.open(eventFilePath);
		if (!out.good())
		{
			std::cout << "Could not open '"
				<< eventFilePath << "' for writing."
				<< std::endl;
			return;
		}
		out << "scenario id,"
			<< "scenario start time (P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]),"
			<< "elapsed (hours)\n";
		for (size_t scenIdx = 0;
			scenIdx < Simulation_ScenarioCount(s);
			++scenIdx)
		{
			std::string const& scenarioTag = s.ScenarioMap.Tags[scenIdx];
			// for this scenario, ensure all schedule-based components
			// have the right schedule set for this scenario
			for (size_t sblIdx = 0;
				sblIdx < s.Model.ScheduledLoads.size();
				++sblIdx)
			{
				if (s.Model.ScheduledLoads[sblIdx]
					.ScenarioIdToLoadId.contains(scenIdx))
				{
					auto loadId =
						s.Model.ScheduledLoads[sblIdx]
						.ScenarioIdToLoadId.at(scenIdx);
					std::vector<TimeAndAmount> schedule{};
					size_t numEntries = s.LoadMap.Loads[loadId].size();
					schedule.reserve(numEntries);
					for (size_t i = 0; i < numEntries; ++i)
					{
						TimeAndAmount tal{};
						tal.Time = Time_ToSeconds(
							s.LoadMap.Loads[loadId][i].Time,
							s.LoadMap.TimeUnits[loadId]);
						tal.Amount = s.LoadMap.Loads[loadId][i].Amount;
						schedule.push_back(std::move(tal));
					}
					s.Model.ScheduledLoads[sblIdx].TimesAndLoads = schedule;
				}
			}
			// TODO: implement load substitution for schedule-based sources
			// for (size_t sbsIdx = 0; sbsIdx < s.Model.ScheduleSrcs.size(); ++sbsIdx) {/* ... */}
			// TODO: implement occurrences of the scenario in time.
			// for now, we know a priori that we have a max occurrence of 1
			std::vector<double> occurrenceTimes_s = { 0.0 };
			for (double t : occurrenceTimes_s)
			{
				double duration_s = Time_ToSeconds(
					s.ScenarioMap.Durations[scenIdx],
					s.ScenarioMap.TimeUnits[scenIdx]);
				std::string scenarioStartTime =
					TimeToISO8601Period(
						static_cast<uint64_t>(std::llround(t)));
				// TODO: compute end time for clipping
				double tEnd = t + duration_s;
				if (option_verbose)
				{
					std::cout << "Running " << s.ScenarioMap.Tags[scenIdx]
						<< " from " << t << " to " << tEnd << " s"
						<< std::endl;
				}
				// TODO: clip reliability schedules here
				s.Model.FinalTime = duration_s;
				SimulationState ss{};
				// TODO: add an optional verbosity flag to SimInfo
				// -- use that to set things like the print flag below
				auto results = Simulate(s.Model, ss, option_verbose);
				// TODO: investigate putting output on another thread
				for (auto const& r : results)
				{
					out << scenarioTag << ","
						<< scenarioStartTime << ","
						<< (r.Time / seconds_per_hour) << "\n";
				}
			}
		}
		out.close();
	}
}