/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_H
#define ERIN_NEXT_H

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

	struct FlowSummary {
		double Time;
		uint32_t Inflow;
		uint32_t OutflowRequest;
		uint32_t OutflowAchieved;
		uint32_t StorageDischarge;
		uint32_t StorageCharge;
		uint32_t Wasteflow;
	};

	struct ConstantLoad {
		uint32_t Load;
	};

	struct TimeAndLoad {
		double Time;
		uint32_t Load;
	};

	struct ScheduleBasedLoad {
		std::vector<TimeAndLoad> TimesAndLoads;
	};

	struct ConstantSource {
		uint32_t Available;
	};

	struct ConstantEfficiencyConverter {
		uint32_t EfficiencyNumerator;
		uint32_t EfficiencyDenominator;
	};

	struct Connection {
		ComponentType From;
		size_t FromIdx;
		size_t FromPort;
		ComponentType To;
		size_t ToIdx;
		size_t ToPort;
		bool IsActiveForward;
		bool IsActiveBack;
	};

	struct Mux {
		size_t NumInports;
		size_t NumOutports;
	};

	struct Store {
		uint32_t Capacity;
		uint32_t MaxChargeRate; // energy per time unit
		uint32_t MaxDischargeRate; // energy per time unit
		uint32_t Stored;
		double TimeOfNextEvent;
	};

	struct Flow {
		uint32_t Requested;
		uint32_t Available;
		uint32_t Actual;
	};

	struct TimeAndFlows {
		double Time;
		std::vector<Flow> Flows;
	};

	// TODO[mok]: consider separating model into const values (that don't change per simulation)
	// ... and state (which does change during the simulation). This might make it easier to log
	// ... state such as storage SOC.
	struct Model {
		std::vector<ConstantSource> ConstSources;
		std::vector<ConstantLoad> ConstLoads;
		std::vector<ScheduleBasedLoad> ScheduledLoads;
		std::vector<ConstantEfficiencyConverter> ConstEffConvs;
		std::vector<Mux> Muxes;
		std::vector<Store> Stores;
		std::vector<Connection> Connections;
		std::vector<Flow> Flows;
	};

	struct ComponentId {
		size_t Id;
		ComponentType Type;
	};

	struct ComponentIdAndWasteConnection {
		ComponentId Id;
		Connection WasteConnection;
	};

	size_t CountActiveConnections(Model& m);
	void ActivateConnectionsForConstantLoads(Model& m);
	void ActivateConnectionsForConstantSources(Model& m);
	void ActivateConnectionsForScheduleBasedLoads(Model& m, double t);
	void ActivateConnectionsForStores(Model& m, double t);
	double EarliestNextEvent(Model& m, double t);
	int FindInflowConnection(
		Model& m, ComponentType ct, size_t compId, size_t inflowPort);
	int FindOutflowConnection(
		Model& m, ComponentType ct, size_t compId, size_t outflowPort);
	void RunActiveConnections(Model& m, double t);
	uint32_t FinalizeFlowValue(uint32_t requested, uint32_t available);
	void FinalizeFlows(Model& m);
	double NextEvent(ScheduleBasedLoad sb, double t);
	double NextEvent(Store s, double t);
	std::string ToString(ComponentType ct);
	void PrintFlows(Model& m, double t);
	FlowSummary SummarizeFlows(Model& m, double t);
	void PrintFlowSummary(FlowSummary s);
	void PrintModelState(Model& model);
	std::vector<Flow> CopyFlows(std::vector<Flow> flows);
	std::vector<TimeAndFlows> Simulate(Model& m, bool print);
	ComponentId Model_AddConstantLoad(Model& m, uint32_t load);
	ComponentId Model_AddScheduleBasedLoad(
		Model& m, double* times, uint32_t* loads, size_t numItems);
	ComponentId Model_AddScheduleBasedLoad(
		Model& m, std::vector<TimeAndLoad> timesAndLoads);
	ComponentId Model_AddConstantSource(Model& m, uint32_t available);
	ComponentId Model_AddMux(Model& m, size_t numInports, size_t numOutports);
	ComponentId Model_AddStore(
		Model& m, uint32_t capacity, uint32_t maxCharge, uint32_t maxDischarge,
		uint32_t initialStorage);
	ComponentIdAndWasteConnection Model_AddConstantEfficiencyConverter(
		Model& m, uint32_t eff_numerator, uint32_t eff_denominator);
	Connection Model_AddConnection(
		Model& m, ComponentId& from, size_t fromPort,
		ComponentId& to, size_t toPort);
	bool SameConnection(Connection a, Connection b);
	std::optional<Flow> ModelResults_GetFlowForConnection(
		Model& m, Connection conn, double time,
		std::vector<TimeAndFlows> timeAndFlows);

}

#endif // ERIN_NEXT_H