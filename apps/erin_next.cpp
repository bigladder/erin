/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <cassert>
#include <stdexcept>

// FlowSummary
struct FlowSummary {
	uint32_t Inflow;
	uint32_t Outflow;
	uint32_t Wasteflow;
};

// ComponentType
enum class ComponentType
{
	ConstantLoadType,
	ConstantSourceType,
	ConstantEfficiencyConverterType,
	WasteSinkType,
};

// ConstantLoad
struct ConstantLoad {
	uint32_t Load;
};

// ConstantSource
struct ConstantSource {
	uint32_t Available;
};

// ConstantEfficiencyConverter
struct ConstantEfficiencyConverter {
	uint32_t EfficiencyNumerator;
	uint32_t EfficiencyDenominator;
};

// Connection
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

// Flow
struct Flow {
	uint32_t Requested;
	uint32_t Available;
	uint32_t Actual;
};

// Model
struct Model {
	size_t NumConstSources;
	size_t NumConstLoads;
	size_t NumConnectionsAndFlows;
	size_t NumConstantEfficiencyConverters;
	size_t NumWasteSinks;
	ConstantSource* ConstSources;
	ConstantLoad* ConstLoads;
	ConstantEfficiencyConverter* ConstEffConvs;
	Connection* Connections;
	Flow* Flows;
};

// HEADER
static size_t CountActiveConnections(Model& m);
static void ActivateConnectionsForConstantLoads(Model& m);
static void ActivateConnectionsForConstantSources(Model& m);
static int FindInflowConnection(Model& m, ComponentType ct, size_t compId, size_t inflowPort);
static int FindOutflowConnection(Model& m, ComponentType ct, size_t compId, size_t outflowPort);
static void RunActiveConnections(Model& m);
static void FinalizeFlows(Model& m);
static std::string ToString(ComponentType ct);
static void PrintFlows(Model& m, double t);
static FlowSummary SummarizeFlows(Model& m);
static void PrintFlowSummary(FlowSummary s);
static void Simulate(Model& m);
static void ExampleOne(void);
static void ExampleTwo(void);
static void ExampleThree(void);
static void ExampleThreeA(void);

// IMPLEMENTATION
static size_t
CountActiveConnections(Model& m) {
	size_t count = 0;
	for (size_t connIdx = 0; connIdx < m.NumConnectionsAndFlows; ++connIdx) {
		if (m.Connections[connIdx].IsActiveBack || m.Connections[connIdx].IsActiveForward)
		{
			++count;
		}
	}
	return count;
}

static void
ActivateConnectionsForConstantLoads(Model& model) {
	for (size_t loadIdx = 0; loadIdx < model.NumConstLoads; ++loadIdx) {
		for (size_t connIdx = 0; connIdx < model.NumConnectionsAndFlows; ++connIdx) {
			if (model.Connections[connIdx].To == ComponentType::ConstantLoadType
				&& model.Connections[connIdx].ToIdx == loadIdx)
			{
				model.Connections[connIdx].IsActiveBack =
					model.Flows[connIdx].Requested != model.ConstLoads[loadIdx].Load;
				model.Flows[connIdx].Requested = model.ConstLoads[loadIdx].Load;
			}
		}
	}
}

static void
ActivateConnectionsForConstantSources(Model& model) {
	for (size_t srcIdx = 0; srcIdx < model.NumConstSources; ++srcIdx) {
		for (size_t connIdx = 0; connIdx < model.NumConnectionsAndFlows; ++connIdx) {
			if (model.Connections[connIdx].From == ComponentType::ConstantSourceType
				&& model.Connections[connIdx].FromIdx == srcIdx) {
				model.Connections[connIdx].IsActiveForward =
					model.Flows[connIdx].Available != model.ConstSources[srcIdx].Available;
				model.Flows[connIdx].Available = model.ConstSources[srcIdx].Available;
			}
		}
	}
}

static int
FindInflowConnection(Model& m, ComponentType ct, size_t compId, size_t inflowPort) {
	for (size_t connIdx = 0; connIdx < m.NumConnectionsAndFlows; ++connIdx) {
		if (m.Connections[connIdx].To == ct && m.Connections[connIdx].ToIdx == compId && m.Connections[connIdx].ToPort == inflowPort) {
			return (int)connIdx;
		}
	}
	return -1;
}

static int
FindOutflowConnection(Model& m, ComponentType ct, size_t compId, size_t outflowPort) {
	for (size_t connIdx = 0; connIdx < m.NumConnectionsAndFlows; ++connIdx) {
		if (m.Connections[connIdx].From == ct && m.Connections[connIdx].FromIdx == compId && m.Connections[connIdx].FromPort == outflowPort) {
			return (int)connIdx;
		}
	}
	return -1;
}

