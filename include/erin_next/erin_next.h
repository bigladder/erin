/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_H
#define ERIN_NEXT_H

#include "erin_next/erin_next_timestate.h"
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_reliability.h"
#include "erin_next/erin_next_time_and_load.h"
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
		WasteSinkType,
	};

	// Holds the various flow types encountered
	// Note: each entry added must be unique and the index into this
	// vector is the flow type used through the rest of the simulation
	struct FlowType {
		std::vector<std::string> Type;
	};

	// Information about a scenario
	struct ScenarioType {
		std::vector<std::string> Tags;
	};

	struct LoadDict {
		std::vector<std::string> Tags;
		std::vector<TimeUnit> TimeUnits;
		std::vector<std::string> RateUnits;
		std::vector<std::vector<TimeAndLoad>> Loads;
	};

	// NOTE: the struct below is indexed by a size_t which is the id for a
	// component
	struct ComponentDict {
		// to lookup a component by tag
		// std::vector<std::string> tag{};
		// the index into the component vector for the given component type
		std::vector<size_t> Idx;
		std::vector<ComponentType> CompType;
		std::vector<std::string> Tag;
		// The below gives each component's inflow type and outflow type
		// there are indices into FlowType
		std::vector<size_t> InflowType;
		std::vector<size_t> OutflowType;
	};

	struct FlowSummary
	{
		double Time;
		uint32_t Inflow;
		uint32_t OutflowRequest;
		uint32_t OutflowAchieved;
		uint32_t StorageDischarge;
		uint32_t StorageCharge;
		uint32_t Wasteflow;
	};

	struct ConstantLoad
	{
		uint32_t Load;
		size_t InflowConn;
	};

	struct TimeAndAvailability
	{
		double Time;
		uint32_t Available;

		TimeAndAvailability(double t, uint32_t a): Time{t}, Available{a} {}
	};

	struct ScheduleBasedLoad
	{
		std::vector<TimeAndLoad> TimesAndLoads;
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
		uint32_t Available;
		size_t OutflowConn;
	};

	struct ScheduleBasedSource
	{
		std::vector<TimeAndAvailability> TimeAndAvails;
		size_t OutflowConn;
		size_t WasteflowConn;
	};

	struct ConstantEfficiencyConverter
	{
		uint32_t EfficiencyNumerator;
		uint32_t EfficiencyDenominator;
		size_t InflowConn;
		size_t OutflowConn;
		std::optional<size_t> LossflowConn;
		size_t WasteflowConn;
	};

	struct Connection
	{
		ComponentType From;
		size_t FromIdx;
		size_t FromPort;
		size_t FromId; // index into ComponentDict
		ComponentType To;
		size_t ToIdx;
		size_t ToPort;
		size_t ToId; // index into ComponentDict
		size_t FlowTypeId;
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
		uint32_t Capacity;
		// energy per time unit
		uint32_t MaxChargeRate;
		// energy per time unit
		uint32_t MaxDischargeRate;
		// amount at or below which we request charge
		uint32_t ChargeAmount;
		uint32_t InitialStorage;
		size_t InflowConn;
		size_t OutflowConn;
	};

	struct Flow
	{
		uint32_t Requested;
		uint32_t Available;
		uint32_t Actual;
	};

	struct TimeAndFlows
	{
		double Time;
		std::vector<Flow> Flows;
		std::vector<uint32_t> StorageAmounts;
	};

	struct Model
	{
		// TODO: rename type from Component to ComponentType for consistency;
		// alternate names ComponentDict? FlowDict? Implies lookup...
		// NOTE: should we have another struct called Simulation which
		// holds the ComponentDict, FlowDict, ScenarioDict, LoadDict, etc.
		// and then creates and runs models as the scenario is run???
		ComponentDict ComponentMap;
		std::vector<ConstantSource> ConstSources;
		std::vector<ScheduleBasedSource> ScheduledSrcs;
		std::vector<ConstantLoad> ConstLoads;
		std::vector<ScheduleBasedLoad> ScheduledLoads;
		std::vector<ConstantEfficiencyConverter> ConstEffConvs;
		std::vector<Mux> Muxes;
		std::vector<Store> Stores;
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
		std::set<size_t> ActiveConnectionsBack;
		std::set<size_t> ActiveConnectionsFront;
		std::vector<uint32_t> StorageAmounts;
		std::vector<double> StorageNextEventTimes;
		std::vector<Flow> Flows;
		std::vector<size_t> ScheduleBasedLoadIdx;
		std::vector<size_t> ScheduleBasedSourceIdx;
	};

	struct TagAndPort
	{
		std::string Tag;
		size_t Port;
	};

	size_t
	Component_AddComponentReturningId(
		ComponentDict& c,
		ComponentType ct,
		size_t idx,
		size_t inflowType = 0,
		size_t outflowType = 0,
		std::string const& tag = "");

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
	EarliestNextEvent(Model const& m, SimulationState const& ss, double t);

	int
	FindInflowConnection(
		Model const& m, ComponentType ct, size_t compId, size_t inflowPort);

	int
	FindOutflowConnection(
		Model const& m, ComponentType ct, size_t compId, size_t outflowPort);

	void
	RunMuxPostFinalization(
		Model const& m, SimulationState& ss, size_t compIdx);

	void
	RunActiveConnections(Model& m, SimulationState& ss, double t);

	void
	RunConnectionsPostFinalization(Model& model, SimulationState& ss, double t);

	void
	RunConnectionsBackward(Model& model, SimulationState& ss);

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
	Simulate(Model& m, SimulationState& ss, bool print);

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
		std::vector<TimeAndLoad> timesAndLoads);

	size_t
	Model_AddScheduleBasedLoad(
		Model& m,
		std::vector<TimeAndLoad> timesAndLoads,
		std::map<size_t, size_t> scenarioIdToLoadId);

	size_t
	Model_AddScheduleBasedLoad(
		Model& m,
		std::vector<TimeAndLoad> timesAndLoads,
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
		SimulationState& ss,
		std::vector<TimeAndAvailability> xs);

	size_t
	Model_AddMux(Model& m, size_t numInports, size_t numOutports);

	size_t
	Model_AddStore(
		Model& m,
		uint32_t capacity,
		uint32_t maxCharge,
		uint32_t maxDischarge,
		uint32_t nochargeAmount,
		uint32_t initialStorage);

	ComponentIdAndWasteConnection
	Model_AddConstantEfficiencyConverter(
		Model& m,
		SimulationState& ss,
		uint32_t eff_numerator,
		uint32_t eff_denominator);

	Connection
	Model_AddConnection(
		Model& m,
		SimulationState& ss,
		size_t from,
		size_t fromPort,
		size_t to,
		size_t toPort);

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
	ParseNetwork(FlowType const& ft, Model& model, toml::table const& table);

	void
	ParseNetworks(FlowType const& ft, Model& model, toml::table const& table);

	std::optional<size_t>
	Model_FindCompIdByTag(Model const& m, std::string const& tag);

	std::optional<size_t>
	FlowType_GetIdByTag(FlowType const& ft, std::string const& tag);

	void
	Model_PrintConnections(Model const& m, FlowType const& ft);
}

#endif
