#include "erin_next/erin_next.h"
#include <cmath>

namespace erin_next {

	constexpr bool do_debug = false;

	static unsigned int numBackwardPasses = 0;
	static unsigned int numForwardPasses = 0;
	static unsigned int numPostPasses = 0;
	static unsigned int grandTotalPasses = 0;

	void
	Debug_PrintNumberOfPasses(bool onlyGrandTotal)
	{
		if constexpr (do_debug)
		{
			if (onlyGrandTotal)
			{
				std::cout << "Grand total        : " << grandTotalPasses << std::endl;
			}
			else
			{
				std::cout << "Number of:" << std::endl;
				std::cout << "... backward passes: " << numBackwardPasses << std::endl;
				std::cout << "... forward passes : " << numForwardPasses << std::endl;
				std::cout << "... post passes    : " << numPostPasses << std::endl;
				std::cout << "... total passes   : "
					<< (numBackwardPasses + numForwardPasses + numPostPasses)
					<< std::endl;
			}
		}
	}

	void
	Debug_ResetNumberOfPasses(bool resetAll)
	{
		if constexpr (do_debug)
		{
			grandTotalPasses += numBackwardPasses + numForwardPasses + numPostPasses;
			numBackwardPasses = 0;
			numForwardPasses = 0;
			numPostPasses = 0;
			if (resetAll)
			{
				grandTotalPasses = 0;
			}
		}
	}

	size_t
	Component_AddComponentReturningId(Component& c, ComponentType ct, size_t idx)
	{
		auto id{ c.CompType.size() };
		c.CompType.push_back(ct);
		c.Idx.push_back(idx);
		return id;
	}

