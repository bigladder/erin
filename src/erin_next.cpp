#include "erin_next/erin_next.h"
#include <cmath>
#include <cstdlib>

namespace erin_next
{

	size_t
	Component_AddComponentReturningId(
		ComponentDict& c,
		ComponentType ct,
		size_t idx)
	{
		return Component_AddComponentReturningId(
			c, ct, idx, std::vector<size_t>(), std::vector<size_t>(), "");
	}

	size_t
	Component_AddComponentReturningId(
		ComponentDict& c,
		ComponentType ct,
		size_t idx,
		std::vector<size_t> inflowType,
		std::vector<size_t> outflowType,
		std::string const& tag)
	{
		auto id{ c.CompType.size() };
		c.CompType.push_back(ct);
		c.Idx.push_back(idx);
		c.Tag.push_back(tag);
		c.InflowType.push_back(inflowType);
		c.OutflowType.push_back(outflowType);
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

	void
	SimulationState_AddActiveConnectionBack(SimulationState& ss, size_t connIdx)
	{
		ss.ActiveConnectionsBack.insert(connIdx);
	}

	void
	SimulationState_AddActiveConnectionForward(
		SimulationState& ss,
		size_t connIdx)
	{
		ss.ActiveConnectionsFront.insert(connIdx);
	}

	void
	ActivateConnectionsForConstantLoads(Model const& model, SimulationState& ss)
	{
		for (size_t loadIdx = 0; loadIdx < model.ConstLoads.size(); ++loadIdx)
		{
			size_t connIdx = model.ConstLoads[loadIdx].InflowConn;
			if (ss.Flows[connIdx].Requested != model.ConstLoads[loadIdx].Load)
			{
				ss.ActiveConnectionsBack.insert(connIdx);
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
				ss.ActiveConnectionsFront.insert(connIdx);
			}
			ss.Flows[connIdx].Available = m.ConstSources[srcIdx].Available;
		}
	}

	void
	ActivateConnectionsForScheduleBasedLoads(
		Model const& m,
		SimulationState& ss,
		double t)
	{
		for (size_t i = 0; i < m.ScheduledLoads.size(); ++i)
		{
			size_t connIdx = m.ScheduledLoads[i].InflowConn;
			size_t idx = ss.ScheduleBasedLoadIdx[i];
			if (idx < m.ScheduledLoads[i].TimesAndLoads.size())
			{
				auto const& tal =
					m.ScheduledLoads[i].TimesAndLoads[idx];
				if (tal.Time == t)
				{

					if (ss.Flows[connIdx].Requested != tal.Amount)
					{
						ss.ActiveConnectionsBack.insert(connIdx);
					}
					ss.Flows[connIdx].Requested = tal.Amount;
				}
			}
		}
	}

	void
	ActivateConnectionsForScheduleBasedSources(
		Model const& m,
		SimulationState& ss,
		double t)
	{
		for (size_t i = 0; i < m.ScheduledSrcs.size(); ++i)
		{
			auto idx = ss.ScheduleBasedSourceIdx[i];
			if (idx < m.ScheduledSrcs[i].TimeAndAvails.size())
			{
				auto const& taa =
					m.ScheduledSrcs[i].TimeAndAvails[idx];
				if (taa.Time == t)
				{
					auto outIdx = m.ScheduledSrcs[i].OutflowConn;
					if (ss.Flows[outIdx].Available != taa.Amount)
					{
						ss.ActiveConnectionsFront.insert(outIdx);
					}
					ss.Flows[outIdx].Available = taa.Amount;
					auto spillage =
						ss.Flows[outIdx].Available > ss.Flows[outIdx].Requested
						? (
							ss.Flows[outIdx].Available
							- ss.Flows[outIdx].Requested)
						: 0;
					auto wasteIdx = m.ScheduledSrcs[i].WasteflowConn;
					ss.Flows[wasteIdx].Requested = spillage;
					ss.Flows[wasteIdx].Available = spillage;
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
					ss.StorageAmounts[storeIdx] > 0
					? m.Stores[storeIdx].MaxDischargeRate
					: 0);
				if (ss.Flows[outflowConn].Available != available)
				{
					ss.ActiveConnectionsFront.insert(outflowConn);
				}
				ss.Flows[outflowConn].Available = available;
				uint32_t request = ss.Flows[outflowConn].Requested + (
					ss.StorageAmounts[storeIdx]
						<= m.Stores[storeIdx].ChargeAmount
					? m.Stores[storeIdx].MaxChargeRate
					: 0);
				if (ss.Flows[inflowConn].Requested != request)
				{
					ss.ActiveConnectionsFront.insert(inflowConn);
				}
				ss.Flows[inflowConn].Requested = request;
			}
		}
	}

	void
	ActivateConnectionsForReliability(
		Model& m,
		SimulationState& ss,
		double time)
	{
		for (auto const& rel : m.Reliabilities)
		{
			for (auto const& ts : rel.TimeStates)
			{
				if (ts.time == time)
				{
					// reset the associated component's flows depending on
					// the component's reliability state
					if (ts.state)
					{
						// restore the component to active state
						Model_SetComponentToRepaired(m, ss, rel.ComponentId);
					}
					else
					{
						// set component as down for repairs
						Model_SetComponentToFailed(m, ss, rel.ComponentId);
					}
				}
				else if (ts.time > time)
				{
					break;
				}
			}
		}
	}

	double
	GetNextTime(double nextTime, size_t count, std::function<double(size_t)> f)
	{
		for (size_t i = 0; i < count; ++i)
		{
			double nextTimeForComponent = f(i);
			if (nextTime == infinity
				|| (nextTimeForComponent >= 0.0
					&& nextTimeForComponent < nextTime))
			{
				nextTime = nextTimeForComponent;
			}
		}
		return nextTime;
	}

	double
	EarliestNextEvent(Model const& m, SimulationState const& ss, double t)
	{
		double next = infinity;
		next = GetNextTime(next, m.ScheduledLoads.size(),
			[&](size_t i) -> double {
				return NextEvent(m.ScheduledLoads[i], i, ss); });
		next = GetNextTime(next, m.ScheduledSrcs.size(),
			[&](size_t i) -> double {
				return NextEvent(m.ScheduledSrcs[i], i, ss); });
		next = GetNextTime(next, m.Stores.size(),
			[&](size_t i) -> double {
				return NextStorageEvent(ss, i, t); });
		next = GetNextTime(next, m.Reliabilities.size(),
			[&](size_t i) -> double {
				return NextEvent(m.Reliabilities[i], t); });
		return next;
	}

	std::optional<size_t>
	FindOutflowConnection(
		Model const& m,
		ComponentType ct,
		size_t compId,
		size_t outflowPort)
	{
		for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx)
		{
			if (m.Connections[connIdx].From == ct
				&& m.Connections[connIdx].FromIdx == compId
				&& m.Connections[connIdx].FromPort == outflowPort)
			{
				return connIdx;
			}
		}
		return {};
	}

