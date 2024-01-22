#include "erin_next/erin_next.h"

size_t
CountActiveConnections(Model& m) {
	size_t count = 0;
	for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx) {
		if (m.Connections[connIdx].IsActiveBack || m.Connections[connIdx].IsActiveForward)
		{
			++count;
		}
	}
	return count;
}

void
ActivateConnectionsForConstantLoads(Model& model) {
	for (size_t loadIdx = 0; loadIdx < model.ConstLoads.size(); ++loadIdx) {
		for (size_t connIdx = 0; connIdx < model.Connections.size(); ++connIdx) {
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

void
ActivateConnectionsForConstantSources(Model& model) {
	for (size_t srcIdx = 0; srcIdx < model.ConstSources.size(); ++srcIdx) {
		for (size_t connIdx = 0; connIdx < model.Connections.size(); ++connIdx) {
			if (model.Connections[connIdx].From == ComponentType::ConstantSourceType
				&& model.Connections[connIdx].FromIdx == srcIdx) {
				model.Connections[connIdx].IsActiveForward =
					model.Flows[connIdx].Available != model.ConstSources[srcIdx].Available;
				model.Flows[connIdx].Available = model.ConstSources[srcIdx].Available;
			}
		}
	}
}

void
ActivateConnectionsForScheduleBasedLoads(Model& m, double t) {
	for (size_t schIdx = 0; schIdx < m.ScheduledLoads.size(); ++schIdx) {
		for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx) {
			if (m.Connections[connIdx].To == ComponentType::ScheduleBasedLoadType
				&& m.Connections[connIdx].ToIdx == schIdx) {
				for (size_t itemIdx = 0; itemIdx < m.ScheduledLoads[schIdx].TimesAndLoads.size(); ++itemIdx) {
					if (m.ScheduledLoads[schIdx].TimesAndLoads[itemIdx].Time == t) {
						m.Connections[connIdx].IsActiveBack =
							m.Flows[connIdx].Requested != m.ScheduledLoads[schIdx].TimesAndLoads[itemIdx].Load;
						m.Flows[connIdx].Requested = m.ScheduledLoads[schIdx].TimesAndLoads[itemIdx].Load;
					}
				}
			}
		}
	}
}

double
EarliestNextEvent(Model& m, double t) {
	double nextTime = -1.0;
	bool nextTimeFound = false;
	// go through all the components that are potential event generators
	for (size_t schIdx = 0; schIdx < m.ScheduledLoads.size(); ++schIdx) {
		double nextTimeForComponent = NextEvent(m.ScheduledLoads[schIdx], t);
		if (!nextTimeFound || (nextTimeForComponent >= 0.0 && nextTimeForComponent < nextTime)) {
			nextTime = nextTimeForComponent;
			nextTimeFound = true;
		}
	}
	return nextTime;
}

int
FindInflowConnection(Model& m, ComponentType ct, size_t compId, size_t inflowPort) {
	for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx) {
		if (m.Connections[connIdx].To == ct && m.Connections[connIdx].ToIdx == compId && m.Connections[connIdx].ToPort == inflowPort) {
			return (int)connIdx;
		}
	}
	return -1;
}

int
FindOutflowConnection(Model& m, ComponentType ct, size_t compId, size_t outflowPort) {
	for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx) {
		if (m.Connections[connIdx].From == ct && m.Connections[connIdx].FromIdx == compId && m.Connections[connIdx].FromPort == outflowPort) {
			return (int)connIdx;
		}
	}
	return -1;
}

