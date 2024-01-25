#include "erin_next/erin_next.h"
#include <cmath>

namespace erin_next {

	static unsigned int numBackwardPasses = 0;
	static unsigned int numForwardPasses = 0;
	static unsigned int numPostPasses = 0;
	static unsigned int grandTotalPasses = 0;

	void Debug_PrintNumberOfPasses(bool onlyGrandTotal)
	{
		if (onlyGrandTotal) {
			std::cout << "Grand total        : " << grandTotalPasses << std::endl;
		}
		else {
			std::cout << "Number of:" << std::endl;
			std::cout << "... backward passes: " << numBackwardPasses << std::endl;
			std::cout << "... forward passes : " << numForwardPasses << std::endl;
			std::cout << "... post passes    : " << numPostPasses << std::endl;
			std::cout << "... total passes   : "
				<< (numBackwardPasses + numForwardPasses + numPostPasses)
				<< std::endl;
		}
	}

	void Debug_ResetNumberOfPasses(bool resetAll)
	{
		grandTotalPasses += numBackwardPasses + numForwardPasses + numPostPasses;
		numBackwardPasses = 0;
		numForwardPasses = 0;
		numPostPasses = 0;
		if (resetAll) {
			grandTotalPasses = 0;
		}
	}

	size_t
	CountActiveConnections(SimulationState& ss)
	{
		return (
			ss.ActiveConnectionsBack.size()
			+ ss.ActiveConnectionsFront.size()
			+ ss.ActiveConnectionsPost.size());
	}

	void
	Helper_AddIfNotAdded(std::vector<size_t>& items, size_t item)
	{
		for (size_t i = 0; i < items.size(); ++i)
		{
			if (items[i] == item)
			{
				return;
			}
		}
		items.push_back(item);
	}

	// TODO: consider switching to a Set<size_t> for the below to eliminate looping over the active indices
	void
	SimulationState_AddActiveConnectionBack(SimulationState& ss, size_t connIdx)
	{
		Helper_AddIfNotAdded(ss.ActiveConnectionsBack, connIdx);
	}

	void
	SimulationState_AddActiveConnectionForward(SimulationState& ss, size_t connIdx)
	{
		Helper_AddIfNotAdded(ss.ActiveConnectionsFront, connIdx);
	}

	void
	SimulationState_AddActiveConnectionPost(SimulationState& ss, size_t connIdx)
	{
		Helper_AddIfNotAdded(ss.ActiveConnectionsPost, connIdx);
	}

	void
	ActivateConnectionsForConstantLoads(Model& model, SimulationState& ss) {
		for (size_t loadIdx = 0; loadIdx < model.ConstLoads.size(); ++loadIdx)
		{
			for (size_t connIdx = 0; connIdx < model.Connections.size(); ++connIdx)
			{
				if (model.Connections[connIdx].To == ComponentType::ConstantLoadType
					&& model.Connections[connIdx].ToIdx == loadIdx)
				{
					if (model.Flows[connIdx].Requested != model.ConstLoads[loadIdx].Load)
					{
						SimulationState_AddActiveConnectionBack(ss, connIdx);
					}
					model.Flows[connIdx].Requested = model.ConstLoads[loadIdx].Load;
				}
			}
		}
	}

	void
	ActivateConnectionsForConstantSources(Model& model, SimulationState& ss)
	{
		for (size_t srcIdx = 0; srcIdx < model.ConstSources.size(); ++srcIdx)
		{
			for (size_t connIdx = 0; connIdx < model.Connections.size(); ++connIdx)
			{
				if (model.Connections[connIdx].From == ComponentType::ConstantSourceType
					&& model.Connections[connIdx].FromIdx == srcIdx)
				{
					if (model.Flows[connIdx].Available != model.ConstSources[srcIdx].Available)
					{
						SimulationState_AddActiveConnectionForward(ss, connIdx);
					}
					model.Flows[connIdx].Available = model.ConstSources[srcIdx].Available;
				}
			}
		}
	}