	void
	UpdateConstantEfficiencyLossflowAndWasteflow(
		Model const& m,
		SimulationState& ss,
		size_t compIdx)
	{
		uint32_t inflowRequest =
			ss.Flows[m.ConstEffConvs[compIdx].InflowConn].Requested;
		uint32_t inflowAvailable =
			ss.Flows[m.ConstEffConvs[compIdx].InflowConn].Available;
		uint32_t outflowRequest =
			ss.Flows[m.ConstEffConvs[compIdx].OutflowConn].Requested;
		uint32_t outflowAvailable =
			ss.Flows[m.ConstEffConvs[compIdx].OutflowConn].Available;
		std::optional<size_t> lossflowConn =
			m.ConstEffConvs[compIdx].LossflowConn;
		uint32_t inflow = FinalizeFlowValue(inflowRequest, inflowAvailable);
		uint32_t outflow = FinalizeFlowValue(outflowRequest, outflowAvailable);
		// NOTE: for COP, we can have outflow > inflow, but we assume no
		// lossflow in that scenario
		// TODO: should COP components be separate components? A COP component
		// with a lossflow hookup would be odd... wasteflow doesn't make sense
		// either... I think the answer is "yes"; we can re-add the check for
		// inflow >= outflow...
		uint32_t nonOutflowAvailable =
			inflow > outflow
			? inflow - outflow
			: 0;
		uint32_t lossflowRequest = 0;
		if (lossflowConn.has_value()) {
			lossflowRequest = ss.Flows[lossflowConn.value()].Requested;
			if (nonOutflowAvailable != ss.Flows[lossflowConn.value()].Available)
			{
				ss.ActiveConnectionsFront.insert(lossflowConn.value());
			}
			ss.Flows[lossflowConn.value()].Available = nonOutflowAvailable;
		}
		size_t wasteflowConn = m.ConstEffConvs[compIdx].WasteflowConn;
		uint32_t wasteflow =
			nonOutflowAvailable > lossflowRequest
			? nonOutflowAvailable - lossflowRequest
			: 0;
		ss.Flows[wasteflowConn].Requested = wasteflow;
		ss.Flows[wasteflowConn].Available = wasteflow;
	}

	void
	RunConstantEfficiencyConverterBackward(
		Model const& m,
		SimulationState& ss,
		size_t connIdx,
		size_t compIdx)
	{
		size_t inflowConn = m.ConstEffConvs[compIdx].InflowConn;
		uint32_t outflowRequest = ss.Flows[connIdx].Requested;
		uint32_t inflowRequest =
			static_cast<uint32_t>(
				std::ceil(
					outflowRequest / m.ConstEffConvs[compIdx].Efficiency));
		// NOTE: for COP, we can have inflow < outflow
		if (inflowRequest != ss.Flows[inflowConn].Requested)
		{
			ss.ActiveConnectionsBack.insert(inflowConn);
		}
		ss.Flows[inflowConn].Requested = inflowRequest;
		UpdateConstantEfficiencyLossflowAndWasteflow(m, ss, compIdx);
	}

	void
	Mux_RequestInflowsIntelligently(
		SimulationState& ss,
		std::vector<size_t> const& inflowConns,
		uint32_t remainingRequest)
	{
		for (size_t inflowConn : inflowConns)
		{
			if (ss.Flows[inflowConn].Requested != remainingRequest)
			{
				ss.ActiveConnectionsBack.insert(inflowConn);
			}
			ss.Flows[inflowConn].Requested = remainingRequest;
			remainingRequest =
				remainingRequest > ss.Flows[inflowConn].Available
				? remainingRequest - ss.Flows[inflowConn].Available
				: 0;
		}
	}

	void
	RunMuxBackward(Model& model, SimulationState& ss, size_t compIdx)
	{
		uint32_t totalRequest = 0;
		for (size_t outflowConnIdx : model.Muxes[compIdx].OutflowConns)
		{
			totalRequest += ss.Flows[outflowConnIdx].Requested;
		}
		Mux_RequestInflowsIntelligently(
			ss, model.Muxes[compIdx].InflowConns, totalRequest);
	}

	void
	RunStoreBackward(
		Model& model,
		SimulationState& ss,
		size_t connIdx,
		size_t compIdx)
	{
		size_t inflowConnIdx = model.Stores[compIdx].InflowConn;
		uint32_t chargeRate =
			ss.StorageAmounts[compIdx] <= model.Stores[compIdx].ChargeAmount
			? model.Stores[compIdx].MaxChargeRate
			: 0;
		if (ss.Flows[inflowConnIdx].Requested !=
			(ss.Flows[connIdx].Requested + chargeRate))
		{
			ss.ActiveConnectionsBack.insert(inflowConnIdx);
		}
		ss.Flows[inflowConnIdx].Requested =
			ss.Flows[connIdx].Requested + chargeRate;
	}

	void
	RunScheduleBasedSourceBackward(
		Model& model,
		SimulationState& ss,
		size_t connIdx,
		size_t compIdx)
	{
		auto outConn = model.ScheduledSrcs[compIdx].OutflowConn;
		assert(outConn == connIdx);
		auto wasteConn = model.ScheduledSrcs[compIdx].WasteflowConn;
		auto schIdx = ss.ScheduleBasedSourceIdx[compIdx];
		auto available =
			model.ScheduledSrcs[compIdx].TimeAndAvails[schIdx].Amount;
		auto spillage =
			available > ss.Flows[outConn].Requested
			? available - ss.Flows[outConn].Requested
			: 0;
		if (ss.Flows[outConn].Available != available)
		{
			ss.ActiveConnectionsFront.insert(outConn);
		}
		ss.Flows[outConn].Available = available;
		ss.Flows[wasteConn].Available = spillage;
		ss.Flows[wasteConn].Requested = spillage;
	}