void
RunActiveConnections(Model& model) {
	for (size_t connIdx = 0; connIdx < model.Connections.size(); ++connIdx) {
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
					uint32_t attenuatedLossflowRequest = 0;
					int lossflowConn = FindOutflowConnection(model, model.Connections[connIdx].From, model.Connections[connIdx].FromIdx, 1);
					if (lossflowConn >= 0) {
						attenuatedLossflowRequest = FinalizeFlowValue(inflowRequest - outflowRequest, model.Flows[(size_t)lossflowConn].Requested);
					}
					int wasteflowConn = FindOutflowConnection(model, model.Connections[connIdx].From, model.Connections[connIdx].FromIdx, 2);
					assert((wasteflowConn >= 0) && "should find a wasteflow connection; model is incorrectly connected");
					uint32_t wasteflowRequest = inflowRequest - outflowRequest - attenuatedLossflowRequest;
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
	for (size_t connIdx = 0; connIdx < model.Connections.size(); ++connIdx) {
		if (model.Connections[connIdx].IsActiveForward) {
			switch (model.Connections[connIdx].To) {
			case (ComponentType::ConstantLoadType):
			case (ComponentType::WasteSinkType):
			case (ComponentType::ScheduleBasedLoadType):
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
				uint32_t nonOutflowAvailable = FinalizeFlowValue(inflowAvailable, inflowRequest) - FinalizeFlowValue(outflowAvailable, outflowRequest);
				uint32_t lossflowAvailable = 0;
				uint32_t lossflowRequest = 0;
				if (lossflowConn >= 0) {
					lossflowRequest = model.Flows[(size_t)lossflowConn].Requested;
					lossflowAvailable = FinalizeFlowValue(nonOutflowAvailable, lossflowRequest);
					nonOutflowAvailable -= lossflowAvailable;
					model.Connections[(size_t)lossflowConn].IsActiveForward = lossflowAvailable != model.Flows[(size_t)lossflowConn].Available;
					model.Flows[(size_t)lossflowConn].Available = lossflowAvailable;
				}
				int wasteflowConn = FindOutflowConnection(model, model.Connections[connIdx].To, model.Connections[connIdx].ToIdx, 2);
				assert((wasteflowConn >= 0) && "should find a wasteflow connection; model is incorrectly connected");
				model.Flows[(size_t)wasteflowConn].Requested = nonOutflowAvailable;
				model.Flows[(size_t)wasteflowConn].Available = nonOutflowAvailable;
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

uint32_t FinalizeFlowValue(uint32_t requested, uint32_t available) {
	return available >= requested ? requested : available;
}

void
FinalizeFlows(Model& model) {
	for (size_t flowIdx = 0; flowIdx < model.Flows.size(); ++flowIdx) {
		model.Flows[flowIdx].Actual = FinalizeFlowValue(model.Flows[flowIdx].Requested, model.Flows[flowIdx].Available);
	}
}

double
NextEvent(ScheduleBasedLoad sb, double t) {
	for (size_t i = 0; i < sb.TimesAndLoads.size(); ++i) {
		if (sb.TimesAndLoads[i].Time > t)
		{
			return sb.TimesAndLoads[i].Time;
		}
	}
	return -1.0;
}

std::string
ToString(ComponentType compType) {
	switch (compType) {
	case (ComponentType::ConstantLoadType):
	{
		return std::string{ "ConstantLoad" };
	} break;
	case (ComponentType::ScheduleBasedLoadType):
	{
		return std::string{ "ScheduleBasedLoad" };
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

void
PrintFlows(Model& m, double t) {
	std::cout << "time: " << t << std::endl;
	for (size_t flowIdx = 0; flowIdx < m.Flows.size(); ++flowIdx) {
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

FlowSummary
SummarizeFlows(Model& m, double t) {
	FlowSummary summary = {};
	for (size_t flowIdx = 0; flowIdx < m.Flows.size(); ++flowIdx) {
		switch (m.Connections[flowIdx].From) {
		case (ComponentType::ConstantSourceType):
		{
			summary.Inflow += m.Flows[flowIdx].Actual;
		} break;
		}

		switch (m.Connections[flowIdx].To) {
		case (ComponentType::ConstantLoadType):
		case (ComponentType::ScheduleBasedLoadType):
		{
			summary.OutflowRequest += m.Flows[flowIdx].Requested;
			summary.OutflowAchieved += m.Flows[flowIdx].Actual;
		} break;
		case (ComponentType::WasteSinkType):
		{
			summary.Wasteflow += m.Flows[flowIdx].Actual;
		} break;
		}
	}
	summary.Time = t;
	return summary;
}

void
PrintFlowSummary(FlowSummary s) {
	uint32_t sum = s.Inflow - (s.OutflowRequest + s.Wasteflow);
	double eff = 100.0 * ((double)s.OutflowRequest) / ((double)s.Inflow);
	double effectiveness = 100.0 * ((double)s.OutflowAchieved) / ((double)s.OutflowRequest);
	std::cout << "Flow Summary @ " << s.Time << ":" << std::endl;
	std::cout << "  Inflow                 : " << s.Inflow << std::endl;
	std::cout << "- Outflow (achieved)     : " << s.OutflowAchieved << std::endl;
	std::cout << "- Wasteflow              : " << s.Wasteflow << std::endl;
	std::cout << "-----------------------------------" << std::endl;
	std::cout << "= Sum                    : " << sum << std::endl;
	std::cout << "  Efficiency             : " << eff << "%"
		<< " (= " << s.OutflowRequest << "/" << s.Inflow << ")" << std::endl;
	std::cout << "  Delivery Effectiveness : " << effectiveness << "%" << std::endl;
}

std::vector<Flow>
CopyFlows(std::vector<Flow> flows) {
	std::vector<Flow> newFlows = {};
	newFlows.reserve(flows.size());
	for (int i = 0; i < flows.size(); ++i) {
		Flow f(flows[i]);
		newFlows.push_back(f);
	}
	return newFlows;
}

std::vector<TimeAndFlows>
Simulate(Model& model, bool print=true) {
	double t = 0.0;
	std::vector<TimeAndFlows> timeAndFlows = {};
	size_t const maxLoopIter = 100;
	for (size_t loopIdx = 0; loopIdx < maxLoopIter; ++loopIdx) {
		ActivateConnectionsForScheduleBasedLoads(model, t);
		if (loopIdx == 0) {
			ActivateConnectionsForConstantLoads(model);
			ActivateConnectionsForConstantSources(model);
		}
		while (CountActiveConnections(model) > 0) {
			RunActiveConnections(model);
		}
		FinalizeFlows(model);
		if (print) {
			PrintFlows(model, t);
			PrintFlowSummary(SummarizeFlows(model, t));
		}
		timeAndFlows.push_back({ t, CopyFlows(model.Flows) });
		double nextTime = EarliestNextEvent(model, t);
		if (nextTime < 0.0) {
			break;
		}
		t = nextTime;
	}
	return timeAndFlows;
}

ComponentId
Model_AddConstantLoad(Model& m, uint32_t load) {
	size_t id = m.ConstLoads.size();
	m.ConstLoads.push_back({ load });
	return { id, ComponentType::ConstantLoadType };
}

ComponentId
Model_AddScheduleBasedLoad(Model& m, double* times, uint32_t* loads, size_t numItems) {
	size_t id = m.ScheduledLoads.size();
	std::vector<TimeAndLoad> timesAndLoads = {};
	timesAndLoads.reserve(numItems);
	for (size_t i = 0; i < numItems; ++i) {
		timesAndLoads.push_back({ times[i], loads[i] });
	}
	m.ScheduledLoads.push_back({ std::move(timesAndLoads) });
	return { id, ComponentType::ScheduleBasedLoadType };
}

ComponentId
Model_AddScheduleBasedLoad(Model& m, std::vector<TimeAndLoad> timesAndLoads) {
	size_t id = m.ScheduledLoads.size();
	m.ScheduledLoads.push_back({ std::vector<TimeAndLoad>(timesAndLoads) });
	return { id, ComponentType::ScheduleBasedLoadType };
}

ComponentId
Model_AddConstantSource(Model& m, uint32_t available) {
	size_t id = m.ConstSources.size();
	m.ConstSources.push_back({ available });
	return { id, ComponentType::ConstantSourceType };
}

ComponentIdAndWasteConnection
Model_AddConstantEfficiencyConverter(Model& m, uint32_t eff_numerator, uint32_t eff_denominator) {
	size_t id = m.ConstEffConvs.size();
	m.ConstEffConvs.push_back({ eff_numerator, eff_denominator });
	ComponentId wasteId = { 0, ComponentType::WasteSinkType };
	ComponentId thisId = { id, ComponentType::ConstantEfficiencyConverterType };
	auto wasteConn = Model_AddConnection(m, thisId, 2, wasteId, 0);
	return { thisId, wasteConn };
}

Connection
Model_AddConnection(Model& m, ComponentId& from, size_t fromPort, ComponentId& to, size_t toPort) {
	Connection c = { from.Type, from.Id, fromPort, to.Type, to.Id, toPort };
	m.Connections.push_back(c);
	m.Flows.push_back({ 0, 0, 0 });
	return c;
}

bool
SameConnection(Connection a, Connection b) {
	return a.From == b.From && a.FromIdx == b.FromIdx && a.FromPort == b.FromPort
		&& a.To == b.To && a.ToIdx == b.ToIdx && a.ToPort == b.ToPort;
}

std::optional<Flow>
ModelResults_GetFlowForConnection(Model& m, Connection conn, double time, std::vector<TimeAndFlows> timeAndFlows) {
	for (size_t connId = 0; connId < m.Connections.size(); ++connId) {
		if (SameConnection(m.Connections[connId], conn)) {
			Flow f = {};
			for (size_t timeAndFlowIdx = 0; timeAndFlowIdx < timeAndFlows.size(); ++timeAndFlowIdx) {
				if (time >= timeAndFlows[timeAndFlowIdx].Time) {
					f = timeAndFlows[timeAndFlowIdx].Flows[connId];
				}
				else {
					break;
				}
			}
			return f;
		}
	}
	return {};
}

void
Example1(bool print) {
	if (print) {
		std::cout << "Example  1:" << std::endl;
	}
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToLoadConn = Model_AddConnection(m, srcId, 0, loadId, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 1 && "size of flows must equal 1"));
	auto srcToLoadResult = ModelResults_GetFlowForConnection(m, srcToLoadConn, 0.0, results);
	assert((srcToLoadResult.has_value() && "connection result should have a value"));
	assert((srcToLoadResult.value().Actual == 10 && "actual value must equal 10"));
	assert((srcToLoadResult.value().Available == 100 && "available must equal 100"));
	assert((srcToLoadResult.value().Requested == 10 && "requested must equal 10"));
	std::cout << "[Example  1] :: PASSED" << std::endl;
}

void
Example2(bool print) {
	if (print) {
		std::cout << "Example  2:" << std::endl;
	}
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
	auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
	auto convToLoadConn = Model_AddConnection(m, convId.Id, 0, loadId, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 3 && "size of flows must equal 3"));
	auto srcToConvResults = ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value() && "source to converter must have results"));
	assert((srcToConvResults.value().Requested == 20 && "requested must equal 20"));
	assert((srcToConvResults.value().Actual == 20 && "actual value must equal 20"));
	assert((srcToConvResults.value().Available == 100 && "available must equal 100"));
	auto convToLoadResults = ModelResults_GetFlowForConnection(m, convToLoadConn, 0.0, results);
	assert((convToLoadResults.has_value() && "converter to load must have results"));
	assert((convToLoadResults.value().Requested == 10 && "requested must equal 10"));
	assert((convToLoadResults.value().Actual == 10 && "actual value must equal 10"));
	assert((convToLoadResults.value().Available == 50 && "available must equal 50"));
	auto convToWasteResults = ModelResults_GetFlowForConnection(m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value() && "converter to waste must have results"));
	assert((convToWasteResults.value().Requested == 10 && "requested must equal 10"));
	assert((convToWasteResults.value().Actual == 10 && "actual value must equal 10"));
	assert((convToWasteResults.value().Available == 10 && "available must equal 10"));
	std::cout << "[Example  2] :: PASSED" << std::endl;
}

void
Example3(bool print) {
	if (print) {
		std::cout << "Example  3:" << std::endl;
	}
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto load1Id = Model_AddConstantLoad(m, 10);
	auto load2Id = Model_AddConstantLoad(m, 2);
	auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
	auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
	auto convToLoad1Conn = Model_AddConnection(m, convId.Id, 0, load1Id, 0);
	auto convToLoad2Conn = Model_AddConnection(m, convId.Id, 1, load2Id, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 4 && "size of flows must equal 4"));
	auto srcToConvResults = ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value() && "source to converter must have results"));
	assert((srcToConvResults.value().Requested == 20 && "requested must equal 20"));
	assert((srcToConvResults.value().Actual == 20 && "actual value must equal 20"));
	assert((srcToConvResults.value().Available == 100 && "available must equal 100"));
	auto convToLoad1Results = ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
	assert((convToLoad1Results.has_value() && "converter to load1 must have results"));
	assert((convToLoad1Results.value().Requested == 10 && "requested must equal 10"));
	assert((convToLoad1Results.value().Actual == 10 && "actual value must equal 10"));
	assert((convToLoad1Results.value().Available == 50 && "available must equal 50"));
	auto convToLoad2Results = ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
	assert((convToLoad2Results.has_value() && "conv to load2 must have results"));
	assert((convToLoad2Results.value().Requested == 2 && "requested must equal 2"));
	assert((convToLoad2Results.value().Actual == 2 && "actual value must equal 2"));
	assert((convToLoad2Results.value().Available == 2 && "available must equal 50"));
	auto convToWasteResults = ModelResults_GetFlowForConnection(m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value() && "conv to waste must have results"));
	assert((convToWasteResults.value().Requested == 8 && "requested must equal 8"));
	assert((convToWasteResults.value().Actual == 8 && "actual value must equal 8"));
	assert((convToWasteResults.value().Available == 8 && "available must equal 48"));
	std::cout << "[Example  3] :: PASSED" << std::endl;
}

void
Example3A(bool print) {
	if (print) {
		std::cout << "Example 3A:" << std::endl;
	}
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto load1Id = Model_AddConstantLoad(m, 10);
	auto load2Id = Model_AddConstantLoad(m, 2);
	auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
	auto convToLoad2Conn = Model_AddConnection(m, convId.Id, 1, load2Id, 0);
	auto convToLoad1Conn = Model_AddConnection(m, convId.Id, 0, load1Id, 0);
	auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 4 && "size of flows must equal 4"));
	auto srcToConvResults = ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value() && "source to converter must have results"));
	assert((srcToConvResults.value().Requested == 20 && "requested must equal 20"));
	assert((srcToConvResults.value().Actual == 20 && "actual value must equal 20"));
	assert((srcToConvResults.value().Available == 100 && "available must equal 100"));
	auto convToLoad1Results = ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
	assert((convToLoad1Results.has_value() && "converter to load1 must have results"));
	assert((convToLoad1Results.value().Requested == 10 && "requested must equal 10"));
	assert((convToLoad1Results.value().Actual == 10 && "actual value must equal 10"));
	assert((convToLoad1Results.value().Available == 50 && "available must equal 50"));
	auto convToLoad2Results = ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
	assert((convToLoad2Results.has_value() && "conv to load2 must have results"));
	assert((convToLoad2Results.value().Requested == 2 && "requested must equal 2"));
	assert((convToLoad2Results.value().Actual == 2 && "actual value must equal 2"));
	assert((convToLoad2Results.value().Available == 2 && "available must equal 50"));
	auto convToWasteResults = ModelResults_GetFlowForConnection(m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value() && "conv to waste must have results"));
	assert((convToWasteResults.value().Requested == 8 && "requested must equal 8"));
	assert((convToWasteResults.value().Actual == 8 && "actual value must equal 8"));
	assert((convToWasteResults.value().Available == 8 && "available must equal 48"));
	std::cout << "[Example 3A] :: PASSED" << std::endl;
}

