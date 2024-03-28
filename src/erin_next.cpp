/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_time_and_amount.h"
#include "erin_next/erin_next_utils.h"
#include <cmath>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace erin
{
    // TODO[mok]: need to rethink this. This adds a branch with an add.
    // Probably a horrible performance issue. Use double but convert to
    // unsigned int when finalize flows?
    inline flow_t
    UtilSafeAdd(flow_t a, flow_t b)
    {
        return (b > (max_flow_W - a)) ? max_flow_W : a + b;
    }

    std::vector<TimeAndAmount>
    ConvertToTimeAndAmounts(
        std::vector<std::vector<double>> const& input,
        double timeToSeconds,
        double rateToWatts
    )
    {
        std::vector<TimeAndAmount> result;
        result.reserve(input.size());
        for (auto const& xs : input)
        {
            assert(xs.size() >= 2);
            TimeAndAmount taa{
                .Time_s = xs[0] * timeToSeconds,
                .Amount_W = static_cast<flow_t>(xs[1] * rateToWatts),
            };
            result.push_back(std::move(taa));
        }
        return result;
    }

    std::optional<FragilityCurveType>
    TagToFragilityCurveType(std::string const& tag)
    {
        if (tag == "linear")
        {
            return FragilityCurveType::Linear;
        }
        if (tag == "tabular")
        {
            return FragilityCurveType::Tabular;
        }
        return {};
    }

    std::string
    FragilityCurveTypeToTag(FragilityCurveType fctype)
    {
        std::string tag;
        switch (fctype)
        {
            case (FragilityCurveType::Linear):
            {
                tag = "linear";
            }
            break;
            case (FragilityCurveType::Tabular):
            {
                tag = "tabular";
            }
            break;
            default:
            {
                std::cerr << "unhandled fragility curve type" << std::endl;
                std::exit(1);
            }
            break;
        }
        return tag;
    }

    std::optional<size_t>
    GetIntensityIdByTag(IntensityDict intenseDict, std::string const& tag)
    {
        for (size_t i = 0; i < intenseDict.Tags.size(); ++i)
        {
            if (intenseDict.Tags[i] == tag)
            {
                return i;
            }
        }
        return {};
    }

    size_t
    Component_AddComponentReturningId(
        ComponentDict& c,
        ComponentType ct,
        size_t idx
    )
    {
        return Component_AddComponentReturningId(
            c, ct, idx, std::vector<size_t>(), std::vector<size_t>(), "", 0.0
        );
    }

    size_t
    Component_AddComponentReturningId(
        ComponentDict& c,
        ComponentType ct,
        size_t idx,
        std::vector<size_t> inflowType,
        std::vector<size_t> outflowType,
        std::string const& tag,
        double initialAge_s
    )
    {
        size_t id = c.CompType.size();
        c.CompType.push_back(ct);
        c.Idx.push_back(idx);
        c.Tag.push_back(tag);
        c.InitialAges_s.push_back(initialAge_s);
        c.InflowType.push_back(inflowType);
        c.OutflowType.push_back(outflowType);
        return id;
    }

    size_t
    CountActiveConnections(SimulationState const& ss)
    {
        return (
            ss.ActiveConnectionsBack.size() + ss.ActiveConnectionsFront.size()
        );
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
        size_t connIdx
    )
    {
        ss.ActiveConnectionsFront.insert(connIdx);
    }

    void
    ActivateConnectionsForConstantLoads(Model const& model, SimulationState& ss)
    {
        for (size_t loadIdx = 0; loadIdx < model.ConstLoads.size(); ++loadIdx)
        {
            size_t connIdx = model.ConstLoads[loadIdx].InflowConn;
            if (ss.Flows[connIdx].Requested_W
                != model.ConstLoads[loadIdx].Load_W)
            {
                ss.ActiveConnectionsBack.insert(connIdx);
            }
            ss.Flows[connIdx].Requested_W = model.ConstLoads[loadIdx].Load_W;
        }
    }

    void
    ActivateConnectionsForConstantSources(Model const& m, SimulationState& ss)
    {
        for (size_t srcIdx = 0; srcIdx < m.ConstSources.size(); ++srcIdx)
        {
            size_t connIdx = m.ConstSources[srcIdx].OutflowConn;
            size_t compId = m.Connections[connIdx].FromId;
            if (ss.UnavailableComponents.contains(compId))
            {
                if (ss.Flows[connIdx].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(connIdx);
                }
                ss.Flows[connIdx].Available_W = 0;
                continue;
            }
            if (ss.Flows[connIdx].Available_W
                != m.ConstSources[srcIdx].Available_W)
            {
                ss.ActiveConnectionsFront.insert(connIdx);
            }
            ss.Flows[connIdx].Available_W = m.ConstSources[srcIdx].Available_W;
        }
    }

    void
    ActivateConnectionsForScheduleBasedLoads(
        Model const& m,
        SimulationState& ss,
        double t
    )
    {
        for (size_t i = 0; i < m.ScheduledLoads.size(); ++i)
        {
            size_t connIdx = m.ScheduledLoads[i].InflowConn;
            size_t idx = ss.ScheduleBasedLoadIdx[i];
            if (idx < m.ScheduledLoads[i].TimesAndLoads.size())
            {
                auto const& tal = m.ScheduledLoads[i].TimesAndLoads[idx];
                if (tal.Time_s == t)
                {

                    if (ss.Flows[connIdx].Requested_W != tal.Amount_W)
                    {
                        ss.ActiveConnectionsBack.insert(connIdx);
                    }
                    ss.Flows[connIdx].Requested_W = tal.Amount_W;
                }
            }
        }
    }

    void
    ActivateConnectionsForScheduleBasedSources(
        Model const& m,
        SimulationState& ss,
        double t
    )
    {
        for (size_t i = 0; i < m.ScheduledSrcs.size(); ++i)
        {
            ScheduleBasedSource const& sbs = m.ScheduledSrcs[i];
            auto outIdx = sbs.OutflowConn;
            size_t compId = m.Connections[outIdx].FromId;
            if (ss.UnavailableComponents.contains(compId))
            {
                if (ss.Flows[outIdx].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(outIdx);
                }
                ss.Flows[outIdx].Available_W = 0;
                continue;
            }
            auto idx = ss.ScheduleBasedSourceIdx[i];
            if (idx < sbs.TimeAndAvails.size())
            {
                auto const& taa = sbs.TimeAndAvails[idx];
                if (taa.Time_s == t)
                {
                    flow_t outAvail_W = taa.Amount_W > sbs.MaxOutflow_W
                        ? sbs.MaxOutflow_W
                        : taa.Amount_W;
                    if (ss.Flows[outIdx].Available_W != outAvail_W)
                    {
                        ss.ActiveConnectionsFront.insert(outIdx);
                    }
                    ss.Flows[outIdx].Available_W = outAvail_W;
                    auto spillage = outAvail_W > ss.Flows[outIdx].Requested_W
                        ? (outAvail_W - ss.Flows[outIdx].Requested_W)
                        : 0;
                    auto wasteIdx = m.ScheduledSrcs[i].WasteflowConn;
                    ss.Flows[wasteIdx].Requested_W = spillage;
                    ss.Flows[wasteIdx].Available_W = spillage;
                }
            }
        }
    }

    void
    ActivateConnectionsForStores(Model& m, SimulationState& ss, double t)
    {
        for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
        {
            Store const& store = m.Stores[storeIdx];
            size_t inflowConn = store.InflowConn;
            size_t compId = m.Connections[inflowConn].ToId;
            size_t outflowConn = store.OutflowConn;
            if (ss.UnavailableComponents.contains(compId))
            {
                if (ss.Flows[inflowConn].Requested_W != 0)
                {
                    ss.ActiveConnectionsBack.insert(inflowConn);
                }
                ss.Flows[inflowConn].Requested_W = 0;
                if (ss.Flows[outflowConn].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = 0;
                continue;
            }
            if (ss.StorageNextEventTimes[storeIdx] == t)
            {
                flow_t available = ss.Flows[inflowConn].Available_W
                    + (ss.StorageAmounts_J[storeIdx] > 0
                           ? store.MaxDischargeRate_W
                           : 0);
                if (ss.Flows[outflowConn].Available_W != available)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = available;
                flow_t request =
                    (ss.Flows[outflowConn].Requested_W > store.MaxOutflow_W
                         ? store.MaxOutflow_W
                         : ss.Flows[outflowConn].Requested_W)
                    + (ss.StorageAmounts_J[storeIdx] <= store.ChargeAmount_J
                           ? store.MaxChargeRate_W
                           : 0);
                if (ss.Flows[inflowConn].Requested_W != request)
                {
                    ss.ActiveConnectionsFront.insert(inflowConn);
                }
                ss.Flows[inflowConn].Requested_W = request;
                continue;
            }
            // TODO: add case for store soc < charge amount
            //       and has an inflow connection
            //       and not requesting charge yet.
        }
    }

    void
    ActivateConnectionsForReliability(
        Model& m,
        SimulationState& ss,
        double time
    )
    {
        for (auto const& rel : m.Reliabilities)
        {
            for (auto const& ts : rel.TimeStates)
            {
                if (ts.time == time)
                {
                    if (ts.state)
                    {
                        Model_SetComponentToRepaired(m, ss, rel.ComponentId);
                        std::cout << "... REPAIRED: "
                                  << m.ComponentMap.Tag[rel.ComponentId] << "["
                                  << rel.ComponentId << "]" << std::endl;
                    }
                    else
                    {
                        Model_SetComponentToFailed(m, ss, rel.ComponentId);
                        std::cout << "... FAILED: "
                                  << m.ComponentMap.Tag[rel.ComponentId] << "["
                                  << rel.ComponentId << "]" << std::endl;
                        std::cout << "... causes: " << std::endl;
                        for (auto const& fragCause : ts.fragilityModeCauses)
                        {
                            std::cout << "... ... fragility mode: " << fragCause
                                      << std::endl;
                        }
                        for (auto const& failCause : ts.failureModeCauses)
                        {
                            std::cout << "... ... failure mode: " << failCause
                                      << std::endl;
                        }
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
        next = GetNextTime(
            next,
            m.ScheduledLoads.size(),
            [&](size_t i) -> double
            { return NextEvent(m.ScheduledLoads[i], i, ss); }
        );
        next = GetNextTime(
            next,
            m.ScheduledSrcs.size(),
            [&](size_t i) -> double
            { return NextEvent(m.ScheduledSrcs[i], i, ss); }
        );
        next = GetNextTime(
            next,
            m.Stores.size(),
            [&](size_t i) -> double { return NextStorageEvent(ss, i, t); }
        );
        next = GetNextTime(
            next,
            m.Reliabilities.size(),
            [&](size_t i) -> double { return NextEvent(m.Reliabilities[i], t); }
        );
        return next;
    }

    std::optional<size_t>
    FindOutflowConnection(
        Model const& m,
        ComponentType ct,
        size_t compId,
        size_t outflowPort
    )
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
        size_t compIdx
    )
    {
        ConstantEfficiencyConverter const& cec = m.ConstEffConvs[compIdx];
        flow_t inflowRequest = ss.Flows[cec.InflowConn].Requested_W;
        flow_t inflowAvailable = ss.Flows[cec.InflowConn].Available_W;
        flow_t outflowRequest = ss.Flows[cec.OutflowConn].Requested_W;
        flow_t outflowAvailable =
            ss.Flows[cec.OutflowConn].Available_W > cec.MaxOutflow_W
            ? cec.MaxOutflow_W
            : ss.Flows[cec.OutflowConn].Available_W;
        std::optional<size_t> lossflowConn = cec.LossflowConn;
        flow_t inflow = FinalizeFlowValue(inflowRequest, inflowAvailable);
        flow_t outflow = FinalizeFlowValue(outflowRequest, outflowAvailable);
        // NOTE: for COP, we can have outflow > inflow, but we assume no
        // lossflow in that scenario
        flow_t nonOutflowAvailable = inflow > outflow ? inflow - outflow : 0;
        flow_t lossflowRequest = 0;
        if (lossflowConn.has_value())
        {
            lossflowRequest = ss.Flows[lossflowConn.value()].Requested_W;
            if (lossflowRequest > cec.MaxLossflow_W)
            {
                lossflowRequest = cec.MaxLossflow_W;
            }
            flow_t nonOutflowAvailableLimited =
                nonOutflowAvailable > cec.MaxLossflow_W ? cec.MaxLossflow_W
                                                        : nonOutflowAvailable;
            if (nonOutflowAvailableLimited
                != ss.Flows[lossflowConn.value()].Available_W)
            {
                ss.ActiveConnectionsFront.insert(lossflowConn.value());
            }
            ss.Flows[lossflowConn.value()].Available_W =
                nonOutflowAvailableLimited;
        }
        size_t wasteflowConn = m.ConstEffConvs[compIdx].WasteflowConn;
        uint32_t wasteflow = nonOutflowAvailable > lossflowRequest
            ? nonOutflowAvailable - lossflowRequest
            : 0;
        ss.Flows[wasteflowConn].Requested_W = wasteflow;
        ss.Flows[wasteflowConn].Available_W = wasteflow;
    }

    void
    RunConstantEfficiencyConverterBackward(
        Model const& m,
        SimulationState& ss,
        size_t outflowConnIdx,
        size_t compIdx
    )
    {
        ConstantEfficiencyConverter const& cec = m.ConstEffConvs[compIdx];
        size_t inflowConn = cec.InflowConn;
        flow_t outflowRequest =
            ss.Flows[outflowConnIdx].Requested_W > cec.MaxOutflow_W
            ? cec.MaxOutflow_W
            : ss.Flows[outflowConnIdx].Requested_W;
        flow_t inflowRequest =
            static_cast<flow_t>(std::ceil(outflowRequest / cec.Efficiency));
        // TODO: re-enable a check for outflow <= inflow after adding COP
        // components NOTE: for COP, we can have inflow < outflow
        if (inflowRequest != ss.Flows[inflowConn].Requested_W)
        {
            ss.ActiveConnectionsBack.insert(inflowConn);
        }
        ss.Flows[inflowConn].Requested_W = inflowRequest;
        UpdateConstantEfficiencyLossflowAndWasteflow(m, ss, compIdx);
    }

    void
    UpdateEnvironmentFlowForMover(
        Model const& m,
        SimulationState& ss,
        size_t moverIdx
    )
    {
        Mover const& mov = m.Movers[moverIdx];
        flow_t inflowReq = ss.Flows[mov.InflowConn].Requested_W;
        flow_t inflowAvail = ss.Flows[mov.InflowConn].Available_W;
        flow_t outflowReq = ss.Flows[mov.OutflowConn].Requested_W;
        flow_t outflowAvail =
            ss.Flows[mov.OutflowConn].Available_W > mov.MaxOutflow_W
            ? mov.MaxOutflow_W
            : ss.Flows[mov.OutflowConn].Available_W;
        flow_t inflow = FinalizeFlowValue(inflowReq, inflowAvail);
        flow_t outflow = FinalizeFlowValue(outflowReq, outflowAvail);
        if (inflow >= outflow)
        {
            ss.Flows[mov.InFromEnvConn].Requested_W = 0;
            ss.Flows[mov.InFromEnvConn].Available_W = 0;
            ss.Flows[mov.InFromEnvConn].Actual_W = 0;
            flow_t waste = inflow - outflow;
            ss.Flows[mov.WasteflowConn].Requested_W = waste;
            ss.Flows[mov.WasteflowConn].Available_W = waste;
            ss.Flows[mov.WasteflowConn].Actual_W = waste;
        }
        else
        {
            flow_t supply = outflow - inflow;
            ss.Flows[mov.InFromEnvConn].Requested_W = supply;
            ss.Flows[mov.InFromEnvConn].Available_W = supply;
            ss.Flows[mov.InFromEnvConn].Actual_W = supply;
            ss.Flows[mov.WasteflowConn].Requested_W = 0;
            ss.Flows[mov.WasteflowConn].Available_W = 0;
            ss.Flows[mov.WasteflowConn].Actual_W = 0;
        }
    }

    void
    RunMoverBackward(
        Model const& m,
        SimulationState& ss,
        size_t outflowConnIdx,
        size_t moverIdx
    )
    {
        Mover const& mov = m.Movers[moverIdx];
        size_t inflowConn = mov.InflowConn;
        flow_t outflowRequest =
            ss.Flows[outflowConnIdx].Requested_W > mov.MaxOutflow_W
            ? mov.MaxOutflow_W
            : ss.Flows[outflowConnIdx].Requested_W;
        flow_t inflowRequest =
            static_cast<flow_t>(std::ceil(outflowRequest / mov.COP));
        if (inflowRequest != ss.Flows[inflowConn].Requested_W)
        {
            ss.ActiveConnectionsBack.insert(inflowConn);
        }
        ss.Flows[inflowConn].Requested_W = inflowRequest;
        UpdateEnvironmentFlowForMover(m, ss, moverIdx);
    }

    void
    Mux_RequestInflowsIntelligently(
        SimulationState& ss,
        std::vector<size_t> const& inflowConns,
        flow_t remainingRequest_W
    )
    {
        for (size_t inflowConn : inflowConns)
        {
            if (ss.Flows[inflowConn].Requested_W != remainingRequest_W)
            {
                ss.ActiveConnectionsBack.insert(inflowConn);
            }
            ss.Flows[inflowConn].Requested_W = remainingRequest_W;
            remainingRequest_W =
                remainingRequest_W > ss.Flows[inflowConn].Available_W
                ? remainingRequest_W - ss.Flows[inflowConn].Available_W
                : 0;
        }
    }

    void
    Mux_BalanceRequestFlows(
        SimulationState& ss,
        std::vector<size_t> const& inflowConns,
        flow_t remainingRequest_W,
        bool logNewActivity
    )
    {
        std::vector<flow_t> requests_W(inflowConns.size(), 0);
        for (size_t i = 0; i < inflowConns.size(); ++i)
        {
            size_t inflowConn = inflowConns[i];
            flow_t req_W =
                remainingRequest_W >= ss.Flows[inflowConn].Available_W
                ? ss.Flows[inflowConn].Available_W
                : remainingRequest_W;
            requests_W[i] = req_W;
            remainingRequest_W -= req_W;
        }
        if (remainingRequest_W > 0)
        {
            requests_W[0] = UtilSafeAdd(requests_W[0], remainingRequest_W);
        }
        for (size_t i = 0; i < inflowConns.size(); ++i)
        {
            size_t inflowConn = inflowConns[i];
            if (logNewActivity
                && ss.Flows[inflowConn].Requested_W != requests_W[i])
            {
                ss.ActiveConnectionsBack.insert(inflowConn);
            }
            ss.Flows[inflowConn].Requested_W = requests_W[i];
        }
    }

    void
    BalanceMuxRequests(
        Model& model,
        SimulationState& ss,
        size_t muxIdx,
        bool isUnavailable
    )
    {
        Mux const& mux = model.Muxes[muxIdx];
        flow_t totalRequest = 0;
        if (isUnavailable)
        {
            Mux_BalanceRequestFlows(ss, mux.InflowConns, totalRequest, true);
        }
        else
        {
            for (size_t i = 0; i < mux.NumOutports; ++i)
            {
                auto outflowConnIdx = mux.OutflowConns[i];
                auto outflowRequest_W =
                    ss.Flows[outflowConnIdx].Requested_W > mux.MaxOutflows_W[i]
                    ? mux.MaxOutflows_W[i]
                    : ss.Flows[outflowConnIdx].Requested_W;
                totalRequest = UtilSafeAdd(totalRequest, outflowRequest_W);
            }
            Mux_BalanceRequestFlows(ss, mux.InflowConns, totalRequest, true);
        }
    }

    void
    RunMuxBackward(Model& model, SimulationState& ss, size_t muxIdx)
    {
        Mux const& mux = model.Muxes[muxIdx];
        flow_t totalRequest = 0;
        for (size_t i = 0; i < mux.NumOutports; ++i)
        {
            auto outflowConnIdx = mux.OutflowConns[i];
            auto outflowRequest_W =
                ss.Flows[outflowConnIdx].Requested_W > mux.MaxOutflows_W[i]
                ? mux.MaxOutflows_W[i]
                : ss.Flows[outflowConnIdx].Requested_W;
            totalRequest = UtilSafeAdd(totalRequest, outflowRequest_W);
        }
        Mux_RequestInflowsIntelligently(
            ss, model.Muxes[muxIdx].InflowConns, totalRequest
        );
    }

    void
    RunStoreBackward(
        Model& model,
        SimulationState& ss,
        size_t outflowConnIdx,
        size_t storeIdx
    )
    {
        Store const& store = model.Stores[storeIdx];
        size_t inflowConnIdx = store.InflowConn;
        assert(outflowConnIdx == store.OutflowConn);
        flow_t chargeRate =
            ss.StorageAmounts_J[storeIdx] <= store.ChargeAmount_J
            ? store.MaxChargeRate_W
            : 0;
        flow_t outRequest_W =
            ss.Flows[outflowConnIdx].Requested_W > store.MaxOutflow_W
            ? store.MaxOutflow_W
            : ss.Flows[outflowConnIdx].Requested_W;
        if (ss.Flows[inflowConnIdx].Requested_W != (outRequest_W + chargeRate))
        {
            ss.ActiveConnectionsBack.insert(inflowConnIdx);
        }
        ss.Flows[inflowConnIdx].Requested_W = outRequest_W + chargeRate;
    }

    void
    RunScheduleBasedSourceBackward(
        Model& model,
        SimulationState& ss,
        size_t outConnIdx,
        size_t sbsIdx
    )
    {
        ScheduleBasedSource const& sbs = model.ScheduledSrcs[sbsIdx];
        assert(outConnIdx == sbs.OutflowConn);
        auto wasteConn = model.ScheduledSrcs[sbsIdx].WasteflowConn;
        auto schIdx = ss.ScheduleBasedSourceIdx[sbsIdx];
        auto available = sbs.TimeAndAvails[schIdx].Amount_W > sbs.MaxOutflow_W
            ? sbs.MaxOutflow_W
            : sbs.TimeAndAvails[schIdx].Amount_W;
        auto spillage = available > ss.Flows[outConnIdx].Requested_W
            ? available - ss.Flows[outConnIdx].Requested_W
            : 0;
        if (ss.Flows[outConnIdx].Available_W != available)
        {
            ss.ActiveConnectionsFront.insert(outConnIdx);
        }
        ss.Flows[outConnIdx].Available_W = available;
        ss.Flows[wasteConn].Available_W = spillage;
        ss.Flows[wasteConn].Requested_W = spillage;
    }

    void
    RunPassthroughBackward(
        Model& m,
        SimulationState& ss,
        size_t outConnIdx,
        size_t ptIdx
    )
    {
        PassThrough const& pt = m.PassThroughs[ptIdx];
        size_t compId = m.Connections[outConnIdx].FromId;
        if (ss.UnavailableComponents.contains(compId))
        {
            if (ss.Flows[pt.InflowConn].Requested_W != 0)
            {
                ss.ActiveConnectionsBack.insert(pt.InflowConn);
            }
            ss.Flows[pt.InflowConn].Requested_W = 0;
        }
        else
        {
            flow_t req_W = ss.Flows[outConnIdx].Requested_W > pt.MaxOutflow_W
                ? pt.MaxOutflow_W
                : ss.Flows[outConnIdx].Requested_W;
            if (ss.Flows[pt.InflowConn].Requested_W != req_W)
            {
                ss.ActiveConnectionsBack.insert(pt.InflowConn);
            }
            ss.Flows[pt.InflowConn].Requested_W = req_W;
        }
    }

    void
    RunConnectionsBackward(Model& model, SimulationState& ss)
    {
        while (!ss.ActiveConnectionsBack.empty())
        {
            auto temp = std::vector<size_t>(
                ss.ActiveConnectionsBack.begin(), ss.ActiveConnectionsBack.end()
            );
            ss.ActiveConnectionsBack.clear();
            for (auto it = temp.cbegin(); it != temp.cend(); ++it)
            {
                size_t connIdx = *it;
                size_t compIdx = model.Connections[connIdx].FromIdx;
                size_t compId = model.Connections[connIdx].FromId;
                if (ss.UnavailableComponents.contains(compId))
                {
                    // TODO: test if we need to call this
                    Model_SetComponentToFailed(model, ss, compId);
                    continue;
                }
                switch (model.Connections[connIdx].From)
                {
                    case ComponentType::ConstantSourceType:
                    {
                    }
                    break;
                    case ComponentType::PassThroughType:
                    {
                        RunPassthroughBackward(model, ss, connIdx, compIdx);
                    }
                    break;
                    case ComponentType::ScheduleBasedSourceType:
                    {
                        RunScheduleBasedSourceBackward(
                            model, ss, connIdx, compIdx
                        );
                    }
                    break;
                    case ComponentType::ConstantEfficiencyConverterType:
                    {
                        switch (model.Connections[connIdx].FromPort)
                        {
                            case 0:
                            {
                                RunConstantEfficiencyConverterBackward(
                                    model, ss, connIdx, compIdx
                                );
                            }
                            break;
                            case 1: // lossflow
                            case 2: // wasteflow
                            {
                                UpdateConstantEfficiencyLossflowAndWasteflow(
                                    model, ss, compIdx
                                );
                            }
                            break;
                            default:
                            {
                                std::cerr << "unhandled port" << std::endl;
                                exit(1);
                            }
                        }
                    }
                    break;
                    case ComponentType::MuxType:
                    {
                        RunMuxBackward(model, ss, compIdx);
                        if (model.Muxes[compIdx].NumOutports > 1)
                        {
                            // NOTE: possibly re-allocate downstream available
                            RunMuxForward(model, ss, compIdx);
                        }
                    }
                    break;
                    case ComponentType::StoreType:
                    {
                        RunStoreBackward(model, ss, connIdx, compIdx);
                    }
                    break;
                    case ComponentType::MoverType:
                    {
                        switch (model.Connections[connIdx].FromPort)
                        {
                            case 0:
                            {
                                RunMoverBackward(model, ss, connIdx, compIdx);
                            }
                            break;
                            case 1:
                            {
                                UpdateEnvironmentFlowForMover(
                                    model, ss, compIdx
                                );
                            }
                            break;
                            default:
                            {
                                WriteErrorMessage(
                                    "<runtime>", "bad port connection for mover"
                                );
                                std::exit(1);
                            }
                            break;
                        }
                    }
                    break;
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
        size_t compIdx
    )
    {
        ConstantEfficiencyConverter const& cec = m.ConstEffConvs[compIdx];
        flow_t inflowAvailable = ss.Flows[connIdx].Available_W;
        size_t outflowConn = cec.OutflowConn;
        flow_t outflowAvailable =
            static_cast<flow_t>(std::floor(cec.Efficiency * inflowAvailable));
        if (outflowAvailable > cec.MaxOutflow_W)
        {
            outflowAvailable = cec.MaxOutflow_W;
        }
        if (outflowAvailable != ss.Flows[outflowConn].Available_W)
        {
            ss.ActiveConnectionsFront.insert(outflowConn);
        }
        ss.Flows[outflowConn].Available_W = outflowAvailable;
        UpdateConstantEfficiencyLossflowAndWasteflow(m, ss, compIdx);
    }

    void
    RunMoverForward(
        Model const& model,
        SimulationState& ss,
        size_t outConnIdx,
        size_t moverIdx
    )
    {
        Mover const& mov = model.Movers[moverIdx];
        flow_t inflowAvailable = ss.Flows[outConnIdx].Available_W;
        size_t outflowConn = mov.OutflowConn;
        flow_t outflowAvailable =
            static_cast<flow_t>(std::floor(mov.COP * inflowAvailable));
        if (outflowAvailable > mov.MaxOutflow_W)
        {
            outflowAvailable = mov.MaxOutflow_W;
        }
        if (outflowAvailable != ss.Flows[outflowConn].Available_W)
        {
            ss.ActiveConnectionsFront.insert(outflowConn);
        }
        ss.Flows[outflowConn].Available_W = outflowAvailable;
        UpdateEnvironmentFlowForMover(model, ss, moverIdx);
    }

    void
    RunMuxForward(Model& model, SimulationState& ss, size_t muxIdx)
    {
        Mux const& mux = model.Muxes[muxIdx];
        flow_t totalAvailable = 0;
        for (size_t inflowConnIdx : mux.InflowConns)
        {
            totalAvailable = UtilSafeAdd(
                totalAvailable, ss.Flows[inflowConnIdx].Available_W
            );
        }
        std::vector<flow_t> outflowAvailables{};
        outflowAvailables.reserve(mux.NumOutports);
        for (size_t i = 0; i < mux.NumOutports; ++i)
        {
            size_t outflowConnIdx = mux.OutflowConns[i];
            flow_t req_W =
                ss.Flows[outflowConnIdx].Requested_W > mux.MaxOutflows_W[i]
                ? mux.MaxOutflows_W[i]
                : ss.Flows[outflowConnIdx].Requested_W;
            flow_t available = req_W >= totalAvailable ? totalAvailable : req_W;
            outflowAvailables.push_back(available);
            totalAvailable -= available;
        }
        if (totalAvailable > 0)
        {
            for (size_t i = 0; i < mux.NumOutports; ++i)
            {
                if (mux.MaxOutflows_W[i] > outflowAvailables[i])
                {
                    flow_t maxAdd = mux.MaxOutflows_W[i] - outflowAvailables[i];
                    flow_t toAdd =
                        maxAdd > totalAvailable ? totalAvailable : maxAdd;
                    flow_t actuallyAdded = outflowAvailables[i];
                    outflowAvailables[i] =
                        UtilSafeAdd(outflowAvailables[i], toAdd);
                    actuallyAdded = outflowAvailables[i] - actuallyAdded;
                    totalAvailable -= actuallyAdded;
                    if (totalAvailable == 0)
                    {
                        break;
                    }
                }
            }
            totalAvailable = 0;
        }
        for (size_t i = 0; i < mux.NumOutports; ++i)
        {
            size_t outflowConnIdx = mux.OutflowConns[i];
            if (ss.Flows[outflowConnIdx].Available_W != outflowAvailables[i])
            {
                ss.ActiveConnectionsFront.insert(outflowConnIdx);
            }
            ss.Flows[outflowConnIdx].Available_W = outflowAvailables[i];
        }
    }

    void
    RunStoreForward(
        Model& model,
        SimulationState& ss,
        size_t inflowConnIdx,
        size_t storeIdx
    )
    {
        Store const& store = model.Stores[storeIdx];
        size_t outflowConn = store.OutflowConn;
        assert(inflowConnIdx == store.InflowConn);
        flow_t available = ss.Flows[inflowConnIdx].Available_W;
        flow_t dischargeAvailable =
            ss.StorageAmounts_J[storeIdx] > 0 ? store.MaxDischargeRate_W : 0;
        available = UtilSafeAdd(available, dischargeAvailable);
        if (available > store.MaxOutflow_W)
        {
            available = store.MaxOutflow_W;
        }
        if (ss.Flows[outflowConn].Available_W != available)
        {
            ss.ActiveConnectionsFront.insert(outflowConn);
        }
        ss.Flows[outflowConn].Available_W = available;
    }

    void
    RunPassthroughForward(
        Model& m,
        SimulationState& ss,
        size_t inflowConnIdx,
        size_t ptIdx
    )
    {
        auto const& pt = m.PassThroughs[ptIdx];
        size_t compId = m.Connections[inflowConnIdx].ToId;
        if (ss.UnavailableComponents.contains(compId))
        {
            if (ss.Flows[pt.OutflowConn].Available_W != 0)
            {
                ss.ActiveConnectionsFront.insert(pt.OutflowConn);
            }
            ss.Flows[pt.OutflowConn].Available_W = 0;
        }
        else
        {
            flow_t avail_W =
                ss.Flows[inflowConnIdx].Available_W > pt.MaxOutflow_W
                ? pt.MaxOutflow_W
                : ss.Flows[inflowConnIdx].Available_W;
            if (ss.Flows[pt.OutflowConn].Available_W != avail_W)
            {
                ss.ActiveConnectionsFront.insert(pt.OutflowConn);
            }
            ss.Flows[pt.OutflowConn].Available_W = avail_W;
        }
    }

    void
    RunConnectionsForward(Model& model, SimulationState& ss)
    {
        while (!ss.ActiveConnectionsFront.empty())
        {
            auto temp = std::vector<size_t>(
                ss.ActiveConnectionsFront.begin(),
                ss.ActiveConnectionsFront.end()
            );
            ss.ActiveConnectionsFront.clear();
            for (auto it = temp.cbegin(); it != temp.cend(); ++it)
            {
                size_t connIdx = *it;
                size_t compIdx = model.Connections[connIdx].ToIdx;
                size_t compId = model.Connections[connIdx].ToId;
                if (ss.UnavailableComponents.contains(compId))
                {
                    // TODO: test if we need to call this
                    Model_SetComponentToFailed(model, ss, compId);
                    continue;
                }
                switch (model.Connections[connIdx].To)
                {
                    case ComponentType::ConstantLoadType:
                    case ComponentType::WasteSinkType:
                    case ComponentType::ScheduleBasedLoadType:
                    {
                    }
                    break;
                    case ComponentType::PassThroughType:
                    {
                        RunPassthroughForward(model, ss, connIdx, compIdx);
                    }
                    break;
                    case ComponentType::ConstantEfficiencyConverterType:
                    {
                        RunConstantEfficiencyConverterForward(
                            model, ss, connIdx, compIdx
                        );
                    }
                    break;
                    case ComponentType::MoverType:
                    {
                        RunMoverForward(model, ss, connIdx, compIdx);
                    }
                    break;
                    case ComponentType::MuxType:
                    {
                        RunMuxForward(model, ss, compIdx);
                        if (model.Muxes[compIdx].NumInports > 1)
                        {
                            // NOTE: possibly re-allocate upstream requests
                            RunMuxBackward(model, ss, compIdx);
                        }
                    }
                    break;
                    case ComponentType::StoreType:
                    {
                        RunStoreForward(model, ss, connIdx, compIdx);
                    }
                    break;
                    default:
                    {
                        std::cerr
                            << "unhandled component type on forward pass: "
                            << ToString(model.Connections[connIdx].To)
                            << std::endl;
                        std::exit(1);
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
        size_t compIdx
    )
    {
        // NOTE: we assume that the charge request never resets once at or
        // below chargeAmount UNTIL you hit 100% SOC again...
        Store const& store = model.Stores[compIdx];
        std::optional<size_t> maybeOutflowConn = FindOutflowConnection(
            model, model.Connections[connIdx].To, compIdx, 0
        );
        assert(
            maybeOutflowConn.has_value()
            && "store must have an outflow connection"
        );
        assert(connIdx == store.InflowConn && "connIdx must be inflowConn");
        size_t outflowConn = maybeOutflowConn.value();
        // TODO: consider using int64_t here
        int netCharge_W = static_cast<int>(ss.Flows[connIdx].Actual_W)
            - static_cast<int>(ss.Flows[outflowConn].Actual_W);
        if (netCharge_W > 0)
        {
            flow_t storeflow_W = static_cast<flow_t>(netCharge_W);
            if (store.WasteflowConn.has_value())
            {
                storeflow_W = static_cast<flow_t>(
                    netCharge_W * store.RoundTripEfficiency
                );
                flow_t wasteflow_W = netCharge_W - storeflow_W;
                size_t wfIdx = store.WasteflowConn.value();
                ss.Flows[wfIdx].Requested_W = wasteflow_W;
                ss.Flows[wfIdx].Available_W = wasteflow_W;
                ss.Flows[wfIdx].Actual_W = wasteflow_W;
            }
            ss.StorageNextEventTimes[compIdx] = t
                + (static_cast<double>(
                       store.Capacity_J - ss.StorageAmounts_J[compIdx]
                   )
                   / static_cast<double>(storeflow_W));
        }
        else if (netCharge_W < 0
                 && (ss.StorageAmounts_J[compIdx] > store.ChargeAmount_J))
        {
            if (store.WasteflowConn.has_value())
            {
                size_t wfIdx = store.WasteflowConn.value();
                ss.Flows[wfIdx].Requested_W = 0;
                ss.Flows[wfIdx].Available_W = 0;
                ss.Flows[wfIdx].Actual_W = 0;
            }
            ss.StorageNextEventTimes[compIdx] = t
                + (static_cast<double>(
                       ss.StorageAmounts_J[compIdx] - store.ChargeAmount_J
                   )
                   / (-1.0 * static_cast<double>(netCharge_W)));
        }
        else if (netCharge_W < 0)
        {
            if (store.WasteflowConn.has_value())
            {
                size_t wfIdx = store.WasteflowConn.value();
                ss.Flows[wfIdx].Requested_W = 0;
                ss.Flows[wfIdx].Available_W = 0;
                ss.Flows[wfIdx].Actual_W = 0;
            }
            ss.StorageNextEventTimes[compIdx] = t
                + (static_cast<double>(ss.StorageAmounts_J[compIdx])
                   / (-1.0 * static_cast<double>(netCharge_W)));
        }
        else // netCharge_W = 0
        {
            if (store.WasteflowConn.has_value())
            {
                size_t wfIdx = store.WasteflowConn.value();
                ss.Flows[wfIdx].Requested_W = 0;
                ss.Flows[wfIdx].Available_W = 0;
                ss.Flows[wfIdx].Actual_W = 0;
            }
            ss.StorageNextEventTimes[compIdx] = infinity;
        }
    }

    void
    RunMuxPostFinalization(Model& model, SimulationState& ss, size_t compIdx)
    {
        // TODO: test if we need to run backward/forward again
        RunMuxBackward(model, ss, compIdx);
        RunMuxForward(model, ss, compIdx);
        // BalanceMuxRequests(model, ss, compIdx);
    }

    void
    RunConnectionsPostFinalization(Model& model, SimulationState& ss, double t)
    {
        for (size_t connIdx = 0; connIdx < model.Connections.size(); ++connIdx)
        {
            size_t compIdx = model.Connections[connIdx].ToIdx;
            size_t compId = model.Connections[connIdx].ToId;
            if (ss.UnavailableComponents.contains(compId))
            {
                continue;
            }
            switch (model.Connections[connIdx].To)
            {
                case ComponentType::StoreType:
                {
                    RunStorePostFinalization(model, ss, t, connIdx, compIdx);
                }
                break;
                case ComponentType::MuxType:
                {
                    RunMuxPostFinalization(model, ss, compIdx);
                }
                break;
                default:
                {
                }
                break;
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
            ss.Flows[flowIdx].Actual_W = FinalizeFlowValue(
                ss.Flows[flowIdx].Requested_W, ss.Flows[flowIdx].Available_W
            );
        }
    }

    double
    NextEvent(
        ScheduleBasedLoad const& sb,
        size_t sbIdx,
        SimulationState const& ss
    )
    {
        auto nextIdx = ss.ScheduleBasedLoadIdx[sbIdx] + 1;
        if (nextIdx >= sb.TimesAndLoads.size())
        {
            return infinity;
        }
        return sb.TimesAndLoads[nextIdx].Time_s;
    }

    double
    NextEvent(
        ScheduleBasedSource const& sb,
        size_t sbIdx,
        SimulationState const& ss
    )
    {
        auto nextIdx = ss.ScheduleBasedSourceIdx[sbIdx] + 1;
        if (nextIdx >= sb.TimeAndAvails.size())
        {
            return infinity;
        }
        return sb.TimeAndAvails[nextIdx].Time_s;
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
    UpdateStoresPerElapsedTime(
        Model const& m,
        SimulationState& ss,
        double elapsedTime_s
    )
    {
        for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
        {
            Store const& store = m.Stores[storeIdx];
            long netEnergyAdded = 0;
            size_t inConn = store.InflowConn;
            size_t outConn = store.OutflowConn;
            size_t compId = m.Connections[outConn].FromId;
            if (ss.UnavailableComponents.contains(compId))
            {
                continue;
            }
            long availableCharge_J =
                static_cast<long>(
                    m.Stores[storeIdx].Capacity_J
                    - ss.StorageAmounts_J[storeIdx]
                );
            long availableDischarge_J =
                -1 * static_cast<long>(ss.StorageAmounts_J[storeIdx]);
            netEnergyAdded += std::lround(
                elapsedTime_s * static_cast<double>(ss.Flows[inConn].Actual_W)
            );
            netEnergyAdded -= std::lround(
                elapsedTime_s * static_cast<double>(ss.Flows[outConn].Actual_W)
            );
            if (store.WasteflowConn.has_value())
            {
                size_t wConn = store.WasteflowConn.value();
                netEnergyAdded -= std::lround(
                    elapsedTime_s * static_cast<double>(ss.Flows[wConn].Actual_W)
                );
            }
            if (netEnergyAdded > availableCharge_J)
            {
                std::cout << "ERROR: netEnergyAdded is greater than capacity!" << std::endl;
                std::cout << "netEnergyAdded (J): " << netEnergyAdded << std::endl;
                std::cout << "availableCharge (J): " << availableCharge_J << std::endl;
                std::cout << "elapsed time (s): " << elapsedTime_s << std::endl;
                std::cout << "stored amount (J): "
                          << ss.StorageAmounts_J[storeIdx] << std::endl;
                std::cout << "inflow (W): " << ss.Flows[inConn].Actual_W
                          << std::endl;
                std::cout << "outflow (W): " << ss.Flows[outConn].Actual_W
                          << std::endl;
                int64_t inflow =
                    static_cast<int64_t>(ss.Flows[inConn].Actual_W);
                int64_t outflow =
                    static_cast<int64_t>(ss.Flows[outConn].Actual_W);
                int64_t wasteflow = 0;
                if (store.WasteflowConn.has_value())
                {
                    size_t wConn = store.WasteflowConn.value();
                    std::cout << "wasteflow (W): " << ss.Flows[wConn].Actual_W
                              << std::endl;
                    wasteflow = static_cast<int64_t>(ss.Flows[wConn].Actual_W);
                }
                std::cout << "flow balance (inflow - (outflow + wasteflow)): "
                          << (inflow - (outflow + wasteflow)) << std::endl;
            }
            assert(
                availableCharge_J >= netEnergyAdded
                && "netEnergyAdded cannot put storage over capacity"
            );
            if (netEnergyAdded < availableDischarge_J)
            {
                std::cout << "ERROR: netEnergyAdded is lower than discharge limit" << std::endl;
                std::cout << "netEnergyAdded (J): " << netEnergyAdded
                          << std::endl;
                std::cout << "availableDischarge (J): " << availableDischarge_J
                          << std::endl;
                std::cout << "elapsed time (s): " << elapsedTime_s << std::endl;
                std::cout << "stored amount (J): "
                          << ss.StorageAmounts_J[storeIdx] << std::endl;
                std::cout << "inflow (W): " << ss.Flows[inConn].Actual_W
                          << std::endl;
                std::cout << "outflow (W): " << ss.Flows[outConn].Actual_W
                          << std::endl;
                int64_t inflow =
                    static_cast<int64_t>(ss.Flows[inConn].Actual_W);
                int64_t outflow =
                    static_cast<int64_t>(ss.Flows[outConn].Actual_W);
                int64_t wasteflow = 0;
                if (store.WasteflowConn.has_value())
                {
                    size_t wConn = store.WasteflowConn.value();
                    std::cout << "wasteflow (W): " << ss.Flows[wConn].Actual_W
                              << std::endl;
                    wasteflow = static_cast<int64_t>(ss.Flows[wConn].Actual_W);
                }
                std::cout << "flow balance (inflow - (outflow + wasteflow)): "
                          << (inflow - (outflow + wasteflow)) << std::endl;
            }
            assert(
                netEnergyAdded >= availableDischarge_J
                && "netEnergyAdded cannot use more energy than available"
            );
            ss.StorageAmounts_J[storeIdx] += netEnergyAdded;
        }
    }

    void
    UpdateScheduleBasedLoadNextEvent(
        Model const& m,
        SimulationState& ss,
        double time
    )
    {
        for (size_t i = 0; i < m.ScheduledLoads.size(); ++i)
        {
            size_t nextIdx = ss.ScheduleBasedLoadIdx[i] + 1;
            if (nextIdx < m.ScheduledLoads[i].TimesAndLoads.size()
                && m.ScheduledLoads[i].TimesAndLoads[nextIdx].Time_s == time)
            {
                ss.ScheduleBasedLoadIdx[i] = nextIdx;
            }
        }
    }

    void
    UpdateScheduleBasedSourceNextEvent(
        Model const& m,
        SimulationState& ss,
        double time
    )
    {
        for (size_t i = 0; i < m.ScheduledSrcs.size(); ++i)
        {
            size_t nextIdx = ss.ScheduleBasedSourceIdx[i] + 1;
            if (nextIdx < m.ScheduledSrcs[i].TimeAndAvails.size()
                && m.ScheduledSrcs[i].TimeAndAvails[nextIdx].Time_s == time)
            {
                ss.ScheduleBasedSourceIdx[i] = nextIdx;
            }
        }
    }

    // TODO: rename to ComponentTypeToString(.)
    std::string
    ToString(ComponentType compType)
    {
        std::string result = "?";
        switch (compType)
        {
            case ComponentType::ConstantLoadType:
            {
                result = "ConstantLoad";
            }
            break;
            case ComponentType::ScheduleBasedLoadType:
            {
                result = "ScheduleBasedLoad";
            }
            break;
            case ComponentType::ConstantSourceType:
            {
                result = "ConstantSource";
            }
            break;
            case ComponentType::ScheduleBasedSourceType:
            {
                result = "ScheduleBasedSource";
            }
            break;
            case ComponentType::ConstantEfficiencyConverterType:
            {
                result = "ConstantEfficiencyConverter";
            }
            break;
            case ComponentType::WasteSinkType:
            {
                result = "WasteSink";
            }
            break;
            case ComponentType::MuxType:
            {
                result = "Mux";
            }
            break;
            case ComponentType::StoreType:
            {
                result = "Store";
            }
            break;
            case ComponentType::PassThroughType:
            {
                result = "PassThrough";
            }
            break;
            case ComponentType::MoverType:
            {
                result = "Mover";
            }
            break;
            case ComponentType::EnvironmentSourceType:
            {
                result = "EnvironmentSource";
            }
            break;
            default:
            {
                WriteErrorMessage(
                    "ToString",
                    "unhandled component type: " + ToString(compType)
                );
                std::exit(1);
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
        if (tag == "ScheduleBasedSource" || tag == "uncontrolled_source")
        {
            return ComponentType::ScheduleBasedSourceType;
        }
        if (tag == "ConstantEfficiencyConverter" || tag == "converter")
        {
            return ComponentType::ConstantEfficiencyConverterType;
        }
        if (tag == "WasteSink")
        {
            return ComponentType::WasteSinkType;
        }
        if (tag == "EnvironmentSource")
        {
            return ComponentType::EnvironmentSourceType;
        }
        if (tag == "Mux" || tag == "mux" || tag == "muxer")
        {
            return ComponentType::MuxType;
        }
        if (tag == "Store" || tag == "store")
        {
            return ComponentType::StoreType;
        }
        if (tag == "PassThrough" || tag == "pass_through")
        {
            return ComponentType::PassThroughType;
        }
        if (tag == "Mover" || tag == "mover")
        {
            return ComponentType::MoverType;
        }
        return {};
    }

    void
    PrintFlows(Model const& m, SimulationState const& ss, double time_s)
    {
        std::cout << "time: " << time_s << " s, "
                  << TimeToISO8601Period(static_cast<uint64_t>(time_s)) << ", "
                  << TimeInSecondsToHours(static_cast<uint64_t>(time_s)) << " h"
                  << std::endl;
        for (size_t flowIdx = 0; flowIdx < ss.Flows.size(); ++flowIdx)
        {
            std::cout << ConnectionToString(
                m.ComponentMap, m.Connections[flowIdx]
            ) << ": " << ss.Flows[flowIdx].Actual_W
                      << " (R: " << ss.Flows[flowIdx].Requested_W
                      << "; A: " << ss.Flows[flowIdx].Available_W << ")"
                      << std::endl;
        }
    }

    FlowSummary
    SummarizeFlows(Model const& m, SimulationState const& ss, double t)
    {
        FlowSummary summary = {
            .Time = t,
            .Inflow = 0,
            .OutflowRequest = 0,
            .OutflowAchieved = 0,
            .StorageDischarge = 0,
            .StorageCharge = 0,
            .Wasteflow = 0,
            .EnvInflow = 0,
        };
        for (size_t flowIdx = 0; flowIdx < ss.Flows.size(); ++flowIdx)
        {
            switch (m.Connections[flowIdx].From)
            {
                case ComponentType::ConstantSourceType:
                {
                    summary.Inflow += ss.Flows[flowIdx].Actual_W;
                }
                break;
                case ComponentType::ScheduleBasedSourceType:
                {
                    // NOTE: for schedule-based sources, the thinking is that
                    // the "available" is actually flowing into the system and,
                    // if not used (i.e., ullage/spillage), it goes to wasteflow
                    if (m.Connections[flowIdx].FromPort == 0)
                    {
                        summary.Inflow += ss.Flows[flowIdx].Available_W;
                    }
                }
                break;
                case ComponentType::StoreType:
                {
                    summary.StorageDischarge += ss.Flows[flowIdx].Actual_W;
                }
                break;
                case ComponentType::EnvironmentSourceType:
                {
                    summary.EnvInflow += ss.Flows[flowIdx].Actual_W;
                }
                break;
                default:
                {
                }
                break;
            }

            switch (m.Connections[flowIdx].To)
            {
                case (ComponentType::ConstantLoadType):
                case (ComponentType::ScheduleBasedLoadType):
                {
                    summary.OutflowRequest += ss.Flows[flowIdx].Requested_W;
                    summary.OutflowAchieved += ss.Flows[flowIdx].Actual_W;
                }
                break;
                case (ComponentType::StoreType):
                {
                    summary.StorageCharge += ss.Flows[flowIdx].Actual_W;
                }
                break;
                case (ComponentType::WasteSinkType):
                {
                    summary.Wasteflow += ss.Flows[flowIdx].Actual_W;
                }
                break;
                default:
                {
                }
                break;
            }
        }
        return summary;
    }

    void
    PrintFlowSummary(FlowSummary s)
    {
        int netDischarge = static_cast<int>(s.StorageDischarge)
            - static_cast<int>(s.StorageCharge);
        int sum = static_cast<int>(s.Inflow) + netDischarge
            + static_cast<int>(s.EnvInflow)
            - (static_cast<int>(s.OutflowAchieved)
               + static_cast<int>(s.Wasteflow));
        double eff =
            (static_cast<double>(s.Inflow) + static_cast<double>(s.EnvInflow) + static_cast<double>(netDischarge))
                > 0.0
            ? 100.0 * (static_cast<double>(s.OutflowAchieved))
                / (static_cast<double>(s.Inflow)
                   + static_cast<double>(netDischarge)
                   + static_cast<double>(s.EnvInflow))
            : 0.0;
        double effectiveness = s.OutflowRequest > 0
            ? 100.0 * (static_cast<double>(s.OutflowAchieved))
                / (static_cast<double>(s.OutflowRequest))
            : 0.0;
        std::cout << "Flow Summary @ " << s.Time << ":" << std::endl;
        std::cout << "  Inflow                 : " << s.Inflow << std::endl;
        std::cout << "+ Storage Net Discharge  : " << netDischarge << std::endl;
        std::cout << "+ Environment Inflow     : " << s.EnvInflow << std::endl;
        std::cout << "- Outflow (achieved)     : " << s.OutflowAchieved
                  << std::endl;
        std::cout << "- Wasteflow              : " << s.Wasteflow << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        std::cout << "= Sum                    : " << sum << std::endl;
        std::cout << "  Efficiency             : " << eff << "%"
                  << " (= " << s.OutflowAchieved << "/"
                  << ((int)s.Inflow + (int)s.EnvInflow + netDischarge) << ")" << std::endl;
        std::cout << "  Delivery Effectiveness : " << effectiveness << "%"
                  << " (= " << s.OutflowAchieved << "/" << s.OutflowRequest
                  << ")" << std::endl;
    }

    std::vector<Flow>
    CopyFlows(std::vector<Flow> flows)
    {
        std::vector<Flow> newFlows = {};
        newFlows.reserve(flows.size());
        for (size_t i = 0; i < flows.size(); ++i)
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
        newAmounts.reserve(ss.StorageAmounts_J.size());
        for (size_t i = 0; i < ss.StorageAmounts_J.size(); ++i)
        {
            newAmounts.push_back(ss.StorageAmounts_J[i]);
        }
        return newAmounts;
    }

    void
    PrintModelState(Model& m, SimulationState& ss)
    {
        for (size_t storeIdx = 0; storeIdx < m.Stores.size(); ++storeIdx)
        {
            std::cout << ToString(ComponentType::StoreType) << "[" << storeIdx
                      << "].InitialStorage (J): "
                      << m.Stores[storeIdx].InitialStorage_J << std::endl;
            std::cout << ToString(ComponentType::StoreType) << "[" << storeIdx
                      << "].StorageAmount (J) : "
                      << ss.StorageAmounts_J[storeIdx] << std::endl;
            std::cout << ToString(ComponentType::StoreType) << "[" << storeIdx
                      << "].Capacity (J)      : "
                      << m.Stores[storeIdx].Capacity_J << std::endl;
            double soc = (double)ss.StorageAmounts_J[storeIdx] * 100.0
                / (double)m.Stores[storeIdx].Capacity_J;
            std::cout << ToString(ComponentType::StoreType) << "[" << storeIdx
                      << "].SOC               : " << soc << " %" << std::endl;
        }
    }

    size_t
    Model_NumberOfComponents(Model const& m)
    {
        return m.ComponentMap.Tag.size();
    }

    // TODO: add schedule-based reliability index?
    void
    Model_SetupSimulationState(Model& model, SimulationState& ss)
    {
        for (size_t i = 0; i < model.Stores.size(); ++i)
        {
            ss.StorageAmounts_J.push_back(model.Stores[i].InitialStorage_J);
        }
        ss.StorageNextEventTimes =
            std::vector<double>(model.Stores.size(), 0.0);
        ss.Flows = std::vector<Flow>(model.Connections.size(), {0, 0, 0});
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
        size_t repairDistId
    )
    {
        auto fmId = m.Rel.add_failure_mode("", failureDistId, repairDistId);
        auto linkId = m.Rel.link_component_with_failure_mode(compId, fmId);
        auto schedule = m.Rel.make_schedule_for_link(
            linkId, m.RandFn, m.DistSys, m.FinalTime
        );
        ScheduleBasedReliability sbr = {};
        sbr.ComponentId = compId;
        sbr.TimeStates = std::move(schedule);
        m.Reliabilities.push_back(std::move(sbr));
        return linkId;
    }

    ComponentIdAndWasteAndEnvironmentConnection
    Model_AddMover(Model& m, double cop)
    {
        return Model_AddMover(m, cop, 0, 0, "");
    }

    ComponentIdAndWasteAndEnvironmentConnection
    Model_AddMover(
        Model& m,
        double cop,
        size_t inflowTypeId,
        size_t outflowTypeId,
        std::string const& tag
    )
    {
        assert(cop > 0.0);
        Mover mov = {
            .COP = cop,
            .InflowConn = 0,
            .OutflowConn = 0,
            .InFromEnvConn = 0,
            .WasteflowConn = 0,
            .MaxOutflow_W = max_flow_W,
        };
        m.Movers.push_back(std::move(mov));
        size_t wasteId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::WasteSinkType,
            0,
            std::vector<size_t>{wasteflowId},
            std::vector<size_t>{},
            "",
            0.0
        );
        size_t envId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::EnvironmentSourceType,
            0,
            std::vector<size_t>{},
            std::vector<size_t>{wasteflowId},
            "",
            0.0
        );
        size_t thisId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::MoverType,
            0,
            std::vector<size_t>{inflowTypeId, wasteflowId},
            std::vector<size_t>{outflowTypeId, wasteflowId},
            tag,
            0.0
        );
        Connection wconn =
            Model_AddConnection(m, thisId, 1, wasteId, 0, wasteflowId);
        Connection econn =
            Model_AddConnection(m, envId, 0, thisId, 1, wasteflowId);
        return {
            .Id = thisId,
            .WasteConn = wconn,
            .EnvConn = econn,
        };
    }

    std::vector<TimeAndFlows>
    Simulate(Model& model, bool print = true)
    {
        double t = 0.0;
        std::vector<TimeAndFlows> timeAndFlows{};
        SimulationState ss{};
        Model_SetupSimulationState(model, ss);
        // TODO: add units to FinalTime (append '_s')
        while (t != infinity && t <= model.FinalTime)
        {
            // schedule each event-generating component for next event
            // by adding to the ActiveComponentBack or ActiveComponentFront
            // arrays
            // note: these two arrays could be sorted by component type for
            // faster running over loops...
            ActivateConnectionsForReliability(model, ss, t);
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
            size_t const maxLoop = 1'000;
            for (size_t loopIter = 0; loopIter <= maxLoop; ++loopIter)
            {
                if (CountActiveConnections(ss) == 0)
                {
                    if (print)
                    {
                        std::cout << "loop iter: " << loopIter << std::endl;
                    }
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
                std::cout << "==== QUIESCENCE REACHED ====" << std::endl;
            }
            TimeAndFlows taf = {};
            taf.Time = t;
            taf.Flows = CopyFlows(ss.Flows);
            taf.StorageAmounts_J = CopyStorageStates(ss);
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
        size_t compId
    )
    {
        if (compId >= m.ComponentMap.CompType.size())
        {
            WriteErrorMessage("", "invalid component id");
            std::exit(1);
        }
        if (ss.UnavailableComponents.contains(compId))
        {
            ss.UnavailableComponents.erase(compId);
        }
        auto idx = m.ComponentMap.Idx[compId];
        switch (m.ComponentMap.CompType[compId])
        {
            case ComponentType::ConstantLoadType:
            {
                auto inflowConn = m.ConstLoads[idx].InflowConn;
                if (ss.Flows[inflowConn].Requested_W
                    != m.ConstLoads[idx].Load_W)
                {
                    ss.ActiveConnectionsBack.insert(inflowConn);
                }
                ss.Flows[inflowConn].Requested_W = m.ConstLoads[idx].Load_W;
            }
            break;
            case ComponentType::ScheduleBasedLoadType:
            {
                auto const inflowConn = m.ConstLoads[idx].InflowConn;
                auto const loadIdx = ss.ScheduleBasedLoadIdx[idx];
                auto const amount =
                    m.ScheduledLoads[idx].TimesAndLoads[loadIdx].Amount_W;
                if (ss.Flows[inflowConn].Requested_W != amount)
                {
                    ss.ActiveConnectionsBack.insert(inflowConn);
                }
                ss.Flows[inflowConn].Requested_W = amount;
            }
            break;
            case ComponentType::ConstantSourceType:
            {
                auto const outflowConn = m.ConstSources[idx].OutflowConn;
                auto const available = m.ConstSources[idx].Available_W;
                if (ss.Flows[outflowConn].Available_W != available)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = available;
            }
            break;
            case ComponentType::ScheduleBasedSourceType:
            {
                // TODO: need to call routine to reset wasteflow connection
                // to the right amount as well.
                auto const outflowConn = m.ScheduledSrcs[idx].OutflowConn;
                auto const availIdx = ss.ScheduleBasedSourceIdx[idx];
                auto const available =
                    m.ScheduledSrcs[idx].TimeAndAvails[availIdx].Amount_W;
                if (ss.Flows[outflowConn].Available_W != available)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = available;
            }
            break;
            case ComponentType::ConstantEfficiencyConverterType:
            {
                auto outflowConn = m.ConstEffConvs[idx].OutflowConn;
                RunConstantEfficiencyConverterBackward(m, ss, outflowConn, idx);
                auto inflowConn = m.ConstEffConvs[idx].InflowConn;
                RunConstantEfficiencyConverterForward(m, ss, inflowConn, idx);
            }
            break;
            case ComponentType::MoverType:
            {
                // TODO: test
                size_t outflowConn = m.Movers[idx].OutflowConn;
                RunMoverBackward(m, ss, outflowConn, idx);
                size_t inflowConn = m.Movers[idx].InflowConn;
                RunMoverForward(m, ss, inflowConn, idx);
            }
            break;
            case ComponentType::MuxType:
            {
                // TODO: check that this works
                // may also need to call mux routines to rebalance
                for (size_t inIdx = 0; inIdx < m.Muxes[idx].NumInports; ++inIdx)
                {
                    auto const inConn = m.Muxes[idx].InflowConns[inIdx];
                    // NOTE: schedule upstream to provide available flow info
                    // again.
                    ss.ActiveConnectionsFront.insert(inConn);
                }
                for (size_t outIdx = 0; outIdx < m.Muxes[idx].NumOutports;
                     ++outIdx)
                {
                    auto const outConn = m.Muxes[idx].OutflowConns[outIdx];
                    ss.ActiveConnectionsBack.insert(outConn);
                }
            }
            break;
            case ComponentType::StoreType:
            {
                // TODO: what energy amount should the store come back with?
                // TODO: need to call routines to do charge request
                WriteErrorMessage("store", "not implemented");
                std::exit(1);
            }
            break;
            case ComponentType::PassThroughType:
            {
                PassThrough const& pt = m.PassThroughs[idx];
                if (ss.Flows[pt.OutflowConn].Requested_W
                    != ss.Flows[pt.InflowConn].Requested_W)
                {
                    ss.ActiveConnectionsBack.insert(pt.OutflowConn);
                }
                ss.Flows[pt.InflowConn].Requested_W =
                    ss.Flows[pt.OutflowConn].Requested_W;
                if (ss.Flows[pt.InflowConn].Available_W
                    != ss.Flows[pt.OutflowConn].Available_W)
                {
                    ss.ActiveConnectionsFront.insert(pt.InflowConn);
                }
                ss.Flows[pt.OutflowConn].Available_W =
                    ss.Flows[pt.InflowConn].Available_W;
            }
            break;
            case ComponentType::WasteSinkType:
            {
                WriteErrorMessage(
                    "waste sink", "should not be repairing pseduo-element waste"
                );
                std::exit(1);
            }
            break;
            default:
            {
                WriteErrorMessage("", "unhandled component type (b)");
                std::exit(1);
            }
        }
    }

    void
    Model_SetComponentToFailed(
        Model const& m,
        SimulationState& ss,
        size_t compId
    )
    {
        if (compId >= m.ComponentMap.CompType.size())
        {
            WriteErrorMessage("", "invalid component id");
            std::exit(1);
        }
        ss.UnavailableComponents.insert(compId);
        auto idx = m.ComponentMap.Idx[compId];
        switch (m.ComponentMap.CompType[compId])
        {
            case ComponentType::ConstantLoadType:
            {
                auto inflowConn = m.ConstLoads[idx].InflowConn;
                if (ss.Flows[inflowConn].Requested_W != 0)
                {
                    ss.ActiveConnectionsBack.insert(inflowConn);
                }
                ss.Flows[inflowConn].Requested_W = 0;
            }
            break;
            case ComponentType::ScheduleBasedLoadType:
            {
                auto inflowConn = m.ScheduledLoads[idx].InflowConn;
                if (ss.Flows[inflowConn].Requested_W != 0)
                {
                    ss.ActiveConnectionsBack.insert(inflowConn);
                }
                ss.Flows[inflowConn].Requested_W = 0;
            }
            break;
            case ComponentType::ConstantSourceType:
            {
                auto outflowConn = m.ConstSources[idx].OutflowConn;
                if (ss.Flows[outflowConn].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = 0;
            }
            break;
            case ComponentType::ConstantEfficiencyConverterType:
            {
                ConstantEfficiencyConverter const& cec = m.ConstEffConvs[idx];
                auto inflowConn = cec.InflowConn;
                if (ss.Flows[inflowConn].Requested_W != 0)
                {
                    ss.ActiveConnectionsBack.insert(inflowConn);
                }
                ss.Flows[inflowConn].Requested_W = 0;
                auto outflowConn = cec.OutflowConn;
                if (ss.Flows[outflowConn].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = 0;
                auto lossflowConn = cec.LossflowConn;
                if (lossflowConn.has_value())
                {
                    auto lossConn = lossflowConn.value();
                    if (ss.Flows[lossConn].Available_W != 0)
                    {
                        ss.ActiveConnectionsFront.insert(lossConn);
                    }
                    ss.Flows[lossConn].Available_W = 0;
                }
                auto wasteConn = cec.WasteflowConn;
                ss.Flows[wasteConn].Available_W = 0;
                ss.Flows[wasteConn].Requested_W = 0;
            }
            break;
            case ComponentType::MoverType:
            {
                Mover const& mov = m.Movers[idx];
                ss.Flows[mov.InflowConn].Requested_W = 0;
                ss.Flows[mov.InflowConn].Available_W = 0;
                ss.Flows[mov.InflowConn].Actual_W = 0;
                ss.Flows[mov.OutflowConn].Requested_W = 0;
                ss.Flows[mov.OutflowConn].Available_W = 0;
                ss.Flows[mov.OutflowConn].Actual_W = 0;
                ss.Flows[mov.InFromEnvConn].Requested_W = 0;
                ss.Flows[mov.InFromEnvConn].Available_W = 0;
                ss.Flows[mov.InFromEnvConn].Actual_W = 0;
                ss.Flows[mov.WasteflowConn].Requested_W = 0;
                ss.Flows[mov.WasteflowConn].Available_W = 0;
                ss.Flows[mov.WasteflowConn].Actual_W = 0;
            }
            break;
            case ComponentType::MuxType:
            {
                for (size_t inIdx = 0; inIdx < m.Muxes[idx].NumInports; ++inIdx)
                {
                    auto inflowConn = m.Muxes[idx].InflowConns[inIdx];
                    if (ss.Flows[inflowConn].Requested_W != 0)
                    {
                        ss.ActiveConnectionsBack.insert(inflowConn);
                    }
                    ss.Flows[inflowConn].Requested_W = 0;
                }
                for (size_t outIdx = 0; outIdx < m.Muxes[idx].NumOutports;
                     ++outIdx)
                {
                    auto outflowConn = m.Muxes[idx].OutflowConns[outIdx];
                    if (ss.Flows[outflowConn].Available_W != 0)
                    {
                        ss.ActiveConnectionsFront.insert(outflowConn);
                    }
                    ss.Flows[outflowConn].Available_W = 0;
                }
            }
            break;
            case ComponentType::StoreType:
            {
                auto inflowConn = m.Stores[idx].InflowConn;
                if (ss.Flows[inflowConn].Requested_W != 0)
                {
                    ss.ActiveConnectionsBack.insert(inflowConn);
                }
                ss.Flows[inflowConn].Requested_W = 0;
                auto outflowConn = m.Stores[idx].OutflowConn;
                if (ss.Flows[outflowConn].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = 0;
            }
            break;
            case ComponentType::PassThroughType:
            {
                auto const& pt = m.PassThroughs[idx];
                if (ss.Flows[pt.InflowConn].Requested_W != 0)
                {
                    ss.ActiveConnectionsBack.insert(pt.InflowConn);
                }
                ss.Flows[pt.InflowConn].Requested_W = 0;
                if (ss.Flows[pt.OutflowConn].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(pt.OutflowConn);
                }
                ss.Flows[pt.OutflowConn].Available_W = 0;
            }
            break;
            case ComponentType::WasteSinkType:
            {
                WriteErrorMessage("waste sink", "waste sink cannot fail");
                std::exit(1);
            }
            break;
            default:
            {
                WriteErrorMessage("", "unhandled component type (c)");
                std::exit(1);
            }
            break;
        }
    }

    size_t
    Model_AddConstantLoad(Model& m, uint32_t load)
    {
        size_t idx = m.ConstLoads.size();
        ConstantLoad cl{};
        cl.Load_W = load;
        m.ConstLoads.push_back(std::move(cl));
        return Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::ConstantLoadType,
            idx,
            std::vector<size_t>{0},
            std::vector<size_t>{},
            "",
            0.0
        );
    }

    size_t
    Model_AddScheduleBasedLoad(
        Model& m,
        double* times,
        uint32_t* loads,
        size_t numItems
    )
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
        std::vector<TimeAndAmount> const& timesAndLoads
    )
    {
        return Model_AddScheduleBasedLoad(
            m, timesAndLoads, std::map<size_t, size_t>{}
        );
    }

    size_t
    Model_AddScheduleBasedLoad(
        Model& m,
        std::vector<TimeAndAmount> const& timesAndLoads,
        std::map<size_t, size_t> const& scenarioIdToLoadId
    )
    {
        return Model_AddScheduleBasedLoad(
            m, timesAndLoads, scenarioIdToLoadId, 0, ""
        );
    }

    size_t
    Model_AddScheduleBasedLoad(
        Model& m,
        std::vector<TimeAndAmount> const& timesAndLoads,
        std::map<size_t, size_t> const& scenarioIdToLoadId,
        size_t inflowTypeId,
        std::string const& tag
    )
    {
        size_t idx = m.ScheduledLoads.size();
        ScheduleBasedLoad sbl = {};
        sbl.TimesAndLoads = timesAndLoads;
        sbl.InflowConn = 0;
        sbl.ScenarioIdToLoadId = scenarioIdToLoadId;
        m.ScheduledLoads.push_back(std::move(sbl));
        return Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::ScheduleBasedLoadType,
            idx,
            std::vector<size_t>{inflowTypeId},
            std::vector<size_t>{},
            tag,
            0.0
        );
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
        std::string const& tag
    )
    {
        size_t idx = m.ConstSources.size();
        ConstantSource cs{};
        cs.Available_W = available;
        m.ConstSources.push_back(std::move(cs));
        return Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::ConstantSourceType,
            idx,
            std::vector<size_t>{},
            std::vector<size_t>{outflowTypeId},
            tag,
            0.0
        );
    }

    ComponentIdAndWasteConnection
    Model_AddScheduleBasedSource(Model& m, std::vector<TimeAndAmount> const& xs)
    {
        return Model_AddScheduleBasedSource(
            m, xs, std::map<size_t, size_t>{}, 0, "", 0.0
        );
    }

    ComponentIdAndWasteConnection
    Model_AddScheduleBasedSource(
        Model& m,
        std::vector<TimeAndAmount> const& xs,
        std::map<size_t, size_t> const& scenarioIdToSourceId,
        size_t outflowId,
        std::string const& tag,
        double initialAge_s
    )
    {
        auto idx = m.ScheduledSrcs.size();
        ScheduleBasedSource sbs = {};
        sbs.TimeAndAvails = xs;
        sbs.ScenarioIdToSourceId = scenarioIdToSourceId;
        m.ScheduledSrcs.push_back(sbs);
        size_t wasteId = Component_AddComponentReturningId(
            m.ComponentMap, ComponentType::WasteSinkType, 0
        );
        size_t thisId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::ScheduleBasedSourceType,
            idx,
            std::vector<size_t>{},
            std::vector<size_t>{outflowId},
            tag,
            initialAge_s
        );
        auto wasteConn = Model_AddConnection(m, thisId, 1, wasteId, 0);
        return {thisId, wasteConn};
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
        std::string const& tag
    )
    {
        size_t idx = m.Muxes.size();
        Mux mux = {};
        mux.NumInports = numInports;
        mux.NumOutports = numOutports;
        mux.InflowConns = std::vector<size_t>(numInports, 0);
        mux.OutflowConns = std::vector<size_t>(numOutports, 0);
        mux.MaxOutflows_W = std::vector<flow_t>(numOutports, max_flow_W);
        m.Muxes.push_back(std::move(mux));
        std::vector<size_t> inflowTypes(numInports, flowId);
        std::vector<size_t> outflowTypes(numOutports, flowId);
        return Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::MuxType,
            idx,
            inflowTypes,
            outflowTypes,
            tag,
            0.0
        );
    }

    size_t
    Model_AddStore(
        Model& m,
        uint32_t capacity,
        uint32_t maxCharge,
        uint32_t maxDischarge,
        uint32_t chargeAmount,
        uint32_t initialStorage
    )
    {
        return Model_AddStore(
            m,
            capacity,
            maxCharge,
            maxDischarge,
            chargeAmount,
            initialStorage,
            0,
            ""
        );
    }

    size_t
    Model_AddStore(
        Model& m,
        flow_t capacity,
        flow_t maxCharge,
        flow_t maxDischarge,
        flow_t chargeAmount,
        flow_t initialStorage,
        size_t flowId,
        std::string const& tag
    )
    {
        assert(
            chargeAmount < capacity && "chargeAmount must be less than capacity"
        );
        assert(
            initialStorage <= capacity
            && "initialStorage must be less than or equal to capacity"
        );
        size_t idx = m.Stores.size();
        Store s = {};
        s.Capacity_J = capacity;
        s.MaxChargeRate_W = maxCharge;
        s.MaxDischargeRate_W = maxDischarge;
        s.ChargeAmount_J = chargeAmount;
        s.InitialStorage_J = initialStorage;
        s.RoundTripEfficiency = 1.0;
        m.Stores.push_back(std::move(s));
        return Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::StoreType,
            idx,
            std::vector<size_t>{flowId},
            std::vector<size_t>{flowId},
            tag,
            0.0
        );
    }

    ComponentIdAndWasteConnection
    Model_AddStoreWithWasteflow(
        Model& m,
        flow_t capacity,
        flow_t maxCharge,
        flow_t maxDischarge,
        flow_t chargeAmount,
        flow_t initialStorage,
        size_t flowId,
        double roundtripEfficiency,
        std::string const& tag
    )
    {
        assert(roundtripEfficiency > 0.0 && roundtripEfficiency <= 1.0);
        size_t id = Model_AddStore(
            m,
            capacity,
            maxCharge,
            maxDischarge,
            chargeAmount,
            initialStorage,
            flowId,
            tag
        );
        size_t storeIdx = m.ComponentMap.Idx[id];
        m.Stores[storeIdx].RoundTripEfficiency = roundtripEfficiency;
        size_t wasteId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::WasteSinkType,
            0,
            std::vector<size_t>{wasteflowId},
            std::vector<size_t>{},
            "",
            0.0
        );
        auto wasteConn = Model_AddConnection(m, id, 1, wasteId, 0, wasteflowId);
        return {
            .Id = id,
            .WasteConnection = std::move(wasteConn),
        };
    }

    ComponentIdAndWasteConnection
    Model_AddConstantEfficiencyConverter(
        Model& m,
        uint32_t eff_numerator,
        uint32_t eff_denominator
    )
    {
        return Model_AddConstantEfficiencyConverter(
            m, (double)eff_numerator / (double)eff_denominator
        );
    }

    ComponentIdAndWasteConnection
    Model_AddConstantEfficiencyConverter(Model& m, double efficiency)
    {
        return Model_AddConstantEfficiencyConverter(m, efficiency, 0, 0, 0, "");
    }

    ComponentIdAndWasteConnection
    Model_AddConstantEfficiencyConverter(
        Model& m,
        double efficiency,
        size_t inflowId,
        size_t outflowId,
        size_t lossflowId,
        std::string const& tag
    )
    {
        // NOTE: the 0th flowId is ""; the non-described flow
        std::vector<size_t> inflowIds{inflowId};
        std::vector<size_t> outflowIds{outflowId, lossflowId, wasteflowId};
        size_t idx = m.ConstEffConvs.size();
        ConstantEfficiencyConverter cec{
            .Efficiency = efficiency,
        };
        m.ConstEffConvs.push_back(std::move(cec));
        size_t wasteId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::WasteSinkType,
            0,
            std::vector<size_t>{wasteflowId},
            std::vector<size_t>{},
            "",
            0.0
        );
        size_t thisId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::ConstantEfficiencyConverterType,
            idx,
            inflowIds,
            outflowIds,
            tag,
            0.0
        );
        auto wasteConn =
            Model_AddConnection(m, thisId, 2, wasteId, 0, wasteflowId);
        return {thisId, wasteConn};
    }

    size_t
    Model_AddPassThrough(Model& m)
    {
        return Model_AddPassThrough(m, 0, "");
    }

    size_t
    Model_AddPassThrough(Model& m, size_t flowId, std::string const& tag)
    {
        size_t idx = m.PassThroughs.size();
        m.PassThroughs.push_back({
            .InflowConn = 0,
            .OutflowConn = 0,
            .MaxOutflow_W = max_flow_W,
        });
        return Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::PassThroughType,
            idx,
            std::vector<size_t>{flowId},
            std::vector<size_t>{flowId},
            tag,
            0.0
        );
    }

    Connection
    Model_AddConnection(
        Model& m,
        size_t fromId,
        size_t fromPort,
        size_t toId,
        size_t toPort
    )
    {
        return Model_AddConnection(m, fromId, fromPort, toId, toPort, 0);
    }

    Connection
    Model_AddConnection(
        Model& m,
        size_t fromId,
        size_t fromPort,
        size_t toId,
        size_t toPort,
        size_t flowId
    )
    {
        ComponentType fromType = m.ComponentMap.CompType[fromId];
        size_t fromIdx = m.ComponentMap.Idx[fromId];
        ComponentType toType = m.ComponentMap.CompType[toId];
        size_t toIdx = m.ComponentMap.Idx[toId];
        Connection c{};
        c.From = fromType;
        c.FromId = fromId;
        c.FromIdx = fromIdx;
        c.FromPort = fromPort;
        c.To = toType;
        c.ToId = toId;
        c.ToIdx = toIdx;
        c.ToPort = toPort;
        c.FlowTypeId = flowId;
        size_t connId = m.Connections.size();
        m.Connections.push_back(c);
        switch (fromType)
        {
            case ComponentType::PassThroughType:
            {
                m.PassThroughs[fromIdx].OutflowConn = connId;
            }
            break;
            case ComponentType::ConstantSourceType:
            {
                m.ConstSources[fromIdx].OutflowConn = connId;
            }
            break;
            case ComponentType::ScheduleBasedSourceType:
            {
                switch (fromPort)
                {
                    case 0:
                    {
                        m.ScheduledSrcs[fromIdx].OutflowConn = connId;
                    }
                    break;
                    case 1:
                    {
                        m.ScheduledSrcs[fromIdx].WasteflowConn = connId;
                    }
                    break;
                    default:
                    {
                        throw std::invalid_argument{
                            "invalid outport for schedule-based source"
                        };
                    }
                    break;
                }
            }
            break;
            case ComponentType::ConstantEfficiencyConverterType:
            {
                switch (fromPort)
                {
                    case 0:
                    {
                        m.ConstEffConvs[fromIdx].OutflowConn = connId;
                    }
                    break;
                    case 1:
                    {
                        m.ConstEffConvs[fromIdx].LossflowConn = connId;
                    }
                    break;
                    case 2:
                    {
                        m.ConstEffConvs[fromIdx].WasteflowConn = connId;
                    }
                    break;
                    default:
                    {
                        throw std::invalid_argument(
                            "Unhandled constant efficiency converter outport"
                        );
                    }
                }
            }
            break;
            case ComponentType::MoverType:
            {
                switch (fromPort)
                {
                    case 0: // outflow
                    {
                        m.Movers[fromIdx].OutflowConn = connId;
                    }
                    break;
                    case 1: // wasteflow
                    {
                        m.Movers[fromIdx].WasteflowConn = connId;
                    }
                    break;
                    default:
                    {
                        WriteErrorMessage("<network>", "bad port for Mover");
                        std::exit(1);
                    }
                    break;
                }
            }
            break;
            case ComponentType::MuxType:
            {
                m.Muxes[fromIdx].OutflowConns[fromPort] = connId;
            }
            break;
            case ComponentType::StoreType:
            {
                switch (fromPort)
                {
                    case 0:
                    {
                        m.Stores[fromIdx].OutflowConn = connId;
                    }
                    break;
                    case 1:
                    {
                        m.Stores[fromIdx].WasteflowConn = connId;
                    }
                    break;
                    default:
                    {
                        WriteErrorMessage(
                            "store port", "unhandled port for store"
                        );
                        std::exit(1);
                    }
                }
            }
            break;
            case ComponentType::EnvironmentSourceType:
            {
                // NOTE: do nothing
            }
            break;
            default:
            {
                WriteErrorMessage(
                    "Model_AddConnection",
                    "unhandled component type: " + ToString(fromType)
                );
                std::exit(1);
            }
        }
        switch (toType)
        {
            case ComponentType::PassThroughType:
            {
                m.PassThroughs[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::ConstantLoadType:
            {
                m.ConstLoads[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::ConstantEfficiencyConverterType:
            {
                m.ConstEffConvs[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::MoverType:
            {
                switch (toPort)
                {
                    case 0:
                    {
                        m.Movers[toIdx].InflowConn = connId;
                    }
                    break;
                    case 1:
                    {
                        m.Movers[toIdx].InFromEnvConn = connId;
                    }
                    break;
                    default:
                    {
                        WriteErrorMessage(
                            "<network>", "bad network connection for mover"
                        );
                        std::exit(1);
                    }
                    break;
                }
            }
            break;
            case ComponentType::MuxType:
            {
                m.Muxes[toIdx].InflowConns[toPort] = connId;
            }
            break;
            case ComponentType::StoreType:
            {
                m.Stores[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::ScheduleBasedLoadType:
            {
                m.ScheduledLoads[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::WasteSinkType:
            {
                // NOTE: do nothing
            }
            break;
            default:
            {
                WriteErrorMessage(
                    "Model_AddConnection",
                    "unhandled component type: " + ToString(toType)
                );
            }
            break;
        }
        return c;
    }

    bool
    SameConnection(Connection a, Connection b)
    {
        return a.From == b.From && a.FromIdx == b.FromIdx
            && a.FromPort == b.FromPort && a.To == b.To && a.ToIdx == b.ToIdx
            && a.ToPort == b.ToPort;
    }

    std::optional<Flow>
    ModelResults_GetFlowForConnection(
        Model const& m,
        Connection conn,
        double time,
        std::vector<TimeAndFlows> timeAndFlows
    )
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
        std::vector<TimeAndFlows> timeAndFlows
    )
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
                && storeIdx < timeAndFlows[i].StorageAmounts_J.size())
            {
                return timeAndFlows[i].StorageAmounts_J[storeIdx];
            }
        }
        return {};
    }

    ScenarioOccurrenceStats
    ModelResults_CalculateScenarioOccurrenceStats(
        size_t scenarioId,
        size_t occurrenceNumber,
        Model const& m,
        std::vector<TimeAndFlows> const& timeAndFlows
    )
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
                 connId < timeAndFlows[eventIdx - 1].Flows.size();
                 ++connId)
            {
                auto fromType =
                    m.ComponentMap.CompType[m.Connections[connId].FromId];
                auto toType =
                    m.ComponentMap.CompType[m.Connections[connId].ToId];
                auto const& flow = timeAndFlows[eventIdx - 1].Flows[connId];
                switch (fromType)
                {
                    case ComponentType::ConstantSourceType:
                    case ComponentType::ScheduleBasedSourceType:
                    {
                        sos.Inflow_kJ += (flow.Actual_W / W_per_kW) * dt;
                    }
                    break;
                    case ComponentType::EnvironmentSourceType:
                    {
                        sos.InFromEnv_kJ += (flow.Actual_W / W_per_kW) * dt;
                    }
                    break;
                    default:
                    {
                    }
                    break;
                }
                switch (toType)
                {
                    case ComponentType::ConstantLoadType:
                    case ComponentType::ScheduleBasedLoadType:
                    {
                        sos.OutflowAchieved_kJ +=
                            (flow.Actual_W / W_per_kW) * dt;
                        sos.OutflowRequest_kJ +=
                            (flow.Requested_W / W_per_kW) * dt;
                        allLoadsMet =
                            allLoadsMet && (flow.Actual_W == flow.Requested_W);
                        if (flow.Actual_W != flow.Requested_W)
                        {
                            sos.LoadNotServed_kJ +=
                                ((flow.Requested_W - flow.Actual_W) / W_per_kW)
                                * dt;
                        }
                    }
                    break;
                    case ComponentType::WasteSinkType:
                    {
                        sos.Wasteflow_kJ += (flow.Actual_W / W_per_kW) * dt;
                    }
                    break;
                    default:
                    {
                    }
                    break;
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
                 storeIdx < timeAndFlows[eventIdx].StorageAmounts_J.size();
                 ++storeIdx)
            {
                double stored_J =
                    static_cast<double>(
                        timeAndFlows[eventIdx].StorageAmounts_J[storeIdx]
                    )
                    - static_cast<double>(
                        timeAndFlows[eventIdx - 1].StorageAmounts_J[storeIdx]
                    );
                if (stored_J > 0.0)
                {
                    sos.StorageCharge_kJ += stored_J / J_per_kJ;
                }
                else
                {
                    sos.StorageDischarge_kJ += -1.0 * (stored_J / J_per_kJ);
                }
            }
        }
        if (sedt > sos.MaxSEDT_s)
        {
            sos.MaxSEDT_s = sedt;
        }
        // calculate availability using reliability schedules
        std::vector<TimeState> relSch;
        for (size_t i = 0; i < m.Reliabilities.size(); ++i)
        {
            relSch = TimeState_Combine(relSch, m.Reliabilities[i].TimeStates);
        }
        TimeState_CountAndTimeFailureEvents(
            relSch,
            m.FinalTime,
            sos.EventCountByFailureModeId,
            sos.EventCountByFragilityModeId,
            sos.TimeByFailureModeId_s,
            sos.TimeByFragilityModeId_s
        );
        sos.Availability_s = TimeState_CalcAvailability_s(relSch, m.FinalTime);
        std::map<size_t, std::vector<TimeState>> relSchByCompId;
        for (size_t i = 0; i < m.Reliabilities.size(); ++i)
        {
            ScheduleBasedReliability const& sbr = m.Reliabilities[i];
            relSchByCompId[sbr.ComponentId] = sbr.TimeStates;
        }
        for (size_t compId = 0; compId < m.ComponentMap.Tag.size(); ++compId)
        {
            if (!sos.EventCountByCompIdByFailureModeId.contains(compId))
            {
                sos.EventCountByCompIdByFailureModeId[compId] =
                    std::map<size_t, size_t>{};
            }
            if (!sos.EventCountByCompIdByFragilityModeId.contains(compId))
            {
                sos.EventCountByCompIdByFragilityModeId[compId] =
                    std::map<size_t, size_t>{};
            }
            if (!sos.TimeByCompIdByFailureModeId_s.contains(compId))
            {
                sos.TimeByCompIdByFailureModeId_s[compId] =
                    std::map<size_t, double>{};
            }
            if (!sos.TimeByCompIdByFragilityModeId_s.contains(compId))
            {
                sos.TimeByCompIdByFragilityModeId_s[compId] =
                    std::map<size_t, double>{};
            }
            if (relSchByCompId.contains(compId))
            {
                TimeState_CountAndTimeFailureEvents(
                    relSchByCompId[compId],
                    m.FinalTime,
                    sos.EventCountByCompIdByFailureModeId[compId],
                    sos.EventCountByCompIdByFragilityModeId[compId],
                    sos.TimeByCompIdByFailureModeId_s[compId],
                    sos.TimeByCompIdByFragilityModeId_s[compId]
                );
                sos.AvailabilityByCompId_s[compId] =
                    TimeState_CalcAvailability_s(
                        relSchByCompId[compId], m.FinalTime
                    );
            }
            else
            {
                // NOTE: if there is no reliability schedule,
                // availability is 100% and there are no failure times or events
                // to count/sum.
                sos.AvailabilityByCompId_s[compId] = m.FinalTime;
            }
        }
        return sos;
    }

    std::optional<TagAndPort>
    ParseTagAndPort(std::string const& s, std::string const& tableName)
    {
        std::string tag = s.substr(0, s.find(":"));
        size_t opening = s.find("(");
        size_t closing = s.find(")");
        if (opening == std::string::npos || closing == std::string::npos)
        {
            std::cout << "[" << tableName << "] "
                      << "unable to parse connection string '" << s << "'"
                      << std::endl;
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
            std::cout << "[network] " << "required key 'connections' missing"
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
                std::cout << "[network] " << "'connections' at index " << i
                          << " must be an array" << std::endl;
                return Result::Failure;
            }
            if (item.as_array().size() < 3)
            {
                std::cout << "[network] " << "'connections' at index " << i
                          << " must be an array of length >= 3" << std::endl;
                return Result::Failure;
            }
            for (int idx = 0; idx < 3; ++idx)
            {
                if (!item.as_array()[idx].is_string())
                {
                    std::cout << "[network] " << "'connections' at index " << i
                              << " and subindex " << idx << " must be a string"
                              << std::endl;
                    return Result::Failure;
                }
            }
            std::string from = item.as_array()[0].as_string();
            std::optional<TagAndPort> maybeFromTap =
                ParseTagAndPort(from, "network");
            if (!maybeFromTap.has_value())
            {
                std::cout << "[network] "
                          << "unable to parse connection string at [" << i
                          << "][0]" << std::endl;
                return Result::Failure;
            }
            TagAndPort fromTap = maybeFromTap.value();
            std::string to = item.as_array()[1].as_string();
            std::optional<TagAndPort> maybeToTap =
                ParseTagAndPort(to, "network");
            if (!maybeToTap.has_value())
            {
                std::cout << "[network] "
                          << "unable to parse connection string at [" << i
                          << "][1]" << std::endl;
                return Result::Failure;
            }
            TagAndPort toTap = maybeToTap.value();
            std::string flow = item.as_array()[2].as_string();
            std::optional<size_t> maybeFlowTypeId =
                FlowDict_GetIdByTag(fd, flow);
            if (!maybeFlowTypeId.has_value())
            {
                std::cout << "[network] " << "could not identify flow type '"
                          << flow << "'" << std::endl;
                return Result::Failure;
            }
            size_t flowTypeId = maybeFlowTypeId.value();
            std::optional<size_t> maybeFromCompId =
                Model_FindCompIdByTag(m, fromTap.Tag);
            if (!maybeFromCompId.has_value())
            {
                std::cout << "[network] "
                          << "could not find component id for tag '" << from
                          << "'" << std::endl;
                return Result::Failure;
            }
            std::optional<size_t> maybeToCompId =
                Model_FindCompIdByTag(m, toTap.Tag);
            if (!maybeToCompId.has_value())
            {
                std::cout << "[network] "
                          << "could not find component id for tag '" << to
                          << "'" << std::endl;
                return Result::Failure;
            }
            size_t fromCompId = maybeFromCompId.value();
            size_t toCompId = maybeToCompId.value();
            if (fromTap.Port >= m.ComponentMap.OutflowType[fromCompId].size())
            {
                std::cout << "[network] " << "port is unaddressable for "
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
                std::ostringstream oss;
                oss << "mismatch of flow types: " << fromTap.Tag << ":outflow="
                    << fd.Type[m.ComponentMap
                                   .OutflowType[fromCompId][fromTap.Port]]
                    << "; connection: " << flow;
                WriteErrorMessage("network", oss.str());
                return Result::Failure;
            }
            if (toTap.Port >= m.ComponentMap.InflowType[toCompId].size())
            {
                std::cout << "[network] " << "port is unaddressable for "
                          << ToString(m.ComponentMap.CompType[toCompId])
                          << ": trying to address " << toTap.Port
                          << " but only "
                          << m.ComponentMap.InflowType[toCompId].size()
                          << " ports available" << std::endl;
                return Result::Failure;
            }
            if (m.ComponentMap.InflowType[toCompId][toTap.Port] != flowTypeId)
            {
                std::cout
                    << "[network] " << "mismatch of flow types: " << toTap.Tag
                    << ":inflow="
                    << fd.Type[m.ComponentMap.OutflowType[toCompId][toTap.Port]]
                    << "; connection: " << flow << std::endl;
                return Result::Failure;
            }
            Model_AddConnection(
                m, fromCompId, fromTap.Port, toCompId, toTap.Port, flowTypeId
            );
        }
        return Result::Success;
    }

    std::optional<size_t>
    Model_FindCompIdByTag(Model const& m, std::string const& tag)
    {
        for (size_t i = 0; i < m.ComponentMap.Tag.size(); ++i)
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
        for (size_t idx = 0; idx < fd.Type.size(); ++idx)
        {
            if (fd.Type[idx] == tag)
            {
                return idx;
            }
        }
        return {};
    }

    std::string
    ConnectionToString(
        ComponentDict const& cd,
        Connection const& c,
        bool compact
    )
    {
        std::string fromTag = cd.Tag[c.FromId];
        if (fromTag.empty() && c.From == ComponentType::WasteSinkType)
        {
            fromTag = "WASTE";
        }
        else if (fromTag.empty()
                 && c.From == ComponentType::EnvironmentSourceType)
        {
            fromTag = "ENV";
        }
        std::string toTag = cd.Tag[c.ToId];
        if (toTag.empty() && c.To == ComponentType::WasteSinkType)
        {
            toTag = "WASTE";
        }
        else if (toTag.empty() && c.To == ComponentType::EnvironmentSourceType)
        {
            toTag = "ENV";
        }
        std::ostringstream oss{};
        oss << fromTag
            << (compact ? "" : ("[" + std::to_string(c.FromId) + "]"))
            << ":OUT(" << c.FromPort << ")"
            << (compact ? "" : (": " + ToString(c.From))) << " => " << toTag
            << (compact ? "" : ("[" + std::to_string(c.ToId) + "]")) << ":IN("
            << c.ToPort << ")" << (compact ? "" : (": " + ToString(c.To)));
        return oss.str();
    }

    std::string
    ConnectionToString(
        ComponentDict const& cd,
        FlowDict const& fd,
        Connection const& c,
        bool compact
    )
    {
        std::ostringstream oss{};
        oss << ConnectionToString(cd, c, compact)
            << "; flow: " << fd.Type[c.FlowTypeId];
        return oss.str();
    }

    // TODO: extract connection printing
    void
    Model_PrintConnections(Model const& m, FlowDict const& ft)
    {
        for (size_t i = 0; i < m.Connections.size(); ++i)
        {
            std::cout << i << ": "
                      << ConnectionToString(
                             m.ComponentMap, ft, m.Connections[i]
                         )
                      << std::endl;
        }
    }

    std::ostream&
    operator<<(std::ostream& os, Flow const& flow)
    {
        os << "Flow{Requested_W=" << flow.Requested_W << ", "
           << "Available_W=" << flow.Available_W << ", "
           << "Actual_W=" << flow.Actual_W << "}";
        return os;
    }

    double
    Interpolate1d(double x, double x0, double y0, double x1, double y1)
    {
        if (x < x0)
        {
            return y0;
        }
        if (x > x1)
        {
            return y1;
        }
        double dx = x1 - x0;
        double dy = y1 - y0;
        double x_ = x - x0;
        if (dx > 0.0)
        {
            return y0 + (x_ * (dy / dx));
        }
        return 0.0;
    }

    double
    LinearFragilityCurve_GetFailureFraction(
        LinearFragilityCurve lfc,
        double intensityLevel
    )
    {
        return Interpolate1d(
            intensityLevel, lfc.LowerBound, 0.0, lfc.UpperBound, 1.0
        );
    }

    double
    TabularFragilityCurve_GetFailureFraction(
        TabularFragilityCurve tfc,
        double intensityLevel
    )
    {
        size_t size = tfc.Intensities.size();
        assert(size == tfc.FailureFractions.size());
        assert(size > 0);
        if (intensityLevel <= tfc.Intensities[0])
        {
            return tfc.FailureFractions[0];
        }
        if (intensityLevel >= tfc.Intensities[size - 1])
        {
            return tfc.FailureFractions[size - 1];
        }
        for (size_t i = 0; i < size; ++i)
        {
            if (intensityLevel == tfc.Intensities[i])
            {
                return tfc.FailureFractions[i];
            }
            if ((i + 1) < size && intensityLevel > tfc.Intensities[i]
                && intensityLevel <= tfc.Intensities[i + 1])
            {
                return Interpolate1d(
                    intensityLevel,
                    tfc.Intensities[i],
                    tfc.FailureFractions[i],
                    tfc.Intensities[i + 1],
                    tfc.FailureFractions[i + 1]
                );
            }
        }
        return 0.0;
    }

    void
    ComponentDict_SetInitialAge(ComponentDict& cd, size_t id, double age_s)
    {
        assert(id < cd.CompType.size());
        cd.InitialAges_s[id] = age_s;
    }

} // namespace erin