static void
RunActiveConnections(Model& model) {
	for (size_t connIdx = 0; connIdx < model.NumConnectionsAndFlows; ++connIdx) {
		if (model.Connections[connIdx].IsActiveBack) {
			switch (model.Connections[connIdx].From) {
			case (ComponentType::ConstantSourceType):
			{
				// nothing to update
			} break;
			case (ComponentType::ConstantEfficiencyConverterType):
			{
				switch (model.Connections[connIdx].FromPort) {
				case 0:
				{
					int inflowConn = FindInflowConnection(model, model.Connections[connIdx].From, model.Connections[connIdx].FromIdx, 0);
					assert((inflowConn >= 0) && "should find an inflow connection; model is incorrectly connected");
					uint32_t outflowRequest = model.Flows[connIdx].Requested;
					uint32_t inflowRequest =
						(model.ConstEffConvs[model.Connections[connIdx].FromIdx].EfficiencyDenominator * outflowRequest)
						/ model.ConstEffConvs[model.Connections[connIdx].FromIdx].EfficiencyNumerator;
					assert((inflowRequest >= outflowRequest) && "inflow must be >= outflow in converter");
					model.Connections[(size_t)inflowConn].IsActiveBack = inflowRequest != model.Flows[(size_t)inflowConn].Requested;
					model.Flows[(size_t)inflowConn].Requested = inflowRequest;
					uint32_t lossflowRequest = 0;
					int lossflowConn = FindOutflowConnection(model, model.Connections[connIdx].From, model.Connections[connIdx].FromIdx, 1);
					if (lossflowConn >= 0) {
						lossflowRequest = (inflowRequest - outflowRequest) >= model.Flows[(size_t)lossflowConn].Requested
							? model.Flows[(size_t)lossflowConn].Requested
							: (inflowRequest - outflowRequest);
					}
					int wasteflowConn = FindOutflowConnection(model, model.Connections[connIdx].From, model.Connections[connIdx].FromIdx, 2);
					assert((wasteflowConn >= 0) && "should find a wasteflow connection; model is incorrectly connected");
					uint32_t wasteflowRequest = inflowRequest >= (outflowRequest + lossflowRequest)
						? inflowRequest - (outflowRequest + lossflowRequest)
						: 0;
					model.Flows[(size_t)wasteflowConn].Requested = wasteflowRequest;
				} break;
				case 1: // lossflow
				case 2: // wasteflow
				{
					// do nothing
				} break;
				default:
				{
					throw std::runtime_error{ "Uhandled port number" };
				}
				}
			} break;
			default:
			{
				std::cout << "Unhandled component type on backward pass: "
					<< ToString(model.Connections[connIdx].From) << std::endl;
			}
			}
			model.Connections[connIdx].IsActiveBack = false;
		}
	}
	for (size_t connIdx = 0; connIdx < model.NumConnectionsAndFlows; ++connIdx) {
		if (model.Connections[connIdx].IsActiveForward) {
			switch (model.Connections[connIdx].To) {
			case (ComponentType::ConstantLoadType):
			case (ComponentType::WasteSinkType):
			{
				// Nothing to do
			} break;
			case (ComponentType::ConstantEfficiencyConverterType):
			{
				uint32_t inflowAvailable = model.Flows[connIdx].Available;
				uint32_t inflowRequest = model.Flows[connIdx].Requested;
				int outflowConn = FindOutflowConnection(model, model.Connections[connIdx].To, model.Connections[connIdx].ToIdx, 0);
				assert((outflowConn >= 0) && "should find an outflow connection; model is incorrectly connected");
				uint32_t outflowAvailable =
					(model.ConstEffConvs[model.Connections[connIdx].ToIdx].EfficiencyNumerator * inflowAvailable)
					/ model.ConstEffConvs[model.Connections[connIdx].ToIdx].EfficiencyDenominator;
				uint32_t outflowRequest = model.Flows[(size_t)outflowConn].Requested;
				assert((inflowAvailable >= outflowAvailable) && "converter forward flow; inflow must be >= outflow");
				model.Connections[(size_t)outflowConn].IsActiveForward = outflowAvailable != model.Flows[(size_t)outflowConn].Available;
				model.Flows[(size_t)outflowConn].Available = outflowAvailable;
				int lossflowConn = FindOutflowConnection(model, model.Connections[connIdx].To, model.Connections[connIdx].ToIdx, 1);
				uint32_t lossflowAvailable = 0;
				uint32_t lossflowRequest = 0;
				if (lossflowConn >= 0) {
					lossflowRequest = model.Flows[(size_t)lossflowConn].Requested;
					lossflowAvailable = inflowAvailable - outflowAvailable;
					model.Connections[(size_t)lossflowConn].IsActiveForward = lossflowAvailable != model.Flows[(size_t)lossflowConn].Available;
					model.Flows[(size_t)lossflowConn].Available = lossflowAvailable;
				}
				int wasteflowConn = FindOutflowConnection(model, model.Connections[connIdx].To, model.Connections[connIdx].ToIdx, 2);
				assert((wasteflowConn >= 0) && "should find a wasteflow connection; model is incorrectly connected");
				model.Flows[(size_t)wasteflowConn].Requested = inflowRequest >= (lossflowRequest + outflowRequest)
					? inflowRequest - (lossflowRequest + outflowRequest)
					: 0;
				model.Flows[(size_t)wasteflowConn].Available = inflowAvailable >= (lossflowRequest + outflowAvailable)
					? inflowAvailable - (lossflowRequest + outflowAvailable)
					: 0;
			} break;
			default:
			{
				throw std::runtime_error{ "Unhandled component type on forward pass: " + ToString(model.Connections[connIdx].To) };
			}
			}
			model.Connections[connIdx].IsActiveForward = false;
		}
	}
}

