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

struct FlowSummary {
	double Time;
	uint32_t Inflow;
	uint32_t OutflowRequest;
	uint32_t OutflowAchieved;
	uint32_t Wasteflow;
};

enum class ComponentType
{
	ConstantLoadType,
	ScheduleBasedLoadType,
	ConstantSourceType,
	ConstantEfficiencyConverterType,
	WasteSinkType,
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

struct Flow {
	uint32_t Requested;
	uint32_t Available;
	uint32_t Actual;
};

struct TimeAndFlows {
	double Time;
	std::vector<Flow> Flows;
};

struct Model {
	std::vector<ConstantSource> ConstSources;
	std::vector<ConstantLoad> ConstLoads;
	std::vector<ScheduleBasedLoad> ScheduledLoads;
	std::vector < ConstantEfficiencyConverter> ConstEffConvs;
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

static size_t CountActiveConnections(Model& m);
static void ActivateConnectionsForConstantLoads(Model& m);
static void ActivateConnectionsForConstantSources(Model& m);
static void ActivateConnectionsForScheduleBasedLoads(Model& m, double t);
static double EarliestNextEvent(Model& m, double t);
static int FindInflowConnection(Model& m, ComponentType ct, size_t compId, size_t inflowPort);
static int FindOutflowConnection(Model& m, ComponentType ct, size_t compId, size_t outflowPort);
static void RunActiveConnections(Model& m);
static uint32_t FinalizeFlowValue(uint32_t requested, uint32_t available);
static void FinalizeFlows(Model& m);
static double NextEvent(ScheduleBasedLoad sb, double t);
static std::string ToString(ComponentType ct);
static void PrintFlows(Model& m, double t);
static FlowSummary SummarizeFlows(Model& m, double t);
static void PrintFlowSummary(FlowSummary s);
static std::vector<Flow> CopyFlows(std::vector<Flow> flows);
static std::vector<TimeAndFlows> Simulate(Model& m, bool print);
static ComponentId Model_AddConstantLoad(Model& m, uint32_t load);
static ComponentId Model_AddScheduleBasedLoad(Model& m, double* times, uint32_t* loads, size_t numItems);
static ComponentId Model_AddScheduleBasedLoad(Model& m, std::vector<TimeAndLoad> timesAndLoads);
static ComponentId Model_AddConstantSource(Model& m, uint32_t available);
static ComponentIdAndWasteConnection Model_AddConstantEfficiencyConverter(Model& m, uint32_t eff_numerator, uint32_t eff_denominator);
static Connection Model_AddConnection(Model& m, ComponentId& from, size_t fromPort, ComponentId& to, size_t toPort);
static bool SameConnection(Connection a, Connection b);
static std::optional<Flow> ModelResults_GetFlowForConnection(Model& m, Connection conn, double time, std::vector<TimeAndFlows> timeAndFlows);
static void Example1(bool doPrint);
static void Example2(bool doPrint);
static void Example3(bool doPrint);
static void Example3A(bool doPrint);
static void Example4(bool doPrint);
static void Example5(bool doPrint);

#endif // ERIN_NEXT_H