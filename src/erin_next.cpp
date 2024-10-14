/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next.h"
#include "erin/logging.h"
#include "erin_next/erin_next_lookup_table.h"
#include "erin_next/erin_next_time_and_amount.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_utils.h"
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <numeric>
#include <string>
#include <unordered_set>
#include <utility>
#include <fmt/core.h>

namespace erin
{

    void
    AddConnectionIssue(
        std::vector<std::string>& issues,
        std::string componentTag,
        size_t compId,
        size_t compPort,
        size_t compSubtypeIdx,
        ComponentType compType,
        Connection const& conn,
        size_t connIdx,
        FlowDirection flowDirection
    )
    {
        std::string direction =
            flowDirection == FlowDirection::Outflow ? "outflow" : "inflow ";
        std::ostringstream oss;
        oss << "Inconsistent Connection\n";
        oss << "- ACCORDING TO THE COMPONENT:\n";
        oss << "  - component id:            " << compId << "\n";
        oss << "  - component " << direction << " port:  " << compPort << "\n";
        oss << "  - component type:          " << ToString(compType) << "\n";
        oss << "  - component tag:           " << componentTag << "\n";
        oss << "  - component subtype index: " << compSubtypeIdx << "\n";
        oss << "- ACCORDING TO THE " << direction << " CONNECTION:\n";
        oss << "  - connection id:  " << connIdx << "\n";
        oss << "  - component type: "
            << ToString(
                   flowDirection == FlowDirection::Outflow ? conn.From : conn.To
               )
            << "\n";
        oss << "  - component id:   "
            << (flowDirection == FlowDirection::Outflow ? conn.FromId
                                                        : conn.ToId)
            << "\n";
        oss << "  - port:           "
            << (flowDirection == FlowDirection::Outflow ? conn.FromPort
                                                        : conn.ToPort)
            << "\n";
        oss << "  - subtype index:  "
            << (flowDirection == FlowDirection::Outflow ? conn.FromIdx
                                                        : conn.ToIdx)
            << "\n";
        if (connIdx == 0)
        {
            oss << "NOTE: Since the connection ID is 0 (initialization), "
                << "this could indicate that '" << componentTag
                << "' is declared but not hooked up to the network.";
        }
        issues.push_back(oss.str());
    }