static void
FinalizeFlows(Model& model) {
	for (size_t flowIdx = 0; flowIdx < model.NumConnectionsAndFlows; ++flowIdx) {
		model.Flows[flowIdx].Actual =
			model.Flows[flowIdx].Available >= model.Flows[flowIdx].Requested
			? model.Flows[flowIdx].Requested
			: model.Flows[flowIdx].Available;
	}
}

static std::string
ToString(ComponentType compType) {
	switch (compType) {
	case (ComponentType::ConstantLoadType):
	{
		return std::string{ "ConstantLoad" };
	} break;
	case (ComponentType::ConstantSourceType):
	{
		return std::string{ "ConstantSource" };
	} break;
	case (ComponentType::ConstantEfficiencyConverterType):
	{
		return std::string{ "ConstantEfficiencyConverter" };
	} break;
	case (ComponentType::WasteSinkType):
	{
		return std::string{ "WasteSink" };
	} break;
	default:
	{
		throw std::runtime_error{ "Unhandled component type" };
	}
	}
	return std::string{ "?" };
}

static void
PrintFlows(Model& m, double t) {
	std::cout << "time: " << t << std::endl;
	for (size_t flowIdx = 0; flowIdx < m.NumConnectionsAndFlows; ++flowIdx) {
		std::cout << ToString(m.Connections[flowIdx].From)
			<< "[" << m.Connections[flowIdx].FromIdx << ":" << m.Connections[flowIdx].FromPort << "] => "
			<< ToString(m.Connections[flowIdx].To)
			<< "[" << m.Connections[flowIdx].ToIdx << ":" << m.Connections[flowIdx].ToPort << "]: "
			<< m.Flows[flowIdx].Actual
			<< " (R: " << m.Flows[flowIdx].Requested
			<< "; A: " << m.Flows[flowIdx].Available << ")"
			<< std::endl;
	}
}

static FlowSummary
SummarizeFlows(Model& m) {
	FlowSummary summary = {};
	for (size_t flowIdx = 0; flowIdx < m.NumConnectionsAndFlows; ++flowIdx) {
		switch (m.Connections[flowIdx].From) {
		case (ComponentType::ConstantSourceType):
		{
			summary.Inflow += m.Flows[flowIdx].Actual;
		} break;
		}

		switch (m.Connections[flowIdx].To) {
		case (ComponentType::ConstantLoadType):
		{
			summary.Outflow += m.Flows[flowIdx].Actual;
		} break;
		case (ComponentType::WasteSinkType):
		{
			summary.Wasteflow += m.Flows[flowIdx].Actual;
		} break;
		}
	}
	return summary;
}

static void
PrintFlowSummary(FlowSummary s) {
	uint32_t sum = s.Inflow - (s.Outflow + s.Wasteflow);
	double eff = 100.0 * ((double)s.Outflow) / ((double)s.Inflow);
	std::cout << "Flow Summary:" << std::endl;
	std::cout << "- Inflow   : " << s.Inflow << std::endl;
	std::cout << "- Outflow  : " << s.Outflow << std::endl;
	std::cout << "- Wasteflow: " << s.Wasteflow << std::endl;
	std::cout << "------------------------" << std::endl;
	std::cout << "  Sum      : " << sum << std::endl;
	std::cout << "  Eff      : " << eff << "%" << std::endl;
}

static void
Simulate(Model& model) {
	double t = 0.0;
	ActivateConnectionsForConstantLoads(model);
	ActivateConnectionsForConstantSources(model);
	while (CountActiveConnections(model) > 0) {
		RunActiveConnections(model);
	}
	FinalizeFlows(model);
	PrintFlows(model, t);
	PrintFlowSummary(SummarizeFlows(model));
}

