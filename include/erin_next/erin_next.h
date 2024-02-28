/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_H
#define ERIN_NEXT_H

#include "erin_next/erin_next_timestate.h"
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_reliability.h"
#include "erin_next/erin_next_time_and_amount.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_result.h"
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <optional>
#include <set>
#include <map>
#include <ostream>
#include <functional>

namespace erin_next
{

	double const infinity = -1.0;

	size_t const constEffConvOutflowPort = 0;
	size_t const constEffConvLossflowPort = 1;
	size_t const constEffConvWasteflowPort = 2;

	enum class ComponentType
	{
		ConstantLoadType,
		ScheduleBasedLoadType,
		ConstantSourceType,
		ScheduleBasedSourceType,
		ConstantEfficiencyConverterType,
		MuxType,
		StoreType,
		PassThroughType,
		WasteSinkType,
	};

	// Holds the various flow types encountered
	// NOTE: each entry added must be unique and the index into this
	// vector is the flow type used through the rest of the simulation
	// NOTE: if we end up wanting to have mass-flow and volumetric-flow,
	// we may want to have an enum for FlowCategory or FundamentalFlowType
	// with items of Power, Mass, and/or Volume. Each of those would have
	// a base unit associated with them. The base unit for flow for Power is
	// Watt, for example. For Mass, it might be kg/s (or g/s?).
	struct FlowDict
	{
		std::vector<std::string> Type;
	};

	struct LoadDict
	{
		std::vector<std::string> Tags;
		std::vector<std::vector<TimeAndAmount>> Loads;
	};

	// NOTE: arrays in struct below indexed by size_t which we call ComponentId
	struct ComponentDict
	{
		// The index into the component vector for the given component subtype
		std::vector<size_t> Idx;
		std::vector<ComponentType> CompType;
		std::vector<std::string> Tag;
		std::vector<double> InitialAges_s;
		// Component's inflow type by inport; result indexes FlowDict
		std::vector<std::vector<size_t>> InflowType;
		// Component's outflow type by outport; result indexes FlowDict
		std::vector<std::vector<size_t>> OutflowType;
	};

	struct FlowSummary
	{
		double Time = 0.0;
		uint32_t Inflow = 0;
		uint32_t OutflowRequest = 0;
		uint32_t OutflowAchieved = 0;
		uint32_t StorageDischarge = 0;
		uint32_t StorageCharge = 0;
		uint32_t Wasteflow = 0;
	};

	struct ScenarioOccurrenceStats
	{
		// Id of the scenario; indexes into Simulation.ScenarioMap
		size_t Id = 0;
		// The occurrence of this scenario; 1st occurrence is 1, 2nd is 2, etc.
		size_t OccurrenceNumber;
		double Duration_s = 0.0;
		double Inflow_kJ = 0.0;
		double OutflowRequest_kJ = 0.0;
		double OutflowAchieved_kJ = 0.0;
		double StorageDischarge_kJ = 0.0;
		double StorageCharge_kJ = 0.0;
		double Wasteflow_kJ = 0.0;
		double LoadNotServed_kJ = 0.0;
		double Uptime_s = 0.0;
		double Downtime_s = 0.0;
		double MaxSEDT_s = 0.0;
		double Availability_s = 0.0;
		std::map<size_t, double> AvailabilityByCompId_s;
		// Event Counts
		std::map<size_t, size_t> EventCountByFailureModeId;
		std::map<size_t, std::map<size_t, size_t>>
			EventCountByCompIdByFailureModeId;
		std::map<size_t, size_t> EventCountByFragilityModeId;
		std::map<size_t, std::map<size_t, size_t>>
			EventCountByCompIdByFragilityModeId;
		// Failure/Fragility Times
		std::map<size_t, double> TimeByFailureModeId_s;
		std::map<size_t, std::map<size_t, double>>
			TimeByCompIdByFailureModeId_s;
		std::map<size_t, double> TimeByFragilityModeId_s;
		std::map<size_t, std::map<size_t, double>>
			TimeByCompIdByFragilityModeId_s;
	};

	struct ConstantLoad
	{
		uint32_t Load_W;
		size_t InflowConn;
	};