	size_t
	CountActiveConnections(SimulationState const& ss)
	{
		return (
			ss.ActiveConnectionsBack.size()
			+ ss.ActiveConnectionsFront.size());
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
	ActivateConnectionsForConstantLoads(Model const& model, SimulationState& ss)
	{
		for (size_t loadIdx = 0; loadIdx < model.ConstLoads.size(); ++loadIdx)
		{
			size_t connIdx = model.ConstLoads[loadIdx].InflowConn;
			if (ss.Flows[connIdx].Requested != model.ConstLoads[loadIdx].Load)
			{
				SimulationState_AddActiveConnectionBack(ss, connIdx);
			}
			ss.Flows[connIdx].Requested = model.ConstLoads[loadIdx].Load;
		}
	}

	void
	ActivateConnectionsForConstantSources(Model const& m, SimulationState& ss)
	{
		for (size_t srcIdx = 0; srcIdx < m.ConstSources.size(); ++srcIdx)
		{
			size_t connIdx = m.ConstSources[srcIdx].OutflowConn;
			if (ss.Flows[connIdx].Available != m.ConstSources[srcIdx].Available)
			{
				SimulationState_AddActiveConnectionForward(ss, connIdx);
			}
			ss.Flows[connIdx].Available = m.ConstSources[srcIdx].Available;
		}
	}

	void
	ActivateConnectionsForScheduleBasedLoads(Model const& m, SimulationState& ss, double t)
	{
		for (size_t schIdx = 0; schIdx < m.ScheduledLoads.size(); ++schIdx)
		{
			size_t connIdx = m.ScheduledLoads[schIdx].InflowConn;
			for (TimeAndLoad const& tal : m.ScheduledLoads[schIdx].TimesAndLoads)
			{
				if (tal.Time == t)
				{
					if (ss.Flows[connIdx].Requested != tal.Load)
					{
						SimulationState_AddActiveConnectionBack(ss, connIdx);
					}
					ss.Flows[connIdx].Requested = tal.Load;
					break;
				}
			}
		}
	}

	void
	ActivateConnectionsForStores(Model& m, SimulationState& ss, double t)
	{
		for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
		{
			if (ss.StorageNextEventTimes[storeIdx] == t)
			{
				size_t inflowConn = m.Stores[storeIdx].InflowConn;
				size_t outflowConn = m.Stores[storeIdx].OutflowConn;
				uint32_t available = ss.Flows[inflowConn].Available + (
					ss.StorageAmounts[storeIdx] > 0 ? m.Stores[storeIdx].MaxDischargeRate : 0);
				if (ss.Flows[outflowConn].Available != available)
				{
					SimulationState_AddActiveConnectionForward(ss, outflowConn);
				}
				ss.Flows[outflowConn].Available = available;
				uint32_t request = ss.Flows[outflowConn].Requested + (
					ss.StorageAmounts[storeIdx] <= m.Stores[storeIdx].ChargeAmount
					? m.Stores[storeIdx].MaxChargeRate
					: 0);
				if (ss.Flows[inflowConn].Requested != request)
				{
					SimulationState_AddActiveConnectionForward(ss, inflowConn);
				}
				ss.Flows[inflowConn].Requested = request;
			}
		}
	}

	double
	EarliestNextEvent(Model const& m, SimulationState const& ss, double t)
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
			double nextTimeForComponent = NextStorageEvent(ss, storeIdx, t);
			if (nextTime == infinity || (nextTimeForComponent >= 0.0 && nextTimeForComponent < nextTime))
			{
				nextTime = nextTimeForComponent;
			}
		}
		for (size_t reliabilityIdx = 0; reliabilityIdx < m.Reliabilities.size(); ++reliabilityIdx)
		{
			double nextReliabilityEvent = NextEvent(m.Reliabilities[reliabilityIdx], t);
			if (nextTime == infinity || (nextReliabilityEvent >= 0.0 && nextReliabilityEvent < nextTime))
			{
				nextTime = nextReliabilityEvent;
			}
		}
		return nextTime;
	}

	// TODO[mok]: consider changing this into a std::optional<size_t> as that would better express intent
	// TODO: consider reworking this to be an O(1) operation. Should components store connection indices?
	int
	FindInflowConnection(Model const& m, ComponentType ct, size_t compId, size_t inflowPort)
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
	FindOutflowConnection(Model const& m, ComponentType ct, size_t compId, size_t outflowPort)
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
	RunConstantEfficiencyConverterBackward(
		Model& m,
		SimulationState& ss,
		size_t connIdx,
		size_t compIdx)
	{
		if constexpr (do_debug)
		{
			++numBackwardPasses;
		}
		size_t inflowConn = m.ConstEffConvs[compIdx].InflowConn;
		uint32_t outflowRequest = ss.Flows[connIdx].Requested;
		uint32_t inflowRequest =
			(m.ConstEffConvs[compIdx].EfficiencyDenominator * outflowRequest)
			/ m.ConstEffConvs[compIdx].EfficiencyNumerator;
		assert((inflowRequest >= outflowRequest)
			&& "inflow must be >= outflow in converter");
		if (inflowRequest != ss.Flows[inflowConn].Requested)
		{
			Helper_AddIfNotAdded(ss.ActiveConnectionsBack, inflowConn);
		}
		ss.Flows[inflowConn].Requested = inflowRequest;
		uint32_t attenuatedLossflowRequest = 0;
		std::optional<size_t> lossflowConn =
			m.ConstEffConvs[compIdx].LossflowConn;
		if (lossflowConn.has_value()) {
			attenuatedLossflowRequest = FinalizeFlowValue(
				inflowRequest - outflowRequest,
				ss.Flows[lossflowConn.value()].Requested);
		}
		size_t wasteflowConn = m.ConstEffConvs[compIdx].WasteflowConn;
		uint32_t wasteflowRequest =
			inflowRequest - outflowRequest - attenuatedLossflowRequest;
		ss.Flows[wasteflowConn].Requested = wasteflowRequest;
	}

	void
	RunMuxBackward(Model& model, SimulationState& ss, size_t compIdx)
	{
		if constexpr (do_debug)
		{
			++numBackwardPasses;
		}
		uint32_t totalRequest = 0;
		for (size_t outflowConnIdx : model.Muxes[compIdx].OutflowConns)
		{
			totalRequest += ss.Flows[outflowConnIdx].Requested;
		}
		std::vector<uint32_t> inflowRequests = {};
		inflowRequests.reserve(model.Muxes[compIdx].NumInports);
		for (size_t inflowConnIdx : model.Muxes[compIdx].InflowConns)
		{
			uint32_t request = ss.Flows[inflowConnIdx].Available >= totalRequest
				? totalRequest
				: ss.Flows[inflowConnIdx].Available;
			inflowRequests.push_back(request);
			totalRequest -= request;
		}
		if (totalRequest > 0) {
			inflowRequests[0] += totalRequest;
			totalRequest = 0;
		}
		for (size_t muxInPort = 0; muxInPort < model.Muxes[compIdx].NumInports; ++muxInPort)
		{
			size_t inflowConnIdx = model.Muxes[compIdx].InflowConns[muxInPort];
			if (ss.Flows[inflowConnIdx].Requested != inflowRequests[muxInPort])
			{
				Helper_AddIfNotAdded(ss.ActiveConnectionsBack, inflowConnIdx);
			}
			ss.Flows[inflowConnIdx].Requested = inflowRequests[muxInPort];
		}
	}

	void
	RunStoreBackward(Model& model, SimulationState& ss, size_t connIdx, size_t compIdx)
	{
		if constexpr (do_debug)
		{
			++numBackwardPasses;
		}
		size_t inflowConnIdx = model.Stores[compIdx].InflowConn;
		uint32_t chargeRate =
			ss.StorageAmounts[compIdx] <= model.Stores[compIdx].ChargeAmount
			? model.Stores[compIdx].MaxChargeRate
			: 0;
		if (ss.Flows[inflowConnIdx].Requested != (ss.Flows[connIdx].Requested + chargeRate))
		{
			Helper_AddIfNotAdded(ss.ActiveConnectionsBack, inflowConnIdx);
		}
		ss.Flows[inflowConnIdx].Requested = ss.Flows[connIdx].Requested + chargeRate;
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
							RunConstantEfficiencyConverterBackward(model, ss, connIdx, compIdx);
						} break;
						case 1: // lossflow
						case 2: // wasteflow
						{
						} break;
						default:
						{
							throw std::runtime_error{ "Uhandled port number" };
						}
					}
				} break;
				case (ComponentType::MuxType):
				{
					RunMuxBackward(model, ss, compIdx);
				} break;
				case (ComponentType::StoreType):
				{
					RunStoreBackward(model, ss, connIdx, compIdx);
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
	RunConstantEfficiencyConverterForward(
		Model& m,
		SimulationState& ss,
		size_t connIdx,
		size_t compIdx)
	{
		if constexpr (do_debug)
		{
			++numForwardPasses;
		}
		uint32_t inflowAvailable = ss.Flows[connIdx].Available;
		uint32_t inflowRequest = ss.Flows[connIdx].Requested;
		size_t outflowConn = m.ConstEffConvs[compIdx].OutflowConn;
		uint32_t outflowAvailable =
			(m.ConstEffConvs[compIdx].EfficiencyNumerator * inflowAvailable)
			/ m.ConstEffConvs[compIdx].EfficiencyDenominator;
		uint32_t outflowRequest = ss.Flows[outflowConn].Requested;
		assert((inflowAvailable >= outflowAvailable) && "converter forward flow; inflow must be >= outflow");
		if (outflowAvailable != ss.Flows[outflowConn].Available)
		{
			Helper_AddIfNotAdded(ss.ActiveConnectionsFront, outflowConn);
		}
		ss.Flows[outflowConn].Available = outflowAvailable;
		std::optional<size_t> lossflowConn = m.ConstEffConvs[compIdx].LossflowConn;
		uint32_t nonOutflowAvailable =
			FinalizeFlowValue(inflowAvailable, inflowRequest)
			- FinalizeFlowValue(outflowAvailable, outflowRequest);
		uint32_t lossflowAvailable = 0;
		uint32_t lossflowRequest = 0;
		if (lossflowConn.has_value()) {
			lossflowRequest = ss.Flows[lossflowConn.value()].Requested;
			lossflowAvailable = FinalizeFlowValue(nonOutflowAvailable, lossflowRequest);
			nonOutflowAvailable -= lossflowAvailable;
			if (lossflowAvailable != ss.Flows[lossflowConn.value()].Available)
			{
				Helper_AddIfNotAdded(ss.ActiveConnectionsFront, lossflowConn.value());
			}
			ss.Flows[lossflowConn.value()].Available = lossflowAvailable;
		}
		size_t wasteflowConn = m.ConstEffConvs[compIdx].WasteflowConn;
		ss.Flows[wasteflowConn].Requested = nonOutflowAvailable;
		ss.Flows[wasteflowConn].Available = nonOutflowAvailable;
	}

	void
	RunMuxForward(Model& model, SimulationState& ss, size_t compIdx)
	{
		if constexpr (do_debug)
		{
			++numForwardPasses;
		}
		uint32_t totalAvailable = 0;
		for (size_t inflowConnIdx : model.Muxes[compIdx].InflowConns)
		{
			totalAvailable += ss.Flows[inflowConnIdx].Available;
		}
		std::vector<uint32_t> outflowAvailables = {};
		outflowAvailables.reserve(model.Muxes[compIdx].NumOutports);
		for (size_t outflowConnIdx : model.Muxes[compIdx].OutflowConns)
		{
			uint32_t available = ss.Flows[outflowConnIdx].Requested >= totalAvailable
				? totalAvailable
				: ss.Flows[outflowConnIdx].Requested;
			outflowAvailables.push_back(available);
			totalAvailable -= available;
		}
		if (totalAvailable > 0) {
			outflowAvailables[0] += totalAvailable;
			totalAvailable = 0;
		}
		for (size_t muxOutPort = 0; muxOutPort < model.Muxes[compIdx].NumOutports; ++muxOutPort) {
			size_t outflowConnIdx = model.Muxes[compIdx].OutflowConns[muxOutPort];
			if (ss.Flows[outflowConnIdx].Available != outflowAvailables[muxOutPort])
			{
				Helper_AddIfNotAdded(ss.ActiveConnectionsFront, outflowConnIdx);
			}
			ss.Flows[outflowConnIdx].Available = outflowAvailables[muxOutPort];
		}
	}

	void
	RunStoreForward(Model& model, SimulationState& ss, size_t connIdx, size_t compIdx)
	{
		if constexpr (do_debug)
		{
			++numForwardPasses;
		}
		size_t outflowConn = model.Stores[compIdx].OutflowConn;
		uint32_t available = ss.Flows[connIdx].Available;
		uint32_t dischargeAvailable = ss.StorageAmounts[compIdx] > 0
			? model.Stores[compIdx].MaxDischargeRate
			: 0;
		available += dischargeAvailable;
		if (ss.Flows[outflowConn].Available != available)
		{
			Helper_AddIfNotAdded(ss.ActiveConnectionsFront, outflowConn);
		}
		ss.Flows[outflowConn].Available = available;
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
					RunConstantEfficiencyConverterForward(model, ss, connIdx, compIdx);
				} break;
				case (ComponentType::MuxType):
				{
					RunMuxForward(model, ss, compIdx);
				} break;
				case (ComponentType::StoreType):
				{
					RunStoreForward(model, ss, connIdx, compIdx);
				} break;
				default:
				{
					throw std::runtime_error{ "Unhandled component type on forward pass: " + ToString(model.Connections[connIdx].To) };
				}
			}
		}
	}

	void
	RunStorePostFinalization(Model& model, SimulationState& ss, double t, size_t connIdx, size_t compIdx)
	{
		if constexpr (do_debug)
		{
			++numPostPasses;
		}
		// TODO: need to also add consideration for discharging TO or BELOW chargeAmount (i.e., when you cross chargeAmount from above)
		// NOTE: we assume that the charge request never resets once at or below chargeAmount UNTIL you hit 100% SOC again...
		int outflowConn =
			FindOutflowConnection(
				model, model.Connections[connIdx].To, compIdx, 0);
		assert(outflowConn >= 0 && "store must have an outflow connection");
		int netCharge =
			(int)ss.Flows[connIdx].Actual - (int)ss.Flows[(size_t)outflowConn].Actual;
		if (netCharge > 0)
		{
			ss.StorageNextEventTimes[compIdx] =
				t + ((double)(model.Stores[compIdx].Capacity - ss.StorageAmounts[compIdx])
					/ (double)netCharge);
		}
		else if (netCharge < 0 && (ss.StorageAmounts[compIdx] > model.Stores[compIdx].ChargeAmount))
		{
			ss.StorageNextEventTimes[compIdx] =
				t + ((double)(ss.StorageAmounts[compIdx] - model.Stores[compIdx].ChargeAmount)
					/ (-1.0 * (double)netCharge));
		}
		else if (netCharge < 0) {
			ss.StorageNextEventTimes[compIdx] =
				t + ((double)(ss.StorageAmounts[compIdx]) / (-1.0 * (double)netCharge));
		}
		else {
			ss.StorageNextEventTimes[compIdx] = infinity;
		}
	}

	void
	RunMuxPostFinalization(Model& model, SimulationState& ss, size_t compIdx)
	{
		RunMuxBackward(model, ss, compIdx);
		RunMuxForward(model, ss, compIdx);
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
					RunStorePostFinalization(model, ss, t, connIdx, compIdx);
				} break;
				case (ComponentType::MuxType):
				{
					RunMuxPostFinalization(model, ss, compIdx);
				} break;
			}
		}
	}

	void
	RunActiveConnections(Model& model, SimulationState& ss, double t)
	{
		RunConnectionsBackward(model, ss);
		RunConnectionsForward(model, ss);
		FinalizeFlows(ss);
		RunConnectionsPostFinalization(model, ss, t);
	}

	uint32_t
	FinalizeFlowValue(uint32_t requested, uint32_t available)
	{
		return available >= requested ? requested : available;
	}

	void
	FinalizeFlows(SimulationState& ss)
	{
		for (size_t flowIdx = 0; flowIdx < ss.Flows.size(); ++flowIdx)
		{
			ss.Flows[flowIdx].Actual =
				FinalizeFlowValue(
					ss.Flows[flowIdx].Requested,
					ss.Flows[flowIdx].Available);
		}
	}

	double
	NextEvent(ScheduleBasedLoad const& sb, double t)
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
	NextEvent(ScheduleBasedReliability const& sbr, double t)
	{
		for (size_t i = 0; i < sbr.TimeStates.size(); ++i)
		{
			if (sbr.TimeStates[i].time > t)
			{
				return sbr.TimeStates[i].time;
			}
		}
		return infinity;
	}

	double
	NextStorageEvent(SimulationState const& ss, size_t storeIdx, double t)
	{
		double storeTime = ss.StorageNextEventTimes[storeIdx];
		if (storeTime >= 0.0 && storeTime > t)
		{
			return storeTime;
		}
		return infinity;
	}

	void
	UpdateStoresPerElapsedTime(Model const& m, SimulationState& ss, double elapsedTime)
	{
		for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
		{
			long netEnergyAdded = 0;
			for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx)
			{
				if (m.Connections[connIdx].To == ComponentType::StoreType
					&& m.Connections[connIdx].ToIdx == storeIdx)
				{
					netEnergyAdded += std::lround(elapsedTime * (double)ss.Flows[connIdx].Actual);
				}
				else if (m.Connections[connIdx].From == ComponentType::StoreType
					&& m.Connections[connIdx].FromIdx == storeIdx)
				{
					netEnergyAdded -= std::lround(elapsedTime * (double)ss.Flows[connIdx].Actual);
				}
			}
			assert(static_cast<long>(m.Stores[storeIdx].Capacity - ss.StorageAmounts[storeIdx]) >= netEnergyAdded
				&& "netEnergyAdded cannot put storage over capacity");
			assert(netEnergyAdded >= static_cast<long>(- 1 * (int)ss.StorageAmounts[storeIdx])
				&& "netEnergyAdded cannot use more energy than available");
			ss.StorageAmounts[storeIdx] += netEnergyAdded;
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
	PrintFlows(Model const& m, SimulationState const& ss, double t)
	{
		std::cout << "time: " << t << std::endl;
		for (size_t flowIdx = 0; flowIdx < ss.Flows.size(); ++flowIdx)
		{
			std::cout << ToString(m.Connections[flowIdx].From)
				<< "[" << m.Connections[flowIdx].FromIdx << ":"
				<< m.Connections[flowIdx].FromPort << "] => "
				<< ToString(m.Connections[flowIdx].To)
				<< "[" << m.Connections[flowIdx].ToIdx << ":"
				<< m.Connections[flowIdx].ToPort << "]: "
				<< ss.Flows[flowIdx].Actual
				<< " (R: " << ss.Flows[flowIdx].Requested
				<< "; A: " << ss.Flows[flowIdx].Available << ")"
				<< std::endl;
		}
	}

	FlowSummary
	SummarizeFlows(Model const& m, SimulationState const& ss, double t)
	{
		FlowSummary summary = {};
		summary.Time = t;
		summary.Inflow = 0;
		summary.OutflowAchieved = 0;
		summary.OutflowRequest = 0;
		summary.StorageCharge = 0;
		summary.StorageDischarge = 0;
		summary.Wasteflow = 0;
		for (size_t flowIdx = 0; flowIdx < ss.Flows.size(); ++flowIdx)
		{
			switch (m.Connections[flowIdx].From)
			{
				case (ComponentType::ConstantSourceType):
				{
					summary.Inflow += ss.Flows[flowIdx].Actual;
				} break;
				case (ComponentType::StoreType):
				{
					summary.StorageDischarge += ss.Flows[flowIdx].Actual;
				} break;
			}

			switch (m.Connections[flowIdx].To)
			{
				case (ComponentType::ConstantLoadType):
				case (ComponentType::ScheduleBasedLoadType):
				{
					summary.OutflowRequest += ss.Flows[flowIdx].Requested;
					summary.OutflowAchieved += ss.Flows[flowIdx].Actual;
				} break;
				case (ComponentType::StoreType):
				{
					summary.StorageCharge += ss.Flows[flowIdx].Actual;
				} break;
				case (ComponentType::WasteSinkType):
				{
					summary.Wasteflow += ss.Flows[flowIdx].Actual;
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
	CopyStorageStates(SimulationState& ss)
	{
		std::vector<uint32_t> newAmounts = {};
		newAmounts.reserve(ss.StorageAmounts.size());
		for (size_t i = 0; i < ss.StorageAmounts.size(); ++i)
		{
			newAmounts.push_back(ss.StorageAmounts[i]);
		}
		return newAmounts;
	}

	void
	PrintModelState(Model& m, SimulationState& ss)
	{
		for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
		{
			std::cout << ToString(ComponentType::StoreType)
				<< "[" << storeIdx << "].InitialStorage : "
				<< m.Stores[storeIdx].InitialStorage << std::endl;
			std::cout << ToString(ComponentType::StoreType)
				<< "[" << storeIdx << "].StorageAmount: "
				<< ss.StorageAmounts[storeIdx] << std::endl;
			std::cout << ToString(ComponentType::StoreType)
				<< "[" << storeIdx << "].Capacity: "
				<< m.Stores[storeIdx].Capacity << std::endl;
			double soc = (double)ss.StorageAmounts[storeIdx] * 100.0
				/ (double)m.Stores[storeIdx].Capacity;
			std::cout << ToString(ComponentType::StoreType)
				<< "[" << storeIdx << "].SOC     : "
				<< soc << " %" << std::endl;
		}
	}

	size_t
	Model_NumberOfComponents(Model const& m)
	{
		return m.ConstEffConvs.size()
			+ m.ConstLoads.size()
			+ m.ConstSources.size()
			+ m.Muxes.size()
			+ m.ScheduledLoads.size()
			+ m.Stores.size();
	}

	void
	Model_SetupSimulationState(Model& model, SimulationState& ss)
	{
		for (size_t i = 0; i < model.Stores.size(); ++i)
		{
			ss.StorageAmounts.push_back(model.Stores[i].InitialStorage);
		}
		ss.StorageNextEventTimes = std::vector<double>(model.Stores.size(), 0.0);
		ss.Flows = std::vector<Flow>(model.Connections.size(), { 0, 0, 0 });
	}

	size_t
	Model_AddFixedReliabilityDistribution(Model& m, double dt)
	{
		std::cout << "0" << std::endl;
		return m.DistSys.add_fixed("", dt);
	}

	size_t
	Model_AddFailureModeToComponent(Model& m, size_t compId, size_t failureDistId, size_t repairDistId)
	{
		auto fmId = m.Rel.add_failure_mode("", failureDistId, repairDistId);
		auto linkId = m.Rel.link_component_with_failure_mode(compId, fmId);
		auto schedule = m.Rel.make_schedule_for_link(linkId, m.RandFn, m.DistSys, m.FinalTime);
		ScheduleBasedReliability sbr = {};
		sbr.ComponentId = compId;
		sbr.TimeStates = std::move(schedule);
		m.Reliabilities.push_back(std::move(sbr));
		return linkId;
	}

	// TODO[mok]: add a maximum time as well
	std::vector<TimeAndFlows>
	Simulate(Model& model, SimulationState& ss, bool print = true)
	{
		double t = 0.0;
		std::vector<TimeAndFlows> timeAndFlows = {};
		Model_SetupSimulationState(model, ss);
		// TODO: max loop should be in the while loop; however, we do want to check for max time
		while (t != infinity)
		{
			// schedule each event-generating component for next event
			// by adding to the ActiveComponentBack or ActiveComponentFront arrays
			// note: these two arrays could be sorted by component type for faster
			// running over loops...
			ActivateConnectionsForScheduleBasedLoads(model, ss, t);
			ActivateConnectionsForStores(model, ss, t);
			if (t == 0)
			{
				ActivateConnectionsForConstantLoads(model, ss);
				ActivateConnectionsForConstantSources(model, ss);
			}
			// TODO: add a for-loop for max iter? check both count of active connections and max loop?
			size_t const maxLoop = 10;
			for (size_t loopIter = 0; loopIter <= maxLoop; ++loopIter)
			{
				if (CountActiveConnections(ss) == 0)
				{
					break;
				}
				if (loopIter == maxLoop)
				{
					// TODO: throw an error? exit with error code?
				}
				RunActiveConnections(model, ss, t);
			}
			if constexpr (do_debug)
			{
				Debug_PrintNumberOfPasses();
				Debug_ResetNumberOfPasses();
			}
			if (print)
			{
				PrintFlows(model, ss, t);
				PrintFlowSummary(SummarizeFlows(model, ss, t));
				PrintModelState(model, ss);
			}
			TimeAndFlows taf = {};
			taf.Time = t;
			taf.Flows = CopyFlows(ss.Flows);
			taf.StorageAmounts = CopyStorageStates(ss);
			timeAndFlows.push_back(std::move(taf));
			double nextTime = EarliestNextEvent(model, ss, t);
			if (nextTime == infinity)
			{
				break;
			}
			UpdateStoresPerElapsedTime(model, ss, nextTime - t);
			t = nextTime;
		}
		if constexpr (do_debug)
		{
			Debug_PrintNumberOfPasses(true);
			Debug_ResetNumberOfPasses(true);
		}
		return timeAndFlows;
	}

	size_t
	Model_AddConstantLoad(Model& m, uint32_t load)
	{
		size_t idx = m.ConstLoads.size();
		m.ConstLoads.push_back({ load });
		return Component_AddComponentReturningId(m.ComponentMap, ComponentType::ConstantLoadType, idx);
	}

	size_t
	Model_AddScheduleBasedLoad(Model& m, double* times, uint32_t* loads, size_t numItems)
	{
		size_t idx = m.ScheduledLoads.size();
		std::vector<TimeAndLoad> timesAndLoads = {};
		timesAndLoads.reserve(numItems);
		for (size_t i = 0; i < numItems; ++i)
		{
			timesAndLoads.push_back({ times[i], loads[i] });
		}
		m.ScheduledLoads.push_back({ std::move(timesAndLoads) });
		return Component_AddComponentReturningId(m.ComponentMap, ComponentType::ScheduleBasedLoadType, idx);
	}

	size_t
	Model_AddScheduleBasedLoad(Model& m, std::vector<TimeAndLoad> timesAndLoads)
	{
		size_t idx = m.ScheduledLoads.size();
		m.ScheduledLoads.push_back({ std::vector<TimeAndLoad>(timesAndLoads) });
		return Component_AddComponentReturningId(m.ComponentMap, ComponentType::ScheduleBasedLoadType, idx);
	}

	size_t
	Model_AddConstantSource(Model& m, uint32_t available)
	{
		size_t idx = m.ConstSources.size();
		m.ConstSources.push_back({ available });
		return Component_AddComponentReturningId(m.ComponentMap, ComponentType::ConstantSourceType, idx);
	}

	size_t
	Model_AddMux(Model& m, size_t numInports, size_t numOutports)
	{
		size_t idx = m.Muxes.size();
		Mux mux = {};
		mux.NumInports = numInports;
		mux.NumOutports = numOutports;
		mux.InflowConns = std::vector<size_t>(numInports, 0);
		mux.OutflowConns = std::vector<size_t>(numOutports, 0);
		m.Muxes.push_back( std::move(mux) );
		return Component_AddComponentReturningId(m.ComponentMap, ComponentType::MuxType, idx);
	}

	size_t
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
		size_t idx = m.Stores.size();
		Store s = {};
		s.Capacity = capacity;
		s.MaxChargeRate = maxCharge;
		s.MaxDischargeRate = maxDischarge;
		s.ChargeAmount = chargeAmount;
		s.InitialStorage = initialStorage;
		m.Stores.push_back(s);
		return Component_AddComponentReturningId(m.ComponentMap, ComponentType::StoreType, idx);
	}

	ComponentIdAndWasteConnection
	Model_AddConstantEfficiencyConverter(
		Model& m,
		SimulationState& ss,
		uint32_t eff_numerator,
		uint32_t eff_denominator)
	{
		size_t idx = m.ConstEffConvs.size();
		m.ConstEffConvs.push_back({ eff_numerator, eff_denominator });
		size_t wasteId = Component_AddComponentReturningId(m.ComponentMap, ComponentType::WasteSinkType, 0);
		size_t thisId = Component_AddComponentReturningId(m.ComponentMap, ComponentType::ConstantEfficiencyConverterType, idx);
		auto wasteConn = Model_AddConnection(m, ss, thisId, 2, wasteId, 0);
		return { thisId, wasteConn };
	}

	Connection
	Model_AddConnection(
		Model& m,
		SimulationState& ss,
		size_t from,
		size_t fromPort,
		size_t to,
		size_t toPort)
	{
		ComponentType fromType = m.ComponentMap.CompType[from];
		size_t fromIdx = m.ComponentMap.Idx[from];
		ComponentType toType = m.ComponentMap.CompType[to];
		size_t toIdx = m.ComponentMap.Idx[to];
		Connection c = {
			fromType, fromIdx, fromPort,
			toType, toIdx, toPort
		};
		size_t connId = m.Connections.size();
		m.Connections.push_back(c);
		ss.Flows.push_back({ 0, 0, 0 });
		switch (fromType)
		{
			case (ComponentType::ConstantSourceType):
			{
				m.ConstSources[fromIdx].OutflowConn = connId;
			} break;
			case (ComponentType::ConstantEfficiencyConverterType):
			{
				switch (fromPort)
				{
					case (0):
					{
						m.ConstEffConvs[fromIdx].OutflowConn = connId;
					} break;
					case (1):
					{
						m.ConstEffConvs[fromIdx].LossflowConn = connId;
					} break;
					case (2):
					{
						m.ConstEffConvs[fromIdx].WasteflowConn = connId;
					} break;
					default:
					{
						throw std::invalid_argument(
							"Unhandled port id for outport of constant efficiency converter");
					}
				}
			} break;
			case (ComponentType::MuxType):
			{
				m.Muxes[fromIdx].OutflowConns[fromPort] = connId;
			} break;
			case (ComponentType::StoreType):
			{
				m.Stores[fromIdx].OutflowConn = connId;
			} break;
		}
		switch (toType)
		{
			case (ComponentType::ConstantLoadType):
			{
				m.ConstLoads[toIdx].InflowConn = connId;
			} break;
			case (ComponentType::ConstantEfficiencyConverterType):
			{
				m.ConstEffConvs[toIdx].InflowConn = connId;
			} break;
			case (ComponentType::MuxType):
			{
				m.Muxes[toIdx].InflowConns[toPort] = connId;
			} break;
			case (ComponentType::StoreType):
			{
				m.Stores[toIdx].InflowConn = connId;
			} break;
			case (ComponentType::ScheduleBasedLoadType):
			{
				m.ScheduledLoads[toIdx].InflowConn = connId;
			} break;
		}
		return c;
	}

	bool
	SameConnection(Connection a, Connection b)
	{
		return a.From == b.From && a.FromIdx == b.FromIdx && a.FromPort == b.FromPort
			&& a.To == b.To && a.ToIdx == b.ToIdx && a.ToPort == b.ToPort;
	}

	std::optional<Flow>
	ModelResults_GetFlowForConnection(Model const& m, Connection conn, double time, std::vector<TimeAndFlows> timeAndFlows)
	{
		for (size_t connId = 0; connId < m.Connections.size(); ++connId)
		{
			if (SameConnection(m.Connections[connId], conn))
			{
				Flow f = {};
				for (size_t timeAndFlowIdx = 0; timeAndFlowIdx < timeAndFlows.size(); ++timeAndFlowIdx)
				{
					if (time >= timeAndFlows[timeAndFlowIdx].Time)
					{
						f = timeAndFlows[timeAndFlowIdx].Flows[connId];
					}
					else
					{
						break;
					}
				}
				return f;
			}
		}
		return {};
	}

	std::optional<uint32_t>
	ModelResults_GetStoreState(Model const& m, size_t compId, double time, std::vector<TimeAndFlows> timeAndFlows)
	{
		if (compId >= m.ComponentMap.CompType.size() || m.ComponentMap.CompType[compId] != ComponentType::StoreType)
		{
			return {};
		}
		size_t storeIdx = m.ComponentMap.Idx[compId];
		// TODO: update to also be able to give storage amounts between events by looking at the
		// inflow and outflows to storage and doing the math...
		for (size_t i = 0; i < timeAndFlows.size(); ++i)
		{
			if (time == timeAndFlows[i].Time && storeIdx < timeAndFlows[i].StorageAmounts.size())
			{
				return timeAndFlows[i].StorageAmounts[storeIdx];
			}
		}
		return {};
	}

}