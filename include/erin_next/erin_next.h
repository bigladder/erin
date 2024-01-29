/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_H
#define ERIN_NEXT_H

#include "erin_next/distribution.h"
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <optional>

namespace erin_next {

	double const infinity = -1.0;

	size_t const constEffConvOutflowPort = 0;
	size_t const constEffConvLossflowPort = 1;
	size_t const constEffConvWasteflowPort = 2;

	enum class ComponentType
	{
		ConstantLoadType,
		ScheduleBasedLoadType,
		ConstantSourceType,
		ConstantEfficiencyConverterType,
		MuxType,
		StoreType,
		WasteSinkType,
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

	struct TimeAndLoad
	{
		double Time;
		uint32_t Load;
	};

	struct ScheduleBasedLoad
	{
		std::vector<TimeAndLoad> TimesAndLoads;
		size_t InflowConn;
	};

	struct ConstantSource
	{
		uint32_t Available;
		size_t OutflowConn;
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
		ComponentType To;
		size_t ToIdx;
		size_t ToPort;
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
		uint32_t MaxChargeRate; // energy per time unit
		uint32_t MaxDischargeRate; // energy per time unit
		uint32_t ChargeAmount; // storage level at or below which we request charge
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

	// TODO[mok]: consider separating model into const values (that don't change per simulation)
	// ... and state (which does change during the simulation). This might make it easier to log
	// ... state such as storage SOC.
	struct Model
	{
		std::vector<ConstantSource> ConstSources;
		std::vector<ConstantLoad> ConstLoads;
		std::vector<ScheduleBasedLoad> ScheduledLoads;
		std::vector<ConstantEfficiencyConverter> ConstEffConvs;
		std::vector<Mux> Muxes;
		std::vector<Store> Stores;
		std::vector<Connection> Connections;
		DistributionSystem DistSys;
	};

	struct ComponentId
	{
		size_t Id;
		ComponentType Type;
	};

	struct ComponentIdAndWasteConnection
	{
		ComponentId Id;
		Connection WasteConnection;
	};

	struct SimulationState
	{
		std::vector<size_t> ActiveConnectionsBack;
		std::vector<size_t> ActiveConnectionsFront;
		std::vector<ComponentId> ActiveComponentsBack;
		std::vector<ComponentId> ActiveComponentFront;
		std::vector<uint32_t> StorageAmounts;
		std::vector<double> StorageNextEventTimes;
		std::vector<Flow> Flows;
	};

	void Helper_AddIfNotAdded(std::vector<size_t>& items, size_t item);
	void SimulationState_AddActiveConnectionBack(SimulationState& ss, size_t connIdx);
	void SimulationState_AddActiveConnectionForward(SimulationState& ss, size_t connIdx);
	size_t CountActiveConnections(SimulationState const& ss);
	void ActivateConnectionsForConstantLoads(Model const& m, SimulationState& ss);
	void ActivateConnectionsForConstantSources(Model const& m, SimulationState& ss);
	void ActivateConnectionsForScheduleBasedLoads(Model const& m, SimulationState& ss, double t);
	void ActivateConnectionsForStores(Model& m, SimulationState& ss, double t);
	double EarliestNextEvent(Model const& m, SimulationState const& ss, double t);
	int FindInflowConnection(Model const& m, ComponentType ct, size_t compId, size_t inflowPort);
	int FindOutflowConnection(Model const& m, ComponentType ct, size_t compId, size_t outflowPort);
	void RunMuxPostFinalization(Model const& m, SimulationState& ss, size_t compIdx);
	void RunActiveConnections(Model& m, SimulationState& ss, double t);
	void RunConnectionsPostFinalization(Model& model, SimulationState& ss, double t);
	void RunConnectionsBackward(Model& model, SimulationState& ss);
	void RunConnectionsForward(Model& model, SimulationState& ss);
	uint32_t FinalizeFlowValue(uint32_t requested, uint32_t available);
	void FinalizeFlows(SimulationState& ss);
	double NextEvent(ScheduleBasedLoad const& sb, double t);
	double NextStorageEvent(SimulationState const& ss, size_t storeIdx, double t);
	void UpdateStoresPerElapsedTime(Model const& m, SimulationState& ss, double elapsedTime);
	std::string ToString(ComponentType ct);
	void PrintFlows(Model const& m, SimulationState const&, double t);
	FlowSummary SummarizeFlows(Model const& m, SimulationState const& ss, double t);
	void PrintFlowSummary(FlowSummary s);
	void PrintModelState(Model& model, SimulationState& ss);
	std::vector<Flow> CopyFlows(std::vector<Flow> flows);
	std::vector<uint32_t> CopyStorageStates(SimulationState& ss);
	std::vector<TimeAndFlows> Simulate(Model& m, SimulationState& ss, bool print);
	ComponentId Model_AddConstantLoad(Model& m, uint32_t load);
	ComponentId Model_AddScheduleBasedLoad(Model& m, double* times, uint32_t* loads, size_t numItems);
	ComponentId Model_AddScheduleBasedLoad(Model& m, std::vector<TimeAndLoad> timesAndLoads);
	ComponentId Model_AddConstantSource(Model& m, uint32_t available);
	ComponentId Model_AddMux(Model& m, size_t numInports, size_t numOutports);
	ComponentId Model_AddStore(Model& m, uint32_t capacity, uint32_t maxCharge, uint32_t maxDischarge,uint32_t nochargeAmount, uint32_t initialStorage);
	ComponentIdAndWasteConnection Model_AddConstantEfficiencyConverter(Model& m, SimulationState& ss, uint32_t eff_numerator, uint32_t eff_denominator);
	Connection Model_AddConnection(Model& m, SimulationState& ss, ComponentId& from, size_t fromPort, ComponentId& to, size_t toPort);
	bool SameConnection(Connection a, Connection b);
	std::optional<Flow> ModelResults_GetFlowForConnection(Model& m, Connection conn, double time, std::vector<TimeAndFlows> timeAndFlows);
	std::optional<uint32_t> ModelResults_GetStoreState(size_t storeId, double time, std::vector<TimeAndFlows> timeAndFlows);
	void Debug_PrintNumberOfPasses(bool onlyGrandTotal = false);
	void Debug_ResetNumberOfPasses(bool resetAll = false);
	void Model_SetupSimulationState(Model& m, SimulationState& ss);
	void RunConstantEfficiencyConverterBackward(Model& m, SimulationState& ss, size_t connIdx, size_t compIdx);
	void RunMuxBackward(Model& model, SimulationState& ss, size_t compIdx);
	void RunStoreBackward(Model& model, SimulationState& ss, size_t connIdx, size_t compIdx);
	void RunConstantEfficiencyConverterForward(Model& model, SimulationState& ss, size_t connIdx, size_t compIdx);
	void RunMuxForward(Model& model, SimulationState& ss, size_t compIdx);
	void RunStoreForward(Model& model, SimulationState& ss, size_t connIdx, size_t compIdx);
	void RunStorePostFinalization(Model& model, SimulationState& ss, double t, size_t connIdx, size_t compIdx);
	size_t Model_NumberOfComponents(Model const& m);
}

#endif // ERIN_NEXT_H