	struct ScheduleBasedLoad
	{
		std::vector<TimeAndAmount> TimesAndLoads;
		size_t InflowConn;
		std::map<size_t, size_t> ScenarioIdToLoadId;
	};

	struct ScheduleBasedReliability
	{
		std::vector<TimeState> TimeStates;
		size_t ComponentId;
	};

	struct ConstantSource
	{
		uint32_t Available_W;
		size_t OutflowConn;
	};

	struct ScheduleBasedSource
	{
		std::vector<TimeAndAmount> TimeAndAvails;
		size_t OutflowConn;
		size_t WasteflowConn;
	};

	struct ConstantEfficiencyConverter
	{
		// NOTE: efficiency is a fraction in range (0.0, 1.0]
		double Efficiency;
		size_t InflowConn;
		size_t OutflowConn;
		std::optional<size_t> LossflowConn;
		size_t WasteflowConn;
	};

	struct Connection
	{
		ComponentType From = ComponentType::ConstantSourceType;
		// index into the specific component type's array
		size_t FromIdx = 0;
		size_t FromPort = 0;
		// index into ComponentDict
		size_t FromId = 0;
		ComponentType To = ComponentType::ConstantLoadType;
		// index into the specific component type's array
		size_t ToIdx = 0;
		size_t ToPort = 0;
		// index into ComponentDict
		size_t ToId = 0;
		size_t FlowTypeId = 0;
	};

	struct Mux
	{
		size_t NumInports;
		size_t NumOutports;
		std::vector<size_t> InflowConns;
		std::vector<size_t> OutflowConns;
	};

	struct Store
	{
		uint32_t Capacity_J;
		uint32_t MaxChargeRate_W;
		uint32_t MaxDischargeRate_W;
		// amount at or below which we request charge
		uint32_t ChargeAmount_J;
		uint32_t InitialStorage_J;
		size_t InflowConn;
		size_t OutflowConn;
	};

	struct PassThrough
	{
		size_t InflowConn = 0;
		size_t OutflowConn = 0;
	};

	struct Flow
	{
		uint32_t Requested_W = 0;
		uint32_t Available_W = 0;
		uint32_t Actual_W = 0;
	};

	struct TimeAndFlows
	{
		double Time;
		std::vector<Flow> Flows;
		std::vector<uint32_t> StorageAmounts_J;
	};


	struct Model
	{
		ComponentDict ComponentMap;
		std::vector<ConstantSource> ConstSources;
		std::vector<ScheduleBasedSource> ScheduledSrcs;
		std::vector<ConstantLoad> ConstLoads;
		std::vector<ScheduleBasedLoad> ScheduledLoads;
		std::vector<ConstantEfficiencyConverter> ConstEffConvs;
		std::vector<Mux> Muxes;
		std::vector<Store> Stores;
		std::vector<PassThrough> PassThroughs;
		std::vector<Connection> Connections;
		std::vector<ScheduleBasedReliability> Reliabilities;
		DistributionSystem DistSys{};
		ReliabilityCoordinator Rel{};
		std::function<double()> RandFn;
		double FinalTime = 0.0;
	};

	struct ComponentIdAndWasteConnection
	{
		size_t Id;
		Connection WasteConnection;
	};

	struct SimulationState
	{
		std::set<size_t> ActiveConnectionsBack{};
		std::set<size_t> ActiveConnectionsFront{};
		// a set of component id that are unavailable
		std::set<size_t> UnavailableComponents{};
		std::vector<uint32_t> StorageAmounts_J{};
		std::vector<double> StorageNextEventTimes{};
		std::vector<Flow> Flows{};
		std::vector<size_t> ScheduleBasedLoadIdx{};
		std::vector<size_t> ScheduleBasedSourceIdx{};
	};

	struct TagAndPort
	{
		std::string Tag;
		size_t Port;
	};

	enum class FragilityResult
	{
		IsFailed,
		HasSurvived,
	};

	enum class FragilityCurveType
	{
		Linear,
		Tabular,
	};

	struct LinearFragilityCurve
	{
		// indexes IntensityDict
		size_t VulnerabilityId = 0;
		double LowerBound = 0.0;
		double UpperBound = 1.0;
	};