	void
	ActivateConnectionsForScheduleBasedLoads(Model& m, SimulationState& ss, double t)
	{
		for (size_t schIdx = 0; schIdx < m.ScheduledLoads.size(); ++schIdx)
		{
			for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx)
			{
				if (m.Connections[connIdx].To == ComponentType::ScheduleBasedLoadType
					&& m.Connections[connIdx].ToIdx == schIdx)
				{
					for (size_t itemIdx = 0; itemIdx < m.ScheduledLoads[schIdx].TimesAndLoads.size(); ++itemIdx)
					{
						if (m.ScheduledLoads[schIdx].TimesAndLoads[itemIdx].Time == t)
						{
							if (m.Flows[connIdx].Requested != m.ScheduledLoads[schIdx].TimesAndLoads[itemIdx].Load)
							{
								SimulationState_AddActiveConnectionBack(ss, connIdx);
							}
							m.Flows[connIdx].Requested = m.ScheduledLoads[schIdx].TimesAndLoads[itemIdx].Load;
						}
					}
				}
			}
		}
	}

	void
	ActivateConnectionsForStores(Model& m, SimulationState& ss, double t)
	{
		for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
		{
			if (m.Stores[storeIdx].TimeOfNextEvent == t)
			{
				int inflowConn = FindInflowConnection(m, ComponentType::StoreType, storeIdx, 0);
				int outflowConn = FindOutflowConnection(m, ComponentType::StoreType, storeIdx, 0);
				assert(inflowConn >= 0 && "we should have an inflow connection");
				assert(outflowConn >= 0 && "we should have an outflow connection");
				uint32_t available = m.Flows[(size_t)inflowConn].Available + (
					m.Stores[storeIdx].Stored > 0 ? m.Stores[storeIdx].MaxDischargeRate : 0);
				if (m.Flows[(size_t)outflowConn].Available != available)
				{
					SimulationState_AddActiveConnectionForward(ss, (size_t)outflowConn);
				}
				m.Flows[(size_t)outflowConn].Available = available;
				uint32_t request = m.Flows[(size_t)outflowConn].Requested + (
					m.Stores[storeIdx].Stored <= m.Stores[storeIdx].ChargeAmount
					? m.Stores[storeIdx].MaxChargeRate
					: 0);
				if (m.Flows[(size_t)inflowConn].Requested != request)
				{
					SimulationState_AddActiveConnectionForward(ss, (size_t)inflowConn);
				}
				m.Flows[(size_t)inflowConn].Requested = request;
			}
		}
	}

	double
	EarliestNextEvent(Model& m, double t)
	{
		double nextTime = infinity;
		// TODO[mok]: eliminate duplicate code here
		for (size_t schIdx = 0; schIdx < m.ScheduledLoads.size(); ++schIdx)
		{
			double nextTimeForComponent = NextEvent(m.ScheduledLoads[schIdx], t);
			if (nextTime == infinity || (nextTimeForComponent >= 0.0 && nextTimeForComponent < nextTime))
			{
				nextTime = nextTimeForComponent;
			}
		}
		for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
		{
			double nextTimeForComponent = NextEvent(m.Stores[storeIdx], t);
			if (nextTime == infinity || (nextTimeForComponent >= 0.0 && nextTimeForComponent < nextTime))
			{
				nextTime = nextTimeForComponent;
			}
		}
		return nextTime;
	}

	// TODO[mok]: consider changing this into a std::optional<size_t> as that would better express intent
	// TODO: consider reworking this to be an O(1) operation. Should components store connection indices?
	int
	FindInflowConnection(Model& m, ComponentType ct, size_t compId, size_t inflowPort)
	{
		for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx)
		{
			if (m.Connections[connIdx].To == ct
				&& m.Connections[connIdx].ToIdx == compId
				&& m.Connections[connIdx].ToPort == inflowPort)
			{
				return (int)connIdx;
			}
		}
		return -1;
	}

	// TODO[mok]: consider changing this into a std::optional<size_t> as that would better express intent
	int
	FindOutflowConnection(Model& m, ComponentType ct, size_t compId, size_t outflowPort)
	{
		for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx)
		{
			if (m.Connections[connIdx].From == ct
				&& m.Connections[connIdx].FromIdx == compId
				&& m.Connections[connIdx].FromPort == outflowPort)
			{
				return (int)connIdx;
			}
		}
		return -1;
	}

	void
	RunConnectionsBackward(Model& model, SimulationState& ss)
	{
		while (ss.ActiveConnectionsBack.size() > 0)
		{
			size_t connIdx = ss.ActiveConnectionsBack.back();
			ss.ActiveConnectionsBack.pop_back();
			size_t compIdx = model.Connections[connIdx].FromIdx;
			switch (model.Connections[connIdx].From)
			{
				case (ComponentType::ConstantSourceType):
				{
				} break;
				case (ComponentType::ConstantEfficiencyConverterType):
				{
					switch (model.Connections[connIdx].FromPort)
					{
						case 0:
						{
							++numBackwardPasses;
							int inflowConn = FindInflowConnection(
								model, model.Connections[connIdx].From, compIdx, 0);
							assert((inflowConn >= 0) && "should find an inflow connection; model is incorrectly connected");
							uint32_t outflowRequest = model.Flows[connIdx].Requested;
							uint32_t inflowRequest =
								(model.ConstEffConvs[compIdx].EfficiencyDenominator * outflowRequest)
								/ model.ConstEffConvs[compIdx].EfficiencyNumerator;
							assert((inflowRequest >= outflowRequest) && "inflow must be >= outflow in converter");
							if (inflowRequest != model.Flows[(size_t)inflowConn].Requested)
							{
								Helper_AddIfNotAdded(ss.ActiveConnectionsBack, (size_t)inflowConn);
							}
							model.Flows[(size_t)inflowConn].Requested = inflowRequest;
							uint32_t attenuatedLossflowRequest = 0;
							int lossflowConn = FindOutflowConnection(
								model, model.Connections[connIdx].From, compIdx, 1);
							if (lossflowConn >= 0) {
								attenuatedLossflowRequest = FinalizeFlowValue(
									inflowRequest - outflowRequest, model.Flows[(size_t)lossflowConn].Requested);
							}
							int wasteflowConn = FindOutflowConnection(
								model, model.Connections[connIdx].From, compIdx, 2);
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
					++numBackwardPasses;
					uint32_t totalRequest = 0;
					std::vector<uint32_t> outflowRequests = {};
					outflowRequests.reserve(model.Muxes[compIdx].NumOutports);
					for (size_t muxOutPort = 0; muxOutPort < model.Muxes[compIdx].NumOutports; ++muxOutPort)
					{
						size_t outflowConnIdx = FindOutflowConnection(model, ComponentType::MuxType, compIdx, muxOutPort);
						outflowRequests.push_back(model.Flows[outflowConnIdx].Requested);
						totalRequest += model.Flows[outflowConnIdx].Requested;
					}
					std::vector<uint32_t> inflowRequests = {};
					inflowRequests.reserve(model.Muxes[compIdx].NumInports);
					for (size_t muxInPort = 0; muxInPort < model.Muxes[compIdx].NumInports; ++muxInPort)
					{
						size_t inflowConnIdx = FindInflowConnection(model, ComponentType::MuxType, compIdx, muxInPort);
						uint32_t request = model.Flows[inflowConnIdx].Available >= totalRequest
							? totalRequest
							: model.Flows[inflowConnIdx].Available;
						inflowRequests.push_back(request);
						totalRequest -= request;
					}
					if (totalRequest > 0)
					{
						inflowRequests[0] += totalRequest;
						totalRequest = 0;
					}
					for (size_t muxInPort = 0; muxInPort < model.Muxes[compIdx].NumInports; ++muxInPort)
					{
						int inflowConnIdx = FindInflowConnection(model, ComponentType::MuxType, compIdx, muxInPort);
						assert(inflowConnIdx >= 0 && "mux must have at one inflow per inflow port");
						if (model.Flows[inflowConnIdx].Requested != inflowRequests[muxInPort])
						{
							Helper_AddIfNotAdded(ss.ActiveConnectionsBack, (size_t)inflowConnIdx);
						}
						model.Flows[inflowConnIdx].Requested = inflowRequests[muxInPort];
					}
				} break;
				case (ComponentType::StoreType):
				{
					++numBackwardPasses;
					int inflowConnIdx =
						FindInflowConnection(model, ComponentType::StoreType, model.Connections[connIdx].FromIdx, 0);
					assert(inflowConnIdx >= 0 && "must have an inflow connection to a store");
					uint32_t chargeRate =
						model.Stores[compIdx].Stored <= model.Stores[compIdx].ChargeAmount
						? model.Stores[compIdx].MaxChargeRate
						: 0;
					if (model.Flows[(size_t)inflowConnIdx].Requested != (model.Flows[connIdx].Requested + chargeRate))
					{
						Helper_AddIfNotAdded(ss.ActiveConnectionsBack, (size_t)inflowConnIdx);
					}
					model.Flows[(size_t)inflowConnIdx].Requested = model.Flows[connIdx].Requested + chargeRate;
				} break;
				default:
				{
					std::cout << "Unhandled component type on backward pass: "
						<< ToString(model.Connections[connIdx].From) << std::endl;
				}
			}
		}
	}

	void
	RunConnectionsForward(Model& model, SimulationState& ss)
	{
		while (ss.ActiveConnectionsFront.size() > 0)
		{
			size_t connIdx = ss.ActiveConnectionsFront.back();
			ss.ActiveConnectionsFront.pop_back();
			size_t compIdx = model.Connections[connIdx].ToIdx;
			switch (model.Connections[connIdx].To)
			{
				case (ComponentType::ConstantLoadType):
				case (ComponentType::WasteSinkType):
				case (ComponentType::ScheduleBasedLoadType):
				{
				} break;
				case (ComponentType::ConstantEfficiencyConverterType):
				{
					++numForwardPasses;
					uint32_t inflowAvailable = model.Flows[connIdx].Available;
					uint32_t inflowRequest = model.Flows[connIdx].Requested;
					int outflowConn = FindOutflowConnection(model, model.Connections[connIdx].To, compIdx, 0);
					assert((outflowConn >= 0) && "should find an outflow connection; model is incorrectly connected");
					uint32_t outflowAvailable =
						(model.ConstEffConvs[compIdx].EfficiencyNumerator * inflowAvailable)
						/ model.ConstEffConvs[compIdx].EfficiencyDenominator;
					uint32_t outflowRequest = model.Flows[(size_t)outflowConn].Requested;
					assert((inflowAvailable >= outflowAvailable) && "converter forward flow; inflow must be >= outflow");
					if (outflowAvailable != model.Flows[(size_t)outflowConn].Available)
					{
						Helper_AddIfNotAdded(ss.ActiveConnectionsFront, (size_t)outflowConn);
					}
					model.Flows[(size_t)outflowConn].Available = outflowAvailable;
					int lossflowConn = FindOutflowConnection(
						model, model.Connections[connIdx].To, compIdx, 1);
					uint32_t nonOutflowAvailable =
						FinalizeFlowValue(inflowAvailable, inflowRequest)
						- FinalizeFlowValue(outflowAvailable, outflowRequest);
					uint32_t lossflowAvailable = 0;
					uint32_t lossflowRequest = 0;
					if (lossflowConn >= 0) {
						lossflowRequest = model.Flows[(size_t)lossflowConn].Requested;
						lossflowAvailable = FinalizeFlowValue(nonOutflowAvailable, lossflowRequest);
						nonOutflowAvailable -= lossflowAvailable;
						if (lossflowAvailable != model.Flows[(size_t)lossflowConn].Available)
						{
							Helper_AddIfNotAdded(ss.ActiveConnectionsFront, (size_t)lossflowConn);
						}
						model.Flows[(size_t)lossflowConn].Available = lossflowAvailable;
					}
					int wasteflowConn = FindOutflowConnection(
						model, model.Connections[connIdx].To, compIdx, 2);
					assert((wasteflowConn >= 0) && "should find a wasteflow connection; model is incorrectly connected");
					model.Flows[(size_t)wasteflowConn].Requested = nonOutflowAvailable;
					model.Flows[(size_t)wasteflowConn].Available = nonOutflowAvailable;
				} break;
				case (ComponentType::MuxType):
				{
					++numForwardPasses;
					uint32_t totalAvailable = 0;
					for (size_t muxInport = 0; muxInport < model.Muxes[compIdx].NumInports; ++muxInport) {
						int inflowConnIdx = FindInflowConnection(
							model, ComponentType::MuxType, compIdx, muxInport);
						assert(inflowConnIdx >= 0 && "must have an inflow connection for each mux inport");
						totalAvailable += model.Flows[inflowConnIdx].Available;
					}
					uint32_t totalRequest = 0;
					for (size_t muxOutport = 0; muxOutport < model.Muxes[compIdx].NumOutports; ++muxOutport) {
						int outflowConnIdx = FindOutflowConnection(
							model, ComponentType::MuxType, compIdx, muxOutport);
						assert(outflowConnIdx >= 0 && "must have an outflow connection for each mux outport");
						totalRequest += model.Flows[(size_t)outflowConnIdx].Requested;
					}
					for (size_t muxOutport = 0; muxOutport < model.Muxes[compIdx].NumOutports; ++muxOutport) {
						int outflowConnIdx = FindOutflowConnection(
							model, ComponentType::MuxType, compIdx, muxOutport);
						assert(outflowConnIdx >= 0 && "must have an outflow connection for each mux outport");
						uint32_t available = model.Flows[(size_t)outflowConnIdx].Requested >= totalAvailable
							? totalAvailable
							: model.Flows[(size_t)outflowConnIdx].Requested;
						if (muxOutport == 0 && totalRequest < totalAvailable) {
							available += totalAvailable - totalRequest;
						}
						totalAvailable -= available;
						if (model.Flows[(size_t)outflowConnIdx].Available != available)
						{
							Helper_AddIfNotAdded(ss.ActiveConnectionsFront, (size_t)outflowConnIdx);
						}
						model.Flows[(size_t)outflowConnIdx].Available = available;
					}
				} break;
				case (ComponentType::StoreType):
				{
					++numForwardPasses;
					int outflowConn =
						FindOutflowConnection(
							model, model.Connections[connIdx].To, compIdx, 0);
					assert(outflowConn >= 0 && "store must have an outflow connection");
					uint32_t available = model.Flows[connIdx].Available;
					uint32_t dischargeAvailable = model.Stores[compIdx].Stored > 0
						? model.Stores[compIdx].MaxDischargeRate
						: 0;
					available += dischargeAvailable;
					if (model.Flows[(size_t)outflowConn].Available != available)
					{
						Helper_AddIfNotAdded(ss.ActiveConnectionsFront, (size_t)outflowConn);
					}
					model.Flows[(size_t)outflowConn].Available = available;
				} break;
				default:
				{
					throw std::runtime_error{ "Unhandled component type on forward pass: " + ToString(model.Connections[connIdx].To) };
				}
			}
		}
	}

	void
	RunConnectionsPostFinalization(Model& model, SimulationState& ss, double t)
	{
		for (size_t connIdx = 0; connIdx < model.Connections.size(); ++connIdx)
		{
			size_t compIdx = model.Connections[connIdx].ToIdx;
			switch (model.Connections[connIdx].To)
			{
				case (ComponentType::ConstantEfficiencyConverterType):
				{
				} break;
				case (ComponentType::StoreType):
				{
					++numPostPasses;
					// TODO: need to also add consideration for discharging TO or BELOW chargeAmount (i.e., when you cross chargeAmount from above)
					// NOTE: we assume that the charge request never resets once at or below chargeAmount UNTIL you hit 100% SOC again...
					int outflowConn =
						FindOutflowConnection(
							model, model.Connections[connIdx].To, compIdx, 0);
					assert(outflowConn >= 0 && "store must have an outflow connection");
					int netCharge =
						(int)model.Flows[connIdx].Actual - (int)model.Flows[(size_t)outflowConn].Actual;
					if (netCharge > 0) {
						model.Stores[compIdx].TimeOfNextEvent =
							t + ((double)(model.Stores[compIdx].Capacity - model.Stores[compIdx].Stored)
								/ (double)netCharge);
					}
					else if (netCharge < 0
						&& (model.Stores[compIdx].Stored
							> model.Stores[compIdx].ChargeAmount))
					{
						model.Stores[compIdx].TimeOfNextEvent =
							t + ((double)(model.Stores[compIdx].Stored - model.Stores[compIdx].ChargeAmount)
								/ (-1.0 * (double)netCharge));
					}
					else if (netCharge < 0) {
						model.Stores[compIdx].TimeOfNextEvent =
							t + ((double)(model.Stores[compIdx].Stored) / (-1.0 * (double)netCharge));
					}
					else {
						model.Stores[compIdx].TimeOfNextEvent = infinity;
					}
				} break;
				case (ComponentType::MuxType):
				{
					++numPostPasses;
					// REQUESTS
					uint32_t totalRequest = 0;
					for (size_t muxOutPort = 0; muxOutPort < model.Muxes[compIdx].NumOutports; ++muxOutPort) {
						size_t outflowConnIdx = FindOutflowConnection(model, ComponentType::MuxType, compIdx, muxOutPort);
						totalRequest += model.Flows[outflowConnIdx].Requested;
					}
					std::vector<uint32_t> inflowRequests = {};
					inflowRequests.reserve(model.Muxes[compIdx].NumInports);
					for (size_t muxInPort = 0; muxInPort < model.Muxes[compIdx].NumInports; ++muxInPort) {
						size_t inflowConnIdx = FindInflowConnection(model, ComponentType::MuxType, compIdx, muxInPort);
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
					for (size_t muxInPort = 0; muxInPort < model.Muxes[compIdx].NumInports; ++muxInPort) {
						int inflowConnIdx = FindInflowConnection(model, ComponentType::MuxType, compIdx, muxInPort);
						assert(inflowConnIdx >= 0 && "mux must have 1 inflow connection per inport");
						if (model.Flows[(size_t)inflowConnIdx].Requested != inflowRequests[muxInPort])
						{
							Helper_AddIfNotAdded(ss.ActiveConnectionsBack, (size_t)inflowConnIdx);
						}
						model.Flows[(size_t)inflowConnIdx].Requested = inflowRequests[muxInPort];
					}
					// AVAILABLE
					uint32_t totalAvailable = 0;
					for (size_t muxInPort = 0; muxInPort < model.Muxes[compIdx].NumInports; ++muxInPort) {
						int inflowConnIdx = FindInflowConnection(model, ComponentType::MuxType, compIdx, muxInPort);
						assert(inflowConnIdx >= 0 && "mux must have 1 inflow connection per inport");
						totalAvailable += model.Flows[(size_t)inflowConnIdx].Available;
					}
					std::vector<uint32_t> outflowAvailables = {};
					outflowAvailables.reserve(model.Muxes[compIdx].NumOutports);
					for (size_t muxOutPort = 0; muxOutPort < model.Muxes[compIdx].NumInports; ++muxOutPort) {
						size_t outflowConnIdx = FindOutflowConnection(model, ComponentType::MuxType, compIdx, muxOutPort);
						uint32_t available = model.Flows[outflowConnIdx].Requested >= totalAvailable
							? totalAvailable
							: model.Flows[outflowConnIdx].Requested;
						outflowAvailables.push_back(available);
						totalAvailable -= available;
					}
					if (totalAvailable > 0) {
						outflowAvailables[0] += totalAvailable;
						totalAvailable = 0;
					}
					for (size_t muxOutPort = 0; muxOutPort < model.Muxes[compIdx].NumOutports; ++muxOutPort) {
						int outflowConnIdx = FindOutflowConnection(
							model, ComponentType::MuxType, compIdx, muxOutPort);
						assert(outflowConnIdx >= 0 && "outflow connection index");
						if (model.Flows[outflowConnIdx].Available != outflowAvailables[muxOutPort])
						{
							Helper_AddIfNotAdded(ss.ActiveConnectionsFront, (size_t)outflowConnIdx);
						}
						model.Flows[outflowConnIdx].Available = outflowAvailables[muxOutPort];
					}
				} break;
			}
		}
	}

	void
	RunActiveConnections(Model& model, SimulationState& ss, double t)
	{
		RunConnectionsBackward(model, ss);
		RunConnectionsForward(model, ss);
		FinalizeFlows(model);
		RunConnectionsPostFinalization(model, ss, t);
	}

	uint32_t
	FinalizeFlowValue(uint32_t requested, uint32_t available)
	{
		return available >= requested ? requested : available;
	}

	void
	FinalizeFlows(Model& model)
	{
		for (size_t flowIdx = 0; flowIdx < model.Flows.size(); ++flowIdx)
		{
			model.Flows[flowIdx].Actual =
				FinalizeFlowValue(
					model.Flows[flowIdx].Requested,
					model.Flows[flowIdx].Available);
		}
	}

	double
	NextEvent(ScheduleBasedLoad sb, double t)
	{
		for (size_t i = 0; i < sb.TimesAndLoads.size(); ++i)
		{
			if (sb.TimesAndLoads[i].Time > t)
			{
				return sb.TimesAndLoads[i].Time;
			}
		}
		return infinity;
	}

	double
	NextEvent(Store s, double t)
	{
		if (s.TimeOfNextEvent >= 0.0 && s.TimeOfNextEvent > t)
		{
			return s.TimeOfNextEvent;
		}
		return infinity;
	}

	void
	UpdateStoresPerElapsedTime(Model& m, double elapsedTime)
	{
		for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
		{
			long netEnergyAdded = 0;
			for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx)
			{
				if (m.Connections[connIdx].To == ComponentType::StoreType
					&& m.Connections[connIdx].ToIdx == storeIdx)
				{
					netEnergyAdded += std::lround(elapsedTime * (double)m.Flows[connIdx].Actual);
				}
				else if (m.Connections[connIdx].From == ComponentType::StoreType
					&& m.Connections[connIdx].FromIdx == storeIdx)
				{
					netEnergyAdded -= std::lround(elapsedTime * (double)m.Flows[connIdx].Actual);
				}
			}
			assert(static_cast<long>(m.Stores[storeIdx].Capacity - m.Stores[storeIdx].Stored) >= netEnergyAdded
				&& "netEnergyAdded cannot put storage over capacity");
			assert(netEnergyAdded >= static_cast<long>(- 1 * (int)m.Stores[storeIdx].Stored)
				&& "netEnergyAdded cannot use more energy than available");
			m.Stores[storeIdx].Stored += netEnergyAdded;
		}
	}

	std::string
	ToString(ComponentType compType)
	{
		std::string result = "?";
		switch (compType)
		{
			case (ComponentType::ConstantLoadType):
			{
				result = "ConstantLoad";
			} break;
			case (ComponentType::ScheduleBasedLoadType):
			{
				result = "ScheduleBasedLoad";
			} break;
			case (ComponentType::ConstantSourceType):
			{
				result = "ConstantSource";
			} break;
			case (ComponentType::ConstantEfficiencyConverterType):
			{
				result = "ConstantEfficiencyConverter";
			} break;
			case (ComponentType::WasteSinkType):
			{
				result = "WasteSink";
			} break;
			case (ComponentType::MuxType):
			{
				result = "Mux";
			} break;
			case (ComponentType::StoreType):
			{
				result = "Store";
			} break;
			default:
			{
				throw std::runtime_error{ "Unhandled component type" };
			}
		}
		return result;
	}

	void
	PrintFlows(Model& m, double t)
	{
		std::cout << "time: " << t << std::endl;
		for (size_t flowIdx = 0; flowIdx < m.Flows.size(); ++flowIdx)
		{
			std::cout << ToString(m.Connections[flowIdx].From)
				<< "[" << m.Connections[flowIdx].FromIdx << ":"
				<< m.Connections[flowIdx].FromPort << "] => "
				<< ToString(m.Connections[flowIdx].To)
				<< "[" << m.Connections[flowIdx].ToIdx << ":"
				<< m.Connections[flowIdx].ToPort << "]: "
				<< m.Flows[flowIdx].Actual
				<< " (R: " << m.Flows[flowIdx].Requested
				<< "; A: " << m.Flows[flowIdx].Available << ")"
				<< std::endl;
		}
	}

	FlowSummary
	SummarizeFlows(Model& m, double t)
	{
		FlowSummary summary = {};
		summary.Time = t;
		summary.Inflow = 0;
		summary.OutflowAchieved = 0;
		summary.OutflowRequest = 0;
		summary.StorageCharge = 0;
		summary.StorageDischarge = 0;
		summary.Wasteflow = 0;
		for (size_t flowIdx = 0; flowIdx < m.Flows.size(); ++flowIdx)
		{
			switch (m.Connections[flowIdx].From)
			{
				case (ComponentType::ConstantSourceType):
				{
					summary.Inflow += m.Flows[flowIdx].Actual;
				} break;
				case (ComponentType::StoreType):
				{
					summary.StorageDischarge += m.Flows[flowIdx].Actual;
				} break;
			}

			switch (m.Connections[flowIdx].To)
			{
				case (ComponentType::ConstantLoadType):
				case (ComponentType::ScheduleBasedLoadType):
				{
					summary.OutflowRequest += m.Flows[flowIdx].Requested;
					summary.OutflowAchieved += m.Flows[flowIdx].Actual;
				} break;
				case (ComponentType::StoreType):
				{
					summary.StorageCharge += m.Flows[flowIdx].Actual;
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
	PrintFlowSummary(FlowSummary s)
	{
		int netDischarge = (int)s.StorageDischarge - (int)s.StorageCharge;
		int sum =
			(int)s.Inflow + netDischarge
			- ((int)s.OutflowAchieved + (int)s.Wasteflow);
		double eff = ((double)s.Inflow + (double)netDischarge) > 0.0
			? 100.0 * ((double)s.OutflowAchieved)
				/ ((double)s.Inflow + (double)netDischarge)
			: 0.0;
		double effectiveness =
			100.0 * ((double)s.OutflowAchieved) / ((double)s.OutflowRequest);
		std::cout << "Flow Summary @ " << s.Time << ":" << std::endl;
		std::cout << "  Inflow                 : " << s.Inflow << std::endl;
		std::cout << "+ Storage Net Discharge  : " << netDischarge << std::endl;
		std::cout << "- Outflow (achieved)     : " << s.OutflowAchieved << std::endl;
		std::cout << "- Wasteflow              : " << s.Wasteflow << std::endl;
		std::cout << "-----------------------------------" << std::endl;
		std::cout << "= Sum                    : " << sum << std::endl;
		std::cout << "  Efficiency             : " << eff << "%"
			<< " (= " << s.OutflowAchieved << "/"
			<< ((int)s.Inflow + netDischarge) << ")" << std::endl;
		std::cout << "  Delivery Effectiveness : " << effectiveness << "%"
			<< " (= " << s.OutflowAchieved << "/" << s.OutflowRequest << ")"
			<< std::endl;
	}

	std::vector<Flow>
	CopyFlows(std::vector<Flow> flows)
	{
		std::vector<Flow> newFlows = {};
		newFlows.reserve(flows.size());
		for (int i = 0; i < flows.size(); ++i)
		{
			Flow f(flows[i]);
			newFlows.push_back(f);
		}
		return newFlows;
	}

	std::vector<uint32_t>
	CopyStorageStates(Model& m)
	{
		std::vector<uint32_t> storageAmounts = {};
		storageAmounts.reserve(m.Stores.size());
		for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
		{
			storageAmounts.push_back(m.Stores[storeIdx].Stored);
		}
		return storageAmounts;
	}

	void
	PrintModelState(Model& m)
	{
		for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
		{
			std::cout << ToString(ComponentType::StoreType)
				<< "[" << storeIdx << "].Storage : "
				<< m.Stores[storeIdx].Stored << std::endl;
			std::cout << ToString(ComponentType::StoreType)
				<< "[" << storeIdx << "].Capacity: "
				<< m.Stores[storeIdx].Capacity << std::endl;
			double soc = (double)m.Stores[storeIdx].Stored * 100.0
				/ (double)m.Stores[storeIdx].Capacity;
			std::cout << ToString(ComponentType::StoreType)
				<< "[" << storeIdx << "].SOC     : "
				<< soc << " %" << std::endl;
		}
	}

	SimulationState
	Model_SetupSimulationState(Model& model)
	{
		SimulationState ss = {};
		ss.ActiveConnectionsBack.reserve(model.Connections.size());
		ss.ActiveConnectionsFront.reserve(model.Connections.size());
		ss.ActiveConnectionsPost.reserve(model.Connections.size());
		return ss;
	}

	// TODO[mok]: add a maximum time as well
	std::vector<TimeAndFlows>
	Simulate(Model& model, bool print = true)
	{
		double t = 0.0;
		std::vector<TimeAndFlows> timeAndFlows = {};
		size_t const maxLoopIter = 100;
		SimulationState ss = Model_SetupSimulationState(model);
		// TODO: max loop should be in the while loop; however, we do want to check for max time
		for (size_t loopIdx = 0; loopIdx < maxLoopIter; ++loopIdx)
		{
			ActivateConnectionsForScheduleBasedLoads(model, ss, t);
			ActivateConnectionsForStores(model, ss, t);
			if (loopIdx == 0) {
				ActivateConnectionsForConstantLoads(model, ss);
				ActivateConnectionsForConstantSources(model, ss);
			}
			while (CountActiveConnections(ss) > 0) {
				RunActiveConnections(model, ss, t);
			}
			Debug_PrintNumberOfPasses();
			Debug_ResetNumberOfPasses();
			if (print) {
				PrintFlows(model, t);
				PrintFlowSummary(SummarizeFlows(model, t));
				PrintModelState(model);
			}
			TimeAndFlows taf = {};
			taf.Time = t;
			taf.Flows = CopyFlows(model.Flows);
			taf.StorageAmounts = CopyStorageStates(model);
			timeAndFlows.push_back(std::move(taf));
			double nextTime = EarliestNextEvent(model, t);
			if (nextTime < 0.0) {
				break;
			}
			UpdateStoresPerElapsedTime(model, nextTime - t);
			t = nextTime;
		}
		Debug_PrintNumberOfPasses(true);
		Debug_ResetNumberOfPasses(true);
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

	ComponentId
	Model_AddStore(
		Model& m,
		uint32_t capacity,
		uint32_t maxCharge,
		uint32_t maxDischarge,
		uint32_t chargeAmount,
		uint32_t initialStorage)
	{
		assert(chargeAmount < capacity && "chargeAmount must be less than capacity");
		assert(initialStorage <= capacity && "initialStorage must be less than or equal to capacity");
		size_t id = m.Stores.size();
		Store s = {};
		s.Capacity = capacity;
		s.MaxChargeRate = maxCharge;
		s.MaxDischargeRate = maxDischarge;
		s.ChargeAmount = chargeAmount;
		s.Stored = initialStorage;
		// NOTE: we schedule an event right away to register available discharging/requested charging
		s.TimeOfNextEvent = 0.0;
		m.Stores.push_back(s);
		return { id, ComponentType::StoreType };
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

	std::optional<uint32_t>
	ModelResults_GetStoreState(size_t storeId, double time, std::vector<TimeAndFlows> timeAndFlows)
	{
		// TODO: update to also be able to give storage amounts between events by looking at the
		// inflow and outflows to storage and doing the math...
		for (size_t i = 0; i < timeAndFlows.size(); ++i) {
			if (time == timeAndFlows[i].Time && storeId < timeAndFlows[i].StorageAmounts.size()) {
				return timeAndFlows[i].StorageAmounts[storeId];
			}
		}
		return {};
	}

	

}