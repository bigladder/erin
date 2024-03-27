/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_SIMULATION_H
#define ERIN_SIMULATION_H
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_simulation_info.h"
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next_scenario.h"
#include "erin_next/erin_next_result.h"
#include "../vendor/toml11/toml.hpp"
#include "erin_next/erin_next_validation.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdlib>

namespace erin
{
	struct Simulation
	{
		FlowDict FlowTypeMap;
		ScenarioDict ScenarioMap;
		LoadDict LoadMap;
		SimulationInfo Info;
		Model TheModel;
		std::vector<LinearFragilityCurve> LinearFragilityCurves;
		std::vector<TabularFragilityCurve> TabularFragilityCurves;
		IntensityDict Intensities;
		ScenarioIntensityDict ScenarioIntensities;
		FragilityCurveDict FragilityCurves;
		ComponentFragilityModeDict ComponentFragilities;
		FragilityModeDict FragilityModes;
		ComponentFailureModeDict ComponentFailureModes;
		FailureModeDict FailureModes;
	};

	std::string
	DoubleToString(double value, unsigned int precision);

	void
	Simulation_Init(Simulation& s);

	size_t
	Simulation_RegisterFlow(Simulation& s, std::string const& flowTag);

	size_t
	Simulation_RegisterScenario(Simulation& s, std::string const& scenarioTag);

	size_t
	Simulation_RegisterIntensity(Simulation& s, std::string const& tag);

	size_t
	Simulation_RegisterIntensityLevelForScenario(
		Simulation& s,
		size_t scenarioId,
		size_t intensityId,
		double intensityLevel
	);

	size_t
	Simulation_RegisterLoadSchedule(
		Simulation& s,
		std::string const& tag,
		std::vector<TimeAndAmount> const& loadSchedule
	);

	std::optional<size_t>
	Simulation_GetLoadIdByTag(Simulation const& s, std::string const& tag);

	void
	Simulation_RegisterAllLoads(Simulation& s, std::vector<Load> const& loads);

	void
	Simulation_PrintComponents(Simulation const& s);

	void
	Simulation_PrintFragilityCurves(Simulation const& s);

	void
	Simulation_PrintFailureModes(Simulation const& s);

	void
	Simulation_PrintFragilityModes(Simulation const& s);

	void
	Simulation_PrintScenarios(Simulation const& s);

	void
	Simulation_PrintLoads(Simulation const& s);

	size_t
	Simulation_ScenarioCount(Simulation const& s);

	Result
	Simulation_ParseSimulationInfo(
		Simulation& s,
		toml::value const& v,
		ValidationInfo const& validationInfo);

	Result
	Simulation_ParseLoads(
		Simulation& s,
		toml::value const& v,
		ValidationInfo const& explicitValidation,
		ValidationInfo const& fileBasedValidation);

	size_t
	Simulation_RegisterFragilityCurve(Simulation& s, std::string const& tag);

	size_t
	Simulation_RegisterFragilityCurve(
		Simulation& s,
		std::string const& tag,
		FragilityCurveType curveType,
		size_t curveIdx
	);

	size_t
	Simulation_RegisterFailureMode(
		Simulation& s,
		std::string const& tag,
		size_t failureId,
		size_t repairId
	);

	size_t
	Simulation_RegisterFragilityMode(
		Simulation& s,
		std::string const& tag,
		size_t fragilityCurveId,
		std::optional<size_t> maybeRepairDistId
	);

	Result
	Simulation_ParseFragilityCurves(Simulation& s, std::string const& v);

	Result
	Simulation_ParseFragilityModes(Simulation& s, toml::value const& v);

	Result
	Simulation_ParseComponents(
		Simulation& s,
		toml::value const& v,
		ComponentValidationMap const& compValidations
	);

	Result
	Simulation_ParseDistributions(Simulation& s, toml::value const& v);

	Result
	Simulation_ParseNetwork(Simulation& s, toml::value const& v);

	Result
	Simulation_ParseScenarios(Simulation& s, toml::value const& v);

	std::optional<Simulation>
	Simulation_ReadFromToml(
	  toml::value const& v,
	  InputValidationMap const& validationInfo);

	void
	Simulation_Print(Simulation const& s);

	void
	Simulation_PrintIntensities(Simulation const& s);

	Result
	SetLoadsForScenario(
		std::vector<ScheduleBasedLoad>& loads,
		LoadDict loadMap,
		size_t scenarioIdx
	);

	Result
	SetSupplyForScenario(
		std::vector<ScheduleBasedSource>& loads,
		LoadDict loadMap,
		size_t scenarioIdx
	);

	std::vector<double>
	DetermineScenarioOccurrenceTimes(
		Simulation& s,
		size_t scenIdx,
		bool isVerbose
	);

	std::map<size_t, double>
	GetIntensitiesForScenario(Simulation& s, size_t scenIdx);

	std::vector<ScheduleBasedReliability>
	CopyReliabilities(Simulation const& s);

	std::vector<ScheduleBasedReliability>
	ApplyReliabilitiesAndFragilities(
		Simulation& s,
		double startTime_s,
		double endTime_s,
		std::map<size_t, double> const& intensityIdToAmount,
		std::map<size_t, std::vector<TimeState>> const& relSchByCompId
	);

	void
	Simulation_Run(Simulation& s);

	bool
	Simulation_IsFailureNameUnique(Simulation& s, std::string const& name);

	bool
	Simulation_IsFailureModeNameUnique(Simulation& s, std::string const& name);

	bool
	Simulation_IsFragilityModeNameUnique(
		Simulation& s,
		std::string const& name
	);

	Result
	Simulation_ParseFailureModes(Simulation& s, toml::value const& v);

	Result
	Simulation_ParseLinearFragilityCurve(
		Simulation& s,
		std::string const& fcName,
		std::string const& tableFullName,
		toml::table const& fcData
	);

	std::optional<size_t>
	Parse_VulnerableTo(
		Simulation const& s,
		toml::table const& fcData,
		std::string const& tableFullName
	);

	std::vector<size_t>
	CalculateConnectionOrder(Simulation const& s);

	std::vector<size_t>
	CalculateScenarioOrder(Simulation const& s);

	std::vector<size_t>
	CalculateComponentOrder(Simulation const& s);

	std::vector<size_t>
	CalculateStoreOrder(Simulation const& s);

	std::vector<size_t>
	CalculateFailModeOrder(Simulation const& s);

	std::vector<size_t>
	CalculateFragilModeOrder(Simulation const& s);

	void
	WriteEventFileHeader(
		std::ofstream& out,
		Model const& m,
		std::vector<size_t> const& connOrder,
		std::vector<size_t> const& storeOrder,
		std::vector<size_t> const& compOrder
	);

	void
	WriteResultsToEventFile(
		std::ofstream& out,
		std::vector<TimeAndFlows> results,
		Simulation const& s,
		std::string const& scenarioTag,
		std::string const& scenarioStartTimeTag,
		std::vector<size_t> const& connOrder
	);

	void
	WriteStatisticsToFile(
		Simulation const& s,
		std::string const& statsFilePath,
		std::vector<ScenarioOccurrenceStats> const& occurrenceStats,
		std::vector<size_t> const& compOrder
	);

}

#endif