	struct TabularFragilityCurve
	{
		size_t VulnerabilityId = 0;
		std::vector<double> Intensities;
		std::vector<double> FailureFractions;
	};

	struct IntensityDict
	{
		std::vector<std::string> Tags{};
	};

	struct ScenarioIntensityDict
	{
		std::vector<size_t> ScenarioIds;
		std::vector<size_t> IntensityIds;
		std::vector<double> IntensityLevels;
	};

	struct FragilityCurveDict
	{
		std::vector<std::string> Tags{};
		std::vector<FragilityCurveType> CurveTypes{};
		std::vector<size_t> CurveId{};
	};

	// TODO: should we call these "tables" instead of dict?
	// more remeniscent of databases...
	struct ComponentFragilityModeDict
	{
		std::vector<size_t> ComponentIds;
		std::vector<size_t> FragilityModeIds;
	};

	struct FragilityModeDict
	{
		std::vector<std::string> Tags{};
		std::vector<size_t> FragilityCurveId{};
		std::vector<std::optional<size_t>> RepairDistIds{};
	};

	struct ComponentFailureModeDict
	{
		// index into ComponentDict
		std::vector<size_t> ComponentIds;
		// index into FailureModeDict
		std::vector<size_t> FailureModeIds;
	};

	struct FailureModeDict
	{
		std::vector<std::string> Tags;
		std::vector<size_t> FailureDistIds;
		std::vector<size_t> RepairDistIds;
	};

	std::optional<FragilityCurveType>
	TagToFragilityCurveType(std::string const& tag);

	std::string
	FragilityCurveTypeToTag(FragilityCurveType fctype);

	std::optional<size_t>
	GetIntensityIdByTag(IntensityDict intenseDict, std::string const& tag);

	size_t
	Component_AddComponentReturningId(
		ComponentDict& c,
		ComponentType ct,
		size_t idx);

	size_t
	Component_AddComponentReturningId(
		ComponentDict& c,
		ComponentType ct,
		size_t idx,
		std::vector<size_t> inflowType,
		std::vector<size_t> outflowType,
		std::string const& tag,
		double initialAge_s);

	void
	Helper_AddIfNotAdded(std::vector<size_t>& items, size_t item);

	void
	SimulationState_AddActiveConnectionBack(
		SimulationState& ss, size_t connIdx);

	void
	SimulationState_AddActiveConnectionForward(
		SimulationState& ss, size_t connIdx);

	size_t
	CountActiveConnections(SimulationState const& ss);

	void
	ActivateConnectionsForConstantLoads(Model const& m, SimulationState& ss);

	void
	ActivateConnectionsForConstantSources(Model const& m, SimulationState& ss);

	void
	ActivateConnectionsForScheduleBasedLoads(
		Model const& m, SimulationState& ss, double t);

	void
	ActivateConnectionsForScheduleBasedSources(
		Model const& m,
		SimulationState& ss,
		double t);

	void
	ActivateConnectionsForStores(Model& m, SimulationState& ss, double t);

	void
	ActivateConnectionsForReliability(
		Model& m, SimulationState& ss, double time);

	double
	GetNextTime(double nextTime, size_t count, std::function<double(size_t)> f);

	double
	EarliestNextEvent(Model const& m, SimulationState const& ss, double t);

	std::optional<size_t>
	FindOutflowConnection(
		Model const& m, ComponentType ct, size_t compId, size_t outflowPort);

	void
	UpdateConstantEfficiencyLossflowAndWasteflow(
		Model const& m,
		SimulationState& ss,
		size_t compIdx);

	void
	RunMuxPostFinalization(
		Model const& m, SimulationState& ss, size_t compIdx);

	void
	RunActiveConnections(Model& m, SimulationState& ss, double t);

	void
	RunConnectionsPostFinalization(Model& model, SimulationState& ss, double t);

	void
	RunPassthroughBackward(
		Model& m, SimulationState& ss, size_t connIdx, size_t compIdx);

	void
	RunConnectionsBackward(Model& model, SimulationState& ss);

	void
	RunPassthroughForward(
		Model& m,
		SimulationState& ss,
		size_t connIdx,
		size_t compIdx);

	void
	RunConnectionsForward(Model& model, SimulationState& ss);