void
Example4(bool print) {
	if (print) {
		std::cout << "Example  4:" << std::endl;
	}
	std::vector<TimeAndLoad> timesAndLoads = {};
	timesAndLoads.push_back({ 0.0, 10 });
	timesAndLoads.push_back({ 3600.0, 200 });
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddScheduleBasedLoad(m, timesAndLoads);
	auto srcToLoadConn = Model_AddConnection(m, srcId, 0, loadId, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 2 && "output must have a size of 2"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 1 && "size of flows[0] must equal 1"));
	auto srcToLoadResults_0 = ModelResults_GetFlowForConnection(m, srcToLoadConn, 0.0, results);
	assert((srcToLoadResults_0.has_value() && "source to load must have results at time=0.0"));
	assert((srcToLoadResults_0.value().Requested == 10 && "requested must equal 10"));
	assert((srcToLoadResults_0.value().Actual == 10 && "actual value must equal 10"));
	assert((srcToLoadResults_0.value().Available == 100 && "available must equal 100"));
	assert((results[1].Time == 3600.0 && "time must equal 3600.0"));
	assert((results[1].Flows.size() == 1 && "size of flows[1] must equal 1"));
	auto srcToLoadResults_3600 = ModelResults_GetFlowForConnection(m, srcToLoadConn, 3600.0, results);
	assert((srcToLoadResults_3600.has_value() && "source to load must have results at time=3600.0"));
	assert((srcToLoadResults_3600.value().Requested == 200 && "requested must equal 200"));
	assert((srcToLoadResults_3600.value().Actual == 100 && "actual value must equal 100"));
	assert((srcToLoadResults_3600.value().Available == 100 && "available must equal 100"));
	std::cout << "[Example  4] :: PASSED" << std::endl;
}