	void
	RunConnectionsBackward(Model& model, SimulationState& ss)
	{
		while (!ss.ActiveConnectionsBack.empty())
		{
			auto temp = std::vector<size_t>(
				ss.ActiveConnectionsBack.begin(),
				ss.ActiveConnectionsBack.end());
			ss.ActiveConnectionsBack.clear();
			for (auto it = temp.cbegin(); it != temp.cend(); ++it)
			{
				size_t connIdx = *it;
				size_t compIdx = model.Connections[connIdx].FromIdx;
				switch (model.Connections[connIdx].From)
				{
					case (ComponentType::ConstantSourceType):
					{
					} break;
					case (ComponentType::ScheduleBasedSourceType):
					{
						RunScheduleBasedSourceBackward(
							model, ss, connIdx, compIdx);
					} break;
					case (ComponentType::ConstantEfficiencyConverterType):
					{
						switch (model.Connections[connIdx].FromPort)
						{
							case 0:
							{
								RunConstantEfficiencyConverterBackward(
									model, ss, connIdx, compIdx);
							} break;
							case 1: // lossflow
							case 2: // wasteflow
							{
								UpdateConstantEfficiencyLossflowAndWasteflow(
									model, ss, compIdx);
							} break;
							default:
							{
								throw std::runtime_error{ "Uhandled port" };
							}
						}
					} break;
					case (ComponentType::MuxType):
					{
						RunMuxBackward(model, ss, compIdx);
						if (model.Muxes[compIdx].NumOutports > 1)
						{
							// NOTE: possibly re-allocate downstream available
							RunMuxForward(model, ss, compIdx);
						}
					} break;
					case (ComponentType::StoreType):
					{
						RunStoreBackward(model, ss, connIdx, compIdx);
					} break;
					default:
					{
						std::cout
							<< "Unhandled component type on backward pass: "
							<< ToString(model.Connections[connIdx].From)
							<< std::endl;
					}
				}
			}
		}
	}

	void
	RunConstantEfficiencyConverterForward(
		Model const& m,
		SimulationState& ss,
		size_t connIdx,
		size_t compIdx)
	{
		uint32_t inflowAvailable = ss.Flows[connIdx].Available;
		size_t outflowConn = m.ConstEffConvs[compIdx].OutflowConn;
		uint32_t outflowAvailable =
			static_cast<uint32_t>(
				std::floor(
					m.ConstEffConvs[compIdx].Efficiency * inflowAvailable));
		// NOTE: disabling to accommodate COP components for now
		// TODO: create a separate COP component (like converter but no lossport
		// or wasteport)
		//assert(
		//	(inflowAvailable >= outflowAvailable)
		//	&& "converter forward flow; inflow must be >= outflow");
		if (outflowAvailable != ss.Flows[outflowConn].Available)
		{
			ss.ActiveConnectionsFront.insert(outflowConn);
		}
		ss.Flows[outflowConn].Available = outflowAvailable;
		UpdateConstantEfficiencyLossflowAndWasteflow(m, ss, compIdx);
	}

	void
	RunMuxForward(Model& model, SimulationState& ss, size_t compIdx)
	{
		uint32_t totalAvailable = 0;
		for (size_t inflowConnIdx : model.Muxes[compIdx].InflowConns)
		{
			totalAvailable += ss.Flows[inflowConnIdx].Available;
		}
		std::vector<uint32_t> outflowAvailables = {};
		outflowAvailables.reserve(model.Muxes[compIdx].NumOutports);
		for (size_t outflowConnIdx : model.Muxes[compIdx].OutflowConns)
		{
			uint32_t available =
				ss.Flows[outflowConnIdx].Requested >= totalAvailable
				? totalAvailable
				: ss.Flows[outflowConnIdx].Requested;
			outflowAvailables.push_back(available);
			totalAvailable -= available;
		}
		if (totalAvailable > 0) {
			outflowAvailables[0] += totalAvailable;
			totalAvailable = 0;
		}
		for (size_t muxOutPort = 0;
			muxOutPort < model.Muxes[compIdx].NumOutports;
			++muxOutPort)
		{
			size_t outflowConnIdx =
				model.Muxes[compIdx].OutflowConns[muxOutPort];
			if (ss.Flows[outflowConnIdx].Available
				!= outflowAvailables[muxOutPort])
			{
				ss.ActiveConnectionsFront.insert(outflowConnIdx);
			}
			ss.Flows[outflowConnIdx].Available = outflowAvailables[muxOutPort];
		}
	}

	void
	RunStoreForward(
		Model& model,
		SimulationState& ss,
		size_t connIdx,
		size_t compIdx)
	{
		size_t outflowConn = model.Stores[compIdx].OutflowConn;
		uint32_t available = ss.Flows[connIdx].Available;
		uint32_t dischargeAvailable = ss.StorageAmounts[compIdx] > 0
			? model.Stores[compIdx].MaxDischargeRate
			: 0;
		available += dischargeAvailable;
		if (ss.Flows[outflowConn].Available != available)
		{
			ss.ActiveConnectionsFront.insert(outflowConn);
		}
		ss.Flows[outflowConn].Available = available;
	}