	uint32_t
	FinalizeFlowValue(uint32_t requested, uint32_t available);

	void
	FinalizeFlows(SimulationState& ss);

	double
	NextEvent(
		ScheduleBasedLoad const& sb,
		size_t sbIdx,
		SimulationState const& ss);

	double
	NextEvent(
		ScheduleBasedSource const& sb,
		size_t sbIdx,
		SimulationState const& ss);

	double
	NextEvent(ScheduleBasedReliability const& sbr, double t);

	double
	NextStorageEvent(SimulationState const& ss, size_t storeIdx, double t);

	void
	UpdateStoresPerElapsedTime(
		Model const& m, SimulationState& ss, double elapsedTime);

	// TODO: change name to `std::string ComponentTypeToString(ComponentType);`
	std::string
	ToString(ComponentType ct);

	std::optional<ComponentType>
	TagToComponentType(std::string const& tag);

	void
	PrintFlows(Model const& m, SimulationState const&, double t);

	FlowSummary
	SummarizeFlows(Model const& m, SimulationState const& ss, double t);

	void
	PrintFlowSummary(FlowSummary s);

	void
	PrintModelState(Model& model, SimulationState& ss);

	std::vector<Flow>
	CopyFlows(std::vector<Flow> flows);

	std::vector<uint32_t>
	CopyStorageStates(SimulationState& ss);

	std::vector<TimeAndFlows>
	Simulate(Model& m, bool print);

	void
	Model_SetComponentToRepaired(
		Model const& m, SimulationState& ss, size_t compId);

	void
	Model_SetComponentToFailed(
		Model const& m, SimulationState& ss, size_t compId);

	size_t
	Model_AddConstantLoad(Model& m, uint32_t load);

	size_t
	Model_AddScheduleBasedLoad(
		Model& m, double* times, uint32_t* loads, size_t numItems);

	size_t
	Model_AddScheduleBasedLoad(
		Model& m,
		std::vector<TimeAndAmount> timesAndLoads);

	size_t
	Model_AddScheduleBasedLoad(
		Model& m,
		std::vector<TimeAndAmount> timesAndLoads,
		std::map<size_t, size_t> scenarioIdToLoadId);

	size_t
	Model_AddScheduleBasedLoad(
		Model& m,
		std::vector<TimeAndAmount> timesAndLoads,
		std::map<size_t, size_t> scenarioIdToLoadId,
		size_t inflowTypeId,
		std::string const& tag);

	size_t
	Model_AddConstantSource(Model& m, uint32_t available);

	size_t
	Model_AddConstantSource(
		Model& m,
		uint32_t available,
		size_t outflowTypeId,
		std::string const& tag);
	
	ComponentIdAndWasteConnection
	Model_AddScheduleBasedSource(
		Model& m,
		std::vector<TimeAndAmount> xs);

	size_t
	Model_AddMux(Model& m, size_t numInports, size_t numOutports);

	size_t
	Model_AddMux(
		Model& m,
		size_t numInports,
		size_t numOutports,
		size_t flowId,
		std::string const& tag);

	size_t
	Model_AddStore(
		Model& m,
		uint32_t capacity,
		uint32_t maxCharge,
		uint32_t maxDischarge,
		uint32_t nochargeAmount,
		uint32_t initialStorage);

	size_t
	Model_AddStore(
		Model& m,
		uint32_t capacity,
		uint32_t maxCharge,
		uint32_t maxDischarge,
		uint32_t chargeAmount,
		uint32_t initialStorage,
		size_t flowId,
		std::string const& tag);

	ComponentIdAndWasteConnection
	Model_AddConstantEfficiencyConverter(
		Model& m,
		uint32_t eff_numerator,
		uint32_t eff_denominator);

	ComponentIdAndWasteConnection
	Model_AddConstantEfficiencyConverter(
		Model& m,
		double efficiency);

	ComponentIdAndWasteConnection
	Model_AddConstantEfficiencyConverter(
		Model& m,
		double efficiency,
		size_t inflowId,
		size_t outflowId,
		size_t lossflowId,
		std::string const& tag);

	size_t
	Model_AddPassThrough(Model& m);

	size_t
	Model_AddPassThrough(Model& m, size_t flowId, std::string const& tag);