void
Example5(bool print) {
	if (print) {
		std::cout << "Example  5:" << std::endl;
	}
	std::vector<TimeAndLoad> timesAndLoads = {};
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto load1Id = Model_AddConstantLoad(m, 10);
	auto load2Id = Model_AddConstantLoad(m, 7);
	auto load3Id = Model_AddConstantLoad(m, 5);
	auto conv1 = Model_AddConstantEfficiencyConverter(m, 1, 4);
	auto conv2 = Model_AddConstantEfficiencyConverter(m, 1, 4);
	auto conv3 = Model_AddConstantEfficiencyConverter(m, 1, 4);
	auto srcToConv1Conn = Model_AddConnection(m, srcId, 0, conv1.Id, 0);
	auto conv1ToLoad1Conn = Model_AddConnection(m, conv1.Id, 0, load1Id, 0);
	auto conv1ToConv2Conn = Model_AddConnection(m, conv1.Id, 1, conv2.Id, 0);
	auto conv2ToLoad2Conn = Model_AddConnection(m, conv2.Id, 0, load2Id, 0);
	auto conv2ToConv3Conn = Model_AddConnection(m, conv2.Id, 1, conv3.Id, 0);
	auto conv3ToLoad3Conn = Model_AddConnection(m, conv3.Id, 0, load3Id, 0);
	auto results = Simulate(m, print);
	auto srcToConv1Results = ModelResults_GetFlowForConnection(m, srcToConv1Conn, 0.0, results);
	auto conv1ToLoad1Results = ModelResults_GetFlowForConnection(m, conv1ToLoad1Conn, 0.0, results);
	auto conv1ToConv2Results = ModelResults_GetFlowForConnection(m, conv1ToConv2Conn, 0.0, results);
	auto conv2ToLoad2Results = ModelResults_GetFlowForConnection(m, conv2ToLoad2Conn, 0.0, results);
	auto conv2ToConv3Results = ModelResults_GetFlowForConnection(m, conv2ToConv3Conn, 0.0, results);
	auto conv3ToLoad3Results = ModelResults_GetFlowForConnection(m, conv3ToLoad3Conn, 0.0, results);
	assert((srcToConv1Results.value().Actual == 40 && "src to conv1 should flow 40"));
	assert((conv1ToLoad1Results.value().Actual == 10 && "conv1 to load1 should flow 10"));
	assert((conv1ToConv2Results.value().Actual == 28 && "conv1 to conv2 should flow 28"));
	assert((conv2ToLoad2Results.value().Actual == 7 && "conv1 to conv2 should flow 7"));
	assert((conv2ToConv3Results.value().Actual == 20 && "conv2 to conv3 should flow 21"));
	assert((conv3ToLoad3Results.value().Actual == 5 && "conv3 to load3 should flow 5"));
	std::cout << "[Example  5] :: PASSED" << std::endl;
}

int
main(int argc, char** argv) {
	Example1(false);
	Example2(false);
	Example3(false);
	Example3A(false);
	Example4(false);
	Example5(false);
	return EXIT_SUCCESS;
}