	void
	RunConnectionsForward(Model& model, SimulationState& ss)
	{
		while (!ss.ActiveConnectionsFront.empty())
		{
			auto temp = std::vector<size_t>(
				ss.ActiveConnectionsFront.begin(),
				ss.ActiveConnectionsFront.end());
			ss.ActiveConnectionsFront.clear();
			for (auto it = temp.cbegin(); it != temp.cend(); ++it)
			{
				size_t connIdx = *it;
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
						RunConstantEfficiencyConverterForward(
							model, ss, connIdx, compIdx);
					} break;
					case (ComponentType::MuxType):
					{
						RunMuxForward(model, ss, compIdx);
						if (model.Muxes[compIdx].NumInports > 1)
						{
							// NOTE: possibly re-allocate upstream requests
							RunMuxBackward(model, ss, compIdx);
						}
					} break;
					case (ComponentType::StoreType):
					{
						RunStoreForward(model, ss, connIdx, compIdx);
					} break;
					default:
					{
						throw std::runtime_error{
							"Unhandled component type on forward pass: "
							+ ToString(model.Connections[connIdx].To) };
					}
				}
			}
		}
	}

	void
	RunStorePostFinalization(
		Model& model,
		SimulationState& ss,
		double t,
		size_t connIdx,
		size_t compIdx)
	{
		// NOTE: we assume that the charge request never resets once at or
		// below chargeAmount UNTIL you hit 100% SOC again...
		std::optional<size_t> outflowConn =
			FindOutflowConnection(
				model, model.Connections[connIdx].To, compIdx, 0);
		assert(outflowConn.has_value() && "store must have an outflow connection");
		int netCharge =
			(int)ss.Flows[connIdx].Actual
			- (int)ss.Flows[outflowConn.value()].Actual;
		if (netCharge > 0)
		{
			ss.StorageNextEventTimes[compIdx] =
				t + ((double)(
					model.Stores[compIdx].Capacity - ss.StorageAmounts[compIdx]
				) / (double)netCharge);
		}
		else if (netCharge < 0
			&& (ss.StorageAmounts[compIdx]
				> model.Stores[compIdx].ChargeAmount))
		{
			ss.StorageNextEventTimes[compIdx] =
				t + ((double)(
					ss.StorageAmounts[compIdx]
					- model.Stores[compIdx].ChargeAmount
				) / (-1.0 * (double)netCharge));
		}
		else if (netCharge < 0) {
			ss.StorageNextEventTimes[compIdx] =
				t + ((double)(ss.StorageAmounts[compIdx])
					/ (-1.0 * (double)netCharge));
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
	NextEvent(
		ScheduleBasedLoad const& sb,
		size_t sbIdx,
		SimulationState const& ss)
	{
		auto nextIdx = ss.ScheduleBasedLoadIdx[sbIdx] + 1;
		if (nextIdx >= sb.TimesAndLoads.size())
		{
			return infinity;
		}
		return sb.TimesAndLoads[nextIdx].Time;
	}

	double
	NextEvent(
		ScheduleBasedSource const& sb,
		size_t sbIdx,
		SimulationState const& ss)
	{
		auto nextIdx = ss.ScheduleBasedSourceIdx[sbIdx] + 1;
		if (nextIdx >= sb.TimeAndAvails.size())
		{
			return infinity;
		}
		return sb.TimeAndAvails[nextIdx].Time;
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

	// TODO: need to account for whether this component is up/down
	void
	UpdateStoresPerElapsedTime(
		Model const& m,
		SimulationState& ss,
		double elapsedTime)
	{
		for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
		{
			long netEnergyAdded = 0;
			for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx)
			{
				if (m.Connections[connIdx].To == ComponentType::StoreType
					&& m.Connections[connIdx].ToIdx == storeIdx)
				{
					netEnergyAdded += std::lround(
						elapsedTime * (double)ss.Flows[connIdx].Actual);
				}
				else if (m.Connections[connIdx].From == ComponentType::StoreType
					&& m.Connections[connIdx].FromIdx == storeIdx)
				{
					netEnergyAdded -=
						std::lround(
							elapsedTime * (double)ss.Flows[connIdx].Actual);
				}
			}
			assert(
				static_cast<long>(
					m.Stores[storeIdx].Capacity - ss.StorageAmounts[storeIdx])
				>= netEnergyAdded
				&& "netEnergyAdded cannot put storage over capacity");
			assert(netEnergyAdded
				>= static_cast<long>(- 1 * (int)ss.StorageAmounts[storeIdx])
				&& "netEnergyAdded cannot use more energy than available");
			ss.StorageAmounts[storeIdx] += netEnergyAdded;
		}
	}

	void
	UpdateScheduleBasedLoadNextEvent(
		Model const& m,
		SimulationState& ss,
		double time)
	{
		for (size_t i = 0; i < m.ScheduledLoads.size(); ++i)
		{
			size_t nextIdx = ss.ScheduleBasedLoadIdx[i] + 1;
			if (nextIdx < m.ScheduledLoads[i].TimesAndLoads.size()
				&& m.ScheduledLoads[i].TimesAndLoads[nextIdx].Time == time)
			{
				ss.ScheduleBasedLoadIdx[i] = nextIdx;
			}
		}
	}

	void
	UpdateScheduleBasedSourceNextEvent(
		Model const& m,
		SimulationState& ss,
		double time)
	{
		for (size_t i = 0; i < m.ScheduledSrcs.size(); ++i)
		{
			size_t nextIdx = ss.ScheduleBasedSourceIdx[i] + 1;
			if (nextIdx < m.ScheduledSrcs[i].TimeAndAvails.size()
				&& m.ScheduledSrcs[i].TimeAndAvails[nextIdx].Time == time)
			{
				ss.ScheduleBasedSourceIdx[i] = nextIdx;
			}
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
			case (ComponentType::ScheduleBasedSourceType):
			{
				result = "ScheduleBasedSource";
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

	std::optional<ComponentType>
	TagToComponentType(std::string const& tag)
	{
		if (tag == "ConstantLoad")
		{
			return ComponentType::ConstantLoadType;
		}
		if (tag == "ScheduleBasedLoad" || tag == "load")
		{
			return ComponentType::ScheduleBasedLoadType;
		}
		if (tag == "ConstantSource" || tag == "source")
		{
			return ComponentType::ConstantSourceType;
		}
		if (tag == "ScheduleBasedSource")
		{
			return ComponentType::ScheduleBasedSourceType;
		}
		if (tag == "ConstantEfficiencyConverter")
		{
			return ComponentType::ConstantEfficiencyConverterType;
		}
		if (tag == "WasteSink")
		{
			return ComponentType::WasteSinkType;
		}
		if (tag == "Mux" || tag == "mux" || tag == "muxer")
		{
			return ComponentType::MuxType;
		}
		if (tag == "Store")
		{
			return ComponentType::StoreType;
		}
		return {};
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
				case (ComponentType::ScheduleBasedSourceType):
				{
					// NOTE: for schedule-based sources, the thinking is that
					// the "available" is actually flowing into the system and,
					// if not used (i.e., ullage/spillage), it goes to wasteflow
					if (m.Connections[flowIdx].FromPort == 0)
					{
						summary.Inflow += ss.Flows[flowIdx].Available;
					}
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
			s.OutflowRequest > 0
			? 100.0 * ((double)s.OutflowAchieved) / ((double)s.OutflowRequest)
			: 0.0;
		std::cout << "Flow Summary @ " << s.Time << ":" << std::endl;
		std::cout << "  Inflow                 : " << s.Inflow << std::endl;
		std::cout << "+ Storage Net Discharge  : " << netDischarge << std::endl;
		std::cout << "- Outflow (achieved)     : "
			<< s.OutflowAchieved << std::endl;
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
		ss.StorageNextEventTimes =
			std::vector<double>(model.Stores.size(), 0.0);
		ss.Flows = std::vector<Flow>(model.Connections.size(), { 0, 0, 0 });
		ss.ScheduleBasedLoadIdx =
			std::vector<size_t>(model.ScheduledLoads.size(), 0);
		ss.ScheduleBasedSourceIdx =
			std::vector<size_t>(model.ScheduledSrcs.size(), 0);
		ss.Flows.clear();
		ss.Flows.reserve(model.Connections.size());
		for (size_t i = 0; i < model.Connections.size(); ++i)
		{
			ss.Flows.push_back(Flow{});
		}
	}

	size_t
	Model_AddFixedReliabilityDistribution(Model& m, double dt)
	{
		return m.DistSys.add_fixed("", dt);
	}

	size_t
	Model_AddFailureModeToComponent(
		Model& m,
		size_t compId,
		size_t failureDistId,
		size_t repairDistId)
	{
		auto fmId = m.Rel.add_failure_mode("", failureDistId, repairDistId);
		auto linkId = m.Rel.link_component_with_failure_mode(compId, fmId);
		auto schedule =
			m.Rel.make_schedule_for_link(
				linkId, m.RandFn, m.DistSys, m.FinalTime);
		ScheduleBasedReliability sbr = {};
		sbr.ComponentId = compId;
		sbr.TimeStates = std::move(schedule);
		m.Reliabilities.push_back(std::move(sbr));
		return linkId;
	}

	std::vector<TimeAndFlows>
	Simulate(Model& model, bool print = true)
	{
		double t = 0.0;
		std::vector<TimeAndFlows> timeAndFlows{};
		SimulationState ss{};
		Model_SetupSimulationState(model, ss);
		while (t != infinity && t <= model.FinalTime)
		{
			// schedule each event-generating component for next event
			// by adding to the ActiveComponentBack or ActiveComponentFront
			// arrays
			// note: these two arrays could be sorted by component type for
			// faster running over loops...
			ActivateConnectionsForScheduleBasedLoads(model, ss, t);
			ActivateConnectionsForScheduleBasedSources(model, ss, t);
			ActivateConnectionsForStores(model, ss, t);
			// TODO: remove this if statement after we add a delay capability to
			// constant loads and constant sources
			if (t == 0)
			{
				ActivateConnectionsForConstantLoads(model, ss);
				ActivateConnectionsForConstantSources(model, ss);
			}
			ActivateConnectionsForReliability(model, ss, t);
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
			if (t == model.FinalTime)
			{
				break;
			}
			double nextTime = EarliestNextEvent(model, ss, t);
			if ((nextTime == infinity && t < model.FinalTime)
				|| (nextTime > model.FinalTime))
			{
				nextTime = model.FinalTime;
			}
			UpdateStoresPerElapsedTime(model, ss, nextTime - t);
			UpdateScheduleBasedLoadNextEvent(model, ss, nextTime);
			UpdateScheduleBasedSourceNextEvent(model, ss, nextTime);
			t = nextTime;
		}
		return timeAndFlows;
	}

	void
	Model_SetComponentToRepaired(
		Model const& m,
		SimulationState& ss,
		size_t compId)
	{
		if (compId >= m.ComponentMap.CompType.size())
		{
			throw std::runtime_error("invalid component id");
		}
		auto idx = m.ComponentMap.Idx[compId];
		switch (m.ComponentMap.CompType[compId])
		{
			case (ComponentType::ConstantLoadType):
			{
			} break;
			case (ComponentType::ScheduleBasedLoadType):
			{
			} break;
			case (ComponentType::ConstantSourceType):
			{
			} break;
			case (ComponentType::ConstantEfficiencyConverterType):
			{
				auto outflowConn = m.ConstEffConvs[idx].OutflowConn;
				RunConstantEfficiencyConverterBackward(m, ss, outflowConn, idx);
				auto inflowConn = m.ConstEffConvs[idx].InflowConn;
				RunConstantEfficiencyConverterForward(m, ss, inflowConn, idx);
			} break;
			case (ComponentType::MuxType):
			{
			} break;
			case (ComponentType::StoreType):
			{
			} break;
			case (ComponentType::WasteSinkType):
			{
			} break;
			default:
			{
				throw std::runtime_error("unhandled component type");
			}
		}
	}

	void
	Model_SetComponentToFailed(
		Model const& m,
		SimulationState& ss,
		size_t compId)
	{
		if (compId >= m.ComponentMap.CompType.size())
		{
			throw std::runtime_error("invalid component id");
		}
		auto idx = m.ComponentMap.Idx[compId];
		switch (m.ComponentMap.CompType[compId])
		{
			case (ComponentType::ConstantLoadType):
			{
			} break;
			case (ComponentType::ScheduleBasedLoadType):
			{
			} break;
			case (ComponentType::ConstantSourceType):
			{
			} break;
			case (ComponentType::ConstantEfficiencyConverterType):
			{
				auto inflowConn = m.ConstEffConvs[idx].InflowConn;
				if (ss.Flows[inflowConn].Requested != 0)
				{
					ss.ActiveConnectionsBack.insert(inflowConn);
				}
				ss.Flows[inflowConn].Requested = 0;
				auto outflowConn = m.ConstEffConvs[idx].OutflowConn;
				if (ss.Flows[outflowConn].Available != 0)
				{
					ss.ActiveConnectionsFront.insert(outflowConn);
				}
				ss.Flows[outflowConn].Available = 0;
				auto lossflowConn = m.ConstEffConvs[idx].LossflowConn;
				if (lossflowConn.has_value())
				{
					auto lossConn = lossflowConn.value();
					if (ss.Flows[lossConn].Available != 0)
					{
						ss.ActiveConnectionsFront.insert(lossConn);
					}
					ss.Flows[lossConn].Available = 0;
				}
				auto wasteConn = m.ConstEffConvs[idx].WasteflowConn;
				ss.Flows[wasteConn].Available = 0;
				ss.Flows[wasteConn].Requested = 0;
			} break;
			case (ComponentType::MuxType):
			{
			} break;
			case (ComponentType::StoreType):
			{
			} break;
			case (ComponentType::WasteSinkType):
			{
			} break;
			default:
			{
				throw std::runtime_error("unhandled component type");
			} break;
		}
	}

	size_t
	Model_AddConstantLoad(Model& m, uint32_t load)
	{
		size_t idx = m.ConstLoads.size();
		m.ConstLoads.push_back({ load });
		return Component_AddComponentReturningId(
			m.ComponentMap, ComponentType::ConstantLoadType, idx,
			std::vector<size_t>{ 0 }, std::vector<size_t>{}, "");
	}

	size_t
	Model_AddScheduleBasedLoad(
		Model& m,
		double* times,
		uint32_t* loads,
		size_t numItems)
	{
		std::vector<TimeAndAmount> timesAndLoads = {};
		timesAndLoads.reserve(numItems);
		for (size_t i = 0; i < numItems; ++i)
		{
			TimeAndAmount tal{times[i], loads[i]};
			timesAndLoads.push_back(std::move(tal));
		}
		return Model_AddScheduleBasedLoad(m, timesAndLoads);
	}

	size_t
	Model_AddScheduleBasedLoad(
		Model& m,
		std::vector<TimeAndAmount> timesAndLoads)
	{
		return Model_AddScheduleBasedLoad(
			m, timesAndLoads, std::map<size_t, size_t>{});
	}

	size_t
	Model_AddScheduleBasedLoad(
		Model& m,
		std::vector<TimeAndAmount> timesAndLoads,
		std::map<size_t, size_t> scenarioIdToLoadId)
	{
		return Model_AddScheduleBasedLoad(
			m, timesAndLoads, scenarioIdToLoadId, 0, "");
	}

	size_t
	Model_AddScheduleBasedLoad(
		Model& m,
		std::vector<TimeAndAmount> timesAndLoads,
		std::map<size_t, size_t> scenarioIdToLoadId,
		size_t inflowTypeId,
		std::string const& tag)
	{
		size_t idx = m.ScheduledLoads.size();
		ScheduleBasedLoad sbl = {};
		sbl.TimesAndLoads = timesAndLoads;
		sbl.InflowConn = 0;
		sbl.ScenarioIdToLoadId = scenarioIdToLoadId;
		m.ScheduledLoads.push_back(std::move(sbl));
		return Component_AddComponentReturningId(
			m.ComponentMap, ComponentType::ScheduleBasedLoadType, idx,
			std::vector<size_t>{ inflowTypeId },
			std::vector<size_t>{},
			tag);
	}

	size_t
	Model_AddConstantSource(Model& m, uint32_t available)
	{
		return Model_AddConstantSource(m, available, 0, "");
	}

	size_t
	Model_AddConstantSource(
		Model& m,
		uint32_t available,
		size_t outflowTypeId,
		std::string const& tag)
	{
		size_t idx = m.ConstSources.size();
		m.ConstSources.push_back({ available });
		return Component_AddComponentReturningId(
			m.ComponentMap, ComponentType::ConstantSourceType, idx,
			std::vector<size_t>{},
			std::vector<size_t>{ outflowTypeId },
			tag);
	}

	ComponentIdAndWasteConnection
	Model_AddScheduleBasedSource(
		Model& m,
		std::vector<TimeAndAmount> xs)
	{
		auto idx = m.ScheduledSrcs.size();
		ScheduleBasedSource sbs = {};
		sbs.TimeAndAvails = xs;
		m.ScheduledSrcs.push_back(sbs);
		size_t wasteId = Component_AddComponentReturningId(
			m.ComponentMap, ComponentType::WasteSinkType, 0);
		size_t thisId = Component_AddComponentReturningId(
			m.ComponentMap,
			ComponentType::ScheduleBasedSourceType,
			idx);
		auto wasteConn = Model_AddConnection(m, thisId, 1, wasteId, 0);
		return { thisId, wasteConn };
	}

	size_t
	Model_AddMux(Model& m, size_t numInports, size_t numOutports)
	{
		return Model_AddMux(m, numInports, numOutports, 0, "");
	}

	size_t
	Model_AddMux(
		Model& m,
		size_t numInports,
		size_t numOutports,
		size_t flowId,
		std::string const& tag)
	{
		size_t idx = m.Muxes.size();
		Mux mux = {};
		mux.NumInports = numInports;
		mux.NumOutports = numOutports;
		mux.InflowConns = std::vector<size_t>(numInports, 0);
		mux.OutflowConns = std::vector<size_t>(numOutports, 0);
		m.Muxes.push_back(std::move(mux));
		std::vector<size_t> inflowTypes(numInports, flowId);
		std::vector<size_t> outflowTypes(numOutports, flowId);
		return Component_AddComponentReturningId(
			m.ComponentMap, ComponentType::MuxType, idx,
			inflowTypes, outflowTypes, tag);
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
		assert(chargeAmount < capacity
			&& "chargeAmount must be less than capacity");
		assert(initialStorage <= capacity
			&& "initialStorage must be less than or equal to capacity");
		size_t idx = m.Stores.size();
		Store s = {};
		s.Capacity = capacity;
		s.MaxChargeRate = maxCharge;
		s.MaxDischargeRate = maxDischarge;
		s.ChargeAmount = chargeAmount;
		s.InitialStorage = initialStorage;
		m.Stores.push_back(s);
		return Component_AddComponentReturningId(
			m.ComponentMap, ComponentType::StoreType, idx);
	}

	ComponentIdAndWasteConnection
	Model_AddConstantEfficiencyConverter(
		Model& m,
		uint32_t eff_numerator,
		uint32_t eff_denominator)
	{
		return Model_AddConstantEfficiencyConverter(
			m, (double)eff_numerator / (double)eff_denominator);
	}

	ComponentIdAndWasteConnection
	Model_AddConstantEfficiencyConverter(
		Model& m,
		double efficiency)
	{
		size_t idx = m.ConstEffConvs.size();
		m.ConstEffConvs.push_back({ efficiency });
		size_t wasteId = Component_AddComponentReturningId(
			m.ComponentMap, ComponentType::WasteSinkType, 0);
		size_t thisId = Component_AddComponentReturningId(
			m.ComponentMap,
			ComponentType::ConstantEfficiencyConverterType,
			idx);
		auto wasteConn = Model_AddConnection(m, thisId, 2, wasteId, 0);
		return { thisId, wasteConn };
	}

	Connection
	Model_AddConnection(
		Model& m,
		size_t from,
		size_t fromPort,
		size_t to,
		size_t toPort)
	{
		// TODO: add flow type to add connection and check
		size_t flowTypeId = 0;
		ComponentType fromType = m.ComponentMap.CompType[from];
		size_t fromIdx = m.ComponentMap.Idx[from];
		ComponentType toType = m.ComponentMap.CompType[to];
		size_t toIdx = m.ComponentMap.Idx[to];
		Connection c = {
			fromType, fromIdx, fromPort, from,
			toType, toIdx, toPort, to, flowTypeId
		};
		size_t connId = m.Connections.size();
		m.Connections.push_back(c);
		switch (fromType)
		{
			case (ComponentType::ConstantSourceType):
			{
				m.ConstSources[fromIdx].OutflowConn = connId;
			} break;
			case (ComponentType::ScheduleBasedSourceType):
			{
				switch (fromPort)
				{
					case (0):
					{
						m.ScheduledSrcs[fromIdx].OutflowConn = connId;
					} break;
					case (1):
					{
						m.ScheduledSrcs[fromIdx].WasteflowConn = connId;
					} break;
					default:
					{
						throw std::invalid_argument{
							"invalid outport for schedule-based source"
						};
					} break;
				}
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
							"Unhandled constant efficiency converter outport");
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
			default:
			{
				throw new std::runtime_error{ "unhandled component type" };
			}
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
		return a.From == b.From && a.FromIdx == b.FromIdx
			&& a.FromPort == b.FromPort
			&& a.To == b.To && a.ToIdx == b.ToIdx && a.ToPort == b.ToPort;
	}

	std::optional<Flow>
	ModelResults_GetFlowForConnection(
		Model const& m,
		Connection conn,
		double time,
		std::vector<TimeAndFlows> timeAndFlows)
	{
		for (size_t connId = 0; connId < m.Connections.size(); ++connId)
		{
			if (SameConnection(m.Connections[connId], conn))
			{
				Flow f = {};
				for (size_t i = 0; i < timeAndFlows.size(); ++i)
				{
					if (time >= timeAndFlows[i].Time)
					{
						f = timeAndFlows[i].Flows[connId];
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
	ModelResults_GetStoreState(
		Model const& m,
		size_t compId,
		double time,
		std::vector<TimeAndFlows> timeAndFlows)
	{
		if (compId >= m.ComponentMap.CompType.size()
			|| m.ComponentMap.CompType[compId] != ComponentType::StoreType)
		{
			return {};
		}
		size_t storeIdx = m.ComponentMap.Idx[compId];
		// TODO: update to also be able to give storage amounts between events
		// by looking at the inflow and outflows to storage and doing the
		// math...
		for (size_t i = 0; i < timeAndFlows.size(); ++i)
		{
			if (time == timeAndFlows[i].Time
				&& storeIdx < timeAndFlows[i].StorageAmounts.size())
			{
				return timeAndFlows[i].StorageAmounts[storeIdx];
			}
		}
		return {};
	}

	ScenarioOccurrenceStats
	ModelResults_CalculateScenarioOccurrenceStats(
		size_t scenarioId,
		size_t occurrenceNumber,
		Model const& m,
		std::vector<TimeAndFlows> const& timeAndFlows)
	{
		ScenarioOccurrenceStats sos{};
		sos.Id = scenarioId;
		sos.OccurrenceNumber = occurrenceNumber;
		double lastTime = timeAndFlows.size() > 0 ? timeAndFlows[0].Time : 0.0;
		bool wasDown = false;
		double sedt = 0.0;
		for (size_t eventIdx = 1; eventIdx < timeAndFlows.size(); ++eventIdx)
		{
			double dt = timeAndFlows[eventIdx].Time - lastTime;
			// TODO: in Simulation, ensure we ALWAYS have an event at final time
			// in order that scenario duration equals what we have here; this
			// is a good check.
			sos.Duration_s += dt;
			lastTime = timeAndFlows[eventIdx].Time;
			bool allLoadsMet = true;
			for (size_t connId = 0;
				connId < timeAndFlows[eventIdx-1].Flows.size();
				++connId)
			{
				auto fromType =
					m.ComponentMap.CompType[m.Connections[connId].FromId];
				auto toType =
					m.ComponentMap.CompType[m.Connections[connId].ToId];
				auto const& flow = timeAndFlows[eventIdx - 1].Flows[connId];
				switch (fromType)
				{
					case (ComponentType::ConstantSourceType):
					case (ComponentType::ScheduleBasedSourceType):
					{
						sos.Inflow_kJ += flow.Actual * dt;
					} break;
				}
				switch (toType)
				{
					case (ComponentType::ConstantLoadType):
					case (ComponentType::ScheduleBasedLoadType):
					{
						sos.OutflowAchieved_kJ += flow.Actual * dt;
						sos.OutflowRequest_kJ += flow.Requested * dt;
						allLoadsMet =
							allLoadsMet && (flow.Actual == flow.Requested);
						if (flow.Actual != flow.Requested)
						{
							sos.LoadNotServed_kJ +=
								(flow.Requested - flow.Actual) * dt;
						}
					} break;
					case (ComponentType::WasteSinkType):
					{
						sos.Wasteflow_kJ +=
							timeAndFlows[eventIdx-1].Flows[connId].Actual
							* dt;
					} break;
				}
			}
			if (allLoadsMet)
			{
				sos.Uptime_s += dt;
				if (wasDown && sedt > sos.MaxSEDT_s)
				{
					sos.MaxSEDT_s = sedt;
				}
				sedt = 0.0;
				wasDown = false;
			}
			else
			{
				sos.Downtime_s += dt;
				if (wasDown)
				{
					sedt += dt;
				}
				else
				{
					sedt = dt;
				}
				wasDown = true;
			}
			for (size_t storeIdx = 0;
				storeIdx < timeAndFlows[eventIdx].StorageAmounts.size();
				++storeIdx)
			{
				double stored =
					static_cast<double>(timeAndFlows[eventIdx]
						.StorageAmounts[storeIdx])
					- static_cast<double>(timeAndFlows[eventIdx]
						.StorageAmounts[storeIdx - 1]);
				if (stored > 0)
				{
					sos.StorageCharge_kJ += stored;
				}
				else
				{
					sos.StorageDischarge_kJ += -1.0 * stored;
				}
			}
		}
		if (sedt > sos.MaxSEDT_s)
		{
			sos.MaxSEDT_s = sedt;
		}
		return sos;
	}

	std::optional<TagAndPort>
	ParseTagAndPort(std::string const& s, std::string const& tableName)
	{
		std::string tag = s.substr(0, s.find(":"));
		size_t opening = s.find("(");
		size_t closing = s.find(")");
		if (opening == std::string::npos
			|| closing == std::string::npos)
		{
			std::cout << "[" << tableName << "] "
				<< "unable to parse connection string '"
				<< s << "'" << std::endl;
			return {};
		}
		size_t count = closing - (opening + 1);
		std::string port = s.substr(opening + 1, count);
		TagAndPort tap = {};
		tap.Tag = tag;
		tap.Port = std::atoi(port.c_str());
		return tap;
	}

	Result
	ParseNetwork(FlowDict const& fd, Model& m, toml::table const& table)
	{
		if (!table.contains("connections"))
		{
			std::cout << "[network] "
				<< "required key 'connections' missing"
				<< std::endl;
			return Result::Failure;
		}
		if (!table.at("connections").is_array())
		{
			std::cout << "[network] 'connections' is not an array" << std::endl;
			return Result::Failure;
		}
		toml::array connArray = table.at("connections").as_array();
		for (size_t i = 0; i < connArray.size(); ++i)
		{
			toml::value const& item = connArray[i];
			if (!item.is_array())
			{
				std::cout << "[network] "
					<< "'connections' at index " << i
					<< " must be an array" << std::endl;
				return Result::Failure;
			}
			if (item.as_array().size() < 3)
			{
				std::cout << "[network] "
					<< "'connections' at index " << i
					<< " must be an array of length >= 3" << std::endl;
				return Result::Failure;
			}
			for (int idx = 0; idx < 3; ++idx)
			{
				if (!item.as_array()[idx].is_string())
				{
					std::cout << "[network] "
						<< "'connections' at index " << i
						<< " and subindex " << idx
						<< " must be a string" << std::endl;
					return Result::Failure;
				}
			}
			std::string from = item.as_array()[0].as_string();
			std::optional<TagAndPort> maybeFromTap =
				ParseTagAndPort(from, "network");
			if (!maybeFromTap.has_value())
			{
				std::cout << "[network] "
					<< "unable to parse connection string at ["
					<< i << "][0]" << std::endl;
				return Result::Failure;
			}
			TagAndPort fromTap = maybeFromTap.value();
			std::string to = item.as_array()[1].as_string();
			std::optional<TagAndPort> maybeToTap =
				ParseTagAndPort(to, "network");
			if (!maybeToTap.has_value())
			{
				std::cout << "[network] "
					<< "unable to parse connection string at ["
					<< i << "][1]" << std::endl;
				return Result::Failure;
			}
			TagAndPort toTap = maybeToTap.value();
			std::string flow = item.as_array()[2].as_string();
			std::optional<size_t> maybeFlowTypeId =
				FlowDict_GetIdByTag(fd, flow);
			if (!maybeFlowTypeId.has_value())
			{
				std::cout << "[network] "
					<< "could not identify flow type '"
					<< flow << "'" << std::endl;
				return Result::Failure;
			}
			size_t flowTypeId = maybeFlowTypeId.value();
			std::optional<size_t> maybeFromCompId =
				Model_FindCompIdByTag(m, fromTap.Tag);
			if (!maybeFromCompId.has_value())
			{
				std::cout << "[network] "
					<< "could not find component id for tag '"
					<< from << "'" << std::endl;
				return Result::Failure;
			}
			std::optional<size_t> maybeToCompId =
				Model_FindCompIdByTag(m, toTap.Tag);
			if (!maybeToCompId.has_value())
			{
				std::cout << "[network] "
					<< "could not find component id for tag '"
					<< to << "'" << std::endl;
				return Result::Failure;
			}
			size_t fromCompId = maybeFromCompId.value();
			size_t toCompId = maybeToCompId.value();
			if (fromTap.Port >= m.ComponentMap.OutflowType[fromCompId].size())
			{
				std::cout << "[network] "
					<< "port is unaddressable for "
					<< ToString(m.ComponentMap.CompType[fromCompId])
					<< ": trying to address " << fromTap.Port
					<< " but only "
					<< m.ComponentMap.OutflowType[fromCompId].size()
					<< " ports available" << std::endl;
				return Result::Failure;
			}
			if (m.ComponentMap.OutflowType[fromCompId][fromTap.Port]
				!= flowTypeId)
			{
				std::cout << "[network] "
					<< "mismatch of flow types: "
					<< fromTap.Tag << ":outflow="
					<< fd.Type[m.ComponentMap.OutflowType[fromCompId][fromTap.Port]]
					<< "; connection: "
					<< flow << std::endl;
				return Result::Failure;
			}
			if (toTap.Port >= m.ComponentMap.InflowType[toCompId].size())
			{
				std::cout << "[network] "
					<< "port is unaddressable for "
					<< ToString(m.ComponentMap.CompType[toCompId])
					<< ": trying to address " << toTap.Port
					<< " but only "
					<< m.ComponentMap.InflowType[toCompId].size()
					<< " ports available" << std::endl;
				return Result::Failure;
			}
			if (m.ComponentMap.InflowType[toCompId][toTap.Port]
				!= flowTypeId)
			{
				std::cout << "[network] "
					<< "mismatch of flow types: "
					<< toTap.Tag << ":inflow="
					<< fd.Type[m.ComponentMap.OutflowType[toCompId][toTap.Port]]
					<< "; connection: "
					<< flow << std::endl;
				return Result::Failure;
			}
			// TODO: use Model_AddConnection(m,);
			Connection c = {};
			c.From = m.ComponentMap.CompType[fromCompId];
			c.FromIdx = m.ComponentMap.Idx[fromCompId];
			c.FromPort = fromTap.Port;
			c.FromId = fromCompId;
			c.To = m.ComponentMap.CompType[toCompId];
			c.ToIdx = m.ComponentMap.Idx[toCompId];
			c.ToPort = toTap.Port;
			c.ToId = toCompId;
			c.FlowTypeId = flowTypeId;
			m.Connections.push_back(c);
		}
		return Result::Success;
	}

	std::optional<size_t>
	Model_FindCompIdByTag(Model const& m, std::string const& tag)
	{
		for (size_t i=0; i < m.ComponentMap.Tag.size(); ++i)
		{
			if (m.ComponentMap.Tag[i] == tag)
			{
				return i;
			}
		}
		return {};
	}

	std::optional<size_t>
	FlowDict_GetIdByTag(FlowDict const& fd, std::string const& tag)
	{
		for (int idx=0; idx < fd.Type.size(); ++idx)
		{
			if (fd.Type[idx] == tag)
			{
				return idx;
			}
		}
		return {};
	}

	void
	Model_PrintConnections(Model const& m, FlowDict const& ft)
	{
		for (int i=0; i < m.Connections.size(); ++i)
		{
			std::cout << i << ": "
				<< ToString(m.Connections[i].From)
				<< "[" << m.Connections[i].FromId << "]:OUT("
				<< m.Connections[i].FromPort << ") -- "
				<< m.ComponentMap.Tag[m.Connections[i].FromId]
				<< " => "
				<< ToString(m.Connections[i].To)
				<< "[" << m.Connections[i].ToId << "]:IN("
				<< m.Connections[i].ToPort << ") -- "
				<< m.ComponentMap.Tag[m.Connections[i].ToId]
				<< ", flow: "
				<< ft.Type[m.Connections[i].FlowTypeId]
				<< std::endl;
		}
	}

	std::ostream&
	operator<<(std::ostream& os, Flow const& flow)
	{
		os << "Flow{Requested=" << flow.Requested << ", "
			<< "Available=" << flow.Available << ", "
			<< "Actual=" << flow.Actual << "}";
		return os;
	}

}