	Connection
	Model_AddConnection(
		Model& m,
		size_t from,
		size_t fromPort,
		size_t to,
		size_t toPort);

	Connection
	Model_AddConnection(
		Model& m,
		size_t fromId,
		size_t fromPort,
		size_t toId,
		size_t toPort,
		size_t flowId);

	bool
	SameConnection(Connection a, Connection b);

	std::optional<Flow>
	ModelResults_GetFlowForConnection(
		Model const& m,
		Connection conn,
		double time,
		std::vector<TimeAndFlows> timeAndFlows);

	std::optional<uint32_t>
	ModelResults_GetStoreState(
		Model const& m,
		size_t compId,
		double time,
		std::vector<TimeAndFlows> timeAndFlows);

	ScenarioOccurrenceStats
	ModelResults_CalculateScenarioOccurrenceStats(
		size_t scenarioId,
		size_t occurrenceNumber,
		Model const& m,
		std::vector<TimeAndFlows> const& timeAndFlows);

	void
	Debug_PrintNumberOfPasses(bool onlyGrandTotal = false);

	void
	Debug_ResetNumberOfPasses(bool resetAll = false);

	void
	Model_SetupSimulationState(Model& m, SimulationState& ss);

	void
	RunConstantEfficiencyConverterBackward(
		Model const& m, SimulationState& ss, size_t connIdx, size_t compIdx);

	void
	Mux_RequestInflowsIntelligently(
		SimulationState& ss,
		std::vector<size_t> const& inflowConns,
		uint32_t remainingRequest);

	void
	RunMuxBackward(Model& model, SimulationState& ss, size_t compIdx);

	void
	RunStoreBackward(
		Model& model, SimulationState& ss, size_t connIdx, size_t compIdx);

	void
	RunConstantEfficiencyConverterForward(
		Model const& model, SimulationState& ss, size_t connIdx, size_t compIdx);

	void
	RunMuxForward(Model& model, SimulationState& ss, size_t compIdx);

	void
	RunStoreForward(
		Model& model, SimulationState& ss, size_t connIdx, size_t compIdx);

	void
	RunStorePostFinalization(
		Model& model,
		SimulationState& ss,
		double t,
		size_t connIdx,
		size_t compIdx);

	void
	RunScheduleBasedSourceBackward(
		Model& model,
		SimulationState& ss,
		size_t connIdx,
		size_t compIdx);

	size_t
	Model_NumberOfComponents(Model const& m);

	size_t
	Model_AddFixedReliabilityDistribution(Model& m, double dt);

	size_t
	Model_AddFailureModeToComponent(
		Model& m, size_t compId, size_t failureDistId, size_t repairDistId);

	void
	UpdateScheduleBasedLoadNextEvent(
		Model const& m,
		SimulationState& ss,
		double time);

	void
	UpdateScheduleBasedSourceNextEvent(
		Model const& model,
		SimulationState& ss,
		double time);

	std::optional<TagAndPort>
	ParseTagAndPort(std::string const& s, std::string const& tableName);

	Result
	ParseNetwork(FlowDict const& ft, Model& model, toml::table const& table);

	std::optional<size_t>
	Model_FindCompIdByTag(Model const& m, std::string const& tag);

	std::optional<size_t>
	FlowDict_GetIdByTag(FlowDict const& ft, std::string const& tag);

	void
	Model_PrintConnections(Model const& m, FlowDict const& ft);

	std::ostream&
	operator<<(std::ostream& os, Flow const& flow);

	std::string
	ConnectionToString(
		ComponentDict const& cd,
		Connection const& c,
		bool compact=false);

	std::string
	ConnectionToString(
		ComponentDict const& cd,
		FlowDict const& fd,
		Connection const& c,
		bool compact=false);

	double
	Interpolate1d(double x, double x0, double y0, double x1, double y1);

	double
	LinearFragilityCurve_GetFailureFraction(
		LinearFragilityCurve lfc,
		double intensityLevel);

	double
	TabularFragilityCurve_GetFailureFraction(
		TabularFragilityCurve tfc,
		double intensityLevel);

	void
	ComponentDict_SetInitialAge(ComponentDict& cd, size_t id, double age_s);

}

#endif
