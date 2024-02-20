/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_simulation.h"
#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_utils.h"
#include "erin_next/erin_next_toml.h"
#include <assert.h>
#include <fstream>
#include <vector>
#include <map>

namespace erin_next
{
	void
	Simulation_Init(Simulation& s)
	{
		// NOTE: we register a 'null' flow. This allows users to 'opt-out'
		// of flow specification by passing empty strings. Effectively, this
		// allows any connections to occur which is nice for simple examples.
		Simulation_RegisterFlow(s, "");
		s.TheModel.RandFn = []() { return 0.4; };
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
		return ScenarioDict_RegisterScenario(s.ScenarioMap, scenarioTag);
	}

	size_t
	Simulation_RegisterIntensity(Simulation& s, std::string const& tag)
	{
		for (size_t i=0; i<s.Intensities.Tags.size(); ++i)
		{
			if (s.Intensities.Tags[i] == tag)
			{
				return i;
			}
		}
		size_t id = s.Intensities.Tags.size();
		s.Intensities.Tags.push_back(tag);
		return id;
	}

	size_t
	Simulation_RegisterIntensityLevelForScenario(
		Simulation& s,
		size_t scenarioId,
		size_t intensityId,
		double intensityLevel)
	{
		for (size_t i = 0; i < s.ScenarioIntensities.IntensityIds.size(); ++i)
		{
			if (s.ScenarioIntensities.IntensityIds[i] == intensityId
				&& s.ScenarioIntensities.ScenarioIds[i] == scenarioId)
			{
				s.ScenarioIntensities.IntensityLevels[i] = intensityLevel;
				return i;
			}
		}
		size_t id = s.ScenarioIntensities.IntensityIds.size();
		s.ScenarioIntensities.ScenarioIds.push_back(scenarioId);
		s.ScenarioIntensities.IntensityIds.push_back(intensityId);
		s.ScenarioIntensities.IntensityLevels.push_back(intensityLevel);
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
			s.LoadMap.TimeUnits.push_back(loads[i].TheTimeUnit);
		}
	}

	void
	Simulation_PrintComponents(Simulation const& s)
	{
		Model const& m = s.TheModel;
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
			switch (m.ComponentMap.CompType[i])
			{
				case (ComponentType::ScheduleBasedLoadType):
				{
					ScheduleBasedLoad const& sbl =
						m.ScheduledLoads[m.ComponentMap.Idx[i]];
					for (auto const& keyValue : sbl.ScenarioIdToLoadId)
					{
						std::cout << "-- for scenario: "
							<< s.ScenarioMap.Tags[keyValue.first]
							<< ", use load: "
							<< s.LoadMap.Tags[keyValue.second]
							<< std::endl;
					}
				} break;
			}
		}
	}

	void
	Simulation_PrintFragilityCurves(Simulation const& s)
	{
		assert(s.FragilityCurves.CurveId.size() ==
			s.FragilityCurves.CurveTypes.size());
		assert(s.FragilityCurves.CurveId.size() ==
			s.FragilityCurves.Tags.size());
		for (size_t i=0; i<s.FragilityCurves.CurveId.size(); ++i)
		{
			std::cout << i << ": "
				<< FragilityCurveTypeToTag(s.FragilityCurves.CurveTypes[i])
				<< " -- " << s.FragilityCurves.Tags[i] << std::endl;
			size_t idx = s.FragilityCurves.CurveId[i];
			switch (s.FragilityCurves.CurveTypes[i])
			{
				case (FragilityCurveType::Linear):
				{
					std::cout << "-- lower bound: "
						<< s.LinearFragilityCurves[idx].LowerBound
						<< std::endl;
					std::cout << "-- upper bound: "
						<< s.LinearFragilityCurves[idx].UpperBound
						<< std::endl;
					size_t intensityId =
						s.LinearFragilityCurves[idx].VulnerabilityId;
					std::cout << "-- vulnerable to: "
						<< s.Intensities.Tags[intensityId]
						<< "[" << intensityId << "]" << std::endl;
				} break;
			}
		}
	}

	void
	Simulation_PrintFragilityModes(Simulation const& s)
	{
		for (size_t i = 0; i < s.FragilityModes.Tags.size(); ++i)
		{
			std::cout << i << ": " << s.FragilityModes.Tags[i] << std::endl;
			std::cout << "-- fragility curve: "
				<< s.FragilityCurves.Tags[s.FragilityModes.FragilityCurveId[i]]
				<< "[" << s.FragilityModes.FragilityCurveId[i] << "]"
				<< std::endl;
			if (s.FragilityModes.RepairDistIds[i].has_value())
			{
				std::optional<Distribution> maybeDist =
					s.TheModel.DistSys.get_dist_by_id(
						s.FragilityModes.RepairDistIds[i].value());
				if (maybeDist.has_value())
				{
					Distribution d = maybeDist.value();
					std::cout << "-- repair dist: "
						<< d.Tag
						<< "[" << s.FragilityModes.RepairDistIds[i].value()
						<< "]" << std::endl;
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
			std::cout << "- duration: " << s.ScenarioMap.Durations[i]
				<< " " << TimeUnitToTag(s.ScenarioMap.TimeUnits[i])
				<< std::endl;
			auto maybeDist =
				s.TheModel.DistSys.get_dist_by_id(
					s.ScenarioMap.OccurrenceDistributionIds[i]);
			if (maybeDist.has_value())
			{
				Distribution d = maybeDist.value();
				std::cout << "- occurrence distribution: "
					<< dist_type_to_tag(d.Type)
					<< "[" << 
						s.ScenarioMap.OccurrenceDistributionIds[i] << "] -- "
					<< d.Tag << std::endl;
			}
			std::cout << "- max occurrences: ";
			if (s.ScenarioMap.MaxOccurrences[i].has_value())
			{
				std::cout << s.ScenarioMap.MaxOccurrences[i].value()
					<< std::endl;
			}
			else
			{
				std::cout << "no limit" << std::endl;
			}
			bool printedHeader = false;
			for (size_t siIdx=0;
				siIdx < s.ScenarioIntensities.IntensityIds.size();
				++siIdx)
			{
				if (s.ScenarioIntensities.ScenarioIds[siIdx] == i)
				{
					if (!printedHeader)
					{
						std::cout << "- intensities:" << std::endl;
						printedHeader = true;
					}
					auto intId = s.ScenarioIntensities.IntensityIds[siIdx];
					auto const& intTag = s.Intensities.Tags[intId];
					std::cout << "-- " << intTag << "[" << intId << "]: "
						<< s.ScenarioIntensities.IntensityLevels[siIdx]
						<< std::endl;
				}
			}
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

	// TODO: change this to a std::optional<size_t> GetFragilityCurveByTag()
	// if it returns !*.has_value(), register with the bogus data explicitly.
	size_t
	Simulation_RegisterFragilityCurve(Simulation& s, std::string const& tag)
	{
		return Simulation_RegisterFragilityCurve(
			s, tag, FragilityCurveType::Linear, 0);
	}

	size_t
	Simulation_RegisterFragilityCurve(
		Simulation& s,
		std::string const& tag,
		FragilityCurveType curveType,
		size_t curveIdx)
	{
		for (size_t i = 0; i < s.FragilityCurves.Tags.size(); ++i)
		{
			if (s.FragilityCurves.Tags[i] == tag)
			{
				s.FragilityCurves.CurveId[i] = curveIdx;
				s.FragilityCurves.CurveTypes[i] = curveType;
				return i;
			}
		}
		size_t id = s.FragilityCurves.Tags.size();
		s.FragilityCurves.Tags.push_back(tag);
		s.FragilityCurves.CurveId.push_back(curveIdx);
		s.FragilityCurves.CurveTypes.push_back(curveType);
		return id;
	}

	size_t
	Simulation_RegisterFragilityMode(
		Simulation& s,
		std::string const& tag,
		size_t fragilityCurveId,
		std::optional<size_t> maybeRepairDistId)
	{
		for (size_t i=0; i<s.FragilityModes.Tags.size(); ++i)
		{
			if (s.FragilityModes.Tags[i] == tag)
			{
				s.FragilityModes.FragilityCurveId[i] = fragilityCurveId;
				s.FragilityModes.RepairDistIds[i] = maybeRepairDistId;
				return i;
			}
		}
		size_t id = s.FragilityModes.Tags.size();
		s.FragilityModes.Tags.push_back(tag);
		s.FragilityModes.FragilityCurveId.push_back(fragilityCurveId);
		s.FragilityModes.RepairDistIds.push_back(maybeRepairDistId);
		return id;
	}

	Result
	Simulation_ParseFragilityCurves(Simulation& s, toml::value const& v)
	{
		if (v.contains("fragility_curve"))
		{
			if (!v.at("fragility_curve").is_table())
			{
				std::cout << "[fragility_curve] not a table" << std::endl;
				return Result::Failure;
			}
			for (auto const& pair : v.at("fragility_curve").as_table())
			{
				std::string const& fcName = pair.first;
				std::string tableFullName =
					"fragility_curve." + fcName;
				if (!pair.second.is_table())
				{
					std::cout << "[" << tableFullName << "] not a table"
						<< std::endl;
					return Result::Failure;
				}
				toml::table const& fcData = pair.second.as_table();
				if (!fcData.contains("type"))
				{
					std::cout << "[" << tableFullName << "] "
						<< "does not contain required value 'type'"
						<< std::endl;
					return Result::Failure;
				}
				std::string const& typeStr = fcData.at("type").as_string();
				std::optional<FragilityCurveType> maybeFct =
					TagToFragilityCurveType(typeStr);
				if (!maybeFct.has_value())
				{
					std::cout << "[" << tableFullName << "] "
						<< "could not interpret type as string"
						<< std::endl;
					return Result::Failure;
				}
				FragilityCurveType fct = maybeFct.value();
				switch (fct)
				{
					case (FragilityCurveType::Linear):
					{
						if (!fcData.contains("lower_bound"))
						{
							std::cout << "[" << tableFullName << "] "
								<< "missing required field 'lower_bound'"
								<< std::endl;
							return Result::Failure;
						}
						if (!(fcData.at("lower_bound").is_floating()
							|| fcData.at("lower_bound").is_integer()))
						{
							std::cout << "[" << tableFullName << "] "
								<< "field 'lower_bound' not a number"
								<< std::endl;
							return Result::Failure;
						}
						std::optional<double> maybeLowerBound =
							TOMLTable_ParseDouble(
								fcData,
								"lower_bound",
								tableFullName);
						if (!maybeLowerBound.has_value())
						{
							std::cout << "[" << tableFullName << "] "
								<< "field 'lower_bound' has no value"
								<< std::endl;
							return Result::Failure;
						}
						double lowerBound = maybeLowerBound.value();
						if (!fcData.contains("upper_bound"))
						{
							std::cout << "[" << tableFullName << "] "
								<< "missing required field 'upper_bound'"
								<< std::endl;
							return Result::Failure;
						}
						if (!(fcData.at("upper_bound").is_floating()
							|| fcData.at("upper_bound").is_integer()))
						{
							std::cout << "[" << tableFullName << "] "
								<< "field 'upper_bound' not a number"
								<< std::endl;
							return Result::Failure;
						}
						std::optional<double> maybeUpperBound =
							TOMLTable_ParseDouble(
								fcData,
								"upper_bound",
								tableFullName);
						if (!maybeUpperBound.has_value())
						{
							std::cout << "[" << tableFullName << "] "
								<< "field 'upper_bound' has no value"
								<< std::endl;
							return Result::Failure;
						}
						double upperBound = maybeUpperBound.value();
						if (!fcData.contains("vulnerable_to"))
						{
							std::cout << "[" << tableFullName << "] "
								<< "missing required field 'vulnerable_to'"
								<< std::endl;
							return Result::Failure;
						}
						if (!fcData.at("vulnerable_to").is_string())
						{
							std::cout << "[" << tableFullName << "] "
								<< "field 'vulnerable_to' not a string"
								<< std::endl;
							return Result::Failure;
						}
						std::string const& vulnerStr =
							fcData.at("vulnerable_to").as_string();
						std::optional<size_t> maybeIntId =
							GetIntensityIdByTag(s.Intensities, vulnerStr);
						if (!maybeIntId.has_value())
						{
							std::cout << "[" << tableFullName << "] "
								<< "could not find referenced intensity '"
								<< vulnerStr << "' for 'vulnerable_to'"
								<< std::endl;
							return Result::Failure;
						}
						size_t intensityId = maybeIntId.value();
						LinearFragilityCurve lfc{};
						lfc.LowerBound = lowerBound;
						lfc.UpperBound = upperBound;
						lfc.VulnerabilityId = intensityId;
						size_t idx = s.LinearFragilityCurves.size();
						s.LinearFragilityCurves.push_back(std::move(lfc));
						Simulation_RegisterFragilityCurve(s, fcName, fct, idx);
					} break;
					default:
					{
						throw std::runtime_error{
							"unhandled fragility curve type"};
					} break;
				}
			}
		}
		return Result::Success;
	}

	Result
	Simulation_ParseFragilityModes(Simulation& s, toml::value const& v)
	{
		if (v.contains("fragility_mode"))
		{
			if (!v.at("fragility_mode").is_table())
			{
				return Result::Failure;
			}
			toml::table const& fmTable = v.at("fragility_mode").as_table();
			for (auto const& pair : fmTable)
			{
				std::string const& fmName = pair.first;
				if (!pair.second.is_table())
				{
					// TODO: add error message
					return Result::Failure;
				}
				toml::table const& fmValueTable = pair.second.as_table();
				if (!fmValueTable.contains("fragility_curve"))
				{
					// TODO: add error message
					return Result::Failure;
				}
				if (!fmValueTable.at("fragility_curve").is_string())
				{
					// TODO: add error message
					return Result::Failure;
				}
				std::string const& fcTag =
					fmValueTable.at("fragility_curve").as_string();
				size_t fcId =
					Simulation_RegisterFragilityCurve(s, fcTag);
				std::optional<size_t> maybeRepairDistId = {};
				if (fmValueTable.contains("repair_dist"))
				{
					if (!fmValueTable.at("repair_dist").is_string())
					{
						// TODO: add error message
						return Result::Failure;
					}
					std::string const& repairDistTag =
						fmValueTable.at("repair_dist").as_string();
					maybeRepairDistId =
						s.TheModel.DistSys.lookup_dist_by_tag(repairDistTag);
				}
				Simulation_RegisterFragilityMode(
					s, fmName, fcId, maybeRepairDistId);
			}
		}
		return Result::Success;
	}

	Result
	Simulation_ParseComponents(Simulation& s, toml::value const& v)
	{
		if (v.contains("components") && v.at("components").is_table())
		{
			return ParseComponents(s, s.TheModel, v.at("components").as_table());
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
			ParseDistributions(s.TheModel.DistSys, v.at("dist").as_table());
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
				s.FlowTypeMap, s.TheModel, v.at(n).as_table());
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
			auto result = ParseScenarios(
				s.ScenarioMap, s.TheModel.DistSys, v.at("scenarios").as_table());
			if (result == Result::Success)
			{
				for (auto const& pair : v.at("scenarios").as_table())
				{
					std::string const& scenarioName = pair.first;
					std::optional<size_t> maybeScenarioId =
						ScenarioDict_GetScenarioByTag(
							s.ScenarioMap, scenarioName);
					if (!maybeScenarioId.has_value())
					{
						std::cout << "[scenarios] "
							<< "could not find scenario id for '"
							<< scenarioName << "'" << std::endl;
						return Result::Failure;
					}
					size_t scenarioId = maybeScenarioId.value();
					std::string fullName = "scenarios." + scenarioName;
					if (!pair.second.is_table())
					{
						std::cout << "[" << fullName << "] "
							<< "must be a table" << std::endl;
					}
					toml::table const& data = pair.second.as_table();
					if (data.contains("intensity"))
					{
						if (!data.at("intensity").is_table())
						{
							std::cout << "[" << fullName << ".intensity] "
								<< "must be a table" << std::endl;
							return Result::Failure;
						}
						for (auto const& p : data.at("intensity").as_table())
						{
							std::string const& intensityTag = p.first;
							if (!(p.second.is_integer()
								|| p.second.is_floating()))
							{
								std::cout << "[" << fullName
									<< ".intensity." << intensityTag << "] "
									<< "must be a number" << std::endl;
								return Result::Failure;
							}
							std::optional<double> maybeValue =
								TOML_ParseNumericValueAsDouble(p.second);
							if (!maybeValue.has_value())
							{
								std::cout << "[" << fullName
									<< ".intensity." << intensityTag << "] "
									<< "must be a number" << std::endl;
								return Result::Failure;
							}
							double value = maybeValue.value();
							size_t intensityId =
								Simulation_RegisterIntensity(s, intensityTag);
							Simulation_RegisterIntensityLevelForScenario(
								s, scenarioId, intensityId, value);
						}
					}
				}
			}
			return result;
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
		if (Simulation_ParseComponents(s, v) == Result::Failure)
		{
			return {};
		}
		if (Simulation_ParseDistributions(s, v) == Result::Failure)
		{
			return {};
		}
		if (Simulation_ParseFragilityModes(s, v) == Result::Failure)
		{
			std::cout << "Problem parsing fragility modes..." << std::endl;
			return {};
		}
		if (Simulation_ParseNetwork(s, v) == Result::Failure)
		{
			return {};
		}
		if (Simulation_ParseScenarios(s, v) == Result::Failure)
		{
			return {};
		}
		if (Simulation_ParseFragilityCurves(s, v) == Result::Failure)
		{
			std::cout << "Problem parsing fragility curves..." << std::endl;
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
		Simulation_PrintComponents(s);
		std::cout << "\nDistributions:" << std::endl;
		s.TheModel.DistSys.print_distributions();
		std::cout << "\nFragility Curves:" << std::endl;
		Simulation_PrintFragilityCurves(s);
		std::cout << "\nFragility Modes:" << std::endl;
		Simulation_PrintFragilityModes(s);
		std::cout << "\nConnections:" << std::endl;
		Model_PrintConnections(s.TheModel, s.FlowTypeMap);
		std::cout << "\nScenarios:" << std::endl;
		Simulation_PrintScenarios(s);
		std::cout << "\nIntensities:" << std::endl;
		Simulation_PrintIntensities(s);
	}

	void
	Simulation_PrintIntensities(Simulation const& s)
	{
		for (size_t i=0; i < s.Intensities.Tags.size(); ++i)
		{
			std::cout << i << ": " << s.Intensities.Tags[i] << std::endl;
		}
	}

	void
	Simulation_Run(Simulation& s)
	{
		// TODO: expose proper options
		bool option_verbose = true;
		std::string eventFilePath{ "out.csv" };
		std::string statsFilePath{ "stats.csv" };
		// TODO: check the components and network:
		// -- that all components are hooked up to something
		// -- that no port is double linked
		// -- that all connections have the correct flows
		// -- that required ports are linked
		// -- check that we have a proper acyclic graph?
		// TODO: generate reliability information from time = 0 to final time
		// ... from sim info. Those schedules will be clipped to the times of
		// ... each scenario's instance. We will also need to merge it with
		// ... fragility failure curves. We may want to consolidate to, at most
		// ... one curve per component but also track the (possibly multiple)
		// ... failure modes and fragility modes causing the failure
		// ... (provenance). Note: the fragility part will need to be moved
		// ... as fragility is "by scenario".
		// TODO: generate a data structure to hold all results.
		// TODO: set random function for Model based on SimInfo
		// TODO: split out file writing into separate thread? See
		// https://codetrips.com/2020/07/26/modern-c-writing-a-thread-safe-queue/comment-page-1/
		// NOW, we want to do a simulation for each scenario
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
			<< "elapsed (hours)";
		for (auto const& conn : s.TheModel.Connections)
		{
			std::string const& fromTag = s.TheModel.ComponentMap.Tag[conn.FromId];
			std::string const& toTag = s.TheModel.ComponentMap.Tag[conn.ToId];
			out << ","
				<< fromTag << ":OUT(" << conn.FromPort << ") => "
				<< toTag << ":IN(" << conn.ToPort << ") (kW)";
		}
		out << "\n";
		std::vector<ScenarioOccurrenceStats> occurrenceStats{};
		for (size_t scenIdx = 0;
			scenIdx < Simulation_ScenarioCount(s);
			++scenIdx)
		{
			std::string const& scenarioTag = s.ScenarioMap.Tags[scenIdx];
			std::cout << "Scenario: " << scenarioTag << std::endl;
			// for this scenario, ensure all schedule-based components
			// have the right schedule set for this scenario
			for (size_t sblIdx = 0;
				sblIdx < s.TheModel.ScheduledLoads.size();
				++sblIdx)
			{
				if (s.TheModel.ScheduledLoads[sblIdx]
					.ScenarioIdToLoadId.contains(scenIdx))
				{
					auto loadId =
						s.TheModel.ScheduledLoads[sblIdx]
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
					s.TheModel.ScheduledLoads[sblIdx].TimesAndLoads = schedule;
				}
				else
				{
					std::cout << "ERROR:"
						<< "Unhandled scenario id in ScenarioIdToLoadId"
						<< std::endl;
					return;
				}
			}
			// TODO: implement load substitution for schedule-based sources
			// for (size_t sbsIdx = 0; sbsIdx < s.Model.ScheduleSrcs.size(); ++sbsIdx) {/* ... */}
			// TODO: implement occurrences of the scenario in time.
			// for now, we know a priori that we have a max occurrence of 1
			std::vector<double> occurrenceTimes_s{};
			auto const& maybeMaxOccurrences =
				s.ScenarioMap.MaxOccurrences[scenIdx];
			size_t maxOccurrence =
				maybeMaxOccurrences.has_value()
				? maybeMaxOccurrences.value()
				: 1'000;
			auto const distId =
				s.ScenarioMap.OccurrenceDistributionIds[scenIdx];
			double scenarioStartTime_s = 0.0;
			double maxTime_s =
				Time_ToSeconds(s.Info.MaxTime, s.Info.TheTimeUnit);
			for (size_t numOccurrences = 0;
				numOccurrences < maxOccurrence;
				++numOccurrences)
			{
				scenarioStartTime_s +=
					s.TheModel.DistSys.next_time_advance(distId);
				if (scenarioStartTime_s >= maxTime_s)
				{
					break;
				}
				occurrenceTimes_s.push_back(scenarioStartTime_s);
			}
			std::cout << "Occurrences: " << occurrenceTimes_s.size()
				<< std::endl;
			for (size_t occIdx=0; occIdx < occurrenceTimes_s.size(); ++occIdx)
			{
				std::cout << "-- "
					<< SecondsToPrettyString(occurrenceTimes_s[occIdx])
					<< std::endl;
			}
			// TODO: initialize total scenario stats (i.e.,
			// over all occurrences)
			std::map<size_t, double> intensityIdToAmount{};
			for (size_t siIdx=0;
				siIdx<s.ScenarioIntensities.ScenarioIds.size();
				++siIdx)
			{
				if (s.ScenarioIntensities.ScenarioIds[siIdx] == scenIdx)
				{
					auto intensityId =
						s.ScenarioIntensities.IntensityIds[siIdx];
					intensityIdToAmount[intensityId]
						= s.ScenarioIntensities.IntensityLevels[siIdx];
				}
			}
			for (size_t occIdx = 0; occIdx < occurrenceTimes_s.size(); ++occIdx)
			{
				double t = occurrenceTimes_s[occIdx];
				double duration_s = Time_ToSeconds(
					s.ScenarioMap.Durations[scenIdx],
					s.ScenarioMap.TimeUnits[scenIdx]);
				std::cout << "Occurrence #" << (occIdx + 1) << " at "
					<< t << std::endl;
				// NOTE: if there are no intensities in this scenario, there
				// are no fragilities to apply/check.
				if (intensityIdToAmount.size() > 0)
				{
					std::cout << "... Applying fragilities" << std::endl;
					// NOTE: if there are no components having fragility modes,
					// there is nothing to do.
					for (size_t cfmIdx=0;
						cfmIdx<s.ComponentFragilities.ComponentIds.size();
						++cfmIdx)
					{
						size_t fmId =
							s.ComponentFragilities.FragilityModeIds[cfmIdx];
						size_t fcId =
							s.FragilityModes.FragilityCurveId[fmId];
						std::optional<size_t> repairId =
							s.FragilityModes.RepairDistIds[fmId];
						FragilityCurveType curveType =
							s.FragilityCurves.CurveTypes[fcId];
						size_t fcIdx =
							s.FragilityCurves.CurveId[fcId];
						bool isFailed = false;
						switch (curveType)
						{
							case (FragilityCurveType::Linear):
							{
								LinearFragilityCurve lfc =
									s.LinearFragilityCurves[fcIdx];
								size_t vulnerId = lfc.VulnerabilityId;
								if (intensityIdToAmount.contains(vulnerId))
								{
									double level =
										intensityIdToAmount[vulnerId];
									if (level > lfc.UpperBound)
									{
										isFailed = true;
									}
									else if (level < lfc.LowerBound)
									{
										isFailed = false;
									}
									else
									{
										double x = s.TheModel.RandFn();
										double range =
											lfc.UpperBound - lfc.LowerBound;
										double failureFrac =
											(level - lfc.LowerBound)
											/ range;
										isFailed = x <= failureFrac;
									}
								}
							} break;
							default:
							{
								throw std::runtime_error{
									"Unhandled FragilityCurveType"
								};
							} break;
						}
						// NOTE: if we are not failed, there is nothing to do
						if (isFailed)
						{
							// now we have to find the affected component
							// and assign/update a reliability schedule for it
							// including any repair distribution if we have
							// one.
							size_t compId =
								s.ComponentFragilities.ComponentIds[cfmIdx];
							// does the component have a reliability signal?
							bool hasReliabilityAlready = false;
							size_t reliabilityId = 0;
							for (size_t rIdx=0;
								rIdx<s.TheModel.Reliabilities.size();
								++rIdx)
							{
								if (s.TheModel.Reliabilities[rIdx].ComponentId
									== compId)
								{
									hasReliabilityAlready = true;
									reliabilityId = rIdx;
									break;
								}
							}
							if (repairId.has_value())
							{
								throw std::runtime_error{
									"not implemented yet"
								};
							}
							else
							{
								// failed for the duration of the scenario
								std::vector<TimeState> newTimeStates{};
								TimeState ts{};
								ts.state = false;
								ts.time = 0.0;
								newTimeStates.push_back(std::move(ts));
								if (hasReliabilityAlready)
								{
									s.TheModel.Reliabilities[reliabilityId]
										.TimeStates = newTimeStates;
								}
								else
								{
									ScheduleBasedReliability sbr{};
									sbr.ComponentId = compId;
									sbr.TimeStates = newTimeStates;
									s.TheModel.Reliabilities.push_back(
										std::move(sbr));
								}
							}
						}
					}
				}
				// END handle impacts due to fragility
				std::string scenarioStartTimeTag =
					TimeToISO8601Period(
						static_cast<uint64_t>(std::llround(t)));
				// TODO: compute end time for clipping
				double tEnd = t + duration_s;
				if (option_verbose)
				{
					std::cout << "Running " << s.ScenarioMap.Tags[scenIdx]
						<< " from " << scenarioStartTimeTag << " for "
						<< s.ScenarioMap.Durations[scenIdx] << " "
						<< TimeUnitToTag(s.ScenarioMap.TimeUnits[scenIdx])
						<< " (" << t << " to " << tEnd << " seconds)"
						<< std::endl;
				}
				// TODO: clip reliability schedules here
				s.TheModel.FinalTime = duration_s;
				// TODO: add an optional verbosity flag to SimInfo
				// -- use that to set things like the print flag below
				auto results = Simulate(s.TheModel, option_verbose);
				// TODO: investigate putting output on another thread
				for (auto const& r : results)
				{
					out << scenarioTag << ","
						<< scenarioStartTimeTag << ","
						<< (r.Time / seconds_per_hour);
					for (size_t connIdx = 0;
						connIdx < r.Flows.size();
						++connIdx)
					{
						out << "," << r.Flows[connIdx].Actual;
					}
					out << "\n";
				}
				ScenarioOccurrenceStats sos =
					ModelResults_CalculateScenarioOccurrenceStats(
						scenIdx, occIdx + 1, s.TheModel, results);
				occurrenceStats.push_back(std::move(sos));
			}
			std::cout << "Scenario " << scenarioTag << " finished" << std::endl;
			// TODO: merge per-occurrence stats with global for the current
			// scenario
		}
		out.close();
		// TODO: write out stats file
		std::ofstream stats;
		stats.open(statsFilePath);
		if (!stats.good())
		{
			std::cout << "Could not open '"
				<< statsFilePath << "' for writing."
				<< std::endl;
			return;
		}
		stats
			<< "scenario id,"
			<< "occurrence number,"
			<< "duration (h),"
			<< "total source (kJ),"
			<< "total load (kJ),"
			<< "total storage (kJ),"
			<< "total waste (kJ),"
			<< "energy balance (source-(load+storage+waste)) (kJ),"
			<< "site efficiency (%),"
			<< "uptime (h),"
			<< "downtime (h),"
			<< "load not served (kJ),"
			<< "energy robustness [ER] (%),"
			<< "energy availability [EA] (%),"
			<< "max single event downtime [MaxSEDT] (h)"
			<< std::endl;
		for (auto const& os : occurrenceStats)
		{
			double stored = os.StorageCharge_kJ - os.StorageDischarge_kJ;
			double balance =
				os.Inflow_kJ
				- (os.OutflowAchieved_kJ + stored + os.Wasteflow_kJ);
			double efficiency =
				os.Inflow_kJ > 0.0
				? os.OutflowAchieved_kJ * 100.0 / os.Inflow_kJ
				: 0.0;
			double ER =
				os.OutflowRequest_kJ > 0.0
				? (os.OutflowAchieved_kJ * 100.0 / os.OutflowRequest_kJ)
				: 0.0;
			double EA =
				os.Duration_s > 0.0
				? (os.Uptime_s * 100.0 / (os.Duration_s))
				: 0.0;
			stats << s.ScenarioMap.Tags[os.Id]
				<< "," << os.OccurrenceNumber
				<< "," << (os.Duration_s / seconds_per_hour)
				<< "," << os.Inflow_kJ
				<< "," << os.OutflowAchieved_kJ
				<< "," << stored
				<< "," << os.Wasteflow_kJ
				<< "," << balance
				<< "," << efficiency
				<< "," << (os.Uptime_s / seconds_per_hour)
				<< "," << (os.Downtime_s / seconds_per_hour)
				<< "," << os.LoadNotServed_kJ
				<< "," << ER
				<< "," << EA
				<< "," << (os.MaxSEDT_s / seconds_per_hour)
				<< std::endl;
		}
		
	}
}