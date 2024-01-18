#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string>

// ComponentType
enum class ComponentType
{
	ConstantLoadType,
	ConstantSourceType,
};

// ConstantLoad
struct ConstantLoad {
	uint32_t Load;
};

// ConstantSource
struct ConstantSource {
	uint32_t Available;
};

// Connection
struct Connection {
	ComponentType From;
	size_t FromIdx;
	ComponentType To;
	size_t ToIdx;
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
	ConstantSource* ConstSources;
	ConstantLoad* ConstLoads;
	Connection* Connections;
	Flow* Flows;
};

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
			model.Connections[connIdx].IsActiveBack = (
				model.Connections[connIdx].To == ComponentType::ConstantLoadType
				&& model.Connections[connIdx].ToIdx == loadIdx
				&& model.Flows[connIdx].Requested != model.ConstLoads[loadIdx].Load
			);
		}
	}
}

static void
ActivateConnectionsForConstantSources(Model& model) {
	for (size_t srcIdx = 0; srcIdx < model.NumConstSources; ++srcIdx) {
		for (size_t connIdx = 0; connIdx < model.NumConnectionsAndFlows; ++connIdx) {
			model.Connections[connIdx].IsActiveForward = (
				model.Connections[connIdx].From == ComponentType::ConstantSourceType
				&& model.Connections[connIdx].FromIdx == srcIdx
				&& model.Flows[connIdx].Available != model.ConstSources[srcIdx].Available
			);
		}
	}
}

static void
RunActiveConnections(Model& model) {
	for (size_t connIdx = 0; connIdx < model.NumConnectionsAndFlows; ++connIdx) {
		if (model.Connections[connIdx].IsActiveBack) {
			switch (model.Connections[connIdx].To) {
				case (ComponentType::ConstantLoadType):
				{
					model.Flows[connIdx].Requested = model.ConstLoads[model.Connections[connIdx].ToIdx].Load;
					model.Connections[connIdx].IsActiveBack = false;
				} break;
				default:
				{
					std::cout << "Unhandled component type on backward pass" << std::endl;
				}
			}
		}
	}
	for (size_t connIdx = 0; connIdx < model.NumConnectionsAndFlows; ++connIdx) {
		if (model.Connections[connIdx].IsActiveForward) {
			switch (model.Connections[connIdx].From) {
				case (ComponentType::ConstantSourceType):
				{
					model.Flows[connIdx].Available = model.ConstSources[model.Connections[connIdx].FromIdx].Available;
					model.Connections[connIdx].IsActiveForward = false;
				} break;
				default:
				{
					std::cout << "Unhandled component type on forward pass" << std::endl;
				}
			}
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
	}
	return std::string{ "?" };
}

static void
PrintFlows(Model& m, double t) {
	std::cout << "time: " << t << std::endl;
	for (size_t flowIdx = 0; flowIdx < m.NumConnectionsAndFlows; ++flowIdx) {
		std::cout << ToString(m.Connections[flowIdx].From)
			<< "[" << m.Connections[flowIdx].FromIdx << "] => "
			<< ToString(m.Connections[flowIdx].To)
			<< "[" << m.Connections[flowIdx].ToIdx << "]: "
			<< m.Flows[flowIdx].Actual
			<< " (R: " << m.Flows[flowIdx].Requested
			<< "; A: " << m.Flows[flowIdx].Available << ")"
			<< std::endl;
	}
}

static void
Simulate(Model &model) {
	double t = 0.0;
	ActivateConnectionsForConstantLoads(model);
	ActivateConnectionsForConstantSources(model);
	while (CountActiveConnections(model) > 0) {
		RunActiveConnections(model);
		FinalizeFlows(model);
		ActivateConnectionsForConstantLoads(model);
		ActivateConnectionsForConstantSources(model);
	}
	PrintFlows(model, t);
}

int
main(int argc, char** argv) {
	ConstantSource sources[] = { { 100 }  };
	ConstantLoad loads[] = { { 10 } };
	Connection conns[] = {
		Connection{ ComponentType::ConstantSourceType, 0, ComponentType::ConstantLoadType, 0 },
	};
	Flow flows[] = { { 0, 0, 0 } };
	Model m = {};
	m.NumConstSources = 1;
	m.NumConstLoads = 1;
	m.NumConnectionsAndFlows = 1;
	m.Connections = conns;
	m.ConstLoads = loads;
	m.ConstSources = sources;
	m.Flows = flows;
	Simulate(m);
	return EXIT_SUCCESS;
}