static void
ExampleOne() {
	ConstantSource sources[] = { { 100 } };
	ConstantLoad loads[] = { { 10 } };
	Connection conns[] = {
		{ ComponentType::ConstantSourceType, 0, 0, ComponentType::ConstantLoadType, 0, 0 },
	};
	Flow flows[] = { { 0, 0, 0 } };
	Model m = {};
	m.NumConstSources = 1;
	m.NumConstLoads = 1;
	m.NumConnectionsAndFlows = 1;
	m.NumWasteSinks = 0;
	m.Connections = conns;
	m.ConstLoads = loads;
	m.ConstSources = sources;
	m.Flows = flows;
	Simulate(m);
}

static void
ExampleTwo() {
	ConstantSource sources[] = { { 100 } };
	ConstantLoad loads[] = { { 10 } };
	ConstantEfficiencyConverter convs[] = { { 1, 2 } };
	Connection conns[] = {
		{ ComponentType::ConstantSourceType, 0, 0, ComponentType::ConstantEfficiencyConverterType, 0, 0 },
		{ ComponentType::ConstantEfficiencyConverterType, 0, 0, ComponentType::ConstantLoadType, 0, 0 },
		{ ComponentType::ConstantEfficiencyConverterType, 0, 2, ComponentType::WasteSinkType, 0, 0 },
	};
	Flow flows[] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
	Model m = {};
	m.NumConstSources = 1;
	m.NumConstLoads = 1;
	m.NumConnectionsAndFlows = 3;
	m.NumConstantEfficiencyConverters = 1;
	m.NumWasteSinks = 1;
	m.Connections = conns;
	m.ConstLoads = loads;
	m.ConstEffConvs = convs;
	m.ConstSources = sources;
	m.Flows = flows;
	Simulate(m);
}

static void
ExampleThree() {
	ConstantSource sources[] = { { 100 } };
	ConstantLoad loads[] = { { 10 }, { 2 } };
	ConstantEfficiencyConverter convs[] = { { 1, 2 } };
	Connection conns[] = {
		{ ComponentType::ConstantSourceType, 0, 0, ComponentType::ConstantEfficiencyConverterType, 0, 0 },
		{ ComponentType::ConstantEfficiencyConverterType, 0, 0, ComponentType::ConstantLoadType, 0, 0 },
		{ ComponentType::ConstantEfficiencyConverterType, 0, 1, ComponentType::ConstantLoadType, 1, 0 },
		{ ComponentType::ConstantEfficiencyConverterType, 0, 2, ComponentType::WasteSinkType, 0, 0 },
	};
	Flow flows[] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
	Model m = {};
	m.NumConstSources = 1;
	m.NumConstLoads = 2;
	m.NumConnectionsAndFlows = 4;
	m.NumConstantEfficiencyConverters = 1;
	m.NumWasteSinks = 1;
	m.Connections = conns;
	m.ConstLoads = loads;
	m.ConstEffConvs = convs;
	m.ConstSources = sources;
	m.Flows = flows;
	Simulate(m);
}

static void
ExampleThreeA() {
	ConstantSource sources[] = { { 100 } };
	ConstantLoad loads[] = { { 10 }, { 2 } };
	ConstantEfficiencyConverter convs[] = { { 1, 2 } };
	Connection conns[] = {
		{ ComponentType::ConstantEfficiencyConverterType, 0, 2, ComponentType::WasteSinkType, 0, 0 },
		{ ComponentType::ConstantEfficiencyConverterType, 0, 1, ComponentType::ConstantLoadType, 1, 0 },
		{ ComponentType::ConstantEfficiencyConverterType, 0, 0, ComponentType::ConstantLoadType, 0, 0 },
		{ ComponentType::ConstantSourceType, 0, 0, ComponentType::ConstantEfficiencyConverterType, 0, 0 },
	};
	Flow flows[] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
	Model m = {};
	m.NumConstSources = 1;
	m.NumConstLoads = 2;
	m.NumConnectionsAndFlows = 4;
	m.NumConstantEfficiencyConverters = 1;
	m.NumWasteSinks = 1;
	m.Connections = conns;
	m.ConstLoads = loads;
	m.ConstEffConvs = convs;
	m.ConstSources = sources;
	m.Flows = flows;
	Simulate(m);
}

int
main(int argc, char** argv) {
	std::cout << "Example 1:" << std::endl;
	ExampleOne();
	std::cout << "Example 2:" << std::endl;
	ExampleTwo();
	std::cout << "Example 3:" << std::endl;
	ExampleThree();
	std::cout << "Example 3A:" << std::endl;
	ExampleThreeA();
	return EXIT_SUCCESS;
}
