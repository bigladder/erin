#include "erin_next/erin_next.h"

namespace erin_next {

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
				case (ComponentType::MuxType):
				{
					uint32_t totalRequest = 0;
					std::vector<uint32_t> outflowRequests = {};
					outflowRequests.reserve(model.Muxes[model.Connections[connIdx].FromIdx].NumOutports);
					for (size_t muxOutPort = 0; muxOutPort < model.Muxes[model.Connections[connIdx].FromIdx].NumOutports; ++muxOutPort) {
						size_t outflowConnIdx = FindOutflowConnection(model, ComponentType::MuxType, model.Connections[connIdx].FromIdx, muxOutPort);
						outflowRequests.push_back(model.Flows[outflowConnIdx].Requested);
						totalRequest += model.Flows[outflowConnIdx].Requested;
					}
					std::vector<uint32_t> inflowRequests = {};
					inflowRequests.reserve(model.Muxes[model.Connections[connIdx].FromIdx].NumInports);
					for (size_t muxInPort = 0; muxInPort < model.Muxes[model.Connections[connIdx].FromIdx].NumInports; ++muxInPort) {
						size_t inflowConnIdx = FindInflowConnection(model, ComponentType::MuxType, model.Connections[connIdx].FromIdx, muxInPort);
						uint32_t request = model.Flows[inflowConnIdx].Available >= totalRequest
							? totalRequest
							: model.Flows[inflowConnIdx].Available;
						inflowRequests.push_back(request);
						totalRequest -= request;
					}
					if (totalRequest > 0) {
						inflowRequests[0] += totalRequest;
						totalRequest = 0;
					}
					for (size_t muxInPort = 0; muxInPort < model.Muxes[model.Connections[connIdx].FromIdx].NumInports; ++muxInPort) {
						size_t inflowConnIdx = FindInflowConnection(model, ComponentType::MuxType, model.Connections[connIdx].FromIdx, muxInPort);
						model.Connections[inflowConnIdx].IsActiveBack = model.Flows[inflowConnIdx].Requested != inflowRequests[muxInPort];
						model.Flows[inflowConnIdx].Requested = inflowRequests[muxInPort];
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
				case (ComponentType::MuxType):
				{
					uint32_t totalAvailable = 0;
					for (size_t muxInport = 0; muxInport < model.Muxes[model.Connections[connIdx].ToIdx].NumInports; ++muxInport) {
						size_t inflowConnIdx = FindInflowConnection(model, ComponentType::MuxType, model.Connections[connIdx].ToIdx, muxInport);
						totalAvailable += model.Flows[inflowConnIdx].Available;
					}
					for (size_t muxOutport = 0; muxOutport < model.Muxes[model.Connections[connIdx].ToIdx].NumOutports; ++muxOutport) {
						size_t outflowConnIdx = FindOutflowConnection(model, ComponentType::MuxType, model.Connections[connIdx].ToIdx, muxOutport);
						uint32_t available = model.Flows[outflowConnIdx].Requested >= totalAvailable
							? totalAvailable
							: model.Flows[outflowConnIdx].Requested;
						totalAvailable -= available;
						model.Connections[outflowConnIdx].IsActiveForward = model.Flows[outflowConnIdx].Available != available;
						model.Flows[outflowConnIdx].Available = available;
					}
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

	uint32_t
	FinalizeFlowValue(uint32_t requested, uint32_t available) {
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
		case (ComponentType::MuxType):
		{
			return std::string{ "Mux" };
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
		summary.Time = t;
		summary.Inflow = 0;
		summary.OutflowAchieved = 0;
		summary.OutflowRequest = 0;
		summary.Wasteflow = 0;
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
		return summary;
	}

	void
	PrintFlowSummary(FlowSummary s) {
		uint32_t sum = s.Inflow - (s.OutflowAchieved + s.Wasteflow);
		double eff = 100.0 * ((double)s.OutflowAchieved) / ((double)s.Inflow);
		double effectiveness = 100.0 * ((double)s.OutflowAchieved) / ((double)s.OutflowRequest);
		std::cout << "Flow Summary @ " << s.Time << ":" << std::endl;
		std::cout << "  Inflow                 : " << s.Inflow << std::endl;
		std::cout << "- Outflow (achieved)     : " << s.OutflowAchieved << std::endl;
		std::cout << "- Wasteflow              : " << s.Wasteflow << std::endl;
		std::cout << "-----------------------------------" << std::endl;
		std::cout << "= Sum                    : " << sum << std::endl;
		std::cout << "  Efficiency             : " << eff << "%"
			<< " (= " << s.OutflowAchieved << "/" << s.Inflow << ")" << std::endl;
		std::cout << "  Delivery Effectiveness : " << effectiveness << "%"
			<< " (= " << s.OutflowAchieved << "/" << s.OutflowRequest << ")" << std::endl;
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
	Simulate(Model& model, bool print = true) {
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

	ComponentId
	Model_AddMux(Model& m, size_t numInports, size_t numOutports) {
		size_t id = m.Muxes.size();
		m.Muxes.push_back({ numInports, numOutports });
		return { id, ComponentType::MuxType };
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

}