    std::vector<std::string>
    Model_CheckNetwork(Model const& m)
    {
        std::vector<std::string> issues;
        std::unordered_set<std::string> connectedOutflowPorts;
        std::unordered_set<std::string> connectedInflowPorts;
        std::unordered_set<std::string> compTags;
        {
            for (auto const& tag : m.ComponentMap.Tag)
            {
                if (!tag.empty() && compTags.contains(tag))
                {
                    issues.push_back(
                        "multiple components with name '" + tag + "'"
                    );
                }
                compTags.insert(tag);
            }
        }
        assert(
            m.ComponentMap.CompType.size() == m.ComponentMap.InflowType.size()
        );
        assert(
            m.ComponentMap.CompType.size() == m.ComponentMap.OutflowType.size()
        );
        assert(m.ComponentMap.CompType.size() == m.ComponentMap.Idx.size());
        assert(
            m.ComponentMap.CompType.size()
            == m.ComponentMap.InitialAges_s.size()
        );
        assert(m.ComponentMap.CompType.size() == m.ComponentMap.Report.size());
        assert(m.ComponentMap.CompType.size() == m.ComponentMap.Tag.size());
        std::unordered_map<size_t, std::set<size_t>> muxCompIdToInflowConns;
        std::unordered_map<size_t, std::set<size_t>> muxCompIdToOutflowConns;
        for (size_t compId = 0; compId < m.ComponentMap.CompType.size();
             ++compId)
        {
            assert(compId < m.ComponentMap.CompType.size());
            if (m.ComponentMap.CompType[compId] == ComponentType::MuxType)
            {
                muxCompIdToInflowConns[compId] = std::set<size_t>{};
                muxCompIdToOutflowConns[compId] = std::set<size_t>{};
            }
            ComponentType compType = m.ComponentMap.CompType[compId];
            size_t idx = m.ComponentMap.Idx[compId];
            std::string const& tag = m.ComponentMap.Tag[compId];
            size_t nConns = m.Connections.size();
            ignore(nConns);
            switch (compType)
            {
                case ComponentType::ConstantEfficiencyConverterType:
                {
                    assert(idx < m.ConstEffConvs.size());
                    ConstantEfficiencyConverter const& cec =
                        m.ConstEffConvs[idx];
                    size_t inflowConnIdx = cec.InflowConn;
                    assert(inflowConnIdx < nConns);
                    Connection const& inflowConn = m.Connections[inflowConnIdx];
                    size_t inflowPort = 0;
                    if ((inflowConn.To != compType)
                        || (inflowConn.ToId != compId)
                        || (inflowConn.ToIdx != idx)
                        || (inflowConn.ToPort != inflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            inflowPort,
                            idx,
                            compType,
                            inflowConn,
                            inflowConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                    size_t outflowConnIdx = cec.OutflowConn;
                    assert(outflowConnIdx < nConns);
                    Connection const& outflowConn =
                        m.Connections[outflowConnIdx];
                    size_t outflowPort = 0;
                    if ((outflowConn.From != compType)
                        || (outflowConn.FromId != compId)
                        || (outflowConn.FromIdx != idx)
                        || (outflowConn.FromPort != outflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            outflowPort,
                            idx,
                            compType,
                            outflowConn,
                            outflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                    size_t wasteflowConnIdx = cec.WasteflowConn;
                    assert(wasteflowConnIdx < nConns);
                    Connection const& wfConn = m.Connections[wasteflowConnIdx];
                    size_t wfPort = 2;
                    if ((wfConn.From != compType) || (wfConn.FromId != compId)
                        || (wfConn.FromIdx != idx)
                        || (wfConn.FromPort != wfPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            wfPort,
                            idx,
                            compType,
                            wfConn,
                            wasteflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                    if (cec.LossflowConn.has_value())
                    {
                        size_t lfConnIdx = cec.LossflowConn.value();
                        assert(lfConnIdx < nConns);
                        Connection const& lfConn = m.Connections[lfConnIdx];
                        size_t lfPort = 1;
                        if ((lfConn.From != compType)
                            || (lfConn.FromId != compId)
                            || (lfConn.FromIdx != idx)
                            || (lfConn.FromPort != lfPort))
                        {
                            AddConnectionIssue(
                                issues,
                                tag,
                                compId,
                                lfPort,
                                idx,
                                compType,
                                lfConn,
                                lfConnIdx,
                                FlowDirection::Outflow
                            );
                        }
                    }
                }
                break;
                case ComponentType::VariableEfficiencyConverterType:
                {
                    assert(idx < m.VarEffConvs.size());
                    VariableEfficiencyConverter const& vec = m.VarEffConvs[idx];
                    size_t inflowConnIdx = vec.InflowConn;
                    assert(inflowConnIdx < nConns);
                    Connection const& inflowConn = m.Connections[inflowConnIdx];
                    size_t inflowPort = 0;
                    if ((inflowConn.To != compType)
                        || (inflowConn.ToId != compId)
                        || (inflowConn.ToIdx != idx)
                        || (inflowConn.ToPort != inflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            inflowPort,
                            idx,
                            compType,
                            inflowConn,
                            inflowConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                    size_t outflowConnIdx = vec.OutflowConn;
                    assert(outflowConnIdx < nConns);
                    Connection const& outflowConn =
                        m.Connections[outflowConnIdx];
                    size_t outflowPort = 0;
                    if ((outflowConn.From != compType)
                        || (outflowConn.FromId != compId)
                        || (outflowConn.FromIdx != idx)
                        || (outflowConn.FromPort != outflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            outflowPort,
                            idx,
                            compType,
                            outflowConn,
                            outflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                    size_t wasteflowConnIdx = vec.WasteflowConn;
                    assert(wasteflowConnIdx < nConns);
                    Connection const& wfConn = m.Connections[wasteflowConnIdx];
                    size_t wfPort = 2;
                    if ((wfConn.From != compType) || (wfConn.FromId != compId)
                        || (wfConn.FromIdx != idx)
                        || (wfConn.FromPort != wfPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            wfPort,
                            idx,
                            compType,
                            wfConn,
                            wasteflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                    if (vec.LossflowConn.has_value())
                    {
                        size_t lfConnIdx = vec.LossflowConn.value();
                        assert(lfConnIdx < nConns);
                        Connection const& lfConn = m.Connections[lfConnIdx];
                        size_t lfPort = 1;
                        if ((lfConn.From != compType)
                            || (lfConn.FromId != compId)
                            || (lfConn.FromIdx != idx)
                            || (lfConn.FromPort != lfPort))
                        {
                            AddConnectionIssue(
                                issues,
                                tag,
                                compId,
                                lfPort,
                                idx,
                                compType,
                                lfConn,
                                lfConnIdx,
                                FlowDirection::Outflow
                            );
                        }
                    }
                }
                break;
                case ComponentType::ConstantLoadType:
                {
                    assert(idx < m.ConstLoads.size());
                    ConstantLoad const& comp = m.ConstLoads[idx];
                    size_t inflowConnIdx = comp.InflowConn;
                    Connection const& inflowConn = m.Connections[inflowConnIdx];
                    size_t inflowPort = 0;
                    if ((inflowConn.To != compType)
                        || (inflowConn.ToId != compId)
                        || (inflowConn.ToIdx != idx)
                        || (inflowConn.ToPort != inflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            inflowPort,
                            idx,
                            compType,
                            inflowConn,
                            inflowConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                }
                break;
                case ComponentType::ConstantSourceType:
                {
                    assert(idx < m.ConstSources.size());
                    ConstantSource const& comp = m.ConstSources[idx];
                    size_t outflowConnIdx = comp.OutflowConn;
                    assert(outflowConnIdx < nConns);
                    Connection const& outflowConn =
                        m.Connections[outflowConnIdx];
                    size_t outflowPort = 0;
                    if ((outflowConn.From != compType)
                        || (outflowConn.FromId != compId)
                        || (outflowConn.FromIdx != idx)
                        || (outflowConn.FromPort != outflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            outflowPort,
                            idx,
                            compType,
                            outflowConn,
                            outflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                }
                break;
                case ComponentType::EnvironmentSourceType:
                {
                    // pass
                }
                break;
                case ComponentType::MoverType:
                {
                    assert(idx < m.Movers.size());
                    Mover const& comp = m.Movers[idx];
                    size_t inflowConnIdx = comp.InflowConn;
                    assert(inflowConnIdx < nConns);
                    Connection const& inflowConn = m.Connections[inflowConnIdx];
                    size_t inflowPort = 0;
                    if ((inflowConn.To != compType)
                        || (inflowConn.ToId != compId)
                        || (inflowConn.ToIdx != idx)
                        || (inflowConn.ToPort != inflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            inflowPort,
                            idx,
                            compType,
                            inflowConn,
                            inflowConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                    size_t outflowConnIdx = comp.OutflowConn;
                    assert(outflowConnIdx < nConns);
                    Connection const& outflowConn =
                        m.Connections[outflowConnIdx];
                    size_t outflowPort = 0;
                    if ((outflowConn.From != compType)
                        || (outflowConn.FromId != compId)
                        || (outflowConn.FromIdx != idx)
                        || (outflowConn.FromPort != outflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            outflowPort,
                            idx,
                            compType,
                            outflowConn,
                            outflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                    size_t envConnIdx = comp.InFromEnvConn;
                    assert(envConnIdx < nConns);
                    Connection const& envConn = m.Connections[envConnIdx];
                    size_t envInflowPort = 1;
                    if ((envConn.To != compType) || (envConn.ToId != compId)
                        || (envConn.ToIdx != idx)
                        || (envConn.ToPort != envInflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            envInflowPort,
                            idx,
                            compType,
                            envConn,
                            envConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                    size_t wConnIdx = comp.WasteflowConn;
                    assert(wConnIdx < nConns);
                    Connection const& wConn = m.Connections[wConnIdx];
                    size_t wasteOutflowPort = 1;
                    if ((wConn.From != compType) || (wConn.FromId != compId)
                        || (wConn.FromIdx != idx)
                        || (wConn.FromPort != wasteOutflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            wasteOutflowPort,
                            idx,
                            compType,
                            wConn,
                            wConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                }
                break;
                case ComponentType::VariableEfficiencyMoverType:
                {
                    assert(idx < m.VarEffMovers.size());
                    VariableEfficiencyMover const& comp = m.VarEffMovers[idx];
                    size_t inflowConnIdx = comp.InflowConn;
                    assert(inflowConnIdx < nConns);
                    Connection const& inflowConn = m.Connections[inflowConnIdx];
                    size_t inflowPort = 0;
                    if ((inflowConn.To != compType)
                        || (inflowConn.ToId != compId)
                        || (inflowConn.ToIdx != idx)
                        || (inflowConn.ToPort != inflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            inflowPort,
                            idx,
                            compType,
                            inflowConn,
                            inflowConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                    size_t outflowConnIdx = comp.OutflowConn;
                    assert(outflowConnIdx < nConns);
                    Connection const& outflowConn =
                        m.Connections[outflowConnIdx];
                    size_t outflowPort = 0;
                    if ((outflowConn.From != compType)
                        || (outflowConn.FromId != compId)
                        || (outflowConn.FromIdx != idx)
                        || (outflowConn.FromPort != outflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            outflowPort,
                            idx,
                            compType,
                            outflowConn,
                            outflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                    size_t envConnIdx = comp.InFromEnvConn;
                    assert(envConnIdx < nConns);
                    Connection const& envConn = m.Connections[envConnIdx];
                    size_t envInflowPort = 1;
                    if ((envConn.To != compType) || (envConn.ToId != compId)
                        || (envConn.ToIdx != idx)
                        || (envConn.ToPort != envInflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            envInflowPort,
                            idx,
                            compType,
                            envConn,
                            envConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                    size_t wConnIdx = comp.WasteflowConn;
                    assert(wConnIdx < nConns);
                    Connection const& wConn = m.Connections[wConnIdx];
                    size_t wasteOutflowPort = 1;
                    if ((wConn.From != compType) || (wConn.FromId != compId)
                        || (wConn.FromIdx != idx)
                        || (wConn.FromPort != wasteOutflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            wasteOutflowPort,
                            idx,
                            compType,
                            wConn,
                            wConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                }
                break;
                case ComponentType::MuxType:
                {
                    assert(idx < m.Muxes.size());
                    Mux const& comp = m.Muxes[idx];
                    for (size_t inPort = 0; inPort < comp.NumInports; ++inPort)
                    {
                        size_t inflowConnIdx = comp.InflowConns[inPort];
                        Connection const& inflowConn =
                            m.Connections[inflowConnIdx];
                        if ((inflowConn.To != compType)
                            || (inflowConn.ToId != compId)
                            || (inflowConn.ToIdx != idx)
                            || (inflowConn.ToPort != inPort))
                        {
                            AddConnectionIssue(
                                issues,
                                tag,
                                compId,
                                inPort,
                                idx,
                                compType,
                                inflowConn,
                                inflowConnIdx,
                                FlowDirection::Inflow
                            );
                        }
                    }
                    for (size_t outPort = 0; outPort < comp.NumOutports;
                         ++outPort)
                    {
                        size_t outflowConnIdx = comp.OutflowConns[outPort];
                        Connection const& outflowConn =
                            m.Connections[outflowConnIdx];
                        if ((outflowConn.From != compType)
                            || (outflowConn.FromId != compId)
                            || (outflowConn.FromIdx != idx)
                            || (outflowConn.FromPort != outPort))
                        {
                            AddConnectionIssue(
                                issues,
                                tag,
                                compId,
                                outPort,
                                idx,
                                compType,
                                outflowConn,
                                outflowConnIdx,
                                FlowDirection::Outflow
                            );
                        }
                    }
                }
                break;
                case ComponentType::PassThroughType:
                {
                    assert(idx < m.PassThroughs.size());
                    PassThrough const& comp = m.PassThroughs[idx];
                    size_t inflowConnIdx = comp.InflowConn;
                    Connection const& inflowConn = m.Connections[inflowConnIdx];
                    size_t inflowPort = 0;
                    if ((inflowConn.To != compType)
                        || (inflowConn.ToId != compId)
                        || (inflowConn.ToIdx != idx)
                        || (inflowConn.ToPort != inflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            inflowPort,
                            idx,
                            compType,
                            inflowConn,
                            inflowConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                    size_t outflowConnIdx = comp.OutflowConn;
                    Connection const& outflowConn =
                        m.Connections[outflowConnIdx];
                    size_t outflowPort = 0;
                    if ((outflowConn.From != compType)
                        || (outflowConn.FromId != compId)
                        || (outflowConn.FromIdx != idx)
                        || (outflowConn.FromPort != outflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            outflowPort,
                            idx,
                            compType,
                            outflowConn,
                            outflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                }
                break;
                case ComponentType::SwitchType:
                {
                    assert(idx < m.Switches.size());
                    Switch const& comp = m.Switches[idx];
                    size_t primaryInflowConnIdx = comp.InflowConnPrimary;
                    Connection const& primaryInflowConn = m.Connections[primaryInflowConnIdx];
                    size_t primaryInflowPort = 0;
                    if ((primaryInflowConn.To != compType)
                        || (primaryInflowConn.ToId != compId)
                        || (primaryInflowConn.ToIdx != idx)
                        || (primaryInflowConn.ToPort != primaryInflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            primaryInflowPort,
                            idx,
                            compType,
                            primaryInflowConn,
                            primaryInflowConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                    size_t secondaryInflowConnIdx = comp.InflowConnSecondary;
                    Connection const& secondaryInflowConn = m.Connections[secondaryInflowConnIdx];
                    size_t secondaryInflowPort = 1;
                    if ((secondaryInflowConn.To != compType)
                        || (secondaryInflowConn.ToId != compId)
                        || (secondaryInflowConn.ToIdx != idx)
                        || (secondaryInflowConn.ToPort != secondaryInflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            secondaryInflowPort,
                            idx,
                            compType,
                            secondaryInflowConn,
                            secondaryInflowConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                    size_t outflowConnIdx = comp.OutflowConn;
                    Connection const& outflowConn =
                        m.Connections[outflowConnIdx];
                    size_t outflowPort = 0;
                    if ((outflowConn.From != compType)
                        || (outflowConn.FromId != compId)
                        || (outflowConn.FromIdx != idx)
                        || (outflowConn.FromPort != outflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            outflowPort,
                            idx,
                            compType,
                            outflowConn,
                            outflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                }
                break;
                case ComponentType::ScheduleBasedLoadType:
                {
                    assert(idx < m.ScheduledLoads.size());
                    ScheduleBasedLoad const& comp = m.ScheduledLoads[idx];
                    size_t inflowConnIdx = comp.InflowConn;
                    Connection const& inflowConn = m.Connections[inflowConnIdx];
                    size_t inflowPort = 0;
                    if ((inflowConn.To != compType)
                        || (inflowConn.ToId != compId)
                        || (inflowConn.ToIdx != idx)
                        || (inflowConn.ToPort != inflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            inflowPort,
                            idx,
                            compType,
                            inflowConn,
                            inflowConnIdx,
                            FlowDirection::Inflow
                        );
                    }
                }
                break;
                case ComponentType::ScheduleBasedSourceType:
                {
                    assert(idx < m.ScheduledSrcs.size());
                    ScheduleBasedSource const& comp = m.ScheduledSrcs[idx];
                    size_t outflowConnIdx = comp.OutflowConn;
                    Connection const& outflowConn =
                        m.Connections[outflowConnIdx];
                    size_t outflowPort = 0;
                    if ((outflowConn.From != compType)
                        || (outflowConn.FromId != compId)
                        || (outflowConn.FromIdx != idx)
                        || (outflowConn.FromPort != outflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            outflowPort,
                            idx,
                            compType,
                            outflowConn,
                            outflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                    size_t wfConnIdx = comp.WasteflowConn;
                    Connection const& wfConn = m.Connections[wfConnIdx];
                    size_t wfPort = 1;
                    if ((wfConn.From != compType) || (wfConn.FromId != compId)
                        || (wfConn.FromIdx != idx)
                        || (wfConn.FromPort != wfPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            wfPort,
                            idx,
                            compType,
                            wfConn,
                            wfConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                }
                break;
                case ComponentType::StoreType:
                {
                    assert(idx < m.Stores.size());
                    Store const& comp = m.Stores[idx];
                    size_t outflowConnIdx = comp.OutflowConn;
                    Connection const& outflowConn =
                        m.Connections[outflowConnIdx];
                    size_t outflowPort = 0;
                    if ((outflowConn.From != compType)
                        || (outflowConn.FromId != compId)
                        || (outflowConn.FromIdx != idx)
                        || (outflowConn.FromPort != outflowPort))
                    {
                        AddConnectionIssue(
                            issues,
                            tag,
                            compId,
                            outflowPort,
                            idx,
                            compType,
                            outflowConn,
                            outflowConnIdx,
                            FlowDirection::Outflow
                        );
                    }
                    if (comp.InflowConn.has_value())
                    {
                        size_t inflowConnIdx = comp.InflowConn.value();
                        Connection const& inflowConn =
                            m.Connections[inflowConnIdx];
                        size_t inflowPort = 0;
                        if ((inflowConn.To != compType)
                            || (inflowConn.ToId != compId)
                            || (inflowConn.ToIdx != idx)
                            || (inflowConn.ToPort != inflowPort))
                        {
                            AddConnectionIssue(
                                issues,
                                tag,
                                compId,
                                inflowPort,
                                idx,
                                compType,
                                inflowConn,
                                inflowConnIdx,
                                FlowDirection::Inflow
                            );
                        }
                    }
                    if (comp.WasteflowConn.has_value())
                    {
                        size_t wfConnIdx = comp.WasteflowConn.value();
                        Connection const& wfConn = m.Connections[wfConnIdx];
                        size_t wfPort = 1;
                        if ((wfConn.From != compType)
                            || (wfConn.FromId != compId)
                            || (wfConn.FromIdx != idx)
                            || (wfConn.FromPort != wfPort))
                        {
                            AddConnectionIssue(
                                issues,
                                tag,
                                compId,
                                wfPort,
                                idx,
                                compType,
                                wfConn,
                                wfConnIdx,
                                FlowDirection::Outflow
                            );
                        }
                    }
                }
                break;
                case ComponentType::WasteSinkType:
                {
                    // pass
                }
                break;
                default:
                {
                    std::cout << "unhandled component type: " + ToString(compType)
                              << std::endl;
                    std::exit(1);
                }
                break;
            }
        }
        for (size_t connIdx = 0; connIdx < m.Connections.size(); ++connIdx)
        {
            Connection const& conn = m.Connections[connIdx];
            // asser that component port links back to connection
            ComponentType fromType = conn.From;
            ComponentType toType = conn.To;
            if (fromType == ComponentType::MuxType)
            {
                auto const& fromMux = m.Muxes[conn.FromIdx];
                if (conn.FromPort >= fromMux.OutflowConns.size())
                {
                    std::ostringstream oss;
                    oss << "mux outflow connection inconsistent\n";
                    oss << "conn.FromId: " << conn.FromId << "\n";
                    oss << "conn.FromPort: " << conn.FromPort << "\n";
                    oss << "mux num outflow connections: "
                        << fromMux.NumOutports << "\n";
                    oss << "mux num outflow connections from count: "
                        << fromMux.OutflowConns.size() << "\n";
                    issues.push_back(oss.str());
                }
                else if (fromMux.OutflowConns[conn.FromPort] != connIdx)
                {
                    std::ostringstream oss;
                    oss << "mux outflow connection inconsistent\n";
                    oss << "connId: " << connIdx << "\n";
                    oss << "conn.FromId: " << conn.FromId << "\n";
                    oss << "conn.FromPort: " << conn.FromPort << "\n";
                    oss << "fromMux.OutflowConns[conn.FromPort]: "
                        << fromMux.OutflowConns[conn.FromPort] << "\n";
                    issues.push_back(oss.str());
                }
                if (fromMux.NumOutports != fromMux.OutflowConns.size())
                {
                    std::ostringstream oss;
                    oss << "mux num outflows inconsistent\n";
                    oss << "mux num outflow connections: "
                        << fromMux.NumOutports << "\n";
                    oss << "mux num outflow connections from count: "
                        << fromMux.OutflowConns.size() << "\n";
                    issues.push_back(oss.str());
                }
                if (muxCompIdToOutflowConns[conn.FromId].contains(connIdx))
                {
                    std::ostringstream oss;
                    oss << "mux has multiple instances of the same outflow "
                           "connection\n";
                    oss << "connIdx: " << connIdx << "\n";
                    issues.push_back(oss.str());
                }
                muxCompIdToOutflowConns[conn.FromId].insert(connIdx);
            }
            if (toType == ComponentType::MuxType)
            {
                auto const& toMux = m.Muxes[conn.ToIdx];
                if (conn.ToPort >= toMux.InflowConns.size())
                {
                    std::ostringstream oss;
                    oss << "mux inflow connection inconsistent\n";
                    oss << "conn.ToId: " << conn.ToId << "\n";
                    oss << "conn.ToPort: " << conn.ToPort << "\n";
                    oss << "mux num inflow connections: " << toMux.NumInports
                        << "\n";
                    oss << "mux num inflow connections from count: "
                        << toMux.InflowConns.size() << "\n";
                    issues.push_back(oss.str());
                }
                else if (toMux.InflowConns[conn.ToPort] != connIdx)
                {
                    std::ostringstream oss;
                    oss << "mux inflow connection inconsistent\n";
                    oss << "connId: " << connIdx << "\n";
                    oss << "conn.ToId: " << conn.ToId << "\n";
                    oss << "conn.FromPort: " << conn.ToPort << "\n";
                    oss << "toMux.InflowConns[conn.FromPort]: "
                        << toMux.OutflowConns[conn.ToPort] << "\n";
                    issues.push_back(oss.str());
                }
                if (toMux.NumInports != toMux.InflowConns.size())
                {
                    std::ostringstream oss;
                    oss << "mux num inflows inconsistent\n";
                    oss << "mux num inflow connections: " << toMux.NumInports
                        << "\n";
                    oss << "mux num inflow connections from count: "
                        << toMux.InflowConns.size() << "\n";
                    issues.push_back(oss.str());
                }
                if (muxCompIdToInflowConns[conn.ToId].contains(connIdx))
                {
                    std::ostringstream oss;
                    oss << "mux has multiple instances of the same inflow "
                           "connection\n";
                    oss << "connIdx: " << connIdx << "\n";
                    issues.push_back(oss.str());
                }
                muxCompIdToInflowConns[conn.ToId].insert(connIdx);
            }
            std::string outflowCompPort;
            {
                std::ostringstream oss;
                oss << conn.FromId << ":" << conn.FromPort;
                outflowCompPort = oss.str();
            };
            if (connectedOutflowPorts.contains(outflowCompPort))
            {
                std::ostringstream oss;
                oss << "Port multiply connected: " << "- outflowCompPort: "
                    << outflowCompPort << "\n"
                    << "- compId: " << conn.FromId << "\n"
                    << "- outflowPort: " << conn.FromPort << "\n"
                    << "- tag: " << m.ComponentMap.Tag[conn.FromId] << "\n"
                    << "- type: "
                    << ToString(m.ComponentMap.CompType[conn.FromId]) << "\n";
                issues.push_back(oss.str());
            }
            connectedOutflowPorts.insert(outflowCompPort);
            std::string inflowCompPort;
            {
                std::ostringstream oss;
                oss << conn.ToId << ":" << conn.ToPort;
                inflowCompPort = oss.str();
            };
            if (connectedInflowPorts.contains(inflowCompPort))
            {
                std::ostringstream oss;
                oss << "Port multiply connected: " << "- inflowCompPort: "
                    << inflowCompPort << "\n"
                    << "- compId: " << conn.ToId << "\n"
                    << "- outflowPort: " << conn.ToPort << "\n"
                    << "- tag: " << m.ComponentMap.Tag[conn.ToId] << "\n"
                    << "- type: "
                    << ToString(m.ComponentMap.CompType[conn.ToId]) << "\n";
                issues.push_back(oss.str());
            }
            connectedInflowPorts.insert(inflowCompPort);
        }
        for (size_t compId = 0; compId < m.ComponentMap.CompType.size();
             ++compId)
        {
            if (m.ComponentMap.CompType[compId] == ComponentType::MuxType)
            {
                Mux const& mux = m.Muxes[m.ComponentMap.Idx[compId]];
                if (mux.NumInports != muxCompIdToInflowConns[compId].size())
                {
                    std::ostringstream oss;
                    oss << "mux specifies " << mux.NumInports
                        << " inports but the number of connections are "
                        << muxCompIdToInflowConns[compId].size() << "\n";
                    issues.push_back(oss.str());
                }
                if (mux.NumOutports != muxCompIdToOutflowConns[compId].size())
                {
                    std::ostringstream oss;
                    oss << "mux specifies " << mux.NumOutports
                        << " outports but the number of connections are "
                        << muxCompIdToOutflowConns[compId].size() << "\n";
                    issues.push_back(oss.str());
                }
            }
        }
        return issues;
    }

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
            assert(xs[1] * rateToWatts <= max_flow_W);
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
        double initialAge_s,
        bool report
    )
    {
        size_t id = c.CompType.size();
        c.CompType.push_back(ct);
        c.Idx.push_back(idx);
        c.Tag.push_back(tag);
        c.InitialAges_s.push_back(initialAge_s);
        c.InflowType.push_back(inflowType);
        c.OutflowType.push_back(outflowType);
        c.Report.push_back(report);
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

    SwitchState
    SimulationState_GetSwitchState(
        SimulationState const& ss,
        size_t const& switchIdx
    )
    {
        assert(switchIdx < ss.SwitchStates.size());
        return ss.SwitchStates[switchIdx];
    }

    void
    SimulationState_SetSwitchState(
        SimulationState& ss,
        size_t const& switchIdx,
        SwitchState newState
    )
    {
        assert(switchIdx < ss.SwitchStates.size());
        ss.SwitchStates[switchIdx] = newState;
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
            std::optional<size_t> maybeInflowConn = store.InflowConn;
            size_t outflowConn = store.OutflowConn;
            size_t compId = m.Connections[outflowConn].FromId;
            if (ss.UnavailableComponents.contains(compId))
            {
                if (maybeInflowConn.has_value())
                {
                    size_t inflowConn = maybeInflowConn.value();
                    if (ss.Flows[inflowConn].Requested_W != 0)
                    {
                        ss.ActiveConnectionsBack.insert(inflowConn);
                    }
                    ss.Flows[inflowConn].Requested_W = 0;
                }
                if (ss.Flows[outflowConn].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = 0;
                continue;
            }
            bool isSource = !maybeInflowConn.has_value();
            if (ss.StorageNextEventTimes[storeIdx] == t || isSource)
            {
                flow_t available_W = ss.StorageAmounts_J[storeIdx] > 0
                    ? store.MaxDischargeRate_W
                    : 0;
                if (maybeInflowConn.has_value())
                {
                    size_t inflowConn = maybeInflowConn.value();
                    available_W = UtilSafeAdd(available_W, ss.Flows[inflowConn].Available_W);
                }
                if (available_W > store.MaxOutflow_W)
                {
                    available_W = store.MaxOutflow_W;
                }
                if (ss.Flows[outflowConn].Available_W != available_W)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = available_W;
                if (maybeInflowConn.has_value())
                {
                    flow_t request_W =
                        (ss.Flows[outflowConn].Requested_W > store.MaxOutflow_W
                             ? store.MaxOutflow_W
                             : ss.Flows[outflowConn].Requested_W)
                        + (ss.StorageAmounts_J[storeIdx] <= store.ChargeAmount_J
                               ? store.MaxChargeRate_W
                               : 0);
                    size_t inflowConn = maybeInflowConn.value();
                    if (ss.Flows[inflowConn].Requested_W != request_W)
                    {
                        ss.ActiveConnectionsBack.insert(inflowConn);
                    }
                    ss.Flows[inflowConn].Requested_W = request_W;
                }
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
        double time,
        bool verbose
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
                        if (verbose)
                        {
                            std::cout << "... REPAIRED: "
                                      << m.ComponentMap.Tag[rel.ComponentId]
                                      << "[" << rel.ComponentId << "]"
                                      << std::endl;
                        }
                    }
                    else
                    {
                        Model_SetComponentToFailed(m, ss, rel.ComponentId);
                        if (verbose)
                        {
                            std::cout << "... FAILED: "
                                      << m.ComponentMap.Tag[rel.ComponentId]
                                      << "[" << rel.ComponentId << "]"
                                      << std::endl;
                            std::cout << "... causes: " << std::endl;
                            for (auto const& fragCause : ts.fragilityModeCauses)
                            {
                                std::cout
                                    << "... ... fragility mode: " << fragCause
                                    << std::endl;
                            }
                            for (auto const& failCause : ts.failureModeCauses)
                            {
                                std::cout
                                    << "... ... failure mode: " << failCause
                                    << std::endl;
                            }
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
    UpdateConverterLossflowAndWasteflow(
        SimulationState& ss,
        size_t inflowConn,
        size_t outflowConn,
        std::optional<size_t> lossflowConn,
        size_t wasteflowConn,
        flow_t maxOutflow_W,
        flow_t maxLossflow_W
    )
    {
        flow_t inflowRequest = ss.Flows[inflowConn].Requested_W;
        flow_t inflowAvailable = ss.Flows[inflowConn].Available_W;
        flow_t outflowRequest = ss.Flows[outflowConn].Requested_W;
        flow_t outflowAvailable =
            ss.Flows[outflowConn].Available_W > maxOutflow_W
            ? maxOutflow_W
            : ss.Flows[outflowConn].Available_W;
        flow_t inflow = FinalizeFlowValue(inflowRequest, inflowAvailable);
        flow_t outflow = FinalizeFlowValue(outflowRequest, outflowAvailable);
        // NOTE: for COP, we can have outflow > inflow, but we assume no
        // lossflow in that scenario
        flow_t nonOutflowAvailable = inflow > outflow ? inflow - outflow : 0;
        flow_t lossflowRequest = 0;
        if (lossflowConn.has_value())
        {
            lossflowRequest = ss.Flows[lossflowConn.value()].Requested_W;
            if (lossflowRequest > maxLossflow_W)
            {
                lossflowRequest = maxLossflow_W;
            }
            flow_t nonOutflowAvailableLimited =
                nonOutflowAvailable > maxLossflow_W ? maxLossflow_W
                                                    : nonOutflowAvailable;
            if (nonOutflowAvailableLimited
                != ss.Flows[lossflowConn.value()].Available_W)
            {
                ss.ActiveConnectionsFront.insert(lossflowConn.value());
            }
            ss.Flows[lossflowConn.value()].Available_W =
                nonOutflowAvailableLimited;
        }
        flow_t wasteflow = nonOutflowAvailable > lossflowRequest
            ? nonOutflowAvailable - lossflowRequest
            : 0;
        ss.Flows[wasteflowConn].Requested_W = wasteflow;
        ss.Flows[wasteflowConn].Available_W = wasteflow;
    }

    void
    UpdateConstantEfficiencyLossflowAndWasteflow(
        Model const& m,
        SimulationState& ss,
        size_t compIdx
    )
    {
        ConstantEfficiencyConverter const& cec = m.ConstEffConvs[compIdx];
        UpdateConverterLossflowAndWasteflow(
            ss,
            cec.InflowConn,
            cec.OutflowConn,
            cec.LossflowConn,
            cec.WasteflowConn,
            cec.MaxOutflow_W,
            cec.MaxLossflow_W
        );
    }

    void
    UpdateVariableEfficiencyLossflowAndWasteflow(
        Model const& m,
        SimulationState& ss,
        size_t compIdx
    )
    {
        VariableEfficiencyConverter const& vec = m.VarEffConvs[compIdx];
        UpdateConverterLossflowAndWasteflow(
            ss,
            vec.InflowConn,
            vec.OutflowConn,
            vec.LossflowConn,
            vec.WasteflowConn,
            vec.MaxOutflow_W,
            vec.MaxLossflow_W
        );
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
        assert(cec.OutflowConn == outflowConnIdx);
        flow_t outflowRequest_W =
            ss.Flows[outflowConnIdx].Requested_W > cec.MaxOutflow_W
            ? cec.MaxOutflow_W
            : ss.Flows[outflowConnIdx].Requested_W;
        flow_t inflowRequest_W =
            static_cast<flow_t>(std::ceil(outflowRequest_W / cec.Efficiency));
        assert(inflowRequest_W >= outflowRequest_W);
        if (inflowRequest_W != ss.Flows[cec.InflowConn].Requested_W)
        {
            ss.ActiveConnectionsBack.insert(cec.InflowConn);
        }
        ss.Flows[cec.InflowConn].Requested_W = inflowRequest_W;
        UpdateConstantEfficiencyLossflowAndWasteflow(m, ss, compIdx);
    }

    void
    RunVariableEfficiencyConverterBackward(
        Model const& m,
        SimulationState& ss,
        size_t outflowConnIdx,
        size_t compIdx
    )
    {
        VariableEfficiencyConverter const& vec = m.VarEffConvs[compIdx];
        assert(outflowConnIdx == vec.OutflowConn);
        size_t inflowConnIdx = vec.InflowConn;
        flow_t outflowRequest_W =
            ss.Flows[outflowConnIdx].Requested_W > vec.MaxOutflow_W
            ? vec.MaxOutflow_W
            : ss.Flows[outflowConnIdx].Requested_W;
        double efficiency = LookupTable_LookupInterp(
            vec.OutflowsForEfficiency_W,
            vec.Efficiencies,
            static_cast<double>(outflowRequest_W)
        );
        assert(efficiency > 0.0 && efficiency <= 1.0);
        flow_t inflowRequest_W =
            static_cast<flow_t>(std::ceil(outflowRequest_W / efficiency));
        if (inflowRequest_W != ss.Flows[inflowConnIdx].Requested_W)
        {
            ss.ActiveConnectionsBack.insert(inflowConnIdx);
        }
        ss.Flows[inflowConnIdx].Requested_W = inflowRequest_W;
        UpdateVariableEfficiencyLossflowAndWasteflow(m, ss, compIdx);
    }

    void
    UpdateEnvironmentFlowForAllMovers(
        SimulationState& ss,
        size_t inflowConn,
        size_t envConn,
        size_t outflowConn,
        size_t wasteflowConn,
        flow_t maxOutflow_W
    )
    {
        flow_t inflowReq = ss.Flows[inflowConn].Requested_W;
        flow_t inflowAvail = ss.Flows[inflowConn].Available_W;
        flow_t outflowReq = ss.Flows[outflowConn].Requested_W;
        flow_t outflowAvail = ss.Flows[outflowConn].Available_W > maxOutflow_W
            ? maxOutflow_W
            : ss.Flows[outflowConn].Available_W;
        flow_t inflow = FinalizeFlowValue(inflowReq, inflowAvail);
        flow_t outflow = FinalizeFlowValue(outflowReq, outflowAvail);
        if (inflow >= outflow)
        {
            ss.Flows[envConn].Requested_W = 0;
            ss.Flows[envConn].Available_W = 0;
            ss.Flows[envConn].Actual_W = 0;
            flow_t waste = inflow - outflow;
            ss.Flows[wasteflowConn].Requested_W = waste;
            ss.Flows[wasteflowConn].Available_W = waste;
            ss.Flows[wasteflowConn].Actual_W = waste;
        }
        else
        {
            flow_t supply = outflow - inflow;
            ss.Flows[envConn].Requested_W = supply;
            ss.Flows[envConn].Available_W = supply;
            ss.Flows[envConn].Actual_W = supply;
            ss.Flows[wasteflowConn].Requested_W = 0;
            ss.Flows[wasteflowConn].Available_W = 0;
            ss.Flows[wasteflowConn].Actual_W = 0;
        }
    }

    void
    UpdateEnvironmentFlowForMover(
        Model const& m,
        SimulationState& ss,
        size_t moverIdx
    )
    {
        Mover const& mov = m.Movers[moverIdx];
        UpdateEnvironmentFlowForAllMovers(
            ss,
            mov.InflowConn,
            mov.InFromEnvConn,
            mov.OutflowConn,
            mov.WasteflowConn,
            mov.MaxOutflow_W
        );
    }

    void
    UpdateEnvironmentFlowForVariableEfficiencyMover(
        Model const& m,
        SimulationState& ss,
        size_t moverIdx
    )
    {
        VariableEfficiencyMover const& mov = m.VarEffMovers[moverIdx];
        UpdateEnvironmentFlowForAllMovers(
            ss,
            mov.InflowConn,
            mov.InFromEnvConn,
            mov.OutflowConn,
            mov.WasteflowConn,
            mov.MaxOutflow_W
        );
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
    RunVariableEfficiencyMoverBackward(
        Model const& m,
        SimulationState& ss,
        size_t outflowConnIdx,
        size_t moverIdx
    )
    {
        VariableEfficiencyMover const& mov = m.VarEffMovers[moverIdx];
        size_t inflowConn = mov.InflowConn;
        flow_t outflowRequest_W =
            ss.Flows[outflowConnIdx].Requested_W > mov.MaxOutflow_W
            ? mov.MaxOutflow_W
            : ss.Flows[outflowConnIdx].Requested_W;
        double cop = LookupTable_LookupInterp(
            mov.OutflowsForCop_W,
            mov.COPs,
            static_cast<double>(outflowRequest_W)
        );
        // outflow = COP * inflow
        // inflow = outflow / COP
        flow_t inflowRequest_W = static_cast<flow_t>(
            std::ceil(static_cast<double>(outflowRequest_W) / cop)
        );
        if (inflowRequest_W != ss.Flows[inflowConn].Requested_W)
        {
            ss.ActiveConnectionsBack.insert(inflowConn);
        }
        ss.Flows[inflowConn].Requested_W = inflowRequest_W;
        UpdateEnvironmentFlowForVariableEfficiencyMover(m, ss, moverIdx);
    }

    void
    RunSwitchBackward(
        Model const& m,
        SimulationState& ss,
        size_t outflowConnIdx,
        size_t switchIdx
    )
    {
        assert(switchIdx < ss.SwitchStates.size());
        auto switchState = ss.SwitchStates[switchIdx];
        auto const& theSwitch = m.Switches[switchIdx];
        assert(theSwitch.InflowConnPrimary < m.Connections.size());
        auto inflow0ConnIdx = theSwitch.InflowConnPrimary;
        auto inflow1ConnIdx = theSwitch.InflowConnSecondary;
        switch (switchState)
        {
            case SwitchState::Primary:
            {
                // send request on primary
                if (ss.Flows[inflow0ConnIdx].Requested_W
                    != ss.Flows[outflowConnIdx].Requested_W)
                {
                    ss.ActiveConnectionsBack.insert(inflow0ConnIdx);
                }
                ss.Flows[inflow0ConnIdx].Requested_W =
                    ss.Flows[outflowConnIdx].Requested_W;
                // set request on secondary to 0
                if (ss.Flows[inflow0ConnIdx].Requested_W != 0)
                {
                    ss.ActiveConnectionsBack.insert(inflow1ConnIdx);
                }
                ss.Flows[inflow1ConnIdx].Requested_W = 0;
            }
            break;
            case SwitchState::Secondary:
            {
                // send request on secondary
                if (ss.Flows[inflow0ConnIdx].Requested_W != 0)
                {
                    ss.ActiveConnectionsBack.insert(inflow0ConnIdx);
                }
                ss.Flows[inflow0ConnIdx].Requested_W = 0;
                // set request on primary to 0
                if (ss.Flows[inflow1ConnIdx].Requested_W
                    != ss.Flows[outflowConnIdx].Requested_W)
                {
                    ss.ActiveConnectionsBack.insert(inflow1ConnIdx);
                }
                ss.Flows[inflow1ConnIdx].Requested_W =
                    ss.Flows[outflowConnIdx].Requested_W;
            }
            break;
            default:
            {
                WriteErrorMessage("<runtime>", "unhandled switch state");
                std::exit(1);
            }
            break;
        }
    }

    void
    Mux_RequestInflowsIntelligently(
        SimulationState& ss,
        std::vector<size_t> const& inflowConns,
        flow_t remainingRequest_W
    )
    {
        for (size_t inflowConnIdx : inflowConns)
        {
            if (ss.Flows[inflowConnIdx].Requested_W != remainingRequest_W)
            {
                ss.ActiveConnectionsBack.insert(inflowConnIdx);
            }
            ss.Flows[inflowConnIdx].Requested_W = remainingRequest_W;
            remainingRequest_W =
                remainingRequest_W > ss.Flows[inflowConnIdx].Available_W
                ? remainingRequest_W - ss.Flows[inflowConnIdx].Available_W
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
        assert(muxIdx < model.Muxes.size());
        Mux const& mux = model.Muxes[muxIdx];
        flow_t totalOutflowRequest_W = 0;
        for (size_t i = 0; i < mux.NumOutports; ++i)
        {
            auto outflowConnIdx = mux.OutflowConns[i];
            auto outflowRequest_W =
                ss.Flows[outflowConnIdx].Requested_W > mux.MaxOutflows_W[i]
                ? mux.MaxOutflows_W[i]
                : ss.Flows[outflowConnIdx].Requested_W;
            totalOutflowRequest_W = UtilSafeAdd(totalOutflowRequest_W, outflowRequest_W);
        }
        Mux_RequestInflowsIntelligently(
            ss, model.Muxes[muxIdx].InflowConns, totalOutflowRequest_W
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
        assert(outflowConnIdx == store.OutflowConn);
        if (store.InflowConn.has_value())
        {
            flow_t chargeRate_W =
                ss.StorageAmounts_J[storeIdx] <= store.ChargeAmount_J
                ? store.MaxChargeRate_W
                : 0;
            flow_t outRequest_W =
                ss.Flows[outflowConnIdx].Requested_W > store.MaxOutflow_W
                ? store.MaxOutflow_W
                : ss.Flows[outflowConnIdx].Requested_W;
            flow_t totalRequest_W = outRequest_W + chargeRate_W;
            size_t inflowConnIdx = store.InflowConn.value();
            if (ss.Flows[inflowConnIdx].Requested_W != totalRequest_W)
            {
                ss.ActiveConnectionsBack.insert(inflowConnIdx);
            }
            ss.Flows[inflowConnIdx].Requested_W = totalRequest_W;
        }
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
                    case ComponentType::VariableEfficiencyConverterType:
                    {
                        switch (model.Connections[connIdx].FromPort)
                        {
                            case 0:
                            {
                                RunVariableEfficiencyConverterBackward(
                                    model, ss, connIdx, compIdx
                                );
                            }
                            break;
                            case 1: // lossflow
                            case 2: // wasteflow
                            {
                                UpdateVariableEfficiencyLossflowAndWasteflow(
                                    model, ss, compIdx
                                );
                            }
                            break;
                            default:
                            {
                                WriteErrorMessage(
                                    "RunComponentsBackward",
                                    "unhandled port on variable efficiency "
                                    "converter"
                                );
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
                    case ComponentType::VariableEfficiencyMoverType:
                    {
                        switch (model.Connections[connIdx].FromPort)
                        {
                            case 0:
                            {
                                RunVariableEfficiencyMoverBackward(
                                    model, ss, connIdx, compIdx
                                );
                            }
                            break;
                            case 1:
                            {
                                UpdateEnvironmentFlowForVariableEfficiencyMover(
                                    model, ss, compIdx
                                );
                            }
                            break;
                            default:
                            {
                                WriteErrorMessage(
                                    "<runtime>",
                                    "bad port connection for variable "
                                    "efficiency mover"
                                );
                                std::exit(1);
                            }
                            break;
                        }
                    }
                    break;
                    case ComponentType::SwitchType:
                    {
                        RunSwitchBackward(model, ss, connIdx, compIdx);
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
        size_t inflowConnIdx,
        size_t compIdx
    )
    {
        ConstantEfficiencyConverter const& cec = m.ConstEffConvs[compIdx];
        assert(cec.InflowConn == inflowConnIdx);
        flow_t inflowAvailable_W = ss.Flows[inflowConnIdx].Available_W;
        flow_t outflowAvailable_W =
            static_cast<flow_t>(std::floor(cec.Efficiency * inflowAvailable_W));
        assert(inflowAvailable_W >= outflowAvailable_W);
        if (outflowAvailable_W > cec.MaxOutflow_W)
        {
            outflowAvailable_W = cec.MaxOutflow_W;
        }
        if (outflowAvailable_W != ss.Flows[cec.OutflowConn].Available_W)
        {
            ss.ActiveConnectionsFront.insert(cec.OutflowConn);
        }
        ss.Flows[cec.OutflowConn].Available_W = outflowAvailable_W;
        UpdateConstantEfficiencyLossflowAndWasteflow(m, ss, compIdx);
    }

    void
    RunVariableEfficiencyConverterForward(
        Model const& m,
        SimulationState& ss,
        size_t inflowConnIdx,
        size_t compIdx
    )
    {
        VariableEfficiencyConverter const& vec = m.VarEffConvs[compIdx];
        assert(inflowConnIdx == vec.InflowConn);
        size_t outflowConn = vec.OutflowConn;
        flow_t inflowAvailable_W = ss.Flows[inflowConnIdx].Available_W;
        double efficiency = LookupTable_LookupInterp(
            vec.InflowsForEfficiency_W,
            vec.Efficiencies,
            static_cast<double>(inflowAvailable_W)
        );
        assert(efficiency > 0.0 && efficiency <= 1.0);
        flow_t outflowAvailable =
            static_cast<flow_t>(std::floor(efficiency * inflowAvailable_W));
        if (outflowAvailable > vec.MaxOutflow_W)
        {
            outflowAvailable = vec.MaxOutflow_W;
        }
        if (outflowAvailable != ss.Flows[outflowConn].Available_W)
        {
            ss.ActiveConnectionsFront.insert(outflowConn);
        }
        ss.Flows[outflowConn].Available_W = outflowAvailable;
        UpdateVariableEfficiencyLossflowAndWasteflow(m, ss, compIdx);
    }

    void
    RunMoverForward(
        Model const& model,
        SimulationState& ss,
        size_t outConnIdx,
        size_t moverIdx
    )
    {
        assert(moverIdx < model.Movers.size());
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
    RunVariableEfficiencyMoverForward(
        Model const& model,
        SimulationState& ss,
        size_t outConnIdx,
        size_t moverIdx
    )
    {
        assert(moverIdx < model.VarEffMovers.size());
        VariableEfficiencyMover const& mov = model.VarEffMovers[moverIdx];
        flow_t inflowAvailable_W = ss.Flows[outConnIdx].Available_W;
        size_t outflowConn = mov.OutflowConn;
        double cop = LookupTable_LookupInterp(
            mov.InflowsForCop_W,
            mov.COPs,
            static_cast<double>(inflowAvailable_W)
        );
        // outflow = cop * inflow
        flow_t outflowAvailable_W = static_cast<flow_t>(
            std::floor(cop * static_cast<double>(inflowAvailable_W))
        );
        if (outflowAvailable_W > mov.MaxOutflow_W)
        {
            outflowAvailable_W = mov.MaxOutflow_W;
        }
        if (outflowAvailable_W != ss.Flows[outflowConn].Available_W)
        {
            ss.ActiveConnectionsFront.insert(outflowConn);
        }
        ss.Flows[outflowConn].Available_W = outflowAvailable_W;
        UpdateEnvironmentFlowForVariableEfficiencyMover(model, ss, moverIdx);
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
        assert(store.InflowConn.has_value());
        assert(inflowConnIdx == store.InflowConn.value());
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
    RunSwitchForward(
        Model& model,
        SimulationState& ss,
        size_t inflowConnIdx,
        size_t switchIdx
    )
    {
        assert(switchIdx < model.Switches.size());
        auto const& theSwitch = model.Switches[switchIdx];
        assert(switchIdx < ss.SwitchStates.size());
        auto switchState = ss.SwitchStates[switchIdx];
        auto inflow0ConnIdx = theSwitch.InflowConnPrimary;
        auto inflow1ConnIdx = theSwitch.InflowConnSecondary;
        auto outflowConnIdx = theSwitch.OutflowConn;
        if (switchState == SwitchState::Primary
            && inflowConnIdx == inflow0ConnIdx)
        {
            if (ss.Flows[outflowConnIdx].Available_W
                != ss.Flows[inflow0ConnIdx].Available_W)
            {
                ss.ActiveConnectionsFront.insert(outflowConnIdx);
            }
            ss.Flows[outflowConnIdx].Available_W =
                ss.Flows[inflow0ConnIdx].Available_W;
        }
        else if (switchState == SwitchState::Secondary
                 && inflowConnIdx == inflow1ConnIdx)
        {
            if (ss.Flows[outflowConnIdx].Available_W
                != ss.Flows[inflow1ConnIdx].Available_W)
            {
                ss.ActiveConnectionsFront.insert(outflowConnIdx);
            }
            ss.Flows[outflowConnIdx].Available_W =
                ss.Flows[inflow1ConnIdx].Available_W;
        }
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
                    case ComponentType::VariableEfficiencyConverterType:
                    {
                        RunVariableEfficiencyConverterForward(
                            model, ss, connIdx, compIdx
                        );
                    }
                    break;
                    case ComponentType::MoverType:
                    {
                        RunMoverForward(model, ss, connIdx, compIdx);
                    }
                    break;
                    case ComponentType::VariableEfficiencyMoverType:
                    {
                        RunVariableEfficiencyMoverForward(
                            model, ss, connIdx, compIdx
                        );
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
                    case ComponentType::SwitchType:
                    {
                        RunSwitchForward(model, ss, connIdx, compIdx);
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
        size_t compIdx
    )
    {
        // NOTE: we assume that the charge request never resets once at or
        // below chargeAmount UNTIL you hit 100% SOC again...
        Store const& store = model.Stores[compIdx];
        size_t outflowConn = store.OutflowConn;
        std::optional<size_t> maybeInflowConn = store.InflowConn;
        int64_t netCharge_W =
            -1 * static_cast<int64_t>(ss.Flows[outflowConn].Actual_W);
        if (maybeInflowConn.has_value())
        {
            size_t inflowConn = maybeInflowConn.value();
            netCharge_W += static_cast<int64_t>(ss.Flows[inflowConn].Actual_W);
        }
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
        else if (netCharge_W < 0 && store.InflowConn.has_value()
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
            if (model.Connections[connIdx].To == ComponentType::MuxType)
            {
                RunMuxPostFinalization(model, ss, compIdx);
            }
            if (model.Connections[connIdx].From == ComponentType::StoreType)
            {
                RunStorePostFinalization(
                    model, ss, t, model.Connections[connIdx].FromIdx
                );
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

    flow_t
    FinalizeFlowValue(flow_t requested, flow_t available)
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
            int64_t netEnergyAdded_J = 0;
            std::optional<size_t> maybeInConn = store.InflowConn;
            size_t outConn = store.OutflowConn;
            size_t compId = m.Connections[outConn].FromId;
            assert(m.Connections[outConn].From == ComponentType::StoreType);
            if (ss.UnavailableComponents.contains(compId))
            {
                continue;
            }
            int64_t availableCharge_J =
                static_cast<int64_t>(m.Stores[storeIdx].Capacity_J)
                - static_cast<int64_t>(ss.StorageAmounts_J[storeIdx]);
            int64_t availableDischarge_J =
                -1 * static_cast<int64_t>(ss.StorageAmounts_J[storeIdx]);
            if (maybeInConn.has_value())
            {
                size_t inConn = maybeInConn.value();
                double actualInflow_W =
                    static_cast<double>(ss.Flows[inConn].Actual_W);
                netEnergyAdded_J +=
                    std::llround(elapsedTime_s * actualInflow_W);
            }
            double actualOutflow_W =
                static_cast<double>(ss.Flows[outConn].Actual_W);
            netEnergyAdded_J -= std::llround(elapsedTime_s * actualOutflow_W);
            if (store.WasteflowConn.has_value())
            {
                size_t wConn = store.WasteflowConn.value();
                double actualWasteflow_W =
                    static_cast<double>(ss.Flows[wConn].Actual_W);
                netEnergyAdded_J -=
                    std::llround(elapsedTime_s * actualWasteflow_W);
            }
            if (netEnergyAdded_J > availableCharge_J)
            {
                std::cout << "ERROR: netEnergyAdded is greater than capacity!"
                          << std::endl;
                std::cout << "netEnergyAdded (J): " << netEnergyAdded_J
                          << std::endl;
                std::cout << "availableCharge (J): " << availableCharge_J
                          << std::endl;
                std::cout << "elapsed time (s): " << elapsedTime_s << std::endl;
                std::cout << "stored amount (J): "
                          << ss.StorageAmounts_J[storeIdx] << std::endl;
                if (maybeInConn.has_value())
                {
                    size_t inConn = maybeInConn.value();
                    std::cout << "inflow (W): " << ss.Flows[inConn].Actual_W
                              << std::endl;
                }
                std::cout << "outflow (W): " << ss.Flows[outConn].Actual_W
                          << std::endl;
                int64_t inflow = 0;
                if (maybeInConn.has_value())
                {
                    size_t inConn = maybeInConn.value();
                    inflow = static_cast<int64_t>(ss.Flows[inConn].Actual_W);
                }
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
                availableCharge_J >= netEnergyAdded_J
                && "netEnergyAdded cannot put storage over capacity"
            );
            if (netEnergyAdded_J < availableDischarge_J)
            {
                std::cout
                    << "ERROR: netEnergyAdded is lower than discharge limit"
                    << std::endl;
                std::cout << "compId: " << compId << std::endl;
                std::cout << "store idx: " << storeIdx << std::endl;
                std::cout << "tag: " << m.ComponentMap.Tag[compId] << std::endl;
                for (size_t compId_Idx = 0;
                     compId_Idx < m.ComponentMap.Tag.size();
                     ++compId_Idx)
                {
                    if (m.ComponentMap.CompType[compId_Idx]
                            == ComponentType::StoreType
                        && m.ComponentMap.Idx[compId_Idx] == storeIdx)
                    {
                        std::cout << "compId (from search): " << compId_Idx
                                  << std::endl;
                        std::cout << "tag (from search): "
                                  << m.ComponentMap.Tag[compId_Idx]
                                  << std::endl;
                    }
                }
                std::cout << "has inflow? " << store.InflowConn.has_value()
                          << std::endl;
                std::cout << "netEnergyAdded (J): " << netEnergyAdded_J
                          << std::endl;
                std::cout << "availableDischarge (J): " << availableDischarge_J
                          << std::endl;
                std::cout << "elapsed time (s): " << elapsedTime_s << std::endl;
                std::cout << "stored amount (J): "
                          << ss.StorageAmounts_J[storeIdx] << std::endl;
                int64_t inflow_W = 0;
                if (maybeInConn.has_value())
                {
                    size_t inConn = maybeInConn.value();
                    std::cout << "inflow (W): " << ss.Flows[inConn].Actual_W
                              << std::endl;
                    inflow_W = static_cast<int64_t>(ss.Flows[inConn].Actual_W);
                }
                else
                {
                    std::cout << "inflow (W): 0.0 (no inflow connection)"
                              << std::endl;
                }
                std::cout << "outflow (W): " << ss.Flows[outConn].Actual_W
                          << std::endl;
                int64_t outflow_W =
                    static_cast<int64_t>(ss.Flows[outConn].Actual_W);
                int64_t wasteflow_W = 0;
                if (store.WasteflowConn.has_value())
                {
                    size_t wConn = store.WasteflowConn.value();
                    std::cout << "wasteflow (W): " << ss.Flows[wConn].Actual_W
                              << std::endl;
                    wasteflow_W =
                        static_cast<int64_t>(ss.Flows[wConn].Actual_W);
                }
                std::cout << "flow balance (inflow - (outflow + wasteflow)): "
                          << (inflow_W - (outflow_W + wasteflow_W))
                          << std::endl;
            }
            assert(
                netEnergyAdded_J >= availableDischarge_J
                && "netEnergyAdded cannot use more energy than available"
            );
            ss.StorageAmounts_J[storeIdx] += netEnergyAdded_J;
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
            case ComponentType::VariableEfficiencyConverterType:
            {
                result = "VariableEfficiencyConverter";
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
            case ComponentType::VariableEfficiencyMoverType:
            {
                result = "VariableEfficiencyMoverType";
            }
            break;
            case ComponentType::EnvironmentSourceType:
            {
                result = "EnvironmentSource";
            }
            break;
            case ComponentType::SwitchType:
            {
                result = "Switch";
            }
            break;
            default:
            {
                WriteErrorMessage("ToString", "unhandled component type");
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
        if (tag == "VariableEfficiencyConverter"
            || tag == "variable_efficiency_converter")
        {
            return ComponentType::VariableEfficiencyConverterType;
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
        if (tag == "VariableEfficiencyMover"
            || tag == "variable_efficiency_mover")
        {
            return ComponentType::VariableEfficiencyMoverType;
        }
        if (tag == "Switch" || tag == "switch")
        {
            return ComponentType::SwitchType;
        }
        return {};
    }

    std::vector<std::string>
    FlowsToStrings(Model const& m, SimulationState const& ss, double time_s)
    {
        std::vector<std::string> result{};
        result.push_back(fmt::format(
            "time: {} s, {}, {} h",
            time_s,
            TimeToISO8601Period(static_cast<uint64_t>(time_s)),
            TimeInSecondsToHours(static_cast<uint64_t>(time_s))
        ));
        for (size_t flowIdx = 0; flowIdx < ss.Flows.size(); ++flowIdx)
        {
            result.push_back(fmt::format(
                "{}: {} (R: {}; A: {})",
                ConnectionToString(m.ComponentMap, m.Connections[flowIdx]),
                ss.Flows[flowIdx].Actual_W,
                ss.Flows[flowIdx].Requested_W,
                ss.Flows[flowIdx].Available_W
            ));
        }
        return result;
    }

    void
    LogFlows(
        Log const& log,
        Model const& m,
        SimulationState const& ss,
        double time_s
    )
    {
        for (std::string const& s : FlowsToStrings(m, ss, time_s))
        {
            Log_Info(log, s);
        }
    }

    void
    PrintFlows(Model const& m, SimulationState const& ss, double time_s)
    {
        for (std::string const& s : FlowsToStrings(m, ss, time_s))
        {
            std::cout << s << std::endl;
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
                case ComponentType::ConstantLoadType:
                case ComponentType::ScheduleBasedLoadType:
                case ComponentType::ConstantEfficiencyConverterType:
                case ComponentType::VariableEfficiencyConverterType:
                case ComponentType::MuxType:
                case ComponentType::PassThroughType:
                case ComponentType::MoverType:
                case ComponentType::VariableEfficiencyMoverType:
                case ComponentType::WasteSinkType:
                {
                }
                break;
                default:
                {
                    WriteErrorMessage(
                        "SummarizeFlows(.)",
                        "Unhandled From type for connection - from pass"
                    );
                }
                break;
            }

            switch (m.Connections[flowIdx].To)
            {
                case ComponentType::ConstantLoadType:
                case ComponentType::ScheduleBasedLoadType:
                {
                    summary.OutflowRequest += ss.Flows[flowIdx].Requested_W;
                    summary.OutflowAchieved += ss.Flows[flowIdx].Actual_W;
                }
                break;
                case ComponentType::StoreType:
                {
                    summary.StorageCharge += ss.Flows[flowIdx].Actual_W;
                }
                break;
                case ComponentType::WasteSinkType:
                {
                    summary.Wasteflow += ss.Flows[flowIdx].Actual_W;
                }
                break;
                case ComponentType::ConstantSourceType:
                case ComponentType::ScheduleBasedSourceType:
                case ComponentType::ConstantEfficiencyConverterType:
                case ComponentType::VariableEfficiencyConverterType:
                case ComponentType::MuxType:
                case ComponentType::PassThroughType:
                case ComponentType::MoverType:
                case ComponentType::VariableEfficiencyMoverType:
                case ComponentType::EnvironmentSourceType:
                {
                }
                break;
                default:
                {
                    WriteErrorMessage(
                        "SummarizeFlows(.)",
                        "Unhandled From type for connection - to pass"
                    );
                }
                break;
            }
        }
        return summary;
    }

    bool
    PrintFlowSummary(FlowSummary s)
    {
        int64_t netDischarge = static_cast<int64_t>(s.StorageDischarge)
            - static_cast<int64_t>(s.StorageCharge);
        int64_t sum = static_cast<int64_t>(s.Inflow) + netDischarge
            + static_cast<int64_t>(s.EnvInflow)
            - (static_cast<int64_t>(s.OutflowAchieved)
               + static_cast<int64_t>(s.Wasteflow));
        double eff =
            (static_cast<double>(s.Inflow) + static_cast<double>(s.EnvInflow)
             + static_cast<double>(netDischarge))
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
                  << ((int)s.Inflow + (int)s.EnvInflow + netDischarge) << ")"
                  << std::endl;
        std::cout << "  Delivery Effectiveness : " << effectiveness << "%"
                  << " (= " << s.OutflowAchieved << "/" << s.OutflowRequest
                  << ")" << std::endl;
        return sum == 0;
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

    std::vector<flow_t>
    CopyStorageStates(SimulationState& ss)
    {
        std::vector<flow_t> newAmounts = {};
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
        for (size_t i = 0; i < model.Switches.size(); ++i)
        {
            ss.SwitchStates.push_back(SwitchState::Primary);
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
        std::string const& tag,
        bool report
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
            0.0,
            report
        );
        size_t envId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::EnvironmentSourceType,
            0,
            std::vector<size_t>{},
            std::vector<size_t>{wasteflowId},
            "",
            0.0,
            report
        );
        size_t thisId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::MoverType,
            0,
            std::vector<size_t>{inflowTypeId, wasteflowId},
            std::vector<size_t>{outflowTypeId, wasteflowId},
            tag,
            0.0,
            report
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

    ComponentIdAndWasteAndEnvironmentConnection
    Model_AddVariableEfficiencyMover(
        Model& m,
        std::vector<double>&& outflowsForCop_W,
        std::vector<double>&& copByOutflow,
        size_t inflowTypeId,
        size_t outflowTypeId,
        std::string const& tag,
        bool report
    )
    {
        std::vector<double> inflowsForCop_W;
        inflowsForCop_W.reserve(outflowsForCop_W.size());
        for (size_t i = 0; i < outflowsForCop_W.size(); ++i)
        {
            // NOTE: inflow x COP = outflow;
            // therefore, inflow = outflow / COP;
            inflowsForCop_W.push_back(outflowsForCop_W[i] / copByOutflow[i]);
        }
        VariableEfficiencyMover mov = {
            .InflowConn = 0,
            .OutflowConn = 0,
            .InFromEnvConn = 0,
            .WasteflowConn = 0,
            .MaxOutflow_W = max_flow_W,
            .OutflowsForCop_W = std::move(outflowsForCop_W),
            .InflowsForCop_W = std::move(inflowsForCop_W),
            .COPs = std::move(copByOutflow),
        };
        m.VarEffMovers.push_back(std::move(mov));
        size_t wasteId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::WasteSinkType,
            0,
            std::vector<size_t>{wasteflowId},
            std::vector<size_t>{},
            "",
            0.0,
            report
        );
        size_t envId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::EnvironmentSourceType,
            0,
            std::vector<size_t>{},
            std::vector<size_t>{wasteflowId},
            "",
            0.0,
            report
        );
        size_t thisId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::VariableEfficiencyMoverType,
            0,
            std::vector<size_t>{inflowTypeId, wasteflowId},
            std::vector<size_t>{outflowTypeId, wasteflowId},
            tag,
            0.0,
            report
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

    bool
    RunSwitchLogic(Model const& model, SimulationState& ss)
    {
        bool result = false;
        for (size_t switchIdx = 0; switchIdx < ss.SwitchStates.size();
             ++switchIdx)
        {
            auto switchState = ss.SwitchStates[switchIdx];
            assert(switchIdx < model.Switches.size());
            auto const& theSwitch = model.Switches[switchIdx];
            auto in0Conn = theSwitch.InflowConnPrimary;
            auto in1Conn = theSwitch.InflowConnSecondary;
            auto outConn = theSwitch.OutflowConn;
            assert(in0Conn < ss.Flows.size());
            bool primaryIsSufficient =
                ss.Flows[in0Conn].Available_W >= ss.Flows[in0Conn].Requested_W;
            switch (switchState)
            {
                case SwitchState::Primary:
                {
                    if (!primaryIsSufficient)
                    {
                        ss.SwitchStates[switchIdx] = SwitchState::Secondary;
                        ss.ActiveConnectionsBack.insert(outConn);
                        ss.ActiveConnectionsFront.insert(in0Conn);
                        ss.ActiveConnectionsFront.insert(in1Conn);
                        result = true;
                    }
                }
                break;
                case SwitchState::Secondary:
                {
                    if (primaryIsSufficient)
                    {
                        ss.SwitchStates[switchIdx] = SwitchState::Primary;
                        ss.ActiveConnectionsBack.insert(outConn);
                        ss.ActiveConnectionsFront.insert(in0Conn);
                        ss.ActiveConnectionsFront.insert(in1Conn);
                        result = true;
                    }
                }
                break;
                default:
                {
                    WriteErrorMessage("Simulate", "unhandled switch state");
                    std::exit(1);
                }
                break;
            }
        }
        return result;
    }

    std::vector<TimeAndFlows>
    Simulate(Model& model, bool verbose, bool enableSwitchLogic, Log const& log)
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
            ActivateConnectionsForReliability(model, ss, t, verbose);
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
                    if (verbose)
                    {
                        Log_Debug(log, fmt::format("loop iter: {}", loopIter));
                    }
                    break;
                }
                if (loopIter == maxLoop)
                {
                    // TODO: throw an error? exit with error code?
                }
                RunActiveConnections(model, ss, t);
                if (enableSwitchLogic)
                {
                    bool anySwitchChanged = RunSwitchLogic(model, ss);
                    if (anySwitchChanged)
                    {
                        RunActiveConnections(model, ss, t);
                    }
                }
            }
            if (verbose)
            {
                LogFlows(log, model, ss, t);
                if (!PrintFlowSummary(SummarizeFlows(model, ss, t)))
                {
                    Log_Warning(log, "FLOW IMBALANCE!");
                    std::map<size_t, int64_t> sumOfFlowsByCompId;
                    for (size_t connIdx = 0; connIdx < model.Connections.size();
                         ++connIdx)
                    {
                        size_t const& fromId =
                            model.Connections[connIdx].FromId;
                        size_t const& toId = model.Connections[connIdx].ToId;
                        if (!sumOfFlowsByCompId.contains(fromId))
                        {
                            sumOfFlowsByCompId.insert({fromId, 0});
                        }
                        if (!sumOfFlowsByCompId.contains(toId))
                        {
                            sumOfFlowsByCompId.insert({toId, 0});
                        }
                        flow_t flow = ss.Flows[connIdx].Actual_W;
                        sumOfFlowsByCompId[fromId] -= flow;
                        sumOfFlowsByCompId[toId] += flow;
                    }
                    for (auto const& item : sumOfFlowsByCompId)
                    {
                        size_t const& compId = item.first;
                        ComponentType ctype =
                            model.ComponentMap.CompType[compId];
                        if (ctype == ComponentType::ConstantLoadType
                            || ctype == ComponentType::ScheduleBasedLoadType
                            || ctype == ComponentType::WasteSinkType
                            || ctype == ComponentType::ConstantSourceType
                            || ctype == ComponentType::ScheduleBasedSourceType
                            || ctype == ComponentType::EnvironmentSourceType)
                        {
                            continue;
                        }
                        if (item.second != 0)
                        {
                            Log_Warning(
                                log,
                                fmt::format(
                                    "{} doesn't have a zero sum of all flows: "
                                    "{} W",
                                    model.ComponentMap.Tag[item.first],
                                    item.second
                                )
                            );
                        }
                    }
                }
                PrintModelState(model, ss);
                Log_Info(log, "==== QUIESCENCE REACHED ====");
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
                assert(idx < m.ConstEffConvs.size());
                auto outflowConn = m.ConstEffConvs[idx].OutflowConn;
                RunConstantEfficiencyConverterBackward(m, ss, outflowConn, idx);
                auto inflowConn = m.ConstEffConvs[idx].InflowConn;
                RunConstantEfficiencyConverterForward(m, ss, inflowConn, idx);
            }
            break;
            case ComponentType::VariableEfficiencyConverterType:
            {
                assert(idx < m.VarEffConvs.size());
                auto outflowConn = m.VarEffConvs[idx].OutflowConn;
                RunVariableEfficiencyConverterBackward(m, ss, outflowConn, idx);
                auto inflowConn = m.VarEffConvs[idx].InflowConn;
                RunVariableEfficiencyConverterForward(m, ss, inflowConn, idx);
            }
            break;
            case ComponentType::MoverType:
            {
                // TODO: test
                assert(idx < m.Movers.size());
                size_t outflowConn = m.Movers[idx].OutflowConn;
                RunMoverBackward(m, ss, outflowConn, idx);
                size_t inflowConn = m.Movers[idx].InflowConn;
                RunMoverForward(m, ss, inflowConn, idx);
            }
            break;
            case ComponentType::VariableEfficiencyMoverType:
            {
                assert(idx < m.VarEffMovers.size());
                size_t outflowConn = m.VarEffMovers[idx].OutflowConn;
                RunVariableEfficiencyMoverBackward(m, ss, outflowConn, idx);
                size_t inflowConn = m.VarEffMovers[idx].InflowConn;
                RunVariableEfficiencyMoverForward(m, ss, inflowConn, idx);
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
                assert(idx < m.ConstEffConvs.size());
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
            case ComponentType::VariableEfficiencyConverterType:
            {
                assert(idx < m.VarEffConvs.size());
                VariableEfficiencyConverter const& vec = m.VarEffConvs[idx];
                auto inflowConn = vec.InflowConn;
                if (ss.Flows[inflowConn].Requested_W != 0)
                {
                    ss.ActiveConnectionsBack.insert(inflowConn);
                }
                ss.Flows[inflowConn].Requested_W = 0;
                auto outflowConn = vec.OutflowConn;
                if (ss.Flows[outflowConn].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = 0;
                auto lossflowConn = vec.LossflowConn;
                if (lossflowConn.has_value())
                {
                    auto lossConn = lossflowConn.value();
                    if (ss.Flows[lossConn].Available_W != 0)
                    {
                        ss.ActiveConnectionsFront.insert(lossConn);
                    }
                    ss.Flows[lossConn].Available_W = 0;
                }
                auto wasteConn = vec.WasteflowConn;
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
            case ComponentType::VariableEfficiencyMoverType:
            {
                assert(idx < m.VarEffMovers.size());
                VariableEfficiencyMover const& mov = m.VarEffMovers[idx];
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
                // TODO: check! May need to update wasteflow and use efficiency?
                std::optional<size_t> maybeInflowConn =
                    m.Stores[idx].InflowConn;
                if (maybeInflowConn.has_value())
                {
                    size_t inflowConn = maybeInflowConn.value();
                    if (ss.Flows[inflowConn].Requested_W != 0)
                    {
                        ss.ActiveConnectionsBack.insert(inflowConn);
                    }
                    ss.Flows[inflowConn].Requested_W = 0;
                }
                auto outflowConn = m.Stores[idx].OutflowConn;
                if (ss.Flows[outflowConn].Available_W != 0)
                {
                    ss.ActiveConnectionsFront.insert(outflowConn);
                }
                ss.Flows[outflowConn].Available_W = 0;
                if (m.Stores[idx].WasteflowConn.has_value())
                {
                    size_t wConn = m.Stores[idx].WasteflowConn.value();
                    ss.Flows[wConn].Actual_W = 0;
                    ss.Flows[wConn].Available_W = 0;
                    ss.Flows[wConn].Requested_W = 0;
                }
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
    Model_AddSwitch(Model& m, size_t flowTypeId, std::string const& tag)
    {
        Switch s = {
            .InflowConnPrimary = 0,
            .InflowConnSecondary = 0,
            .OutflowConn = 0,
            .MaxOutflow_W = max_flow_W,
        };
        size_t subtypeIndex = m.Switches.size();
        m.Switches.push_back(std::move(s));
        return Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::SwitchType,
            subtypeIndex,
            std::vector<size_t>{flowTypeId, flowTypeId},
            std::vector<size_t>{flowTypeId},
            tag,
            0.0
        );
    }

    size_t
    Model_AddConstantLoad(Model& m, flow_t load)
    {
        return Model_AddConstantLoad(m, load, 0, "", true);
    }

    size_t
    Model_AddConstantLoad(
        Model& m,
        flow_t load,
        size_t inflowTypeId,
        std::string const& tag,
        bool report
    )
    {
        size_t idx = m.ConstLoads.size();
        ConstantLoad cl{};
        cl.Load_W = load;
        m.ConstLoads.push_back(std::move(cl));
        return Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::ConstantLoadType,
            idx,
            std::vector<size_t>{inflowTypeId},
            std::vector<size_t>{},
            tag,
            0.0,
            report
        );
    }

    size_t
    Model_AddScheduleBasedLoad(
        Model& m,
        double* times,
        flow_t* loads,
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
    Model_AddConstantSource(Model& m, flow_t available)
    {
        return Model_AddConstantSource(m, available, 0, "");
    }

    size_t
    Model_AddConstantSource(
        Model& m,
        flow_t available,
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
        Mux mux{
            .NumInports = numInports,
            .NumOutports = numOutports,
            .InflowConns = std::vector<size_t>(numInports, 0),
            .OutflowConns = std::vector<size_t>(numOutports, 0),
            .MaxOutflows_W = std::vector<flow_t>(numOutports, max_flow_W),
        };
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
        flow_t capacity,
        flow_t maxCharge,
        flow_t maxDischarge,
        flow_t chargeAmount,
        flow_t initialStorage
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
        flow_t eff_numerator,
        flow_t eff_denominator
    )
    {
        return Model_AddConstantEfficiencyConverter(
            m, (double)eff_numerator / (double)eff_denominator
        );
    }

    ComponentIdAndWasteConnection
    Model_AddConstantEfficiencyConverter(Model& m, double efficiency)
    {
        return Model_AddConstantEfficiencyConverter(
            m, efficiency, 0, 0, 0, "", true
        );
    }

    ComponentIdAndWasteConnection
    Model_AddConstantEfficiencyConverter(
        Model& m,
        double efficiency,
        size_t inflowId,
        size_t outflowId,
        size_t lossflowId,
        std::string const& tag,
        bool report
    )
    {
        // NOTE: the 0th flowId is ""; the non-described flow
        std::vector<size_t> inflowIds{inflowId};
        std::vector<size_t> outflowIds{outflowId, lossflowId, wasteflowId};
        size_t idx = m.ConstEffConvs.size();
        ConstantEfficiencyConverter cec{
            .Efficiency = efficiency,
            .InflowConn = 0,
            .OutflowConn = 0,
            .LossflowConn = {},
            .WasteflowConn = 0,
            .MaxOutflow_W = max_flow_W,
            .MaxLossflow_W = max_flow_W,
        };
        m.ConstEffConvs.push_back(std::move(cec));
        size_t wasteId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::WasteSinkType,
            0,
            std::vector<size_t>{wasteflowId},
            std::vector<size_t>{},
            "",
            0.0,
            report
        );
        size_t thisId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::ConstantEfficiencyConverterType,
            idx,
            inflowIds,
            outflowIds,
            tag,
            0.0,
            report
        );
        auto wasteConn =
            Model_AddConnection(m, thisId, 2, wasteId, 0, wasteflowId);
        return {thisId, wasteConn};
    }

    ComponentIdAndWasteConnection
    Model_AddVariableEfficiencyConverter(
        Model& m,
        std::vector<double>&& outflowsForEfficiency_W,
        std::vector<double>&& efficiencyByOutflow,
        size_t inflowId,
        size_t outflowId,
        size_t lossflowId,
        std::string const& tag,
        bool report
    )
    {
        // NOTE: the 0th flowId is ""; the non-described flow
        std::vector<size_t> inflowIds{inflowId};
        std::vector<size_t> outflowIds{outflowId, lossflowId, wasteflowId};
        size_t idx = m.VarEffConvs.size();
        std::vector<double> inflowsForEfficiency_W;
        inflowsForEfficiency_W.reserve(outflowsForEfficiency_W.size());
        for (size_t i = 0; i < outflowsForEfficiency_W.size(); ++i)
        {
            inflowsForEfficiency_W.push_back(
                outflowsForEfficiency_W[i] / efficiencyByOutflow[i]
            );
        }
        VariableEfficiencyConverter vec{};
        vec.OutflowsForEfficiency_W = std::move(outflowsForEfficiency_W);
        vec.InflowsForEfficiency_W = std::move(inflowsForEfficiency_W);
        vec.Efficiencies = std::move(efficiencyByOutflow);

        m.VarEffConvs.push_back(std::move(vec));
        size_t wasteId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::WasteSinkType,
            0,
            std::vector<size_t>{wasteflowId},
            std::vector<size_t>{},
            "",
            0.0,
            report
        );
        size_t thisId = Component_AddComponentReturningId(
            m.ComponentMap,
            ComponentType::VariableEfficiencyConverterType,
            idx,
            inflowIds,
            outflowIds,
            tag,
            0.0,
            report
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
        size_t flowId,
        bool checkIntegrity
    )
    {
        ComponentType fromType = m.ComponentMap.CompType[fromId];
        size_t fromIdx = m.ComponentMap.Idx[fromId];
        ComponentType toType = m.ComponentMap.CompType[toId];
        size_t toIdx = m.ComponentMap.Idx[toId];
        Connection c{
            .From = fromType,
            .FromIdx = fromIdx,
            .FromPort = fromPort,
            .FromId = fromId,
            .To = toType,
            .ToIdx = toIdx,
            .ToPort = toPort,
            .ToId = toId,
            .FlowTypeId = flowId,
        };
        size_t connId = m.Connections.size();
        if (checkIntegrity)
        {
            bool issueFound = false;
            for (auto const& conn : m.Connections)
            {
                if (conn.FromId == fromId && conn.FromPort == fromPort)
                {
                    issueFound = true;
                    std::cout
                        << "INTEGRITY VIOLATION: "
                        << "attempt to doubly connect " << "compId=" << fromId
                        << " outport=" << fromPort
                        << " tag=" << m.ComponentMap.Tag[fromId]
                        << " type=" << ToString(m.ComponentMap.CompType[fromId])
                        << std::endl;
                }
                if (conn.ToId == toId && conn.ToPort == toPort)
                {
                    issueFound = true;
                    std::cout
                        << "INTEGRITY VIOLATION: "
                        << "attempt to doubly connect " << "compId=" << toId
                        << " inport=" << toPort
                        << " tag=" << m.ComponentMap.Tag[toId]
                        << " type=" << ToString(m.ComponentMap.CompType[toId])
                        << std::endl;
                }
            }
            if (issueFound)
            {
                std::cout << "Issue when trying to make a connection\n";
                std::exit(1);
            }
        }
        m.Connections.push_back(c);
        switch (fromType)
        {
            case ComponentType::PassThroughType:
            {
                assert(fromIdx < m.PassThroughs.size());
                m.PassThroughs[fromIdx].OutflowConn = connId;
            }
            break;
            case ComponentType::ConstantSourceType:
            {
                assert(fromIdx < m.ConstSources.size());
                m.ConstSources[fromIdx].OutflowConn = connId;
            }
            break;
            case ComponentType::ScheduleBasedSourceType:
            {
                assert(fromIdx < m.ScheduledSrcs.size());
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
                assert(fromIdx < m.ConstEffConvs.size());
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
            case ComponentType::VariableEfficiencyConverterType:
            {
                assert(fromIdx < m.VarEffConvs.size());
                switch (fromPort)
                {
                    case 0:
                    {
                        m.VarEffConvs[fromIdx].OutflowConn = connId;
                    }
                    break;
                    case 1:
                    {
                        m.VarEffConvs[fromIdx].LossflowConn = connId;
                    }
                    break;
                    case 2:
                    {
                        m.VarEffConvs[fromIdx].WasteflowConn = connId;
                    }
                    break;
                    default:
                    {
                        throw std::invalid_argument(
                            "Unhandled variable efficiency converter outport"
                        );
                    }
                }
            }
            break;
            case ComponentType::MoverType:
            {
                assert(fromIdx < m.Movers.size());
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
            case ComponentType::VariableEfficiencyMoverType:
            {
                assert(fromIdx < m.VarEffMovers.size());
                switch (fromPort)
                {
                    case 0: // outflow
                    {
                        m.VarEffMovers[fromIdx].OutflowConn = connId;
                    }
                    break;
                    case 1: // wasteflow
                    {
                        m.VarEffMovers[fromIdx].WasteflowConn = connId;
                    }
                    break;
                    default:
                    {
                        WriteErrorMessage(
                            "<network>", "bad port for VariableEfficiencyMover"
                        );
                        std::exit(1);
                    }
                    break;
                }
            }
            break;
            case ComponentType::MuxType:
            {
                assert(fromIdx < m.Muxes.size());
                assert(fromPort < m.Muxes[fromIdx].OutflowConns.size());
                assert(fromPort < m.Muxes[fromIdx].NumOutports);
                m.Muxes[fromIdx].OutflowConns[fromPort] = connId;
            }
            break;
            case ComponentType::StoreType:
            {
                assert(fromIdx < m.Stores.size());
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
            case ComponentType::SwitchType:
            {
                assert(fromIdx < m.Switches.size());
                m.Switches[fromIdx].OutflowConn = connId;
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
            case ComponentType::SwitchType:
            {
                assert(toIdx < m.Switches.size());
                switch (toPort)
                {
                    case 0:
                    {
                        m.Switches[toIdx].InflowConnPrimary = connId;
                    }
                    break;
                    case 1:
                    {
                        m.Switches[toIdx].InflowConnSecondary = connId;
                    }
                    break;
                    default:
                    {
                        WriteErrorMessage(
                            "Model_AddConnection",
                            "unhandled inport: " + std::to_string(toPort)
                                + " for " + ToString(toType)
                        );
                        std::exit(1);
                    }
                    break;
                }
            }
            break;
            case ComponentType::PassThroughType:
            {
                assert(toIdx < m.PassThroughs.size());
                m.PassThroughs[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::ConstantLoadType:
            {
                assert(toIdx < m.ConstLoads.size());
                m.ConstLoads[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::ConstantEfficiencyConverterType:
            {
                assert(toIdx < m.ConstEffConvs.size());
                m.ConstEffConvs[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::VariableEfficiencyConverterType:
            {
                assert(toIdx < m.VarEffConvs.size());
                m.VarEffConvs[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::MoverType:
            {
                assert(toIdx < m.Movers.size());
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
            case ComponentType::VariableEfficiencyMoverType:
            {
                assert(toIdx < m.VarEffMovers.size());
                switch (toPort)
                {
                    case 0:
                    {
                        m.VarEffMovers[toIdx].InflowConn = connId;
                    }
                    break;
                    case 1:
                    {
                        m.VarEffMovers[toIdx].InFromEnvConn = connId;
                    }
                    break;
                    default:
                    {
                        WriteErrorMessage(
                            "<network>",
                            "bad network connection for variable efficiency "
                            "mover"
                        );
                        std::exit(1);
                    }
                    break;
                }
            }
            break;
            case ComponentType::MuxType:
            {
                assert(toIdx < m.Muxes.size());
                assert(toPort < m.Muxes[toIdx].InflowConns.size());
                assert(toPort < m.Muxes[toIdx].NumInports);
                m.Muxes[toIdx].InflowConns[toPort] = connId;
            }
            break;
            case ComponentType::StoreType:
            {
                assert(toIdx < m.Stores.size());
                m.Stores[toIdx].InflowConn = connId;
            }
            break;
            case ComponentType::ScheduleBasedLoadType:
            {
                assert(toIdx < m.ScheduledLoads.size());
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
                std::exit(1);
            }
            break;
        }
        return c;
    }

    bool
    SameConnection(Connection a, Connection b)
    {
        // TODO: revisit: needs to have .FromId and .ToId and ports but not
        // others
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

    std::optional<flow_t>
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
        FlowDict const& flowDict,
        std::vector<TimeAndFlows> const& timeAndFlows
    )
    {
        ScenarioOccurrenceStats sos{};
        sos.Id = scenarioId;
        sos.OccurrenceNumber = occurrenceNumber;
        double initialStorage_kJ = 0.0;
        double finalStorage_kJ = 0.0;
        if (timeAndFlows.size() > 0)
        {
            for (flow_t const& stored_J : timeAndFlows[0].StorageAmounts_J)
            {
                initialStorage_kJ += static_cast<double>(stored_J) / J_per_kJ;
            }
        }
        double lastTime = timeAndFlows.size() > 0 ? timeAndFlows[0].Time : 0.0;
        bool wasDown = false;
        double sedt_s = 0.0;
        size_t numTimeAndFlows = timeAndFlows.size();
        size_t lastEventIdx = numTimeAndFlows - 1;
        for (size_t eventIdx = 1; eventIdx < numTimeAndFlows; ++eventIdx)
        {
            double dt_s = timeAndFlows[eventIdx].Time - lastTime;
            assert(dt_s > 0.0);
            lastTime = timeAndFlows[eventIdx].Time;
            // TODO: in Simulation, ensure we ALWAYS have an event at final time
            // in order that scenario duration equals what we have here; this
            // is a good check.
            sos.Duration_s += dt_s;
            bool allLoadsMet = true;
            std::map<size_t, bool> allLoadsMetByFlowType;
            size_t prevEventIdx = eventIdx - 1;
            for (size_t connId = 0;
                 connId < timeAndFlows[prevEventIdx].Flows.size();
                 ++connId)
            {
                size_t flowTypeId = m.Connections[connId].FlowTypeId;
                ComponentType fromType =
                    m.ComponentMap.CompType[m.Connections[connId].FromId];
                ComponentType toType =
                    m.ComponentMap.CompType[m.Connections[connId].ToId];
                Flow const& flow = timeAndFlows[prevEventIdx].Flows[connId];
                double actualFlow_W = static_cast<double>(flow.Actual_W);
                double requestedFlow_W = static_cast<double>(flow.Requested_W);
                switch (fromType)
                {
                    case ComponentType::ConstantSourceType:
                    case ComponentType::ScheduleBasedSourceType:
                    {
                        sos.Inflow_kJ += (actualFlow_W / W_per_kW) * dt_s;
                    }
                    break;
                    case ComponentType::EnvironmentSourceType:
                    {
                        sos.InFromEnv_kJ += (actualFlow_W / W_per_kW) * dt_s;
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
                        double outflowAchieved_kJ =
                            (actualFlow_W / W_per_kW) * dt_s;
                        sos.OutflowAchieved_kJ += outflowAchieved_kJ;
                        double outflowRequest_kJ =
                            (requestedFlow_W / W_per_kW) * dt_s;
                        sos.OutflowRequest_kJ += outflowRequest_kJ;
                        bool loadsMet = flow.Actual_W == flow.Requested_W;
                        allLoadsMet = allLoadsMet && loadsMet;
                        double loadNotServed_kJ = 0.0;
                        if (!loadsMet)
                        {
                            assert(flow.Requested_W > flow.Actual_W);
                            loadNotServed_kJ =
                                ((requestedFlow_W - actualFlow_W) / W_per_kW)
                                * dt_s;
                            sos.LoadNotServed_kJ += loadNotServed_kJ;
                        }
                        bool foundFlowTypeStats = false;
                        for (StatsByFlowType& sbf : sos.FlowTypeStats)
                        {
                            if (sbf.FlowTypeId == flowTypeId)
                            {
                                sbf.TotalRequest_kJ += outflowRequest_kJ;
                                sbf.TotalAchieved_kJ += outflowAchieved_kJ;
                                if (!allLoadsMetByFlowType.contains(flowTypeId))
                                {
                                    allLoadsMetByFlowType.insert(
                                        {flowTypeId, true}
                                    );
                                }
                                if (!loadsMet)
                                {
                                    allLoadsMetByFlowType[flowTypeId] = false;
                                }
                                foundFlowTypeStats = true;
                                break;
                            }
                        }
                        if (!foundFlowTypeStats)
                        {
                            if (!allLoadsMetByFlowType.contains(flowTypeId))
                            {
                                allLoadsMetByFlowType.insert({flowTypeId, true}
                                );
                            }
                            if (!loadsMet)
                            {
                                allLoadsMetByFlowType[flowTypeId] = false;
                            }
                            StatsByFlowType sbf{
                                .FlowTypeId = flowTypeId,
                                // NOTE: placeholder; updated later if all loads
                                // for this flow are met
                                .Uptime_s = 0.0,
                                .TotalRequest_kJ = outflowRequest_kJ,
                                .TotalAchieved_kJ = outflowAchieved_kJ,
                            };
                            sos.FlowTypeStats.push_back(std::move(sbf));
                        }
                        size_t compId = m.Connections[connId].ToId;
                        bool foundLoadAndFlowTypeStats = false;
                        for (StatsByLoadAndFlowType& sblf :
                             sos.LoadAndFlowTypeStats)
                        {
                            if (sblf.ComponentId == compId
                                && sblf.Stats.FlowTypeId == flowTypeId)
                            {
                                if (loadsMet)
                                {
                                    sblf.Stats.Uptime_s += dt_s;
                                }
                                sblf.Stats.TotalRequest_kJ += outflowRequest_kJ;
                                sblf.Stats.TotalAchieved_kJ +=
                                    outflowAchieved_kJ;
                                foundLoadAndFlowTypeStats = true;
                                break;
                            }
                        }
                        if (!foundLoadAndFlowTypeStats)
                        {
                            StatsByFlowType sbf{
                                .FlowTypeId = flowTypeId,
                                .Uptime_s = loadsMet ? dt_s : 0.0,
                                .TotalRequest_kJ = outflowRequest_kJ,
                                .TotalAchieved_kJ = outflowAchieved_kJ,
                            };
                            StatsByLoadAndFlowType sblf{
                                .ComponentId = compId,
                                .Stats = std::move(sbf),
                            };
                            sos.LoadAndFlowTypeStats.push_back(std::move(sblf));
                        }
                        bool foundLoadNotServedForComp = false;
                        for (LoadNotServedForComp& lns :
                             sos.LoadNotServedForComponents)
                        {
                            if (lns.ComponentId == compId
                                && lns.FlowTypeId == flowTypeId)
                            {
                                foundLoadNotServedForComp = true;
                                lns.LoadNotServed_kJ += loadNotServed_kJ;
                                break;
                            }
                        }
                        if (!foundLoadNotServedForComp)
                        {
                            LoadNotServedForComp lns{
                                .ComponentId = compId,
                                .FlowTypeId = flowTypeId,
                                .LoadNotServed_kJ = loadNotServed_kJ,
                            };
                            sos.LoadNotServedForComponents.push_back(
                                std::move(lns)
                            );
                        }
                    }
                    break;
                    case ComponentType::WasteSinkType:
                    {
                        sos.Wasteflow_kJ += (actualFlow_W / W_per_kW) * dt_s;
                    }
                    break;
                    default:
                    {
                    }
                    break;
                }
            }
            for (auto const& item : allLoadsMetByFlowType)
            {
                bool foundMatch = false;
                for (StatsByFlowType& sbft : sos.FlowTypeStats)
                {
                    if (sbft.FlowTypeId == item.first)
                    {
                        foundMatch = true;
                        if (item.second)
                        {
                            sbft.Uptime_s += dt_s;
                        }
                        break;
                    }
                }
                ignore(foundMatch);
                assert(foundMatch);
            }
            if (allLoadsMet)
            {
                sos.Uptime_s += dt_s;
                if (wasDown && sedt_s > sos.MaxSEDT_s)
                {
                    sos.MaxSEDT_s = sedt_s;
                }
                sedt_s = 0.0;
                wasDown = false;
            }
            else
            {
                sos.Downtime_s += dt_s;
                if (wasDown)
                {
                    sedt_s += dt_s;
                }
                else
                {
                    sedt_s = dt_s;
                }
                wasDown = true;
            }
            for (size_t storeIdx = 0;
                 storeIdx < timeAndFlows[eventIdx].StorageAmounts_J.size();
                 ++storeIdx)
            {
                double prevStored_J = static_cast<double>(
                    timeAndFlows[prevEventIdx].StorageAmounts_J[storeIdx]
                );
                double currentStored_J = static_cast<double>(
                    timeAndFlows[eventIdx].StorageAmounts_J[storeIdx]
                );
                double increaseInStorage_J = currentStored_J - prevStored_J;
                if (increaseInStorage_J > 0.0)
                {
                    sos.StorageCharge_kJ += increaseInStorage_J / J_per_kJ;
                }
                else
                {
                    sos.StorageDischarge_kJ +=
                        -1.0 * (increaseInStorage_J / J_per_kJ);
                }
                if (eventIdx == lastEventIdx)
                {
                    double amount_J = static_cast<double>(
                        timeAndFlows[eventIdx].StorageAmounts_J[storeIdx]
                    );
                    finalStorage_kJ += amount_J / J_per_kJ;
                }
            }
        }
        sos.ChangeInStorage_kJ = finalStorage_kJ - initialStorage_kJ;
        if (sedt_s > sos.MaxSEDT_s)
        {
            sos.MaxSEDT_s = sedt_s;
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
        // TODO: extract this into a new function
        std::vector<std::string> flowTypeNames;
        flowTypeNames.reserve(sos.FlowTypeStats.size());
        for (auto const& fts : sos.FlowTypeStats)
        {
            flowTypeNames.push_back(flowDict.Type[fts.FlowTypeId]);
        }
        std::vector<size_t> flowTypeNames_idx(flowTypeNames.size());
        std::iota(flowTypeNames_idx.begin(), flowTypeNames_idx.end(), 0);
        std::sort(
            flowTypeNames_idx.begin(),
            flowTypeNames_idx.end(),
            [&](size_t a, size_t b) -> bool
            { return flowTypeNames[a] < flowTypeNames[b]; }
        );
        std::vector<StatsByFlowType> newFlowTypeStats;
        newFlowTypeStats.reserve(sos.FlowTypeStats.size());
        for (size_t ftn_idx : flowTypeNames_idx)
        {
            newFlowTypeStats.push_back(std::move(sos.FlowTypeStats[ftn_idx]));
        }
        sos.FlowTypeStats = std::move(newFlowTypeStats);
        // TODO: extract common parts out to function
        std::vector<std::string> loadFlowTypeNames;
        loadFlowTypeNames.reserve(sos.LoadAndFlowTypeStats.size());
        for (auto const& lfts : sos.LoadAndFlowTypeStats)
        {
            std::string loadName = m.ComponentMap.Tag[lfts.ComponentId];
            std::string flowName = flowDict.Type[lfts.Stats.FlowTypeId];
            std::string sortTag = loadName + "/" + flowName;
            loadFlowTypeNames.push_back(std::move(sortTag));
        }
        std::vector<size_t> loadFlowTypeNames_idx(loadFlowTypeNames.size());
        std::iota(
            loadFlowTypeNames_idx.begin(), loadFlowTypeNames_idx.end(), 0
        );
        std::sort(
            loadFlowTypeNames_idx.begin(),
            loadFlowTypeNames_idx.end(),
            [&](size_t a, size_t b) -> bool
            { return loadFlowTypeNames[a] < loadFlowTypeNames[b]; }
        );
        std::vector<StatsByLoadAndFlowType> newLoadAndFlowTypeStats;
        newLoadAndFlowTypeStats.reserve(sos.LoadAndFlowTypeStats.size());
        for (size_t lftn_idx : loadFlowTypeNames_idx)
        {
            newLoadAndFlowTypeStats.push_back(
                std::move(sos.LoadAndFlowTypeStats[lftn_idx])
            );
        }
        sos.LoadAndFlowTypeStats = std::move(newLoadAndFlowTypeStats);
        // TODO: extract common parts out to function
        std::vector<std::string> loadNotServedFlowTypeNames;
        loadNotServedFlowTypeNames.reserve(sos.LoadAndFlowTypeStats.size());
        for (LoadNotServedForComp const& lns : sos.LoadNotServedForComponents)
        {
            std::string loadName = m.ComponentMap.Tag[lns.ComponentId];
            std::string flowName = flowDict.Type[lns.FlowTypeId];
            std::string sortTag = loadName + "/" + flowName;
            loadNotServedFlowTypeNames.push_back(std::move(sortTag));
        }
        std::vector<size_t> loadNotServedFlowTypeNames_idx(
            loadNotServedFlowTypeNames.size()
        );
        std::iota(
            loadNotServedFlowTypeNames_idx.begin(),
            loadNotServedFlowTypeNames_idx.end(),
            0
        );
        std::sort(
            loadNotServedFlowTypeNames_idx.begin(),
            loadNotServedFlowTypeNames_idx.end(),
            [&](size_t a, size_t b) -> bool {
                return loadNotServedFlowTypeNames[a]
                    < loadNotServedFlowTypeNames[b];
            }
        );
        std::vector<LoadNotServedForComp> newLoadNotServedForComponents;
        newLoadNotServedForComponents.reserve(
            sos.LoadNotServedForComponents.size()
        );
        for (size_t lns_idx : loadNotServedFlowTypeNames_idx)
        {
            newLoadNotServedForComponents.push_back(
                std::move(sos.LoadNotServedForComponents[lns_idx])
            );
        }
        sos.LoadNotServedForComponents =
            std::move(newLoadNotServedForComponents);
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
        TagAndPort tap{
            .Tag = tag,
            .Port = static_cast<size_t>(std::atoi(port.c_str())),
        };
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
            // TODO: std::vector<toml::value> itemAsArray = item.as_array();
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
                std::cout << "[network] port is unaddressable for "
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
                    << "[network] mismatch of flow types: " << toTap.Tag
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
            << " [flow: " << fd.Type[c.FlowTypeId] << "]";
        return oss.str();
    }

    std::string
    NodeConnectionToString(
        Model const& model,
        NodeConnection const& c,
        bool compact,
        bool aggregateGroups
    )
    {
        if (!aggregateGroups)
        {
            return ConnectionToString(
                model.ComponentMap, model.Connections[c.ConnectionId], compact
            );
        }
        std::string fromTag = "";
        std::string toTag = "";
        std::string fromString = "";
        std::string toString = "";

        auto const& componentMap = model.ComponentMap;
        if (c.FromId.index() == 0)
        {
            // component
            auto idx = std::get<0>(c.FromId).id;
            fromTag = componentMap.Tag[idx];
            if (fromTag.empty() && c.From == ComponentType::WasteSinkType)
            {
                fromTag = "WASTE";
            }
            else if (fromTag.empty()
                     && c.From == ComponentType::EnvironmentSourceType)
            {
                fromTag = "ENV";
            }
            fromString = std::to_string(idx);
        }
        else
        {
            // group
            fromTag = std::get<1>(c.FromId).id;
        }

        if (c.ToId.index() == 0)
        {
            auto idx = std::get<0>(c.ToId).id;
            toTag = componentMap.Tag[idx];
            if (toTag.empty() && c.To == ComponentType::WasteSinkType)
            {
                toTag = "WASTE";
            }
            else if (toTag.empty()
                     && c.To == ComponentType::EnvironmentSourceType)
            {
                toTag = "ENV";
            }
            toString = std::to_string(idx);
        }
        else
        {
            // group
            toTag = std::get<1>(c.ToId).id;
        }

        std::ostringstream oss{};
        oss << fromTag << (compact ? "" : ("[" + fromString + "]")) << ":OUT("
            << c.FromPort << ")";

        if (c.FromId.index() == 0)
        {
            oss << (compact ? "" : (": " + ToString(c.From)));
        }

        oss << " => " << toTag << (compact ? "" : ("[" + toString + "]"))
            << ":IN(" << c.ToPort << ")";

        if (c.ToId.index() == 0)
        {
            oss << (compact ? "" : (": " + ToString(c.To)));
        }

        return oss.str();
    }

    std::string
    NodeConnectionToString(
        Model const& model,
        FlowDict const& fd,
        NodeConnection const& nodeConn,
        bool compact,
        bool aggregateGroups
    )
    {
        std::ostringstream oss{};
        oss << NodeConnectionToString(model, nodeConn, compact, aggregateGroups)
            << " [flow: " << fd.Type[nodeConn.FlowTypeId] << "]";
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
        assert(cd.CompType.size() == cd.InitialAges_s.size());
        cd.InitialAges_s[id] = age_s;
    }

    void
    ComponentDict_SetReporting(ComponentDict& cd, size_t id, bool report)
    {
        assert(id < cd.CompType.size());
        assert(cd.CompType.size() == cd.Report.size());
        cd.Report[id] = report;
    }

    void
    AddComponentToGroup(Model& model, size_t id, std::string group)
    {
        // record group association of component
        model.ComponentToGroup.insert({id, group});

        // insert component into group set
        auto& map = model.GroupToComponents;
        if (map.contains(group))
        {
            auto& components_in_group = map[group]; //
            components_in_group.insert(id);
        }
        else
            map.insert({group, {id}});
    }
} // namespace erin
