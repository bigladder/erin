// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include "erin/erin.h"
#include "erin/logging.h"
#include "erin/lookup_table.h"
#include "erin/time_and_amount.h"
#include "erin/units.h"
#include "erin/utils.h"
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
// PRIVATE DECLARATIONS
void add_connection_issue(std::vector<std::string>& issues,
                          std::string component_tag,
                          size_t component_id,
                          size_t component_port,
                          size_t component_subtype_index,
                          ComponentType component_type,
                          Connection const& connection,
                          size_t connection_index,
                          FlowDirection flow_direction);

size_t add_component_returning_id(ComponentDict& c, ComponentType ct, size_t idx);

size_t add_component_returning_id(ComponentDict& c,
                                  ComponentType ct,
                                  size_t idx,
                                  std::vector<size_t> inflow_type,
                                  std::vector<size_t> outflow_type,
                                  std::string const& tag,
                                  double initial_age_s,
                                  bool report = true);

void add_if_not_added(std::vector<size_t>& items, size_t item);

void add_active_connection_back(SimulationState& ss, size_t connection_index);

void add_active_connection_forward(SimulationState& ss, size_t connIdx);

size_t count_active_connections(SimulationState const& ss);

void activate_constant_load_connections(Model const& m, SimulationState& ss);

void activate_constant_source_connections(Model const& m, SimulationState& ss);

void activate_schedule_based_load_connections(Model const& m, SimulationState& ss, double t);

void activate_schedule_based_source_connections(Model const& m, SimulationState& ss, double t);

void activate_store_connections(Model& m, SimulationState& ss, double t);

void activate_reliability_connections(Model& m, SimulationState& ss, double time, bool verbose);

double get_next_time(double next_time, size_t count, std::function<double(size_t)> f);

double earliest_next_event(Model const& m, SimulationState const& ss, double t);

// PRIVATE CONSTANTS
constexpr double const infinite_time = -1.0;

constexpr size_t const wasteflow_id = 0;

void add_connection_issue(std::vector<std::string>& issues,
                          std::string componentTag,
                          size_t compId,
                          size_t compPort,
                          size_t compSubtypeIdx,
                          ComponentType compType,
                          Connection const& conn,
                          size_t connIdx,
                          FlowDirection flowDirection)
{
    std::string direction = flowDirection == FlowDirection::outflow ? "outflow" : "inflow ";
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
        << ToString(flowDirection == FlowDirection::outflow ? conn.from : conn.to) << "\n";
    oss << "  - component id:   "
        << (flowDirection == FlowDirection::outflow ? conn.from_component_id : conn.to_component_id)
        << "\n";
    oss << "  - port:           "
        << (flowDirection == FlowDirection::outflow ? conn.from_port : conn.to_port) << "\n";
    oss << "  - subtype index:  "
        << (flowDirection == FlowDirection::outflow ? conn.from_subtype_index
                                                    : conn.to_subtype_index)
        << "\n";
    if (connIdx == 0)
    {
        oss << "NOTE: Since the connection ID is 0 (initialization), "
            << "this could indicate that '" << componentTag
            << "' is declared but not hooked up to the network.";
    }
    issues.push_back(oss.str());
}

std::vector<std::string> check_network(Model const& m)
{
    std::vector<std::string> issues;
    std::unordered_set<std::string> connectedOutflowPorts;
    std::unordered_set<std::string> connectedInflowPorts;
    std::unordered_set<std::string> compTags;
    {
        for (auto const& tag : m.component.tag)
        {
            if (!tag.empty() && compTags.contains(tag))
            {
                issues.push_back("multiple components with name '" + tag + "'");
            }
            compTags.insert(tag);
        }
    }
    assert(m.component.component_type.size() == m.component.inflow_type.size());
    assert(m.component.component_type.size() == m.component.outflow_type.size());
    assert(m.component.component_type.size() == m.component.subtype_index.size());
    assert(m.component.component_type.size() == m.component.initial_age_s.size());
    assert(m.component.component_type.size() == m.component.report.size());
    assert(m.component.component_type.size() == m.component.tag.size());
    std::unordered_map<size_t, std::set<size_t>> muxCompIdToInflowConns;
    std::unordered_map<size_t, std::set<size_t>> muxCompIdToOutflowConns;
    for (size_t compId = 0; compId < m.component.component_type.size(); ++compId)
    {
        assert(compId < m.component.component_type.size());
        if (m.component.component_type[compId] == ComponentType::mux_type)
        {
            muxCompIdToInflowConns[compId] = std::set<size_t> {};
            muxCompIdToOutflowConns[compId] = std::set<size_t> {};
        }
        ComponentType compType = m.component.component_type[compId];
        size_t idx = m.component.subtype_index[compId];
        std::string const& tag = m.component.tag[compId];
        size_t nConns = m.connection.size();
        ignore(nConns);
        switch (compType)
        {
        case ComponentType::constant_efficiency_converter_type:
        {
            assert(idx < m.constant_efficiency_converter.size());
            ConstantEfficiencyConverter const& cec = m.constant_efficiency_converter[idx];
            size_t inflowConnIdx = cec.inflow_connection_id;
            assert(inflowConnIdx < nConns);
            Connection const& inflowConn = m.connection[inflowConnIdx];
            size_t inflowPort = 0;
            if ((inflowConn.to != compType) || (inflowConn.to_component_id != compId) ||
                (inflowConn.to_subtype_index != idx) || (inflowConn.to_port != inflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     inflowPort,
                                     idx,
                                     compType,
                                     inflowConn,
                                     inflowConnIdx,
                                     FlowDirection::inflow);
            }
            size_t outflowConnIdx = cec.outflow_connection_id;
            assert(outflowConnIdx < nConns);
            Connection const& outflowConn = m.connection[outflowConnIdx];
            size_t outflowPort = 0;
            if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     outflowPort,
                                     idx,
                                     compType,
                                     outflowConn,
                                     outflowConnIdx,
                                     FlowDirection::outflow);
            }
            size_t wasteflowConnIdx = cec.wasteflow_connection_id;
            assert(wasteflowConnIdx < nConns);
            Connection const& wfConn = m.connection[wasteflowConnIdx];
            size_t wfPort = 2;
            if ((wfConn.from != compType) || (wfConn.from_component_id != compId) ||
                (wfConn.from_subtype_index != idx) || (wfConn.from_port != wfPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     wfPort,
                                     idx,
                                     compType,
                                     wfConn,
                                     wasteflowConnIdx,
                                     FlowDirection::outflow);
            }
            if (cec.lossflow_connection_id.has_value())
            {
                size_t lfConnIdx = cec.lossflow_connection_id.value();
                assert(lfConnIdx < nConns);
                Connection const& lfConn = m.connection[lfConnIdx];
                size_t lfPort = 1;
                if ((lfConn.from != compType) || (lfConn.from_component_id != compId) ||
                    (lfConn.from_subtype_index != idx) || (lfConn.from_port != lfPort))
                {
                    add_connection_issue(issues,
                                         tag,
                                         compId,
                                         lfPort,
                                         idx,
                                         compType,
                                         lfConn,
                                         lfConnIdx,
                                         FlowDirection::outflow);
                }
            }
        }
        break;
        case ComponentType::variable_efficiency_converter_type:
        {
            assert(idx < m.variable_efficiency_converter.size());
            VariableEfficiencyConverter const& vec = m.variable_efficiency_converter[idx];
            size_t inflowConnIdx = vec.inflow_connection_id;
            assert(inflowConnIdx < nConns);
            Connection const& inflowConn = m.connection[inflowConnIdx];
            size_t inflowPort = 0;
            if ((inflowConn.to != compType) || (inflowConn.to_component_id != compId) ||
                (inflowConn.to_subtype_index != idx) || (inflowConn.to_port != inflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     inflowPort,
                                     idx,
                                     compType,
                                     inflowConn,
                                     inflowConnIdx,
                                     FlowDirection::inflow);
            }
            size_t outflowConnIdx = vec.outflow_connection_id;
            assert(outflowConnIdx < nConns);
            Connection const& outflowConn = m.connection[outflowConnIdx];
            size_t outflowPort = 0;
            if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     outflowPort,
                                     idx,
                                     compType,
                                     outflowConn,
                                     outflowConnIdx,
                                     FlowDirection::outflow);
            }
            size_t wasteflowConnIdx = vec.wasteflow_connection_id;
            assert(wasteflowConnIdx < nConns);
            Connection const& wfConn = m.connection[wasteflowConnIdx];
            size_t wfPort = 2;
            if ((wfConn.from != compType) || (wfConn.from_component_id != compId) ||
                (wfConn.from_subtype_index != idx) || (wfConn.from_port != wfPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     wfPort,
                                     idx,
                                     compType,
                                     wfConn,
                                     wasteflowConnIdx,
                                     FlowDirection::outflow);
            }
            if (vec.lossflow_connection_id.has_value())
            {
                size_t lfConnIdx = vec.lossflow_connection_id.value();
                assert(lfConnIdx < nConns);
                Connection const& lfConn = m.connection[lfConnIdx];
                size_t lfPort = 1;
                if ((lfConn.from != compType) || (lfConn.from_component_id != compId) ||
                    (lfConn.from_subtype_index != idx) || (lfConn.from_port != lfPort))
                {
                    add_connection_issue(issues,
                                         tag,
                                         compId,
                                         lfPort,
                                         idx,
                                         compType,
                                         lfConn,
                                         lfConnIdx,
                                         FlowDirection::outflow);
                }
            }
        }
        break;
        case ComponentType::constant_load_type:
        {
            assert(idx < m.constant_load.size());
            ConstantLoad const& comp = m.constant_load[idx];
            size_t inflowConnIdx = comp.inflow_connection_id;
            Connection const& inflowConn = m.connection[inflowConnIdx];
            size_t inflowPort = 0;
            if ((inflowConn.to != compType) || (inflowConn.to_component_id != compId) ||
                (inflowConn.to_subtype_index != idx) || (inflowConn.to_port != inflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     inflowPort,
                                     idx,
                                     compType,
                                     inflowConn,
                                     inflowConnIdx,
                                     FlowDirection::inflow);
            }
        }
        break;
        case ComponentType::constant_source_type:
        {
            assert(idx < m.constant_source.size());
            ConstantSource const& comp = m.constant_source[idx];
            size_t outflowConnIdx = comp.outflow_connection_id;
            assert(outflowConnIdx < nConns);
            Connection const& outflowConn = m.connection[outflowConnIdx];
            size_t outflowPort = 0;
            if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     outflowPort,
                                     idx,
                                     compType,
                                     outflowConn,
                                     outflowConnIdx,
                                     FlowDirection::outflow);
            }
        }
        break;
        case ComponentType::environment_source_type:
        {
            // pass
        }
        break;
        case ComponentType::mover_type:
        {
            assert(idx < m.mover.size());
            Mover const& comp = m.mover[idx];
            size_t inflowConnIdx = comp.inflow_connection_id;
            assert(inflowConnIdx < nConns);
            Connection const& inflowConn = m.connection[inflowConnIdx];
            size_t inflowPort = 0;
            if ((inflowConn.to != compType) || (inflowConn.to_component_id != compId) ||
                (inflowConn.to_subtype_index != idx) || (inflowConn.to_port != inflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     inflowPort,
                                     idx,
                                     compType,
                                     inflowConn,
                                     inflowConnIdx,
                                     FlowDirection::inflow);
            }
            size_t outflowConnIdx = comp.outflow_connection_id;
            assert(outflowConnIdx < nConns);
            Connection const& outflowConn = m.connection[outflowConnIdx];
            size_t outflowPort = 0;
            if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     outflowPort,
                                     idx,
                                     compType,
                                     outflowConn,
                                     outflowConnIdx,
                                     FlowDirection::outflow);
            }
            size_t envConnIdx = comp.in_from_env_connection_id;
            assert(envConnIdx < nConns);
            Connection const& envConn = m.connection[envConnIdx];
            size_t envInflowPort = 1;
            if ((envConn.to != compType) || (envConn.to_component_id != compId) ||
                (envConn.to_subtype_index != idx) || (envConn.to_port != envInflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     envInflowPort,
                                     idx,
                                     compType,
                                     envConn,
                                     envConnIdx,
                                     FlowDirection::inflow);
            }
            size_t wConnIdx = comp.wasteflow_connection_id;
            assert(wConnIdx < nConns);
            Connection const& wConn = m.connection[wConnIdx];
            size_t wasteOutflowPort = 1;
            if ((wConn.from != compType) || (wConn.from_component_id != compId) ||
                (wConn.from_subtype_index != idx) || (wConn.from_port != wasteOutflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     wasteOutflowPort,
                                     idx,
                                     compType,
                                     wConn,
                                     wConnIdx,
                                     FlowDirection::outflow);
            }
        }
        break;
        case ComponentType::variable_efficiency_mover_type:
        {
            assert(idx < m.variable_efficiency_mover.size());
            VariableEfficiencyMover const& comp = m.variable_efficiency_mover[idx];
            size_t inflowConnIdx = comp.inflow_connection_id;
            assert(inflowConnIdx < nConns);
            Connection const& inflowConn = m.connection[inflowConnIdx];
            size_t inflowPort = 0;
            if ((inflowConn.to != compType) || (inflowConn.to_component_id != compId) ||
                (inflowConn.to_subtype_index != idx) || (inflowConn.to_port != inflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     inflowPort,
                                     idx,
                                     compType,
                                     inflowConn,
                                     inflowConnIdx,
                                     FlowDirection::inflow);
            }
            size_t outflowConnIdx = comp.outflow_connection_id;
            assert(outflowConnIdx < nConns);
            Connection const& outflowConn = m.connection[outflowConnIdx];
            size_t outflowPort = 0;
            if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     outflowPort,
                                     idx,
                                     compType,
                                     outflowConn,
                                     outflowConnIdx,
                                     FlowDirection::outflow);
            }
            size_t envConnIdx = comp.in_from_env_connection_id;
            assert(envConnIdx < nConns);
            Connection const& envConn = m.connection[envConnIdx];
            size_t envInflowPort = 1;
            if ((envConn.to != compType) || (envConn.to_component_id != compId) ||
                (envConn.to_subtype_index != idx) || (envConn.to_port != envInflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     envInflowPort,
                                     idx,
                                     compType,
                                     envConn,
                                     envConnIdx,
                                     FlowDirection::inflow);
            }
            size_t wConnIdx = comp.wasteflow_connection_id;
            assert(wConnIdx < nConns);
            Connection const& wConn = m.connection[wConnIdx];
            size_t wasteOutflowPort = 1;
            if ((wConn.from != compType) || (wConn.from_component_id != compId) ||
                (wConn.from_subtype_index != idx) || (wConn.from_port != wasteOutflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     wasteOutflowPort,
                                     idx,
                                     compType,
                                     wConn,
                                     wConnIdx,
                                     FlowDirection::outflow);
            }
        }
        break;
        case ComponentType::mux_type:
        {
            assert(idx < m.mux.size());
            Mux const& comp = m.mux[idx];
            for (size_t inPort = 0; inPort < comp.number_of_inports; ++inPort)
            {
                size_t inflowConnIdx = comp.inflow_connection_ids[inPort];
                Connection const& inflowConn = m.connection[inflowConnIdx];
                if ((inflowConn.to != compType) || (inflowConn.to_component_id != compId) ||
                    (inflowConn.to_subtype_index != idx) || (inflowConn.to_port != inPort))
                {
                    add_connection_issue(issues,
                                         tag,
                                         compId,
                                         inPort,
                                         idx,
                                         compType,
                                         inflowConn,
                                         inflowConnIdx,
                                         FlowDirection::inflow);
                }
            }
            for (size_t outPort = 0; outPort < comp.number_of_outports; ++outPort)
            {
                size_t outflowConnIdx = comp.outflow_connection_ids[outPort];
                Connection const& outflowConn = m.connection[outflowConnIdx];
                if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                    (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outPort))
                {
                    add_connection_issue(issues,
                                         tag,
                                         compId,
                                         outPort,
                                         idx,
                                         compType,
                                         outflowConn,
                                         outflowConnIdx,
                                         FlowDirection::outflow);
                }
            }
        }
        break;
        case ComponentType::pass_through_type:
        {
            assert(idx < m.pass_through.size());
            PassThrough const& comp = m.pass_through[idx];
            size_t inflowConnIdx = comp.inflow_connection_id;
            Connection const& inflowConn = m.connection[inflowConnIdx];
            size_t inflowPort = 0;
            if ((inflowConn.to != compType) || (inflowConn.to_component_id != compId) ||
                (inflowConn.to_subtype_index != idx) || (inflowConn.to_port != inflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     inflowPort,
                                     idx,
                                     compType,
                                     inflowConn,
                                     inflowConnIdx,
                                     FlowDirection::inflow);
            }
            size_t outflowConnIdx = comp.outflow_connection_id;
            Connection const& outflowConn = m.connection[outflowConnIdx];
            size_t outflowPort = 0;
            if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     outflowPort,
                                     idx,
                                     compType,
                                     outflowConn,
                                     outflowConnIdx,
                                     FlowDirection::outflow);
            }
        }
        break;
        case ComponentType::switch_type:
        {
            assert(idx < m.transfer_switch.size());
            Switch const& comp = m.transfer_switch[idx];
            size_t primaryInflowConnIdx = comp.inflow_connection_id_primary;
            Connection const& primaryInflowConn = m.connection[primaryInflowConnIdx];
            size_t primaryInflowPort = 0;
            if ((primaryInflowConn.to != compType) ||
                (primaryInflowConn.to_component_id != compId) ||
                (primaryInflowConn.to_subtype_index != idx) ||
                (primaryInflowConn.to_port != primaryInflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     primaryInflowPort,
                                     idx,
                                     compType,
                                     primaryInflowConn,
                                     primaryInflowConnIdx,
                                     FlowDirection::inflow);
            }
            size_t secondaryInflowConnIdx = comp.inflow_connection_id_secondary;
            Connection const& secondaryInflowConn = m.connection[secondaryInflowConnIdx];
            size_t secondaryInflowPort = 1;
            if ((secondaryInflowConn.to != compType) ||
                (secondaryInflowConn.to_component_id != compId) ||
                (secondaryInflowConn.to_subtype_index != idx) ||
                (secondaryInflowConn.to_port != secondaryInflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     secondaryInflowPort,
                                     idx,
                                     compType,
                                     secondaryInflowConn,
                                     secondaryInflowConnIdx,
                                     FlowDirection::inflow);
            }
            size_t outflowConnIdx = comp.outflow_connection_id;
            Connection const& outflowConn = m.connection[outflowConnIdx];
            size_t outflowPort = 0;
            if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     outflowPort,
                                     idx,
                                     compType,
                                     outflowConn,
                                     outflowConnIdx,
                                     FlowDirection::outflow);
            }
        }
        break;
        case ComponentType::schedule_based_load_type:
        {
            assert(idx < m.scheduled_load.size());
            ScheduleBasedLoad const& comp = m.scheduled_load[idx];
            size_t inflowConnIdx = comp.inflow_connection_id;
            Connection const& inflowConn = m.connection[inflowConnIdx];
            size_t inflowPort = 0;
            if ((inflowConn.to != compType) || (inflowConn.to_component_id != compId) ||
                (inflowConn.to_subtype_index != idx) || (inflowConn.to_port != inflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     inflowPort,
                                     idx,
                                     compType,
                                     inflowConn,
                                     inflowConnIdx,
                                     FlowDirection::inflow);
            }
        }
        break;
        case ComponentType::schedule_based_source_type:
        {
            assert(idx < m.scheduled_source.size());
            ScheduleBasedSource const& comp = m.scheduled_source[idx];
            size_t outflowConnIdx = comp.outflow_connection_id;
            Connection const& outflowConn = m.connection[outflowConnIdx];
            size_t outflowPort = 0;
            if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     outflowPort,
                                     idx,
                                     compType,
                                     outflowConn,
                                     outflowConnIdx,
                                     FlowDirection::outflow);
            }
            size_t wfConnIdx = comp.wasteflow_connection_id;
            Connection const& wfConn = m.connection[wfConnIdx];
            size_t wfPort = 1;
            if ((wfConn.from != compType) || (wfConn.from_component_id != compId) ||
                (wfConn.from_subtype_index != idx) || (wfConn.from_port != wfPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     wfPort,
                                     idx,
                                     compType,
                                     wfConn,
                                     wfConnIdx,
                                     FlowDirection::outflow);
            }
        }
        break;
        case ComponentType::store_type:
        {
            assert(idx < m.store.size());
            Store const& comp = m.store[idx];
            size_t outflowConnIdx = comp.outflow_connection_id;
            Connection const& outflowConn = m.connection[outflowConnIdx];
            size_t outflowPort = 0;
            if ((outflowConn.from != compType) || (outflowConn.from_component_id != compId) ||
                (outflowConn.from_subtype_index != idx) || (outflowConn.from_port != outflowPort))
            {
                add_connection_issue(issues,
                                     tag,
                                     compId,
                                     outflowPort,
                                     idx,
                                     compType,
                                     outflowConn,
                                     outflowConnIdx,
                                     FlowDirection::outflow);
            }
            if (comp.inflow_connection_id.has_value())
            {
                size_t inflowConnIdx = comp.inflow_connection_id.value();
                Connection const& inflowConn = m.connection[inflowConnIdx];
                size_t inflowPort = 0;
                if ((inflowConn.to != compType) || (inflowConn.to_component_id != compId) ||
                    (inflowConn.to_subtype_index != idx) || (inflowConn.to_port != inflowPort))
                {
                    add_connection_issue(issues,
                                         tag,
                                         compId,
                                         inflowPort,
                                         idx,
                                         compType,
                                         inflowConn,
                                         inflowConnIdx,
                                         FlowDirection::inflow);
                }
            }
            if (comp.wasteflow_connection_id.has_value())
            {
                size_t wfConnIdx = comp.wasteflow_connection_id.value();
                Connection const& wfConn = m.connection[wfConnIdx];
                size_t wfPort = 1;
                if ((wfConn.from != compType) || (wfConn.from_component_id != compId) ||
                    (wfConn.from_subtype_index != idx) || (wfConn.from_port != wfPort))
                {
                    add_connection_issue(issues,
                                         tag,
                                         compId,
                                         wfPort,
                                         idx,
                                         compType,
                                         wfConn,
                                         wfConnIdx,
                                         FlowDirection::outflow);
                }
            }
        }
        break;
        case ComponentType::waste_sink_type:
        {
            // pass
        }
        break;
        default:
        {
            std::cout << "unhandled component type: " + ToString(compType) << std::endl;
            std::exit(1);
        }
        break;
        }
    }
    for (size_t connIdx = 0; connIdx < m.connection.size(); ++connIdx)
    {
        Connection const& conn = m.connection[connIdx];
        // asser that component port links back to connection
        ComponentType fromType = conn.from;
        ComponentType toType = conn.to;
        if (fromType == ComponentType::mux_type)
        {
            auto const& fromMux = m.mux[conn.from_subtype_index];
            if (conn.from_port >= fromMux.outflow_connection_ids.size())
            {
                std::ostringstream oss;
                oss << "mux outflow connection inconsistent\n";
                oss << "conn.FromId: " << conn.from_component_id << "\n";
                oss << "conn.FromPort: " << conn.from_port << "\n";
                oss << "mux num outflow connections: " << fromMux.number_of_outports << "\n";
                oss << "mux num outflow connections from count: "
                    << fromMux.outflow_connection_ids.size() << "\n";
                issues.push_back(oss.str());
            }
            else if (fromMux.outflow_connection_ids[conn.from_port] != connIdx)
            {
                std::ostringstream oss;
                oss << "mux outflow connection inconsistent\n";
                oss << "connId: " << connIdx << "\n";
                oss << "conn.FromId: " << conn.from_component_id << "\n";
                oss << "conn.FromPort: " << conn.from_port << "\n";
                oss << "fromMux.OutflowConns[conn.FromPort]: "
                    << fromMux.outflow_connection_ids[conn.from_port] << "\n";
                issues.push_back(oss.str());
            }
            if (fromMux.number_of_outports != fromMux.outflow_connection_ids.size())
            {
                std::ostringstream oss;
                oss << "mux num outflows inconsistent\n";
                oss << "mux num outflow connections: " << fromMux.number_of_outports << "\n";
                oss << "mux num outflow connections from count: "
                    << fromMux.outflow_connection_ids.size() << "\n";
                issues.push_back(oss.str());
            }
            if (muxCompIdToOutflowConns[conn.from_component_id].contains(connIdx))
            {
                std::ostringstream oss;
                oss << "mux has multiple instances of the same outflow "
                       "connection\n";
                oss << "connIdx: " << connIdx << "\n";
                issues.push_back(oss.str());
            }
            muxCompIdToOutflowConns[conn.from_component_id].insert(connIdx);
        }
        if (toType == ComponentType::mux_type)
        {
            auto const& toMux = m.mux[conn.to_subtype_index];
            if (conn.to_port >= toMux.inflow_connection_ids.size())
            {
                std::ostringstream oss;
                oss << "mux inflow connection inconsistent\n";
                oss << "conn.ToId: " << conn.to_component_id << "\n";
                oss << "conn.ToPort: " << conn.to_port << "\n";
                oss << "mux num inflow connections: " << toMux.number_of_inports << "\n";
                oss << "mux num inflow connections from count: "
                    << toMux.inflow_connection_ids.size() << "\n";
                issues.push_back(oss.str());
            }
            else if (toMux.inflow_connection_ids[conn.to_port] != connIdx)
            {
                std::ostringstream oss;
                oss << "mux inflow connection inconsistent\n";
                oss << "connId: " << connIdx << "\n";
                oss << "conn.ToId: " << conn.to_component_id << "\n";
                oss << "conn.FromPort: " << conn.to_port << "\n";
                oss << "toMux.InflowConns[conn.FromPort]: "
                    << toMux.outflow_connection_ids[conn.to_port] << "\n";
                issues.push_back(oss.str());
            }
            if (toMux.number_of_inports != toMux.inflow_connection_ids.size())
            {
                std::ostringstream oss;
                oss << "mux num inflows inconsistent\n";
                oss << "mux num inflow connections: " << toMux.number_of_inports << "\n";
                oss << "mux num inflow connections from count: "
                    << toMux.inflow_connection_ids.size() << "\n";
                issues.push_back(oss.str());
            }
            if (muxCompIdToInflowConns[conn.to_component_id].contains(connIdx))
            {
                std::ostringstream oss;
                oss << "mux has multiple instances of the same inflow "
                       "connection\n";
                oss << "connIdx: " << connIdx << "\n";
                issues.push_back(oss.str());
            }
            muxCompIdToInflowConns[conn.to_component_id].insert(connIdx);
        }
        std::string outflowCompPort;
        {
            std::ostringstream oss;
            oss << conn.from_component_id << ":" << conn.from_port;
            outflowCompPort = oss.str();
        };
        if (connectedOutflowPorts.contains(outflowCompPort))
        {
            std::ostringstream oss;
            oss << "Port multiply connected: "
                << "- outflowCompPort: " << outflowCompPort << "\n"
                << "- compId: " << conn.from_component_id << "\n"
                << "- outflowPort: " << conn.from_port << "\n"
                << "- tag: " << m.component.tag[conn.from_component_id] << "\n"
                << "- type: " << ToString(m.component.component_type[conn.from_component_id])
                << "\n";
            issues.push_back(oss.str());
        }
        connectedOutflowPorts.insert(outflowCompPort);
        std::string inflowCompPort;
        {
            std::ostringstream oss;
            oss << conn.to_component_id << ":" << conn.to_port;
            inflowCompPort = oss.str();
        };
        if (connectedInflowPorts.contains(inflowCompPort))
        {
            std::ostringstream oss;
            oss << "Port multiply connected: "
                << "- inflowCompPort: " << inflowCompPort << "\n"
                << "- compId: " << conn.to_component_id << "\n"
                << "- outflowPort: " << conn.to_port << "\n"
                << "- tag: " << m.component.tag[conn.to_component_id] << "\n"
                << "- type: " << ToString(m.component.component_type[conn.to_component_id]) << "\n";
            issues.push_back(oss.str());
        }
        connectedInflowPorts.insert(inflowCompPort);
    }
    for (size_t compId = 0; compId < m.component.component_type.size(); ++compId)
    {
        if (m.component.component_type[compId] == ComponentType::mux_type)
        {
            Mux const& mux = m.mux[m.component.subtype_index[compId]];
            if (mux.number_of_inports != muxCompIdToInflowConns[compId].size())
            {
                std::ostringstream oss;
                oss << "mux specifies " << mux.number_of_inports
                    << " inports but the number of connections are "
                    << muxCompIdToInflowConns[compId].size() << "\n";
                issues.push_back(oss.str());
            }
            if (mux.number_of_outports != muxCompIdToOutflowConns[compId].size())
            {
                std::ostringstream oss;
                oss << "mux specifies " << mux.number_of_outports
                    << " outports but the number of connections are "
                    << muxCompIdToOutflowConns[compId].size() << "\n";
                issues.push_back(oss.str());
            }
        }
    }
    return issues;
}

std::vector<TimeAndAmount> convert_to_time_and_amounts(
    std::vector<std::vector<double>> const& input, double timeToSeconds, double rateToWatts)
{
    std::vector<TimeAndAmount> result;
    result.reserve(input.size());
    for (auto const& xs : input)
    {
        assert(xs.size() >= 2);
        assert(xs[1] * rateToWatts <= max_flow_W);
        TimeAndAmount taa {
            .Time_s = xs[0] * timeToSeconds,
            .Amount_W = static_cast<flow_t>(xs[1] * rateToWatts),
        };
        result.push_back(std::move(taa));
    }
    return result;
}

std::optional<FragilityCurveType> tag_to_fragility_curve_type(std::string const& tag)
{
    if (tag == "linear")
    {
        return FragilityCurveType::linear;
    }
    if (tag == "tabular")
    {
        return FragilityCurveType::tabular;
    }
    return {};
}

std::string fragility_curve_type_to_tag(FragilityCurveType fctype)
{
    std::string tag;
    switch (fctype)
    {
    case (FragilityCurveType::linear):
    {
        tag = "linear";
    }
    break;
    case (FragilityCurveType::tabular):
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

std::optional<size_t> get_intensity_id_by_tag(IntensityDict intenseDict, std::string const& tag)
{
    for (size_t i = 0; i < intenseDict.tag.size(); ++i)
    {
        if (intenseDict.tag[i] == tag)
        {
            return i;
        }
    }
    return {};
}

size_t add_component_returning_id(ComponentDict& c, ComponentType ct, size_t idx)
{
    return add_component_returning_id(
        c, ct, idx, std::vector<size_t>(), std::vector<size_t>(), "", 0.0);
}

size_t add_component_returning_id(ComponentDict& c,
                                  ComponentType ct,
                                  size_t idx,
                                  std::vector<size_t> inflowType,
                                  std::vector<size_t> outflowType,
                                  std::string const& tag,
                                  double initialAge_s,
                                  bool report)
{
    size_t id = c.component_type.size();
    c.component_type.push_back(ct);
    c.subtype_index.push_back(idx);
    c.tag.push_back(tag);
    c.initial_age_s.push_back(initialAge_s);
    c.inflow_type.push_back(inflowType);
    c.outflow_type.push_back(outflowType);
    c.report.push_back(report);
    return id;
}

size_t count_active_connections(SimulationState const& ss)
{
    return (ss.active_connections_back.size() + ss.active_connections_front.size());
}

void add_if_not_added(std::vector<size_t>& items, size_t item)
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

SwitchState get_switch_state(SimulationState const& ss, size_t const& switchIdx)
{
    assert(switchIdx < ss.switch_states.size());
    return ss.switch_states[switchIdx];
}

void set_switch_state(SimulationState& ss, size_t const& switchIdx, SwitchState newState)
{
    assert(switchIdx < ss.switch_states.size());
    ss.switch_states[switchIdx] = newState;
}

void add_active_connection_back(SimulationState& ss, size_t connIdx)
{
    ss.active_connections_back.insert(connIdx);
}

void add_active_connection_forward(SimulationState& ss, size_t connIdx)
{
    ss.active_connections_front.insert(connIdx);
}

void activate_constant_load_connections(Model const& model, SimulationState& ss)
{
    for (size_t loadIdx = 0; loadIdx < model.constant_load.size(); ++loadIdx)
    {
        size_t connIdx = model.constant_load[loadIdx].inflow_connection_id;
        if (ss.flows[connIdx].requested_W != model.constant_load[loadIdx].load_W)
        {
            ss.active_connections_back.insert(connIdx);
        }
        ss.flows[connIdx].requested_W = model.constant_load[loadIdx].load_W;
    }
}

void activate_constant_source_connections(Model const& m, SimulationState& ss)
{
    for (size_t srcIdx = 0; srcIdx < m.constant_source.size(); ++srcIdx)
    {
        size_t connIdx = m.constant_source[srcIdx].outflow_connection_id;
        size_t compId = m.connection[connIdx].from_component_id;
        if (ss.unavailable_components.contains(compId))
        {
            if (ss.flows[connIdx].available_W != 0)
            {
                ss.active_connections_front.insert(connIdx);
            }
            ss.flows[connIdx].available_W = 0;
            continue;
        }
        if (ss.flows[connIdx].available_W != m.constant_source[srcIdx].available_W)
        {
            ss.active_connections_front.insert(connIdx);
        }
        ss.flows[connIdx].available_W = m.constant_source[srcIdx].available_W;
    }
}

void activate_schedule_based_load_connections(Model const& m, SimulationState& ss, double t)
{
    for (size_t i = 0; i < m.scheduled_load.size(); ++i)
    {
        size_t connIdx = m.scheduled_load[i].inflow_connection_id;
        size_t idx = ss.schedule_based_load_index[i];
        if (idx < m.scheduled_load[i].times_and_loads.size())
        {
            auto const& tal = m.scheduled_load[i].times_and_loads[idx];
            if (tal.Time_s == t)
            {

                if (ss.flows[connIdx].requested_W != tal.Amount_W)
                {
                    ss.active_connections_back.insert(connIdx);
                }
                ss.flows[connIdx].requested_W = tal.Amount_W;
            }
        }
    }
}

void activate_schedule_based_source_connections(Model const& m, SimulationState& ss, double t)
{
    for (size_t i = 0; i < m.scheduled_source.size(); ++i)
    {
        ScheduleBasedSource const& sbs = m.scheduled_source[i];
        auto outIdx = sbs.outflow_connection_id;
        size_t compId = m.connection[outIdx].from_component_id;
        if (ss.unavailable_components.contains(compId))
        {
            if (ss.flows[outIdx].available_W != 0)
            {
                ss.active_connections_front.insert(outIdx);
            }
            ss.flows[outIdx].available_W = 0;
            continue;
        }
        auto idx = ss.schedule_based_source_index[i];
        if (idx < sbs.time_and_availables.size())
        {
            auto const& taa = sbs.time_and_availables[idx];
            if (taa.Time_s == t)
            {
                flow_t outAvail_W =
                    taa.Amount_W > sbs.max_outflow_W ? sbs.max_outflow_W : taa.Amount_W;
                if (ss.flows[outIdx].available_W != outAvail_W)
                {
                    ss.active_connections_front.insert(outIdx);
                }
                ss.flows[outIdx].available_W = outAvail_W;
                auto spillage = outAvail_W > ss.flows[outIdx].requested_W
                                    ? (outAvail_W - ss.flows[outIdx].requested_W)
                                    : 0;
                auto wasteIdx = m.scheduled_source[i].wasteflow_connection_id;
                ss.flows[wasteIdx].requested_W = spillage;
                ss.flows[wasteIdx].available_W = spillage;
            }
        }
    }
}

void activate_store_connections(Model& m, SimulationState& ss, double t)
{
    for (size_t storeIdx = 0; storeIdx < m.store.size(); ++storeIdx)
    {
        Store const& store = m.store[storeIdx];
        std::optional<size_t> maybeInflowConn = store.inflow_connection_id;
        size_t outflowConn = store.outflow_connection_id;
        size_t compId = m.connection[outflowConn].from_component_id;
        if (ss.unavailable_components.contains(compId))
        {
            if (maybeInflowConn.has_value())
            {
                size_t inflowConn = maybeInflowConn.value();
                if (ss.flows[inflowConn].requested_W != 0)
                {
                    ss.active_connections_back.insert(inflowConn);
                }
                ss.flows[inflowConn].requested_W = 0;
            }
            if (ss.flows[outflowConn].available_W != 0)
            {
                ss.active_connections_front.insert(outflowConn);
            }
            ss.flows[outflowConn].available_W = 0;
            continue;
        }
        bool isSource = !maybeInflowConn.has_value();
        if (ss.storage_next_event_times[storeIdx] == t || isSource)
        {
            flow_t available_W =
                ss.storage_amounts_J[storeIdx] > 0 ? store.max_discharge_rate_W : 0;
            if (maybeInflowConn.has_value())
            {
                size_t inflowConn = maybeInflowConn.value();
                available_W = safe_add(available_W, ss.flows[inflowConn].available_W);
            }
            if (available_W > store.max_outflow_W)
            {
                available_W = store.max_outflow_W;
            }
            if (ss.flows[outflowConn].available_W != available_W)
            {
                ss.active_connections_front.insert(outflowConn);
            }
            ss.flows[outflowConn].available_W = available_W;
            if (maybeInflowConn.has_value())
            {
                flow_t request_W = (ss.flows[outflowConn].requested_W > store.max_outflow_W
                                        ? store.max_outflow_W
                                        : ss.flows[outflowConn].requested_W) +
                                   (ss.storage_amounts_J[storeIdx] <= store.charge_amount_J
                                        ? store.max_charge_rate_W
                                        : 0);
                size_t inflowConn = maybeInflowConn.value();
                if (ss.flows[inflowConn].requested_W != request_W)
                {
                    ss.active_connections_back.insert(inflowConn);
                }
                ss.flows[inflowConn].requested_W = request_W;
            }
            continue;
        }
        // TODO: add case for store soc < charge amount
        //       and has an inflow connection
        //       and not requesting charge yet.
    }
}

void activate_reliability_connections(Model& m, SimulationState& ss, double time, bool verbose)
{
    for (auto const& rel : m.reliability)
    {
        for (auto const& ts : rel.time_states)
        {
            if (ts.time == time)
            {
                if (ts.state)
                {
                    Model_SetComponentToRepaired(m, ss, rel.component_id);
                    if (verbose)
                    {
                        std::cout << "... REPAIRED: " << m.component.tag[rel.component_id] << "["
                                  << rel.component_id << "]" << std::endl;
                    }
                }
                else
                {
                    Model_SetComponentToFailed(m, ss, rel.component_id);
                    if (verbose)
                    {
                        std::cout << "... FAILED: " << m.component.tag[rel.component_id] << "["
                                  << rel.component_id << "]" << std::endl;
                        std::cout << "... causes: " << std::endl;
                        for (auto const& fragCause : ts.fragilityModeCauses)
                        {
                            std::cout << "... ... fragility mode: " << fragCause << std::endl;
                        }
                        for (auto const& failCause : ts.failureModeCauses)
                        {
                            std::cout << "... ... failure mode: " << failCause << std::endl;
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

double get_next_time(double nextTime, size_t count, std::function<double(size_t)> f)
{
    for (size_t i = 0; i < count; ++i)
    {
        double nextTimeForComponent = f(i);
        if (nextTime == infinite_time ||
            (nextTimeForComponent >= 0.0 && nextTimeForComponent < nextTime))
        {
            nextTime = nextTimeForComponent;
        }
    }
    return nextTime;
}

double earliest_next_event(Model const& m, SimulationState const& ss, double t)
{
    double next = infinite_time;
    next = get_next_time(next,
                       m.scheduled_load.size(),
                       [&](size_t i) -> double { return NextEvent(m.scheduled_load[i], i, ss); });
    next = get_next_time(next,
                       m.scheduled_source.size(),
                       [&](size_t i) -> double { return NextEvent(m.scheduled_source[i], i, ss); });
    next = get_next_time(
        next, m.store.size(), [&](size_t i) -> double { return NextStorageEvent(ss, i, t); });
    next = get_next_time(next,
                       m.reliability.size(),
                       [&](size_t i) -> double { return NextEvent(m.reliability[i], t); });
    return next;
}

std::optional<size_t>
FindOutflowConnection(Model const& m, ComponentType ct, size_t compId, size_t outflowPort)
{
    for (size_t connIdx = 0; connIdx < m.connection.size(); ++connIdx)
    {
        if (m.connection[connIdx].from == ct &&
            m.connection[connIdx].from_subtype_index == compId &&
            m.connection[connIdx].from_port == outflowPort)
        {
            return connIdx;
        }
    }
    return {};
}

void UpdateConverterLossflowAndWasteflow(SimulationState& ss,
                                         size_t inflowConn,
                                         size_t outflowConn,
                                         std::optional<size_t> lossflowConn,
                                         size_t wasteflowConn,
                                         flow_t maxOutflow_W,
                                         flow_t maxLossflow_W)
{
    flow_t inflowRequest = ss.flows[inflowConn].requested_W;
    flow_t inflowAvailable = ss.flows[inflowConn].available_W;
    flow_t outflowRequest = ss.flows[outflowConn].requested_W;
    flow_t outflowAvailable = ss.flows[outflowConn].available_W > maxOutflow_W
                                  ? maxOutflow_W
                                  : ss.flows[outflowConn].available_W;
    flow_t inflow = FinalizeFlowValue(inflowRequest, inflowAvailable);
    flow_t outflow = FinalizeFlowValue(outflowRequest, outflowAvailable);
    // NOTE: for COP, we can have outflow > inflow, but we assume no
    // lossflow in that scenario
    flow_t nonOutflowAvailable = inflow > outflow ? inflow - outflow : 0;
    flow_t lossflowRequest = 0;
    if (lossflowConn.has_value())
    {
        lossflowRequest = ss.flows[lossflowConn.value()].requested_W;
        if (lossflowRequest > maxLossflow_W)
        {
            lossflowRequest = maxLossflow_W;
        }
        flow_t nonOutflowAvailableLimited =
            nonOutflowAvailable > maxLossflow_W ? maxLossflow_W : nonOutflowAvailable;
        if (nonOutflowAvailableLimited != ss.flows[lossflowConn.value()].available_W)
        {
            ss.active_connections_front.insert(lossflowConn.value());
        }
        ss.flows[lossflowConn.value()].available_W = nonOutflowAvailableLimited;
    }
    flow_t wasteflow =
        nonOutflowAvailable > lossflowRequest ? nonOutflowAvailable - lossflowRequest : 0;
    ss.flows[wasteflowConn].requested_W = wasteflow;
    ss.flows[wasteflowConn].available_W = wasteflow;
}

void UpdateConstantEfficiencyLossflowAndWasteflow(Model const& m,
                                                  SimulationState& ss,
                                                  size_t compIdx)
{
    ConstantEfficiencyConverter const& cec = m.constant_efficiency_converter[compIdx];
    UpdateConverterLossflowAndWasteflow(ss,
                                        cec.inflow_connection_id,
                                        cec.outflow_connection_id,
                                        cec.lossflow_connection_id,
                                        cec.wasteflow_connection_id,
                                        cec.max_outflow_W,
                                        cec.max_lossflow_W);
}

void UpdateVariableEfficiencyLossflowAndWasteflow(Model const& m,
                                                  SimulationState& ss,
                                                  size_t compIdx)
{
    VariableEfficiencyConverter const& vec = m.variable_efficiency_converter[compIdx];
    UpdateConverterLossflowAndWasteflow(ss,
                                        vec.inflow_connection_id,
                                        vec.outflow_connection_id,
                                        vec.lossflow_connection_id,
                                        vec.wasteflow_connection_id,
                                        vec.max_outflow_W,
                                        vec.max_lossflow_W);
}

void RunConstantEfficiencyConverterBackward(Model const& m,
                                            SimulationState& ss,
                                            size_t outflowConnIdx,
                                            size_t compIdx)
{
    ConstantEfficiencyConverter const& cec = m.constant_efficiency_converter[compIdx];
    assert(cec.outflow_connection_id == outflowConnIdx);
    flow_t outflowRequest_W = ss.flows[outflowConnIdx].requested_W > cec.max_outflow_W
                                  ? cec.max_outflow_W
                                  : ss.flows[outflowConnIdx].requested_W;
    flow_t inflowRequest_W = static_cast<flow_t>(std::ceil(outflowRequest_W / cec.efficiency));
    assert(inflowRequest_W >= outflowRequest_W);
    if (inflowRequest_W != ss.flows[cec.inflow_connection_id].requested_W)
    {
        ss.active_connections_back.insert(cec.inflow_connection_id);
    }
    ss.flows[cec.inflow_connection_id].requested_W = inflowRequest_W;
    UpdateConstantEfficiencyLossflowAndWasteflow(m, ss, compIdx);
}

void RunVariableEfficiencyConverterBackward(Model const& m,
                                            SimulationState& ss,
                                            size_t outflowConnIdx,
                                            size_t compIdx)
{
    VariableEfficiencyConverter const& vec = m.variable_efficiency_converter[compIdx];
    assert(outflowConnIdx == vec.outflow_connection_id);
    size_t inflowConnIdx = vec.inflow_connection_id;
    flow_t outflowRequest_W = ss.flows[outflowConnIdx].requested_W > vec.max_outflow_W
                                  ? vec.max_outflow_W
                                  : ss.flows[outflowConnIdx].requested_W;
    double efficiency = LookupTable_LookupInterp(
        vec.outflows_for_efficiency_W, vec.efficiencies, static_cast<double>(outflowRequest_W));
    assert(efficiency > 0.0 && efficiency <= 1.0);
    flow_t inflowRequest_W = static_cast<flow_t>(std::ceil(outflowRequest_W / efficiency));
    if (inflowRequest_W != ss.flows[inflowConnIdx].requested_W)
    {
        ss.active_connections_back.insert(inflowConnIdx);
    }
    ss.flows[inflowConnIdx].requested_W = inflowRequest_W;
    UpdateVariableEfficiencyLossflowAndWasteflow(m, ss, compIdx);
}

void UpdateEnvironmentFlowForAllMovers(SimulationState& ss,
                                       size_t inflowConn,
                                       size_t envConn,
                                       size_t outflowConn,
                                       size_t wasteflowConn,
                                       flow_t maxOutflow_W)
{
    flow_t inflowReq = ss.flows[inflowConn].requested_W;
    flow_t inflowAvail = ss.flows[inflowConn].available_W;
    flow_t outflowReq = ss.flows[outflowConn].requested_W;
    flow_t outflowAvail = ss.flows[outflowConn].available_W > maxOutflow_W
                              ? maxOutflow_W
                              : ss.flows[outflowConn].available_W;
    flow_t inflow = FinalizeFlowValue(inflowReq, inflowAvail);
    flow_t outflow = FinalizeFlowValue(outflowReq, outflowAvail);
    if (inflow >= outflow)
    {
        ss.flows[envConn].requested_W = 0;
        ss.flows[envConn].available_W = 0;
        ss.flows[envConn].actual_W = 0;
        flow_t waste = inflow - outflow;
        ss.flows[wasteflowConn].requested_W = waste;
        ss.flows[wasteflowConn].available_W = waste;
        ss.flows[wasteflowConn].actual_W = waste;
    }
    else
    {
        flow_t supply = outflow - inflow;
        ss.flows[envConn].requested_W = supply;
        ss.flows[envConn].available_W = supply;
        ss.flows[envConn].actual_W = supply;
        ss.flows[wasteflowConn].requested_W = 0;
        ss.flows[wasteflowConn].available_W = 0;
        ss.flows[wasteflowConn].actual_W = 0;
    }
}

void UpdateEnvironmentFlowForMover(Model const& m, SimulationState& ss, size_t moverIdx)
{
    Mover const& mov = m.mover[moverIdx];
    UpdateEnvironmentFlowForAllMovers(ss,
                                      mov.inflow_connection_id,
                                      mov.in_from_env_connection_id,
                                      mov.outflow_connection_id,
                                      mov.wasteflow_connection_id,
                                      mov.max_outflow_W);
}

void UpdateEnvironmentFlowForVariableEfficiencyMover(Model const& m,
                                                     SimulationState& ss,
                                                     size_t moverIdx)
{
    VariableEfficiencyMover const& mov = m.variable_efficiency_mover[moverIdx];
    UpdateEnvironmentFlowForAllMovers(ss,
                                      mov.inflow_connection_id,
                                      mov.in_from_env_connection_id,
                                      mov.outflow_connection_id,
                                      mov.wasteflow_connection_id,
                                      mov.max_outflow_W);
}

void RunMoverBackward(Model const& m, SimulationState& ss, size_t outflowConnIdx, size_t moverIdx)
{
    Mover const& mov = m.mover[moverIdx];
    size_t inflowConn = mov.inflow_connection_id;
    flow_t outflowRequest = ss.flows[outflowConnIdx].requested_W > mov.max_outflow_W
                                ? mov.max_outflow_W
                                : ss.flows[outflowConnIdx].requested_W;
    flow_t inflowRequest = static_cast<flow_t>(std::ceil(outflowRequest / mov.COP));
    if (inflowRequest != ss.flows[inflowConn].requested_W)
    {
        ss.active_connections_back.insert(inflowConn);
    }
    ss.flows[inflowConn].requested_W = inflowRequest;
    UpdateEnvironmentFlowForMover(m, ss, moverIdx);
}

void RunVariableEfficiencyMoverBackward(Model const& m,
                                        SimulationState& ss,
                                        size_t outflowConnIdx,
                                        size_t moverIdx)
{
    VariableEfficiencyMover const& mov = m.variable_efficiency_mover[moverIdx];
    size_t inflowConn = mov.inflow_connection_id;
    flow_t outflowRequest_W = ss.flows[outflowConnIdx].requested_W > mov.max_outflow_W
                                  ? mov.max_outflow_W
                                  : ss.flows[outflowConnIdx].requested_W;
    double cop = LookupTable_LookupInterp(
        mov.outflows_for_COP_W, mov.COPs, static_cast<double>(outflowRequest_W));
    // outflow = COP * inflow
    // inflow = outflow / COP
    flow_t inflowRequest_W =
        static_cast<flow_t>(std::ceil(static_cast<double>(outflowRequest_W) / cop));
    if (inflowRequest_W != ss.flows[inflowConn].requested_W)
    {
        ss.active_connections_back.insert(inflowConn);
    }
    ss.flows[inflowConn].requested_W = inflowRequest_W;
    UpdateEnvironmentFlowForVariableEfficiencyMover(m, ss, moverIdx);
}

void RunSwitchBackward(Model const& m, SimulationState& ss, size_t outflowConnIdx, size_t switchIdx)
{
    assert(switchIdx < ss.switch_states.size());
    auto switchState = ss.switch_states[switchIdx];
    auto const& theSwitch = m.transfer_switch[switchIdx];
    assert(theSwitch.inflow_connection_id_primary < m.connection.size());
    auto inflow0ConnIdx = theSwitch.inflow_connection_id_primary;
    auto inflow1ConnIdx = theSwitch.inflow_connection_id_secondary;
    switch (switchState)
    {
    case SwitchState::primary:
    {
        // send request on primary
        if (ss.flows[inflow0ConnIdx].requested_W != ss.flows[outflowConnIdx].requested_W)
        {
            ss.active_connections_back.insert(inflow0ConnIdx);
        }
        ss.flows[inflow0ConnIdx].requested_W = ss.flows[outflowConnIdx].requested_W;
        // set request on secondary to 0
        if (ss.flows[inflow0ConnIdx].requested_W != 0)
        {
            ss.active_connections_back.insert(inflow1ConnIdx);
        }
        ss.flows[inflow1ConnIdx].requested_W = 0;
    }
    break;
    case SwitchState::secondary:
    {
        // send request on secondary
        if (ss.flows[inflow0ConnIdx].requested_W != 0)
        {
            ss.active_connections_back.insert(inflow0ConnIdx);
        }
        ss.flows[inflow0ConnIdx].requested_W = 0;
        // set request on primary to 0
        if (ss.flows[inflow1ConnIdx].requested_W != ss.flows[outflowConnIdx].requested_W)
        {
            ss.active_connections_back.insert(inflow1ConnIdx);
        }
        ss.flows[inflow1ConnIdx].requested_W = ss.flows[outflowConnIdx].requested_W;
    }
    break;
    default:
    {
        write_error_message("<runtime>", "unhandled switch state");
        std::exit(1);
    }
    break;
    }
}

void Mux_RequestInflowsIntelligently(SimulationState& ss,
                                     std::vector<size_t> const& inflowConns,
                                     flow_t remainingRequest_W)
{
    for (size_t inflowConnIdx : inflowConns)
    {
        if (ss.flows[inflowConnIdx].requested_W != remainingRequest_W)
        {
            ss.active_connections_back.insert(inflowConnIdx);
        }
        ss.flows[inflowConnIdx].requested_W = remainingRequest_W;
        remainingRequest_W = remainingRequest_W > ss.flows[inflowConnIdx].available_W
                                 ? remainingRequest_W - ss.flows[inflowConnIdx].available_W
                                 : 0;
    }
}

void Mux_BalanceRequestFlows(SimulationState& ss,
                             std::vector<size_t> const& inflowConns,
                             flow_t remainingRequest_W,
                             bool logNewActivity)
{
    std::vector<flow_t> requests_W(inflowConns.size(), 0);
    for (size_t i = 0; i < inflowConns.size(); ++i)
    {
        size_t inflowConn = inflowConns[i];
        flow_t req_W = remainingRequest_W >= ss.flows[inflowConn].available_W
                           ? ss.flows[inflowConn].available_W
                           : remainingRequest_W;
        requests_W[i] = req_W;
        remainingRequest_W -= req_W;
    }
    if (remainingRequest_W > 0)
    {
        requests_W[0] = safe_add(requests_W[0], remainingRequest_W);
    }
    for (size_t i = 0; i < inflowConns.size(); ++i)
    {
        size_t inflowConn = inflowConns[i];
        if (logNewActivity && ss.flows[inflowConn].requested_W != requests_W[i])
        {
            ss.active_connections_back.insert(inflowConn);
        }
        ss.flows[inflowConn].requested_W = requests_W[i];
    }
}

void BalanceMuxRequests(Model& model, SimulationState& ss, size_t muxIdx, bool isUnavailable)
{
    Mux const& mux = model.mux[muxIdx];
    flow_t totalRequest = 0;
    if (isUnavailable)
    {
        Mux_BalanceRequestFlows(ss, mux.inflow_connection_ids, totalRequest, true);
    }
    else
    {
        for (size_t i = 0; i < mux.number_of_outports; ++i)
        {
            auto outflowConnIdx = mux.outflow_connection_ids[i];
            auto outflowRequest_W = ss.flows[outflowConnIdx].requested_W > mux.max_outflows_W[i]
                                        ? mux.max_outflows_W[i]
                                        : ss.flows[outflowConnIdx].requested_W;
            totalRequest = safe_add(totalRequest, outflowRequest_W);
        }
        Mux_BalanceRequestFlows(ss, mux.inflow_connection_ids, totalRequest, true);
    }
}

void RunMuxBackward(Model& model, SimulationState& ss, size_t muxIdx)
{
    assert(muxIdx < model.mux.size());
    Mux const& mux = model.mux[muxIdx];
    flow_t totalOutflowRequest_W = 0;
    for (size_t i = 0; i < mux.number_of_outports; ++i)
    {
        auto outflowConnIdx = mux.outflow_connection_ids[i];
        auto outflowRequest_W = ss.flows[outflowConnIdx].requested_W > mux.max_outflows_W[i]
                                    ? mux.max_outflows_W[i]
                                    : ss.flows[outflowConnIdx].requested_W;
        totalOutflowRequest_W = safe_add(totalOutflowRequest_W, outflowRequest_W);
    }
    Mux_RequestInflowsIntelligently(
        ss, model.mux[muxIdx].inflow_connection_ids, totalOutflowRequest_W);
}

void RunStoreBackward(Model& model, SimulationState& ss, size_t outflowConnIdx, size_t storeIdx)
{
    Store const& store = model.store[storeIdx];
    assert(outflowConnIdx == store.outflow_connection_id);
    if (store.inflow_connection_id.has_value())
    {
        flow_t chargeRate_W =
            ss.storage_amounts_J[storeIdx] <= store.charge_amount_J ? store.max_charge_rate_W : 0;
        flow_t outRequest_W = ss.flows[outflowConnIdx].requested_W > store.max_outflow_W
                                  ? store.max_outflow_W
                                  : ss.flows[outflowConnIdx].requested_W;
        flow_t totalRequest_W = outRequest_W + chargeRate_W;
        size_t inflowConnIdx = store.inflow_connection_id.value();
        if (ss.flows[inflowConnIdx].requested_W != totalRequest_W)
        {
            ss.active_connections_back.insert(inflowConnIdx);
        }
        ss.flows[inflowConnIdx].requested_W = totalRequest_W;
    }
}

void RunScheduleBasedSourceBackward(Model& model,
                                    SimulationState& ss,
                                    size_t outConnIdx,
                                    size_t sbsIdx)
{
    ScheduleBasedSource const& sbs = model.scheduled_source[sbsIdx];
    assert(outConnIdx == sbs.outflow_connection_id);
    auto wasteConn = model.scheduled_source[sbsIdx].wasteflow_connection_id;
    auto schIdx = ss.schedule_based_source_index[sbsIdx];
    auto available = sbs.time_and_availables[schIdx].Amount_W > sbs.max_outflow_W
                         ? sbs.max_outflow_W
                         : sbs.time_and_availables[schIdx].Amount_W;
    auto spillage = available > ss.flows[outConnIdx].requested_W
                        ? available - ss.flows[outConnIdx].requested_W
                        : 0;
    if (ss.flows[outConnIdx].available_W != available)
    {
        ss.active_connections_front.insert(outConnIdx);
    }
    ss.flows[outConnIdx].available_W = available;
    ss.flows[wasteConn].available_W = spillage;
    ss.flows[wasteConn].requested_W = spillage;
}

void RunPassthroughBackward(Model& m, SimulationState& ss, size_t outConnIdx, size_t ptIdx)
{
    PassThrough const& pt = m.pass_through[ptIdx];
    size_t compId = m.connection[outConnIdx].from_component_id;
    if (ss.unavailable_components.contains(compId))
    {
        if (ss.flows[pt.inflow_connection_id].requested_W != 0)
        {
            ss.active_connections_back.insert(pt.inflow_connection_id);
        }
        ss.flows[pt.inflow_connection_id].requested_W = 0;
    }
    else
    {
        flow_t req_W = ss.flows[outConnIdx].requested_W > pt.max_outflow_W
                           ? pt.max_outflow_W
                           : ss.flows[outConnIdx].requested_W;
        if (ss.flows[pt.inflow_connection_id].requested_W != req_W)
        {
            ss.active_connections_back.insert(pt.inflow_connection_id);
        }
        ss.flows[pt.inflow_connection_id].requested_W = req_W;
    }
}

void RunConnectionsBackward(Model& model, SimulationState& ss)
{
    while (!ss.active_connections_back.empty())
    {
        auto temp = std::vector<size_t>(ss.active_connections_back.begin(),
                                        ss.active_connections_back.end());
        ss.active_connections_back.clear();
        for (auto it = temp.cbegin(); it != temp.cend(); ++it)
        {
            size_t connIdx = *it;
            size_t compIdx = model.connection[connIdx].from_subtype_index;
            size_t compId = model.connection[connIdx].from_component_id;
            if (ss.unavailable_components.contains(compId))
            {
                // TODO: test if we need to call this
                Model_SetComponentToFailed(model, ss, compId);
                continue;
            }
            switch (model.connection[connIdx].from)
            {
            case ComponentType::constant_source_type:
            {
            }
            break;
            case ComponentType::pass_through_type:
            {
                RunPassthroughBackward(model, ss, connIdx, compIdx);
            }
            break;
            case ComponentType::schedule_based_source_type:
            {
                RunScheduleBasedSourceBackward(model, ss, connIdx, compIdx);
            }
            break;
            case ComponentType::constant_efficiency_converter_type:
            {
                switch (model.connection[connIdx].from_port)
                {
                case 0:
                {
                    RunConstantEfficiencyConverterBackward(model, ss, connIdx, compIdx);
                }
                break;
                case 1: // lossflow
                case 2: // wasteflow
                {
                    UpdateConstantEfficiencyLossflowAndWasteflow(model, ss, compIdx);
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
            case ComponentType::variable_efficiency_converter_type:
            {
                switch (model.connection[connIdx].from_port)
                {
                case 0:
                {
                    RunVariableEfficiencyConverterBackward(model, ss, connIdx, compIdx);
                }
                break;
                case 1: // lossflow
                case 2: // wasteflow
                {
                    UpdateVariableEfficiencyLossflowAndWasteflow(model, ss, compIdx);
                }
                break;
                default:
                {
                    write_error_message("RunComponentsBackward",
                                        "unhandled port on variable efficiency "
                                        "converter");
                    exit(1);
                }
                }
            }
            break;
            case ComponentType::mux_type:
            {
                RunMuxBackward(model, ss, compIdx);
                if (model.mux[compIdx].number_of_outports > 1)
                {
                    // NOTE: possibly re-allocate downstream available
                    RunMuxForward(model, ss, compIdx);
                }
            }
            break;
            case ComponentType::store_type:
            {
                RunStoreBackward(model, ss, connIdx, compIdx);
            }
            break;
            case ComponentType::mover_type:
            {
                switch (model.connection[connIdx].from_port)
                {
                case 0:
                {
                    RunMoverBackward(model, ss, connIdx, compIdx);
                }
                break;
                case 1:
                {
                    UpdateEnvironmentFlowForMover(model, ss, compIdx);
                }
                break;
                default:
                {
                    write_error_message("<runtime>", "bad port connection for mover");
                    std::exit(1);
                }
                break;
                }
            }
            break;
            case ComponentType::variable_efficiency_mover_type:
            {
                switch (model.connection[connIdx].from_port)
                {
                case 0:
                {
                    RunVariableEfficiencyMoverBackward(model, ss, connIdx, compIdx);
                }
                break;
                case 1:
                {
                    UpdateEnvironmentFlowForVariableEfficiencyMover(model, ss, compIdx);
                }
                break;
                default:
                {
                    write_error_message("<runtime>",
                                        "bad port connection for variable "
                                        "efficiency mover");
                    std::exit(1);
                }
                break;
                }
            }
            break;
            case ComponentType::switch_type:
            {
                RunSwitchBackward(model, ss, connIdx, compIdx);
            }
            break;
            default:
            {
                std::cout << "Unhandled component type on backward pass: "
                          << ToString(model.connection[connIdx].from) << std::endl;
            }
            }
        }
    }
}

void RunConstantEfficiencyConverterForward(Model const& m,
                                           SimulationState& ss,
                                           size_t inflowConnIdx,
                                           size_t compIdx)
{
    ConstantEfficiencyConverter const& cec = m.constant_efficiency_converter[compIdx];
    assert(cec.inflow_connection_id == inflowConnIdx);
    flow_t inflowAvailable_W = ss.flows[inflowConnIdx].available_W;
    flow_t outflowAvailable_W = static_cast<flow_t>(std::floor(cec.efficiency * inflowAvailable_W));
    assert(inflowAvailable_W >= outflowAvailable_W);
    if (outflowAvailable_W > cec.max_outflow_W)
    {
        outflowAvailable_W = cec.max_outflow_W;
    }
    if (outflowAvailable_W != ss.flows[cec.outflow_connection_id].available_W)
    {
        ss.active_connections_front.insert(cec.outflow_connection_id);
    }
    ss.flows[cec.outflow_connection_id].available_W = outflowAvailable_W;
    UpdateConstantEfficiencyLossflowAndWasteflow(m, ss, compIdx);
}

void RunVariableEfficiencyConverterForward(Model const& m,
                                           SimulationState& ss,
                                           size_t inflowConnIdx,
                                           size_t compIdx)
{
    VariableEfficiencyConverter const& vec = m.variable_efficiency_converter[compIdx];
    assert(inflowConnIdx == vec.inflow_connection_id);
    size_t outflowConn = vec.outflow_connection_id;
    flow_t inflowAvailable_W = ss.flows[inflowConnIdx].available_W;
    double efficiency = LookupTable_LookupInterp(
        vec.inflows_for_efficiency_W, vec.efficiencies, static_cast<double>(inflowAvailable_W));
    assert(efficiency > 0.0 && efficiency <= 1.0);
    flow_t outflowAvailable = static_cast<flow_t>(std::floor(efficiency * inflowAvailable_W));
    if (outflowAvailable > vec.max_outflow_W)
    {
        outflowAvailable = vec.max_outflow_W;
    }
    if (outflowAvailable != ss.flows[outflowConn].available_W)
    {
        ss.active_connections_front.insert(outflowConn);
    }
    ss.flows[outflowConn].available_W = outflowAvailable;
    UpdateVariableEfficiencyLossflowAndWasteflow(m, ss, compIdx);
}

void RunMoverForward(Model const& model, SimulationState& ss, size_t outConnIdx, size_t moverIdx)
{
    assert(moverIdx < model.mover.size());
    Mover const& mov = model.mover[moverIdx];
    flow_t inflowAvailable = ss.flows[outConnIdx].available_W;
    size_t outflowConn = mov.outflow_connection_id;
    flow_t outflowAvailable = static_cast<flow_t>(std::floor(mov.COP * inflowAvailable));
    if (outflowAvailable > mov.max_outflow_W)
    {
        outflowAvailable = mov.max_outflow_W;
    }
    if (outflowAvailable != ss.flows[outflowConn].available_W)
    {
        ss.active_connections_front.insert(outflowConn);
    }
    ss.flows[outflowConn].available_W = outflowAvailable;
    UpdateEnvironmentFlowForMover(model, ss, moverIdx);
}

void RunVariableEfficiencyMoverForward(Model const& model,
                                       SimulationState& ss,
                                       size_t outConnIdx,
                                       size_t moverIdx)
{
    assert(moverIdx < model.variable_efficiency_mover.size());
    VariableEfficiencyMover const& mov = model.variable_efficiency_mover[moverIdx];
    flow_t inflowAvailable_W = ss.flows[outConnIdx].available_W;
    size_t outflowConn = mov.outflow_connection_id;
    double cop = LookupTable_LookupInterp(
        mov.inflows_for_COP_W, mov.COPs, static_cast<double>(inflowAvailable_W));
    // outflow = cop * inflow
    flow_t outflowAvailable_W =
        static_cast<flow_t>(std::floor(cop * static_cast<double>(inflowAvailable_W)));
    if (outflowAvailable_W > mov.max_outflow_W)
    {
        outflowAvailable_W = mov.max_outflow_W;
    }
    if (outflowAvailable_W != ss.flows[outflowConn].available_W)
    {
        ss.active_connections_front.insert(outflowConn);
    }
    ss.flows[outflowConn].available_W = outflowAvailable_W;
    UpdateEnvironmentFlowForVariableEfficiencyMover(model, ss, moverIdx);
}

void RunMuxForward(Model& model, SimulationState& ss, size_t muxIdx)
{
    Mux const& mux = model.mux[muxIdx];
    flow_t totalAvailable_W = 0;
    for (size_t inflowConnIdx : mux.inflow_connection_ids)
    {
        totalAvailable_W = safe_add(totalAvailable_W, ss.flows[inflowConnIdx].available_W);
    }
    std::vector<flow_t> outflowAvailables {};
    outflowAvailables.reserve(mux.number_of_outports);
    for (size_t i = 0; i < mux.number_of_outports; ++i)
    {
        size_t outflowConnIdx = mux.outflow_connection_ids[i];
        flow_t req_W = ss.flows[outflowConnIdx].requested_W > mux.max_outflows_W[i]
                           ? mux.max_outflows_W[i]
                           : ss.flows[outflowConnIdx].requested_W;
        flow_t available_W = req_W >= totalAvailable_W ? totalAvailable_W : req_W;
        outflowAvailables.push_back(available_W);
        totalAvailable_W -= available_W;
    }
    if (totalAvailable_W > 0)
    {
        for (size_t i = 0; i < mux.number_of_outports; ++i)
        {
            if (mux.max_outflows_W[i] > outflowAvailables[i])
            {
                flow_t maxAdd = mux.max_outflows_W[i] - outflowAvailables[i];
                flow_t toAdd = maxAdd > totalAvailable_W ? totalAvailable_W : maxAdd;
                flow_t actuallyAdded = outflowAvailables[i];
                outflowAvailables[i] = safe_add(outflowAvailables[i], toAdd);
                actuallyAdded = outflowAvailables[i] - actuallyAdded;
                totalAvailable_W -= actuallyAdded;
                if (totalAvailable_W == 0)
                {
                    break;
                }
            }
        }
        totalAvailable_W = 0;
    }
    for (size_t i = 0; i < mux.number_of_outports; ++i)
    {
        size_t outflowConnIdx = mux.outflow_connection_ids[i];
        if (ss.flows[outflowConnIdx].available_W != outflowAvailables[i])
        {
            ss.active_connections_front.insert(outflowConnIdx);
        }
        ss.flows[outflowConnIdx].available_W = outflowAvailables[i];
    }
}

void RunStoreForward(Model& model, SimulationState& ss, size_t inflowConnIdx, size_t storeIdx)
{
    Store const& store = model.store[storeIdx];
    assert(store.inflow_connection_id.has_value());
    assert(inflowConnIdx == store.inflow_connection_id.value());
    flow_t dischargeAvailable_W =
        ss.storage_amounts_J[storeIdx] > 0 ? store.max_discharge_rate_W : 0;
    flow_t available_W = safe_add(ss.flows[inflowConnIdx].available_W, dischargeAvailable_W);
    if (available_W > store.max_outflow_W)
    {
        available_W = store.max_outflow_W;
    }
    if (ss.flows[store.outflow_connection_id].available_W != available_W)
    {
        ss.active_connections_front.insert(store.outflow_connection_id);
    }
    ss.flows[store.outflow_connection_id].available_W = available_W;
}

void RunSwitchForward(Model& model, SimulationState& ss, size_t inflowConnIdx, size_t switchIdx)
{
    assert(switchIdx < model.transfer_switch.size());
    auto const& theSwitch = model.transfer_switch[switchIdx];
    assert(switchIdx < ss.switch_states.size());
    auto switchState = ss.switch_states[switchIdx];
    auto inflow0ConnIdx = theSwitch.inflow_connection_id_primary;
    auto inflow1ConnIdx = theSwitch.inflow_connection_id_secondary;
    auto outflowConnIdx = theSwitch.outflow_connection_id;
    if (switchState == SwitchState::primary && inflowConnIdx == inflow0ConnIdx)
    {
        if (ss.flows[outflowConnIdx].available_W != ss.flows[inflow0ConnIdx].available_W)
        {
            ss.active_connections_front.insert(outflowConnIdx);
        }
        ss.flows[outflowConnIdx].available_W = ss.flows[inflow0ConnIdx].available_W;
    }
    else if (switchState == SwitchState::secondary && inflowConnIdx == inflow1ConnIdx)
    {
        if (ss.flows[outflowConnIdx].available_W != ss.flows[inflow1ConnIdx].available_W)
        {
            ss.active_connections_front.insert(outflowConnIdx);
        }
        ss.flows[outflowConnIdx].available_W = ss.flows[inflow1ConnIdx].available_W;
    }
}

void RunPassthroughForward(Model& m, SimulationState& ss, size_t inflowConnIdx, size_t ptIdx)
{
    auto const& pt = m.pass_through[ptIdx];
    size_t compId = m.connection[inflowConnIdx].to_component_id;
    if (ss.unavailable_components.contains(compId))
    {
        if (ss.flows[pt.outflow_connection_id].available_W != 0)
        {
            ss.active_connections_front.insert(pt.outflow_connection_id);
        }
        ss.flows[pt.outflow_connection_id].available_W = 0;
    }
    else
    {
        flow_t avail_W = ss.flows[inflowConnIdx].available_W > pt.max_outflow_W
                             ? pt.max_outflow_W
                             : ss.flows[inflowConnIdx].available_W;
        if (ss.flows[pt.outflow_connection_id].available_W != avail_W)
        {
            ss.active_connections_front.insert(pt.outflow_connection_id);
        }
        ss.flows[pt.outflow_connection_id].available_W = avail_W;
    }
}

void RunConnectionsForward(Model& model, SimulationState& ss)
{
    while (!ss.active_connections_front.empty())
    {
        auto temp = std::vector<size_t>(ss.active_connections_front.begin(),
                                        ss.active_connections_front.end());
        ss.active_connections_front.clear();
        for (auto it = temp.cbegin(); it != temp.cend(); ++it)
        {
            size_t connIdx = *it;
            size_t compIdx = model.connection[connIdx].to_subtype_index;
            size_t compId = model.connection[connIdx].to_component_id;
            if (ss.unavailable_components.contains(compId))
            {
                // TODO: test if we need to call this
                Model_SetComponentToFailed(model, ss, compId);
                continue;
            }
            switch (model.connection[connIdx].to)
            {
            case ComponentType::constant_load_type:
            case ComponentType::waste_sink_type:
            case ComponentType::schedule_based_load_type:
            {
            }
            break;
            case ComponentType::pass_through_type:
            {
                RunPassthroughForward(model, ss, connIdx, compIdx);
            }
            break;
            case ComponentType::constant_efficiency_converter_type:
            {
                RunConstantEfficiencyConverterForward(model, ss, connIdx, compIdx);
            }
            break;
            case ComponentType::variable_efficiency_converter_type:
            {
                RunVariableEfficiencyConverterForward(model, ss, connIdx, compIdx);
            }
            break;
            case ComponentType::mover_type:
            {
                RunMoverForward(model, ss, connIdx, compIdx);
            }
            break;
            case ComponentType::variable_efficiency_mover_type:
            {
                RunVariableEfficiencyMoverForward(model, ss, connIdx, compIdx);
            }
            break;
            case ComponentType::mux_type:
            {
                RunMuxForward(model, ss, compIdx);
                if (model.mux[compIdx].number_of_inports > 1)
                {
                    // NOTE: possibly re-allocate upstream requests
                    RunMuxBackward(model, ss, compIdx);
                }
            }
            break;
            case ComponentType::store_type:
            {
                RunStoreForward(model, ss, connIdx, compIdx);
            }
            break;
            case ComponentType::switch_type:
            {
                RunSwitchForward(model, ss, connIdx, compIdx);
            }
            break;
            default:
            {
                std::cerr << "unhandled component type on forward pass: "
                          << ToString(model.connection[connIdx].to) << std::endl;
                std::exit(1);
            }
            }
        }
    }
}

void RunStorePostFinalization(Model& model, SimulationState& ss, double t, size_t compIdx)
{
    // NOTE: we assume that the charge request never resets once at or
    // below chargeAmount UNTIL you hit 100% SOC again...
    Store const& store = model.store[compIdx];
    size_t outflowConn = store.outflow_connection_id;
    std::optional<size_t> maybeInflowConn = store.inflow_connection_id;
    int64_t netCharge_W = -1 * static_cast<int64_t>(ss.flows[outflowConn].actual_W);
    if (maybeInflowConn.has_value())
    {
        size_t inflowConn = maybeInflowConn.value();
        netCharge_W += static_cast<int64_t>(ss.flows[inflowConn].actual_W);
    }
    if (netCharge_W > 0)
    {
        flow_t storeflow_W = static_cast<flow_t>(netCharge_W);
        if (store.wasteflow_connection_id.has_value())
        {
            storeflow_W = static_cast<flow_t>(netCharge_W * store.roundtrip_efficiency);
            flow_t wasteflow_W = netCharge_W - storeflow_W;
            size_t wfIdx = store.wasteflow_connection_id.value();
            ss.flows[wfIdx].requested_W = wasteflow_W;
            ss.flows[wfIdx].available_W = wasteflow_W;
            ss.flows[wfIdx].actual_W = wasteflow_W;
        }
        ss.storage_next_event_times[compIdx] =
            t + (static_cast<double>(store.capacity_J - ss.storage_amounts_J[compIdx]) /
                 static_cast<double>(storeflow_W));
    }
    else if (netCharge_W < 0 && store.inflow_connection_id.has_value() &&
             (ss.storage_amounts_J[compIdx] > store.charge_amount_J))
    {
        if (store.wasteflow_connection_id.has_value())
        {
            size_t wfIdx = store.wasteflow_connection_id.value();
            ss.flows[wfIdx].requested_W = 0;
            ss.flows[wfIdx].available_W = 0;
            ss.flows[wfIdx].actual_W = 0;
        }
        ss.storage_next_event_times[compIdx] =
            t + (static_cast<double>(ss.storage_amounts_J[compIdx] - store.charge_amount_J) /
                 (-1.0 * static_cast<double>(netCharge_W)));
    }
    else if (netCharge_W < 0)
    {
        if (store.wasteflow_connection_id.has_value())
        {
            size_t wfIdx = store.wasteflow_connection_id.value();
            ss.flows[wfIdx].requested_W = 0;
            ss.flows[wfIdx].available_W = 0;
            ss.flows[wfIdx].actual_W = 0;
        }
        ss.storage_next_event_times[compIdx] =
            t + (static_cast<double>(ss.storage_amounts_J[compIdx]) /
                 (-1.0 * static_cast<double>(netCharge_W)));
    }
    else // netCharge_W = 0
    {
        if (store.wasteflow_connection_id.has_value())
        {
            size_t wfIdx = store.wasteflow_connection_id.value();
            ss.flows[wfIdx].requested_W = 0;
            ss.flows[wfIdx].available_W = 0;
            ss.flows[wfIdx].actual_W = 0;
        }
        ss.storage_next_event_times[compIdx] = infinite_time;
    }
}

void RunMuxPostFinalization(Model& model, SimulationState& ss, size_t compIdx)
{
    // TODO: test if we need to run backward/forward again
    RunMuxBackward(model, ss, compIdx);
    RunMuxForward(model, ss, compIdx);
    // BalanceMuxRequests(model, ss, compIdx);
}

void RunConnectionsPostFinalization(Model& model, SimulationState& ss, double t)
{
    for (size_t storeIdx = 0; storeIdx < model.store.size(); ++storeIdx)
    {
        RunStorePostFinalization(model, ss, t, storeIdx);
    }
}

void RunActiveConnections(Model& model, SimulationState& ss, double t)
{
    constexpr size_t max_times = 100;
    size_t num_times = 0;
    do
    {
        RunConnectionsBackward(model, ss);
        RunConnectionsForward(model, ss);
        ++num_times;
    } while (num_times < max_times &&
             (ss.active_connections_back.size() > 0 || ss.active_connections_front.size() > 0));
    if (num_times == max_times)
    {
        std::cout << "WARNING! iterated " << max_times << " to resolve connections" << std::endl;
    }
    else
    {
        assert(ss.active_connections_back.size() == 0);
        assert(ss.active_connections_front.size() == 0);
    }
    FinalizeFlows(ss);
    RunConnectionsPostFinalization(model, ss, t);
}

flow_t FinalizeFlowValue(flow_t requested, flow_t available)
{
    return available >= requested ? requested : available;
}

void FinalizeFlows(SimulationState& ss)
{
    for (size_t flowIdx = 0; flowIdx < ss.flows.size(); ++flowIdx)
    {
        ss.flows[flowIdx].actual_W =
            FinalizeFlowValue(ss.flows[flowIdx].requested_W, ss.flows[flowIdx].available_W);
    }
}

double NextEvent(ScheduleBasedLoad const& sb, size_t sbIdx, SimulationState const& ss)
{
    auto nextIdx = ss.schedule_based_load_index[sbIdx] + 1;
    if (nextIdx >= sb.times_and_loads.size())
    {
        return infinite_time;
    }
    return sb.times_and_loads[nextIdx].Time_s;
}

double NextEvent(ScheduleBasedSource const& sb, size_t sbIdx, SimulationState const& ss)
{
    auto nextIdx = ss.schedule_based_source_index[sbIdx] + 1;
    if (nextIdx >= sb.time_and_availables.size())
    {
        return infinite_time;
    }
    return sb.time_and_availables[nextIdx].Time_s;
}

double NextEvent(ScheduleBasedReliability const& sbr, double t)
{
    for (size_t i = 0; i < sbr.time_states.size(); ++i)
    {
        if (sbr.time_states[i].time > t)
        {
            return sbr.time_states[i].time;
        }
    }
    return infinite_time;
}

double NextStorageEvent(SimulationState const& ss, size_t storeIdx, double t)
{
    double storeTime = ss.storage_next_event_times[storeIdx];
    if (storeTime >= 0.0 && storeTime > t)
    {
        return storeTime;
    }
    return infinite_time;
}

void UpdateStoresPerElapsedTime(Model const& m, SimulationState& ss, double elapsedTime_s)
{
    for (size_t storeIdx = 0; storeIdx < m.store.size(); ++storeIdx)
    {
        Store const& store = m.store[storeIdx];
        int64_t netEnergyAdded_J = 0;
        std::optional<size_t> maybeInConn = store.inflow_connection_id;
        size_t outConn = store.outflow_connection_id;
        size_t compId = m.connection[outConn].from_component_id;
        assert(m.connection[outConn].from == ComponentType::store_type);
        if (ss.unavailable_components.contains(compId))
        {
            continue;
        }
        int64_t availableCharge_J = static_cast<int64_t>(m.store[storeIdx].capacity_J) -
                                    static_cast<int64_t>(ss.storage_amounts_J[storeIdx]);
        int64_t availableDischarge_J = -1 * static_cast<int64_t>(ss.storage_amounts_J[storeIdx]);
        if (maybeInConn.has_value())
        {
            size_t inConn = maybeInConn.value();
            double actualInflow_W = static_cast<double>(ss.flows[inConn].actual_W);
            netEnergyAdded_J += std::llround(elapsedTime_s * actualInflow_W);
        }
        double actualOutflow_W = static_cast<double>(ss.flows[outConn].actual_W);
        netEnergyAdded_J -= std::llround(elapsedTime_s * actualOutflow_W);
        if (store.wasteflow_connection_id.has_value())
        {
            size_t wConn = store.wasteflow_connection_id.value();
            double actualWasteflow_W = static_cast<double>(ss.flows[wConn].actual_W);
            netEnergyAdded_J -= std::llround(elapsedTime_s * actualWasteflow_W);
        }
        if (netEnergyAdded_J > availableCharge_J)
        {
            std::cout << "ERROR: netEnergyAdded is greater than capacity!" << std::endl;
            std::cout << "netEnergyAdded (J): " << netEnergyAdded_J << std::endl;
            std::cout << "availableCharge (J): " << availableCharge_J << std::endl;
            std::cout << "elapsed time (s): " << elapsedTime_s << std::endl;
            std::cout << "stored amount (J): " << ss.storage_amounts_J[storeIdx] << std::endl;
            if (maybeInConn.has_value())
            {
                size_t inConn = maybeInConn.value();
                std::cout << "inflow (W): " << ss.flows[inConn].actual_W << std::endl;
            }
            std::cout << "outflow (W): " << ss.flows[outConn].actual_W << std::endl;
            int64_t inflow = 0;
            if (maybeInConn.has_value())
            {
                size_t inConn = maybeInConn.value();
                inflow = static_cast<int64_t>(ss.flows[inConn].actual_W);
            }
            int64_t outflow = static_cast<int64_t>(ss.flows[outConn].actual_W);
            int64_t wasteflow = 0;
            if (store.wasteflow_connection_id.has_value())
            {
                size_t wConn = store.wasteflow_connection_id.value();
                std::cout << "wasteflow (W): " << ss.flows[wConn].actual_W << std::endl;
                wasteflow = static_cast<int64_t>(ss.flows[wConn].actual_W);
            }
            std::cout << "flow balance (inflow - (outflow + wasteflow)): "
                      << (inflow - (outflow + wasteflow)) << std::endl;
        }
        assert(availableCharge_J >= netEnergyAdded_J &&
               "netEnergyAdded cannot put storage over capacity");
        if (netEnergyAdded_J < availableDischarge_J)
        {
            std::cout << "ERROR: netEnergyAdded is lower than discharge limit" << std::endl;
            std::cout << "compId: " << compId << std::endl;
            std::cout << "store idx: " << storeIdx << std::endl;
            std::cout << "tag: " << m.component.tag[compId] << std::endl;
            for (size_t compId_Idx = 0; compId_Idx < m.component.tag.size(); ++compId_Idx)
            {
                if (m.component.component_type[compId_Idx] == ComponentType::store_type &&
                    m.component.subtype_index[compId_Idx] == storeIdx)
                {
                    std::cout << "compId (from search): " << compId_Idx << std::endl;
                    std::cout << "tag (from search): " << m.component.tag[compId_Idx] << std::endl;
                }
            }
            std::cout << "has inflow? " << store.inflow_connection_id.has_value() << std::endl;
            std::cout << "netEnergyAdded (J): " << netEnergyAdded_J << std::endl;
            std::cout << "availableDischarge (J): " << availableDischarge_J << std::endl;
            std::cout << "elapsed time (s): " << elapsedTime_s << std::endl;
            std::cout << "stored amount (J): " << ss.storage_amounts_J[storeIdx] << std::endl;
            int64_t inflow_W = 0;
            if (maybeInConn.has_value())
            {
                size_t inConn = maybeInConn.value();
                std::cout << "inflow (W): " << ss.flows[inConn].actual_W << std::endl;
                inflow_W = static_cast<int64_t>(ss.flows[inConn].actual_W);
            }
            else
            {
                std::cout << "inflow (W): 0.0 (no inflow connection)" << std::endl;
            }
            std::cout << "outflow (W): " << ss.flows[outConn].actual_W << std::endl;
            int64_t outflow_W = static_cast<int64_t>(ss.flows[outConn].actual_W);
            int64_t wasteflow_W = 0;
            if (store.wasteflow_connection_id.has_value())
            {
                size_t wConn = store.wasteflow_connection_id.value();
                std::cout << "wasteflow (W): " << ss.flows[wConn].actual_W << std::endl;
                wasteflow_W = static_cast<int64_t>(ss.flows[wConn].actual_W);
            }
            std::cout << "flow balance (inflow - (outflow + wasteflow)): "
                      << (inflow_W - (outflow_W + wasteflow_W)) << std::endl;
        }
        assert(netEnergyAdded_J >= availableDischarge_J &&
               "netEnergyAdded cannot use more energy than available");
        ss.storage_amounts_J[storeIdx] += netEnergyAdded_J;
    }
}

void UpdateScheduleBasedLoadNextEvent(Model const& m, SimulationState& ss, double time)
{
    for (size_t i = 0; i < m.scheduled_load.size(); ++i)
    {
        size_t nextIdx = ss.schedule_based_load_index[i] + 1;
        if (nextIdx < m.scheduled_load[i].times_and_loads.size() &&
            m.scheduled_load[i].times_and_loads[nextIdx].Time_s == time)
        {
            ss.schedule_based_load_index[i] = nextIdx;
        }
    }
}

void UpdateScheduleBasedSourceNextEvent(Model const& m, SimulationState& ss, double time)
{
    for (size_t i = 0; i < m.scheduled_source.size(); ++i)
    {
        size_t nextIdx = ss.schedule_based_source_index[i] + 1;
        if (nextIdx < m.scheduled_source[i].time_and_availables.size() &&
            m.scheduled_source[i].time_and_availables[nextIdx].Time_s == time)
        {
            ss.schedule_based_source_index[i] = nextIdx;
        }
    }
}

// TODO: rename to ComponentTypeToString(.)
std::string ToString(ComponentType compType)
{
    std::string result = "?";
    switch (compType)
    {
    case ComponentType::constant_load_type:
    {
        result = "ConstantLoad";
    }
    break;
    case ComponentType::schedule_based_load_type:
    {
        result = "ScheduleBasedLoad";
    }
    break;
    case ComponentType::constant_source_type:
    {
        result = "ConstantSource";
    }
    break;
    case ComponentType::schedule_based_source_type:
    {
        result = "ScheduleBasedSource";
    }
    break;
    case ComponentType::constant_efficiency_converter_type:
    {
        result = "ConstantEfficiencyConverter";
    }
    break;
    case ComponentType::variable_efficiency_converter_type:
    {
        result = "VariableEfficiencyConverter";
    }
    break;
    case ComponentType::waste_sink_type:
    {
        result = "WasteSink";
    }
    break;
    case ComponentType::mux_type:
    {
        result = "Mux";
    }
    break;
    case ComponentType::store_type:
    {
        result = "Store";
    }
    break;
    case ComponentType::pass_through_type:
    {
        result = "PassThrough";
    }
    break;
    case ComponentType::mover_type:
    {
        result = "Mover";
    }
    break;
    case ComponentType::variable_efficiency_mover_type:
    {
        result = "VariableEfficiencyMoverType";
    }
    break;
    case ComponentType::environment_source_type:
    {
        result = "EnvironmentSource";
    }
    break;
    case ComponentType::switch_type:
    {
        result = "Switch";
    }
    break;
    default:
    {
        write_error_message("ToString", "unhandled component type");
        std::exit(1);
    }
    }
    return result;
}

std::optional<ComponentType> TagToComponentType(std::string const& tag)
{
    if (tag == "ConstantLoad" || tag == "constant_load")
    {
        return ComponentType::constant_load_type;
    }
    if (tag == "ScheduleBasedLoad" || tag == "load")
    {
        return ComponentType::schedule_based_load_type;
    }
    if (tag == "ConstantSource" || tag == "source")
    {
        return ComponentType::constant_source_type;
    }
    if (tag == "ScheduleBasedSource" || tag == "uncontrolled_source")
    {
        return ComponentType::schedule_based_source_type;
    }
    if (tag == "ConstantEfficiencyConverter" || tag == "converter")
    {
        return ComponentType::constant_efficiency_converter_type;
    }
    if (tag == "VariableEfficiencyConverter" || tag == "variable_efficiency_converter")
    {
        return ComponentType::variable_efficiency_converter_type;
    }
    if (tag == "WasteSink")
    {
        return ComponentType::waste_sink_type;
    }
    if (tag == "EnvironmentSource")
    {
        return ComponentType::environment_source_type;
    }
    if (tag == "Mux" || tag == "mux" || tag == "muxer")
    {
        return ComponentType::mux_type;
    }
    if (tag == "Store" || tag == "store")
    {
        return ComponentType::store_type;
    }
    if (tag == "PassThrough" || tag == "pass_through")
    {
        return ComponentType::pass_through_type;
    }
    if (tag == "Mover" || tag == "mover")
    {
        return ComponentType::mover_type;
    }
    if (tag == "VariableEfficiencyMover" || tag == "variable_efficiency_mover")
    {
        return ComponentType::variable_efficiency_mover_type;
    }
    if (tag == "Switch" || tag == "switch")
    {
        return ComponentType::switch_type;
    }
    return {};
}

std::vector<std::string> FlowsToStrings(Model const& m, SimulationState const& ss, double time_s)
{
    std::vector<std::string> result {};
    result.push_back(fmt::format("time: {} s, {}, {} h",
                                 time_s,
                                 time_to_ISO8601_period(static_cast<uint64_t>(time_s)),
                                 time_in_seconds_to_hours(static_cast<uint64_t>(time_s))));
    for (size_t flowIdx = 0; flowIdx < ss.flows.size(); ++flowIdx)
    {
        result.push_back(fmt::format("{}: {} (R: {}; A: {})",
                                     ConnectionToString(m.component, m.connection[flowIdx]),
                                     ss.flows[flowIdx].actual_W,
                                     ss.flows[flowIdx].requested_W,
                                     ss.flows[flowIdx].available_W));
    }
    return result;
}

void LogFlows(Log const& log, Model const& m, SimulationState const& ss, double time_s)
{
    for (std::string const& s : FlowsToStrings(m, ss, time_s))
    {
        Log_info(log, s);
    }
}

void PrintFlows(Model const& m, SimulationState const& ss, double time_s)
{
    for (std::string const& s : FlowsToStrings(m, ss, time_s))
    {
        std::cout << s << std::endl;
    }
}

FlowSummary SummarizeFlows(Model const& m, SimulationState const& ss, double t)
{
    FlowSummary summary = {
        .time_s = t,
        .inflow_W = 0,
        .outflow_request_W = 0,
        .outflow_achieved_W = 0,
        .storage_discharge_W = 0,
        .storage_charge_W = 0,
        .wasteflow_W = 0,
        .env_inflow_W = 0,
    };
    for (size_t flowIdx = 0; flowIdx < ss.flows.size(); ++flowIdx)
    {
        switch (m.connection[flowIdx].from)
        {
        case ComponentType::constant_source_type:
        {
            summary.inflow_W += ss.flows[flowIdx].actual_W;
        }
        break;
        case ComponentType::schedule_based_source_type:
        {
            // NOTE: for schedule-based sources, the thinking is that
            // the "available" is actually flowing into the system and,
            // if not used (i.e., ullage/spillage), it goes to wasteflow
            if (m.connection[flowIdx].from_port == 0)
            {
                summary.inflow_W += ss.flows[flowIdx].available_W;
            }
        }
        break;
        case ComponentType::store_type:
        {
            summary.storage_discharge_W += ss.flows[flowIdx].actual_W;
        }
        break;
        case ComponentType::environment_source_type:
        {
            summary.env_inflow_W += ss.flows[flowIdx].actual_W;
        }
        break;
        case ComponentType::constant_load_type:
        case ComponentType::schedule_based_load_type:
        case ComponentType::constant_efficiency_converter_type:
        case ComponentType::variable_efficiency_converter_type:
        case ComponentType::mux_type:
        case ComponentType::pass_through_type:
        case ComponentType::mover_type:
        case ComponentType::variable_efficiency_mover_type:
        case ComponentType::waste_sink_type:
        case ComponentType::switch_type:
        {
        }
        break;
        default:
        {
            write_error_message("SummarizeFlows(.)",
                                "Unhandled From type for connection - from pass: " +
                                    ToString(m.connection[flowIdx].from));
        }
        break;
        }

        switch (m.connection[flowIdx].to)
        {
        case ComponentType::constant_load_type:
        case ComponentType::schedule_based_load_type:
        {
            summary.outflow_request_W += ss.flows[flowIdx].requested_W;
            summary.outflow_achieved_W += ss.flows[flowIdx].actual_W;
        }
        break;
        case ComponentType::store_type:
        {
            summary.storage_charge_W += ss.flows[flowIdx].actual_W;
        }
        break;
        case ComponentType::waste_sink_type:
        {
            summary.wasteflow_W += ss.flows[flowIdx].actual_W;
        }
        break;
        case ComponentType::constant_source_type:
        case ComponentType::schedule_based_source_type:
        case ComponentType::constant_efficiency_converter_type:
        case ComponentType::variable_efficiency_converter_type:
        case ComponentType::mux_type:
        case ComponentType::pass_through_type:
        case ComponentType::mover_type:
        case ComponentType::variable_efficiency_mover_type:
        case ComponentType::environment_source_type:
        case ComponentType::switch_type:
        {
        }
        break;
        default:
        {
            write_error_message("SummarizeFlows(.)",
                                "Unhandled From type for connection - to pass: " +
                                    ToString(m.connection[flowIdx].to));
        }
        break;
        }
    }
    return summary;
}

bool PrintFlowSummary(FlowSummary s)
{
    int64_t netDischarge =
        static_cast<int64_t>(s.storage_discharge_W) - static_cast<int64_t>(s.storage_charge_W);
    int64_t sum =
        static_cast<int64_t>(s.inflow_W) + netDischarge + static_cast<int64_t>(s.env_inflow_W) -
        (static_cast<int64_t>(s.outflow_achieved_W) + static_cast<int64_t>(s.wasteflow_W));
    double eff = (static_cast<double>(s.inflow_W) + static_cast<double>(s.env_inflow_W) +
                  static_cast<double>(netDischarge)) > 0.0
                     ? 100.0 * (static_cast<double>(s.outflow_achieved_W)) /
                           (static_cast<double>(s.inflow_W) + static_cast<double>(netDischarge) +
                            static_cast<double>(s.env_inflow_W))
                     : 0.0;
    double effectiveness = s.outflow_request_W > 0
                               ? 100.0 * (static_cast<double>(s.outflow_achieved_W)) /
                                     (static_cast<double>(s.outflow_request_W))
                               : 0.0;
    std::cout << "Flow Summary @ " << s.time_s << ":" << std::endl;
    std::cout << "  Inflow                 : " << s.inflow_W << std::endl;
    std::cout << "+ Storage Net Discharge  : " << netDischarge << std::endl;
    std::cout << "+ Environment Inflow     : " << s.env_inflow_W << std::endl;
    std::cout << "- Outflow (achieved)     : " << s.outflow_achieved_W << std::endl;
    std::cout << "- Wasteflow              : " << s.wasteflow_W << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    std::cout << "= Sum                    : " << sum << std::endl;
    std::cout << "  Efficiency             : " << eff << "%"
              << " (= " << s.outflow_achieved_W << "/"
              << ((int)s.inflow_W + (int)s.env_inflow_W + netDischarge) << ")" << std::endl;
    std::cout << "  Delivery Effectiveness : " << effectiveness << "%"
              << " (= " << s.outflow_achieved_W << "/" << s.outflow_request_W << ")" << std::endl;
    return sum == 0;
}

std::vector<Flow> CopyFlows(std::vector<Flow> flows)
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

std::vector<flow_t> CopyStorageStates(SimulationState& ss)
{
    std::vector<flow_t> newAmounts = {};
    newAmounts.reserve(ss.storage_amounts_J.size());
    for (size_t i = 0; i < ss.storage_amounts_J.size(); ++i)
    {
        newAmounts.push_back(ss.storage_amounts_J[i]);
    }
    return newAmounts;
}

void PrintModelState(Model& m, SimulationState& ss)
{
    for (size_t storeIdx = 0; storeIdx < m.store.size(); ++storeIdx)
    {
        std::cout << ToString(ComponentType::store_type) << "[" << storeIdx
                  << "].InitialStorage (J): " << m.store[storeIdx].initial_storage_J << std::endl;
        std::cout << ToString(ComponentType::store_type) << "[" << storeIdx
                  << "].StorageAmount (J) : " << ss.storage_amounts_J[storeIdx] << std::endl;
        std::cout << ToString(ComponentType::store_type) << "[" << storeIdx
                  << "].Capacity (J)      : " << m.store[storeIdx].capacity_J << std::endl;
        double soc =
            (double)ss.storage_amounts_J[storeIdx] * 100.0 / (double)m.store[storeIdx].capacity_J;
        std::cout << ToString(ComponentType::store_type) << "[" << storeIdx
                  << "].SOC               : " << soc << " %" << std::endl;
    }
}

size_t Model_NumberOfComponents(Model const& m) { return m.component.tag.size(); }

// TODO: add schedule-based reliability index?
void Model_SetupSimulationState(Model& model, SimulationState& ss)
{
    for (size_t i = 0; i < model.store.size(); ++i)
    {
        ss.storage_amounts_J.push_back(model.store[i].initial_storage_J);
    }
    ss.storage_next_event_times = std::vector<double>(model.store.size(), 0.0);
    ss.flows = std::vector<Flow>(model.connection.size(), {0, 0, 0});
    ss.schedule_based_load_index = std::vector<size_t>(model.scheduled_load.size(), 0);
    ss.schedule_based_source_index = std::vector<size_t>(model.scheduled_source.size(), 0);
    for (size_t i = 0; i < model.transfer_switch.size(); ++i)
    {
        ss.switch_states.push_back(SwitchState::primary);
    }
}

size_t Model_AddFixedReliabilityDistribution(Model& m, double dt)
{
    return m.dist_sys.add_fixed("", dt);
}

size_t
Model_AddFailureModeToComponent(Model& m, size_t compId, size_t failureDistId, size_t repairDistId)
{
    auto fmId = m.rel_coord.add_failure_mode("", failureDistId, repairDistId);
    auto linkId = m.rel_coord.link_component_with_failure_mode(compId, fmId);
    auto schedule =
        m.rel_coord.make_schedule_for_link(linkId, m.random_function, m.dist_sys, m.final_time_s);
    ScheduleBasedReliability sbr = {};
    sbr.component_id = compId;
    sbr.time_states = std::move(schedule);
    m.reliability.push_back(std::move(sbr));
    return linkId;
}

ComponentIdAndWasteAndEnvironmentConnection Model_AddMover(Model& m, double cop)
{
    return Model_AddMover(m, cop, 0, 0, "");
}

ComponentIdAndWasteAndEnvironmentConnection Model_AddMover(Model& m,
                                                           double cop,
                                                           size_t inflowTypeId,
                                                           size_t outflowTypeId,
                                                           std::string const& tag,
                                                           bool report)
{
    assert(cop > 0.0);
    Mover mov = {
        .COP = cop,
        .inflow_connection_id = 0,
        .outflow_connection_id = 0,
        .in_from_env_connection_id = 0,
        .wasteflow_connection_id = 0,
        .max_outflow_W = max_flow_W,
    };
    m.mover.push_back(std::move(mov));
    size_t wasteId = add_component_returning_id(m.component,
                                                ComponentType::waste_sink_type,
                                                0,
                                                std::vector<size_t> {wasteflow_id},
                                                std::vector<size_t> {},
                                                "",
                                                0.0,
                                                report);
    size_t envId = add_component_returning_id(m.component,
                                              ComponentType::environment_source_type,
                                              0,
                                              std::vector<size_t> {},
                                              std::vector<size_t> {wasteflow_id},
                                              "",
                                              0.0,
                                              report);
    size_t thisId = add_component_returning_id(m.component,
                                               ComponentType::mover_type,
                                               0,
                                               std::vector<size_t> {inflowTypeId, wasteflow_id},
                                               std::vector<size_t> {outflowTypeId, wasteflow_id},
                                               tag,
                                               0.0,
                                               report);
    Connection wconn = Model_AddConnection(m, thisId, 1, wasteId, 0, wasteflow_id);
    Connection econn = Model_AddConnection(m, envId, 0, thisId, 1, wasteflow_id);
    return {
        .id = thisId,
        .waste_connection_id = wconn,
        .environment_connection_id = econn,
    };
}

ComponentIdAndWasteAndEnvironmentConnection
Model_AddVariableEfficiencyMover(Model& m,
                                 std::vector<double>&& outflowsForCop_W,
                                 std::vector<double>&& copByOutflow,
                                 size_t inflowTypeId,
                                 size_t outflowTypeId,
                                 std::string const& tag,
                                 bool report)
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
        .inflow_connection_id = 0,
        .outflow_connection_id = 0,
        .in_from_env_connection_id = 0,
        .wasteflow_connection_id = 0,
        .max_outflow_W = max_flow_W,
        .outflows_for_COP_W = std::move(outflowsForCop_W),
        .inflows_for_COP_W = std::move(inflowsForCop_W),
        .COPs = std::move(copByOutflow),
    };
    m.variable_efficiency_mover.push_back(std::move(mov));
    size_t wasteId = add_component_returning_id(m.component,
                                                ComponentType::waste_sink_type,
                                                0,
                                                std::vector<size_t> {wasteflow_id},
                                                std::vector<size_t> {},
                                                "",
                                                0.0,
                                                report);
    size_t envId = add_component_returning_id(m.component,
                                              ComponentType::environment_source_type,
                                              0,
                                              std::vector<size_t> {},
                                              std::vector<size_t> {wasteflow_id},
                                              "",
                                              0.0,
                                              report);
    size_t thisId = add_component_returning_id(m.component,
                                               ComponentType::variable_efficiency_mover_type,
                                               0,
                                               std::vector<size_t> {inflowTypeId, wasteflow_id},
                                               std::vector<size_t> {outflowTypeId, wasteflow_id},
                                               tag,
                                               0.0,
                                               report);
    Connection wconn = Model_AddConnection(m, thisId, 1, wasteId, 0, wasteflow_id);
    Connection econn = Model_AddConnection(m, envId, 0, thisId, 1, wasteflow_id);
    return {
        .id = thisId,
        .waste_connection_id = wconn,
        .environment_connection_id = econn,
    };
}

bool RunSwitchLogic(Model const& model, SimulationState& ss)
{
    bool result = false;
    for (size_t switchIdx = 0; switchIdx < ss.switch_states.size(); ++switchIdx)
    {
        auto switchState = ss.switch_states[switchIdx];
        assert(switchIdx < model.transfer_switch.size());
        auto const& theSwitch = model.transfer_switch[switchIdx];
        auto in0Conn = theSwitch.inflow_connection_id_primary;
        auto in1Conn = theSwitch.inflow_connection_id_secondary;
        auto outConn = theSwitch.outflow_connection_id;
        assert(in0Conn < ss.flows.size());
        bool primaryIsSufficient = ss.flows[in0Conn].available_W >= ss.flows[in0Conn].requested_W;
        switch (switchState)
        {
        case SwitchState::primary:
        {
            if (!primaryIsSufficient)
            {
                ss.switch_states[switchIdx] = SwitchState::secondary;
                ss.active_connections_back.insert(outConn);
                ss.active_connections_front.insert(in0Conn);
                ss.active_connections_front.insert(in1Conn);
                result = true;
            }
        }
        break;
        case SwitchState::secondary:
        {
            if (primaryIsSufficient)
            {
                ss.switch_states[switchIdx] = SwitchState::primary;
                ss.active_connections_back.insert(outConn);
                ss.active_connections_front.insert(in0Conn);
                ss.active_connections_front.insert(in1Conn);
                result = true;
            }
        }
        break;
        default:
        {
            write_error_message("Simulate", "unhandled switch state");
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
    std::vector<TimeAndFlows> timeAndFlows {};
    SimulationState ss {};
    Model_SetupSimulationState(model, ss);
    // TODO: add units to FinalTime (append '_s')
    while (t != infinite_time && t <= model.final_time_s)
    {
        // schedule each event-generating component for next event
        // by adding to the ActiveComponentBack or ActiveComponentFront
        // arrays
        // note: these two arrays could be sorted by component type for
        // faster running over loops...
        activate_reliability_connections(model, ss, t, verbose);
        activate_schedule_based_load_connections(model, ss, t);
        activate_schedule_based_source_connections(model, ss, t);
        activate_store_connections(model, ss, t);
        // TODO: remove this if statement after we add a delay capability to
        // constant loads and constant sources
        if (t == 0)
        {
            activate_constant_load_connections(model, ss);
            activate_constant_source_connections(model, ss);
        }
        size_t const maxLoop = 1'000;
        for (size_t loopIter = 0; loopIter < maxLoop; ++loopIter)
        {
            if (count_active_connections(ss) == 0)
            {
                if (verbose)
                {
                    Log_debug(log, fmt::format("loop iter: {}", loopIter));
                }
                break;
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
                Log_warning(log, "FLOW IMBALANCE!");
                std::map<size_t, int64_t> sumOfFlowsByCompId;
                for (size_t connIdx = 0; connIdx < model.connection.size(); ++connIdx)
                {
                    size_t const& fromId = model.connection[connIdx].from_component_id;
                    size_t const& toId = model.connection[connIdx].to_component_id;
                    if (!sumOfFlowsByCompId.contains(fromId))
                    {
                        sumOfFlowsByCompId.insert({fromId, 0});
                    }
                    if (!sumOfFlowsByCompId.contains(toId))
                    {
                        sumOfFlowsByCompId.insert({toId, 0});
                    }
                    flow_t flow = ss.flows[connIdx].actual_W;
                    sumOfFlowsByCompId[fromId] -= flow;
                    sumOfFlowsByCompId[toId] += flow;
                }
                for (auto const& item : sumOfFlowsByCompId)
                {
                    size_t const& compId = item.first;
                    ComponentType ctype = model.component.component_type[compId];
                    if (ctype == ComponentType::constant_load_type ||
                        ctype == ComponentType::schedule_based_load_type ||
                        ctype == ComponentType::waste_sink_type ||
                        ctype == ComponentType::store_type ||
                        ctype == ComponentType::constant_source_type ||
                        ctype == ComponentType::schedule_based_source_type ||
                        ctype == ComponentType::environment_source_type)
                    {
                        continue;
                    }
                    if (item.second != 0)
                    {
                        Log_warning(log,
                                    fmt::format("{} doesn't have a zero sum of all flows: "
                                                "{} W",
                                                model.component.tag[item.first],
                                                item.second));
                        for (size_t connIdx = 0; connIdx < model.connection.size(); ++connIdx)
                        {
                            Connection const& conn = model.connection[connIdx];
                            if (conn.to_component_id == item.first)
                            {
                                Log_warning(
                                    log,
                                    fmt::format("* +{:>16d} W (R: +{:>16d} W // A: "
                                                "+{:>16d} W):: {}",
                                                ss.flows[connIdx].actual_W,
                                                ss.flows[connIdx].requested_W,
                                                ss.flows[connIdx].available_W,
                                                ConnectionToString(model.component, conn, true)));
                            }
                            if (conn.from_component_id == item.first)
                            {
                                Log_warning(
                                    log,
                                    fmt::format("* -{:>16d} W (R: -{:>16d} W // A: "
                                                "-{:>16d} W):: {}",
                                                ss.flows[connIdx].actual_W,
                                                ss.flows[connIdx].requested_W,
                                                ss.flows[connIdx].available_W,
                                                ConnectionToString(model.component, conn, true)));
                            }
                        }
                    }
                }
            }
            PrintModelState(model, ss);
            Log_info(log, "==== QUIESCENCE REACHED ====");
        }
        TimeAndFlows taf = {};
        taf.time_s = t;
        taf.flows = CopyFlows(ss.flows);
        taf.storage_amounts_J = CopyStorageStates(ss);
        timeAndFlows.push_back(std::move(taf));
        if (t == model.final_time_s)
        {
            break;
        }
        double nextTime = earliest_next_event(model, ss, t);
        if ((nextTime == infinite_time && t < model.final_time_s) ||
            (nextTime > model.final_time_s))
        {
            nextTime = model.final_time_s;
        }
        UpdateStoresPerElapsedTime(model, ss, nextTime - t);
        UpdateScheduleBasedLoadNextEvent(model, ss, nextTime);
        UpdateScheduleBasedSourceNextEvent(model, ss, nextTime);
        t = nextTime;
    }
    return timeAndFlows;
}

void Model_SetComponentToRepaired(Model const& m, SimulationState& ss, size_t compId)
{
    if (compId >= m.component.component_type.size())
    {
        write_error_message("", "invalid component id");
        std::exit(1);
    }
    if (ss.unavailable_components.contains(compId))
    {
        ss.unavailable_components.erase(compId);
    }
    auto idx = m.component.subtype_index[compId];
    switch (m.component.component_type[compId])
    {
    case ComponentType::constant_load_type:
    {
        auto inflowConn = m.constant_load[idx].inflow_connection_id;
        if (ss.flows[inflowConn].requested_W != m.constant_load[idx].load_W)
        {
            ss.active_connections_back.insert(inflowConn);
        }
        ss.flows[inflowConn].requested_W = m.constant_load[idx].load_W;
    }
    break;
    case ComponentType::schedule_based_load_type:
    {
        auto const inflowConn = m.constant_load[idx].inflow_connection_id;
        auto const loadIdx = ss.schedule_based_load_index[idx];
        auto const amount = m.scheduled_load[idx].times_and_loads[loadIdx].Amount_W;
        if (ss.flows[inflowConn].requested_W != amount)
        {
            ss.active_connections_back.insert(inflowConn);
        }
        ss.flows[inflowConn].requested_W = amount;
    }
    break;
    case ComponentType::constant_source_type:
    {
        auto const outflowConn = m.constant_source[idx].outflow_connection_id;
        auto const available = m.constant_source[idx].available_W;
        if (ss.flows[outflowConn].available_W != available)
        {
            ss.active_connections_front.insert(outflowConn);
        }
        ss.flows[outflowConn].available_W = available;
    }
    break;
    case ComponentType::schedule_based_source_type:
    {
        // TODO: need to call routine to reset wasteflow connection
        // to the right amount as well.
        auto const outflowConn = m.scheduled_source[idx].outflow_connection_id;
        auto const availIdx = ss.schedule_based_source_index[idx];
        auto const available = m.scheduled_source[idx].time_and_availables[availIdx].Amount_W;
        if (ss.flows[outflowConn].available_W != available)
        {
            ss.active_connections_front.insert(outflowConn);
        }
        ss.flows[outflowConn].available_W = available;
    }
    break;
    case ComponentType::constant_efficiency_converter_type:
    {
        assert(idx < m.constant_efficiency_converter.size());
        auto outflowConn = m.constant_efficiency_converter[idx].outflow_connection_id;
        RunConstantEfficiencyConverterBackward(m, ss, outflowConn, idx);
        auto inflowConn = m.constant_efficiency_converter[idx].inflow_connection_id;
        RunConstantEfficiencyConverterForward(m, ss, inflowConn, idx);
    }
    break;
    case ComponentType::variable_efficiency_converter_type:
    {
        assert(idx < m.variable_efficiency_converter.size());
        auto outflowConn = m.variable_efficiency_converter[idx].outflow_connection_id;
        RunVariableEfficiencyConverterBackward(m, ss, outflowConn, idx);
        auto inflowConn = m.variable_efficiency_converter[idx].inflow_connection_id;
        RunVariableEfficiencyConverterForward(m, ss, inflowConn, idx);
    }
    break;
    case ComponentType::mover_type:
    {
        // TODO: test
        assert(idx < m.mover.size());
        size_t outflowConn = m.mover[idx].outflow_connection_id;
        RunMoverBackward(m, ss, outflowConn, idx);
        size_t inflowConn = m.mover[idx].inflow_connection_id;
        RunMoverForward(m, ss, inflowConn, idx);
    }
    break;
    case ComponentType::variable_efficiency_mover_type:
    {
        assert(idx < m.variable_efficiency_mover.size());
        size_t outflowConn = m.variable_efficiency_mover[idx].outflow_connection_id;
        RunVariableEfficiencyMoverBackward(m, ss, outflowConn, idx);
        size_t inflowConn = m.variable_efficiency_mover[idx].inflow_connection_id;
        RunVariableEfficiencyMoverForward(m, ss, inflowConn, idx);
    }
    break;
    case ComponentType::mux_type:
    {
        // TODO: check that this works
        // may also need to call mux routines to rebalance
        for (size_t inIdx = 0; inIdx < m.mux[idx].number_of_inports; ++inIdx)
        {
            auto const inConn = m.mux[idx].inflow_connection_ids[inIdx];
            // NOTE: schedule upstream to provide available flow info
            // again.
            ss.active_connections_front.insert(inConn);
        }
        for (size_t outIdx = 0; outIdx < m.mux[idx].number_of_outports; ++outIdx)
        {
            auto const outConn = m.mux[idx].outflow_connection_ids[outIdx];
            ss.active_connections_back.insert(outConn);
        }
    }
    break;
    case ComponentType::store_type:
    {
        // TODO: what energy amount should the store come back with?
        // TODO: need to call routines to do charge request
        write_error_message("store", "not implemented");
        std::exit(1);
    }
    break;
    case ComponentType::pass_through_type:
    {
        PassThrough const& pt = m.pass_through[idx];
        if (ss.flows[pt.outflow_connection_id].requested_W !=
            ss.flows[pt.inflow_connection_id].requested_W)
        {
            ss.active_connections_back.insert(pt.outflow_connection_id);
        }
        ss.flows[pt.inflow_connection_id].requested_W =
            ss.flows[pt.outflow_connection_id].requested_W;
        if (ss.flows[pt.inflow_connection_id].available_W !=
            ss.flows[pt.outflow_connection_id].available_W)
        {
            ss.active_connections_front.insert(pt.inflow_connection_id);
        }
        ss.flows[pt.outflow_connection_id].available_W =
            ss.flows[pt.inflow_connection_id].available_W;
    }
    break;
    case ComponentType::waste_sink_type:
    {
        write_error_message("waste sink", "should not be repairing pseduo-element waste");
        std::exit(1);
    }
    break;
    default:
    {
        write_error_message("", "unhandled component type (b)");
        std::exit(1);
    }
    }
}

void Model_SetComponentToFailed(Model const& m, SimulationState& ss, size_t compId)
{
    if (compId >= m.component.component_type.size())
    {
        write_error_message("", "invalid component id");
        std::exit(1);
    }
    ss.unavailable_components.insert(compId);
    auto idx = m.component.subtype_index[compId];
    switch (m.component.component_type[compId])
    {
    case ComponentType::constant_load_type:
    {
        auto inflowConn = m.constant_load[idx].inflow_connection_id;
        if (ss.flows[inflowConn].requested_W != 0)
        {
            ss.active_connections_back.insert(inflowConn);
        }
        ss.flows[inflowConn].requested_W = 0;
    }
    break;
    case ComponentType::schedule_based_load_type:
    {
        auto inflowConn = m.scheduled_load[idx].inflow_connection_id;
        if (ss.flows[inflowConn].requested_W != 0)
        {
            ss.active_connections_back.insert(inflowConn);
        }
        ss.flows[inflowConn].requested_W = 0;
    }
    break;
    case ComponentType::constant_source_type:
    {
        auto outflowConn = m.constant_source[idx].outflow_connection_id;
        if (ss.flows[outflowConn].available_W != 0)
        {
            ss.active_connections_front.insert(outflowConn);
        }
        ss.flows[outflowConn].available_W = 0;
    }
    break;
    case ComponentType::constant_efficiency_converter_type:
    {
        assert(idx < m.constant_efficiency_converter.size());
        ConstantEfficiencyConverter const& cec = m.constant_efficiency_converter[idx];
        auto inflowConn = cec.inflow_connection_id;
        if (ss.flows[inflowConn].requested_W != 0)
        {
            ss.active_connections_back.insert(inflowConn);
        }
        ss.flows[inflowConn].requested_W = 0;
        auto outflowConn = cec.outflow_connection_id;
        if (ss.flows[outflowConn].available_W != 0)
        {
            ss.active_connections_front.insert(outflowConn);
        }
        ss.flows[outflowConn].available_W = 0;
        auto lossflowConn = cec.lossflow_connection_id;
        if (lossflowConn.has_value())
        {
            auto lossConn = lossflowConn.value();
            if (ss.flows[lossConn].available_W != 0)
            {
                ss.active_connections_front.insert(lossConn);
            }
            ss.flows[lossConn].available_W = 0;
        }
        auto wasteConn = cec.wasteflow_connection_id;
        ss.flows[wasteConn].available_W = 0;
        ss.flows[wasteConn].requested_W = 0;
    }
    break;
    case ComponentType::variable_efficiency_converter_type:
    {
        assert(idx < m.variable_efficiency_converter.size());
        VariableEfficiencyConverter const& vec = m.variable_efficiency_converter[idx];
        auto inflowConn = vec.inflow_connection_id;
        if (ss.flows[inflowConn].requested_W != 0)
        {
            ss.active_connections_back.insert(inflowConn);
        }
        ss.flows[inflowConn].requested_W = 0;
        auto outflowConn = vec.outflow_connection_id;
        if (ss.flows[outflowConn].available_W != 0)
        {
            ss.active_connections_front.insert(outflowConn);
        }
        ss.flows[outflowConn].available_W = 0;
        auto lossflowConn = vec.lossflow_connection_id;
        if (lossflowConn.has_value())
        {
            auto lossConn = lossflowConn.value();
            if (ss.flows[lossConn].available_W != 0)
            {
                ss.active_connections_front.insert(lossConn);
            }
            ss.flows[lossConn].available_W = 0;
        }
        auto wasteConn = vec.wasteflow_connection_id;
        ss.flows[wasteConn].available_W = 0;
        ss.flows[wasteConn].requested_W = 0;
    }
    break;
    case ComponentType::mover_type:
    {
        Mover const& mov = m.mover[idx];
        ss.flows[mov.inflow_connection_id].requested_W = 0;
        ss.flows[mov.inflow_connection_id].available_W = 0;
        ss.flows[mov.inflow_connection_id].actual_W = 0;
        ss.flows[mov.outflow_connection_id].requested_W = 0;
        ss.flows[mov.outflow_connection_id].available_W = 0;
        ss.flows[mov.outflow_connection_id].actual_W = 0;
        ss.flows[mov.in_from_env_connection_id].requested_W = 0;
        ss.flows[mov.in_from_env_connection_id].available_W = 0;
        ss.flows[mov.in_from_env_connection_id].actual_W = 0;
        ss.flows[mov.wasteflow_connection_id].requested_W = 0;
        ss.flows[mov.wasteflow_connection_id].available_W = 0;
        ss.flows[mov.wasteflow_connection_id].actual_W = 0;
    }
    break;
    case ComponentType::variable_efficiency_mover_type:
    {
        assert(idx < m.variable_efficiency_mover.size());
        VariableEfficiencyMover const& mov = m.variable_efficiency_mover[idx];
        ss.flows[mov.inflow_connection_id].requested_W = 0;
        ss.flows[mov.inflow_connection_id].available_W = 0;
        ss.flows[mov.inflow_connection_id].actual_W = 0;
        ss.flows[mov.outflow_connection_id].requested_W = 0;
        ss.flows[mov.outflow_connection_id].available_W = 0;
        ss.flows[mov.outflow_connection_id].actual_W = 0;
        ss.flows[mov.in_from_env_connection_id].requested_W = 0;
        ss.flows[mov.in_from_env_connection_id].available_W = 0;
        ss.flows[mov.in_from_env_connection_id].actual_W = 0;
        ss.flows[mov.wasteflow_connection_id].requested_W = 0;
        ss.flows[mov.wasteflow_connection_id].available_W = 0;
        ss.flows[mov.wasteflow_connection_id].actual_W = 0;
    }
    break;
    case ComponentType::mux_type:
    {
        for (size_t inIdx = 0; inIdx < m.mux[idx].number_of_inports; ++inIdx)
        {
            auto inflowConn = m.mux[idx].inflow_connection_ids[inIdx];
            if (ss.flows[inflowConn].requested_W != 0)
            {
                ss.active_connections_back.insert(inflowConn);
            }
            ss.flows[inflowConn].requested_W = 0;
        }
        for (size_t outIdx = 0; outIdx < m.mux[idx].number_of_outports; ++outIdx)
        {
            auto outflowConn = m.mux[idx].outflow_connection_ids[outIdx];
            if (ss.flows[outflowConn].available_W != 0)
            {
                ss.active_connections_front.insert(outflowConn);
            }
            ss.flows[outflowConn].available_W = 0;
        }
    }
    break;
    case ComponentType::store_type:
    {
        // TODO: check! May need to update wasteflow and use efficiency?
        std::optional<size_t> maybeInflowConn = m.store[idx].inflow_connection_id;
        if (maybeInflowConn.has_value())
        {
            size_t inflowConn = maybeInflowConn.value();
            if (ss.flows[inflowConn].requested_W != 0)
            {
                ss.active_connections_back.insert(inflowConn);
            }
            ss.flows[inflowConn].requested_W = 0;
        }
        auto outflowConn = m.store[idx].outflow_connection_id;
        if (ss.flows[outflowConn].available_W != 0)
        {
            ss.active_connections_front.insert(outflowConn);
        }
        ss.flows[outflowConn].available_W = 0;
        if (m.store[idx].wasteflow_connection_id.has_value())
        {
            size_t wConn = m.store[idx].wasteflow_connection_id.value();
            ss.flows[wConn].actual_W = 0;
            ss.flows[wConn].available_W = 0;
            ss.flows[wConn].requested_W = 0;
        }
    }
    break;
    case ComponentType::pass_through_type:
    {
        auto const& pt = m.pass_through[idx];
        if (ss.flows[pt.inflow_connection_id].requested_W != 0)
        {
            ss.active_connections_back.insert(pt.inflow_connection_id);
        }
        ss.flows[pt.inflow_connection_id].requested_W = 0;
        if (ss.flows[pt.outflow_connection_id].available_W != 0)
        {
            ss.active_connections_front.insert(pt.outflow_connection_id);
        }
        ss.flows[pt.outflow_connection_id].available_W = 0;
    }
    break;
    case ComponentType::waste_sink_type:
    {
        write_error_message("waste sink", "waste sink cannot fail");
        std::exit(1);
    }
    break;
    default:
    {
        write_error_message("", "unhandled component type (c)");
        std::exit(1);
    }
    break;
    }
}

size_t Model_AddSwitch(Model& m, size_t flowTypeId, std::string const& tag)
{
    Switch s = {
        .inflow_connection_id_primary = 0,
        .inflow_connection_id_secondary = 0,
        .outflow_connection_id = 0,
        .max_outflow_W = max_flow_W,
    };
    size_t subtypeIndex = m.transfer_switch.size();
    m.transfer_switch.push_back(std::move(s));
    return add_component_returning_id(m.component,
                                      ComponentType::switch_type,
                                      subtypeIndex,
                                      std::vector<size_t> {flowTypeId, flowTypeId},
                                      std::vector<size_t> {flowTypeId},
                                      tag,
                                      0.0);
}

size_t Model_AddConstantLoad(Model& m, flow_t load)
{
    return Model_AddConstantLoad(m, load, 0, "", true);
}

size_t Model_AddConstantLoad(
    Model& m, flow_t load, size_t inflowTypeId, std::string const& tag, bool report)
{
    size_t idx = m.constant_load.size();
    ConstantLoad cl {};
    cl.load_W = load;
    m.constant_load.push_back(std::move(cl));
    return add_component_returning_id(m.component,
                                      ComponentType::constant_load_type,
                                      idx,
                                      std::vector<size_t> {inflowTypeId},
                                      std::vector<size_t> {},
                                      tag,
                                      0.0,
                                      report);
}

size_t Model_AddScheduleBasedLoad(Model& m, double* times, flow_t* loads, size_t numItems)
{
    std::vector<TimeAndAmount> timesAndLoads = {};
    timesAndLoads.reserve(numItems);
    for (size_t i = 0; i < numItems; ++i)
    {
        TimeAndAmount tal {times[i], loads[i]};
        timesAndLoads.push_back(std::move(tal));
    }
    return Model_AddScheduleBasedLoad(m, timesAndLoads);
}

size_t Model_AddScheduleBasedLoad(Model& m, std::vector<TimeAndAmount> const& timesAndLoads)
{
    return Model_AddScheduleBasedLoad(m, timesAndLoads, std::map<size_t, size_t> {});
}

size_t Model_AddScheduleBasedLoad(Model& m,
                                  std::vector<TimeAndAmount> const& timesAndLoads,
                                  std::map<size_t, size_t> const& scenarioIdToLoadId)
{
    return Model_AddScheduleBasedLoad(m, timesAndLoads, scenarioIdToLoadId, 0, "");
}

size_t Model_AddScheduleBasedLoad(Model& m,
                                  std::vector<TimeAndAmount> const& timesAndLoads,
                                  std::map<size_t, size_t> const& scenarioIdToLoadId,
                                  size_t inflowTypeId,
                                  std::string const& tag)
{
    size_t idx = m.scheduled_load.size();
    ScheduleBasedLoad sbl = {};
    sbl.times_and_loads = timesAndLoads;
    sbl.inflow_connection_id = 0;
    sbl.scenario_id_to_load_id = scenarioIdToLoadId;
    m.scheduled_load.push_back(std::move(sbl));
    return add_component_returning_id(m.component,
                                      ComponentType::schedule_based_load_type,
                                      idx,
                                      std::vector<size_t> {inflowTypeId},
                                      std::vector<size_t> {},
                                      tag,
                                      0.0);
}

size_t Model_AddConstantSource(Model& m, flow_t available)
{
    return Model_AddConstantSource(m, available, 0, "");
}

size_t
Model_AddConstantSource(Model& m, flow_t available, size_t outflowTypeId, std::string const& tag)
{
    size_t idx = m.constant_source.size();
    ConstantSource cs {};
    cs.available_W = available;
    m.constant_source.push_back(std::move(cs));
    return add_component_returning_id(m.component,
                                      ComponentType::constant_source_type,
                                      idx,
                                      std::vector<size_t> {},
                                      std::vector<size_t> {outflowTypeId},
                                      tag,
                                      0.0);
}

ComponentIdAndWasteConnection Model_AddScheduleBasedSource(Model& m,
                                                           std::vector<TimeAndAmount> const& xs)
{
    return Model_AddScheduleBasedSource(m, xs, std::map<size_t, size_t> {}, 0, "", 0.0);
}

ComponentIdAndWasteConnection
Model_AddScheduleBasedSource(Model& m,
                             std::vector<TimeAndAmount> const& xs,
                             std::map<size_t, size_t> const& scenarioIdToSourceId,
                             size_t outflowId,
                             std::string const& tag,
                             double initialAge_s)
{
    auto idx = m.scheduled_source.size();
    ScheduleBasedSource sbs = {};
    sbs.time_and_availables = xs;
    sbs.scenario_id_to_source_id = scenarioIdToSourceId;
    m.scheduled_source.push_back(sbs);
    size_t wasteId = add_component_returning_id(m.component, ComponentType::waste_sink_type, 0);
    size_t thisId = add_component_returning_id(m.component,
                                               ComponentType::schedule_based_source_type,
                                               idx,
                                               std::vector<size_t> {},
                                               std::vector<size_t> {outflowId},
                                               tag,
                                               initialAge_s);
    auto wasteConn = Model_AddConnection(m, thisId, 1, wasteId, 0);
    return {thisId, wasteConn};
}

size_t Model_AddMux(Model& m, size_t numInports, size_t numOutports)
{
    return Model_AddMux(m, numInports, numOutports, 0, "");
}

size_t
Model_AddMux(Model& m, size_t numInports, size_t numOutports, size_t flowId, std::string const& tag)
{
    size_t idx = m.mux.size();
    Mux mux {
        .number_of_inports = numInports,
        .number_of_outports = numOutports,
        .inflow_connection_ids = std::vector<size_t>(numInports, 0),
        .outflow_connection_ids = std::vector<size_t>(numOutports, 0),
        .max_outflows_W = std::vector<flow_t>(numOutports, max_flow_W),
    };
    m.mux.push_back(std::move(mux));
    std::vector<size_t> inflowTypes(numInports, flowId);
    std::vector<size_t> outflowTypes(numOutports, flowId);
    return add_component_returning_id(
        m.component, ComponentType::mux_type, idx, inflowTypes, outflowTypes, tag, 0.0);
}

size_t Model_AddStore(Model& m,
                      flow_t capacity,
                      flow_t maxCharge,
                      flow_t maxDischarge,
                      flow_t chargeAmount,
                      flow_t initialStorage)
{
    return Model_AddStore(
        m, capacity, maxCharge, maxDischarge, chargeAmount, initialStorage, 0, "");
}

size_t Model_AddStore(Model& m,
                      flow_t capacity,
                      flow_t maxCharge,
                      flow_t maxDischarge,
                      flow_t chargeAmount,
                      flow_t initialStorage,
                      size_t flowId,
                      std::string const& tag)
{
    assert(chargeAmount < capacity && "chargeAmount must be less than capacity");
    assert(initialStorage <= capacity && "initialStorage must be less than or equal to capacity");
    size_t idx = m.store.size();
    Store s = {};
    s.capacity_J = capacity;
    s.max_charge_rate_W = maxCharge;
    s.max_discharge_rate_W = maxDischarge;
    s.charge_amount_J = chargeAmount;
    s.initial_storage_J = initialStorage;
    s.roundtrip_efficiency = 1.0;
    m.store.push_back(std::move(s));
    return add_component_returning_id(m.component,
                                      ComponentType::store_type,
                                      idx,
                                      std::vector<size_t> {flowId},
                                      std::vector<size_t> {flowId},
                                      tag,
                                      0.0);
}

ComponentIdAndWasteConnection Model_AddStoreWithWasteflow(Model& m,
                                                          flow_t capacity,
                                                          flow_t maxCharge,
                                                          flow_t maxDischarge,
                                                          flow_t chargeAmount,
                                                          flow_t initialStorage,
                                                          size_t flowId,
                                                          double roundtripEfficiency,
                                                          std::string const& tag)
{
    assert(roundtripEfficiency > 0.0 && roundtripEfficiency <= 1.0);
    size_t id = Model_AddStore(
        m, capacity, maxCharge, maxDischarge, chargeAmount, initialStorage, flowId, tag);
    size_t storeIdx = m.component.subtype_index[id];
    m.store[storeIdx].roundtrip_efficiency = roundtripEfficiency;
    size_t wasteId = add_component_returning_id(m.component,
                                                ComponentType::waste_sink_type,
                                                0,
                                                std::vector<size_t> {wasteflow_id},
                                                std::vector<size_t> {},
                                                "",
                                                0.0);
    auto wasteConn = Model_AddConnection(m, id, 1, wasteId, 0, wasteflow_id);
    return {
        .id = id,
        .waste_connection_id = std::move(wasteConn),
    };
}

ComponentIdAndWasteConnection
Model_AddConstantEfficiencyConverter(Model& m, flow_t eff_numerator, flow_t eff_denominator)
{
    return Model_AddConstantEfficiencyConverter(m, (double)eff_numerator / (double)eff_denominator);
}

ComponentIdAndWasteConnection Model_AddConstantEfficiencyConverter(Model& m, double efficiency)
{
    return Model_AddConstantEfficiencyConverter(m, efficiency, 0, 0, 0, "", true);
}

ComponentIdAndWasteConnection Model_AddConstantEfficiencyConverter(Model& m,
                                                                   double efficiency,
                                                                   size_t inflowId,
                                                                   size_t outflowId,
                                                                   size_t lossflowId,
                                                                   std::string const& tag,
                                                                   bool report)
{
    assert(efficiency > 0.0 && efficiency <= 1.0);
    // NOTE: the 0th flowId is ""; the non-described flow
    std::vector<size_t> inflowIds {inflowId};
    std::vector<size_t> outflowIds {outflowId, lossflowId, wasteflow_id};
    size_t idx = m.constant_efficiency_converter.size();
    ConstantEfficiencyConverter cec {
        .efficiency = efficiency,
        .inflow_connection_id = 0,
        .outflow_connection_id = 0,
        .lossflow_connection_id = {},
        .wasteflow_connection_id = 0,
        .max_outflow_W = max_flow_W,
        .max_lossflow_W = max_flow_W,
    };
    m.constant_efficiency_converter.push_back(std::move(cec));
    size_t wasteId = add_component_returning_id(m.component,
                                                ComponentType::waste_sink_type,
                                                0,
                                                std::vector<size_t> {wasteflow_id},
                                                std::vector<size_t> {},
                                                "",
                                                0.0,
                                                report);
    size_t thisId = add_component_returning_id(m.component,
                                               ComponentType::constant_efficiency_converter_type,
                                               idx,
                                               inflowIds,
                                               outflowIds,
                                               tag,
                                               0.0,
                                               report);
    auto wasteConn = Model_AddConnection(m, thisId, 2, wasteId, 0, wasteflow_id);
    return {thisId, wasteConn};
}

ComponentIdAndWasteConnection
Model_AddVariableEfficiencyConverter(Model& m,
                                     std::vector<double>&& outflowsForEfficiency_W,
                                     std::vector<double>&& efficiencyByOutflow,
                                     size_t inflowId,
                                     size_t outflowId,
                                     size_t lossflowId,
                                     std::string const& tag,
                                     bool report)
{
    // NOTE: the 0th flowId is ""; the non-described flow
    std::vector<size_t> inflowIds {inflowId};
    std::vector<size_t> outflowIds {outflowId, lossflowId, wasteflow_id};
    size_t idx = m.variable_efficiency_converter.size();
    std::vector<double> inflowsForEfficiency_W;
    inflowsForEfficiency_W.reserve(outflowsForEfficiency_W.size());
    for (size_t i = 0; i < outflowsForEfficiency_W.size(); ++i)
    {
        inflowsForEfficiency_W.push_back(outflowsForEfficiency_W[i] / efficiencyByOutflow[i]);
    }
    VariableEfficiencyConverter vec {};
    vec.outflows_for_efficiency_W = std::move(outflowsForEfficiency_W);
    vec.inflows_for_efficiency_W = std::move(inflowsForEfficiency_W);
    vec.efficiencies = std::move(efficiencyByOutflow);

    m.variable_efficiency_converter.push_back(std::move(vec));
    size_t wasteId = add_component_returning_id(m.component,
                                                ComponentType::waste_sink_type,
                                                0,
                                                std::vector<size_t> {wasteflow_id},
                                                std::vector<size_t> {},
                                                "",
                                                0.0,
                                                report);
    size_t thisId = add_component_returning_id(m.component,
                                               ComponentType::variable_efficiency_converter_type,
                                               idx,
                                               inflowIds,
                                               outflowIds,
                                               tag,
                                               0.0,
                                               report);
    auto wasteConn = Model_AddConnection(m, thisId, 2, wasteId, 0, wasteflow_id);
    return {thisId, wasteConn};
}

size_t Model_AddPassThrough(Model& m) { return Model_AddPassThrough(m, 0, ""); }

size_t Model_AddPassThrough(Model& m, size_t flowId, std::string const& tag)
{
    size_t idx = m.pass_through.size();
    m.pass_through.push_back({
        .inflow_connection_id = 0,
        .outflow_connection_id = 0,
        .max_outflow_W = max_flow_W,
    });
    return add_component_returning_id(m.component,
                                      ComponentType::pass_through_type,
                                      idx,
                                      std::vector<size_t> {flowId},
                                      std::vector<size_t> {flowId},
                                      tag,
                                      0.0);
}

Connection Model_AddConnection(Model& m, size_t fromId, size_t fromPort, size_t toId, size_t toPort)
{
    return Model_AddConnection(m, fromId, fromPort, toId, toPort, 0);
}

Connection Model_AddConnection(Model& m,
                               size_t fromId,
                               size_t fromPort,
                               size_t toId,
                               size_t toPort,
                               size_t flowId,
                               bool checkIntegrity)
{
    ComponentType fromType = m.component.component_type[fromId];
    size_t fromIdx = m.component.subtype_index[fromId];
    ComponentType toType = m.component.component_type[toId];
    size_t toIdx = m.component.subtype_index[toId];
    Connection c {
        .from = fromType,
        .from_subtype_index = fromIdx,
        .from_port = fromPort,
        .from_component_id = fromId,
        .to = toType,
        .to_subtype_index = toIdx,
        .to_port = toPort,
        .to_component_id = toId,
        .flow_type_id = flowId,
    };
    size_t connId = m.connection.size();
    if (checkIntegrity)
    {
        bool issueFound = false;
        for (auto const& conn : m.connection)
        {
            if (conn.from_component_id == fromId && conn.from_port == fromPort)
            {
                issueFound = true;
                std::cout << "INTEGRITY VIOLATION: "
                          << "attempt to doubly connect "
                          << "compId=" << fromId << " outport=" << fromPort
                          << " tag=" << m.component.tag[fromId]
                          << " type=" << ToString(m.component.component_type[fromId]) << std::endl;
            }
            if (conn.to_component_id == toId && conn.to_port == toPort)
            {
                issueFound = true;
                std::cout << "INTEGRITY VIOLATION: "
                          << "attempt to doubly connect "
                          << "compId=" << toId << " inport=" << toPort
                          << " tag=" << m.component.tag[toId]
                          << " type=" << ToString(m.component.component_type[toId]) << std::endl;
            }
        }
        if (issueFound)
        {
            std::cout << "Issue when trying to make a connection\n";
            std::exit(1);
        }
    }
    m.connection.push_back(c);
    switch (fromType)
    {
    case ComponentType::pass_through_type:
    {
        assert(fromIdx < m.pass_through.size());
        m.pass_through[fromIdx].outflow_connection_id = connId;
    }
    break;
    case ComponentType::constant_source_type:
    {
        assert(fromIdx < m.constant_source.size());
        m.constant_source[fromIdx].outflow_connection_id = connId;
    }
    break;
    case ComponentType::schedule_based_source_type:
    {
        assert(fromIdx < m.scheduled_source.size());
        switch (fromPort)
        {
        case 0:
        {
            m.scheduled_source[fromIdx].outflow_connection_id = connId;
        }
        break;
        case 1:
        {
            m.scheduled_source[fromIdx].wasteflow_connection_id = connId;
        }
        break;
        default:
        {
            throw std::invalid_argument {"invalid outport for schedule-based source"};
        }
        break;
        }
    }
    break;
    case ComponentType::constant_efficiency_converter_type:
    {
        assert(fromIdx < m.constant_efficiency_converter.size());
        switch (fromPort)
        {
        case 0:
        {
            m.constant_efficiency_converter[fromIdx].outflow_connection_id = connId;
        }
        break;
        case 1:
        {
            m.constant_efficiency_converter[fromIdx].lossflow_connection_id = connId;
        }
        break;
        case 2:
        {
            m.constant_efficiency_converter[fromIdx].wasteflow_connection_id = connId;
        }
        break;
        default:
        {
            throw std::invalid_argument("Unhandled constant efficiency converter outport");
        }
        }
    }
    break;
    case ComponentType::variable_efficiency_converter_type:
    {
        assert(fromIdx < m.variable_efficiency_converter.size());
        switch (fromPort)
        {
        case 0:
        {
            m.variable_efficiency_converter[fromIdx].outflow_connection_id = connId;
        }
        break;
        case 1:
        {
            m.variable_efficiency_converter[fromIdx].lossflow_connection_id = connId;
        }
        break;
        case 2:
        {
            m.variable_efficiency_converter[fromIdx].wasteflow_connection_id = connId;
        }
        break;
        default:
        {
            throw std::invalid_argument("Unhandled variable efficiency converter outport");
        }
        }
    }
    break;
    case ComponentType::mover_type:
    {
        assert(fromIdx < m.mover.size());
        switch (fromPort)
        {
        case 0: // outflow
        {
            m.mover[fromIdx].outflow_connection_id = connId;
        }
        break;
        case 1: // wasteflow
        {
            m.mover[fromIdx].wasteflow_connection_id = connId;
        }
        break;
        default:
        {
            write_error_message("<network>", "bad port for Mover");
            std::exit(1);
        }
        break;
        }
    }
    break;
    case ComponentType::variable_efficiency_mover_type:
    {
        assert(fromIdx < m.variable_efficiency_mover.size());
        switch (fromPort)
        {
        case 0: // outflow
        {
            m.variable_efficiency_mover[fromIdx].outflow_connection_id = connId;
        }
        break;
        case 1: // wasteflow
        {
            m.variable_efficiency_mover[fromIdx].wasteflow_connection_id = connId;
        }
        break;
        default:
        {
            write_error_message("<network>", "bad port for VariableEfficiencyMover");
            std::exit(1);
        }
        break;
        }
    }
    break;
    case ComponentType::mux_type:
    {
        assert(fromIdx < m.mux.size());
        assert(fromPort < m.mux[fromIdx].outflow_connection_ids.size());
        assert(fromPort < m.mux[fromIdx].number_of_outports);
        m.mux[fromIdx].outflow_connection_ids[fromPort] = connId;
    }
    break;
    case ComponentType::store_type:
    {
        assert(fromIdx < m.store.size());
        switch (fromPort)
        {
        case 0:
        {
            m.store[fromIdx].outflow_connection_id = connId;
        }
        break;
        case 1:
        {
            m.store[fromIdx].wasteflow_connection_id = connId;
        }
        break;
        default:
        {
            write_error_message("store port", "unhandled port for store");
            std::exit(1);
        }
        }
    }
    break;
    case ComponentType::environment_source_type:
    {
        // NOTE: do nothing
    }
    break;
    case ComponentType::switch_type:
    {
        assert(fromIdx < m.transfer_switch.size());
        m.transfer_switch[fromIdx].outflow_connection_id = connId;
    }
    break;
    default:
    {
        write_error_message("Model_AddConnection",
                            "unhandled component type: " + ToString(fromType));
        std::exit(1);
    }
    }
    switch (toType)
    {
    case ComponentType::switch_type:
    {
        assert(toIdx < m.transfer_switch.size());
        switch (toPort)
        {
        case 0:
        {
            m.transfer_switch[toIdx].inflow_connection_id_primary = connId;
        }
        break;
        case 1:
        {
            m.transfer_switch[toIdx].inflow_connection_id_secondary = connId;
        }
        break;
        default:
        {
            write_error_message("Model_AddConnection",
                                "unhandled inport: " + std::to_string(toPort) + " for " +
                                    ToString(toType));
            std::exit(1);
        }
        break;
        }
    }
    break;
    case ComponentType::pass_through_type:
    {
        assert(toIdx < m.pass_through.size());
        m.pass_through[toIdx].inflow_connection_id = connId;
    }
    break;
    case ComponentType::constant_load_type:
    {
        assert(toIdx < m.constant_load.size());
        m.constant_load[toIdx].inflow_connection_id = connId;
    }
    break;
    case ComponentType::constant_efficiency_converter_type:
    {
        assert(toIdx < m.constant_efficiency_converter.size());
        m.constant_efficiency_converter[toIdx].inflow_connection_id = connId;
    }
    break;
    case ComponentType::variable_efficiency_converter_type:
    {
        assert(toIdx < m.variable_efficiency_converter.size());
        m.variable_efficiency_converter[toIdx].inflow_connection_id = connId;
    }
    break;
    case ComponentType::mover_type:
    {
        assert(toIdx < m.mover.size());
        switch (toPort)
        {
        case 0:
        {
            m.mover[toIdx].inflow_connection_id = connId;
        }
        break;
        case 1:
        {
            m.mover[toIdx].in_from_env_connection_id = connId;
        }
        break;
        default:
        {
            write_error_message("<network>", "bad network connection for mover");
            std::exit(1);
        }
        break;
        }
    }
    break;
    case ComponentType::variable_efficiency_mover_type:
    {
        assert(toIdx < m.variable_efficiency_mover.size());
        switch (toPort)
        {
        case 0:
        {
            m.variable_efficiency_mover[toIdx].inflow_connection_id = connId;
        }
        break;
        case 1:
        {
            m.variable_efficiency_mover[toIdx].in_from_env_connection_id = connId;
        }
        break;
        default:
        {
            write_error_message("<network>",
                                "bad network connection for variable efficiency "
                                "mover");
            std::exit(1);
        }
        break;
        }
    }
    break;
    case ComponentType::mux_type:
    {
        assert(toIdx < m.mux.size());
        assert(toPort < m.mux[toIdx].inflow_connection_ids.size());
        assert(toPort < m.mux[toIdx].number_of_inports);
        m.mux[toIdx].inflow_connection_ids[toPort] = connId;
    }
    break;
    case ComponentType::store_type:
    {
        assert(toIdx < m.store.size());
        m.store[toIdx].inflow_connection_id = connId;
    }
    break;
    case ComponentType::schedule_based_load_type:
    {
        assert(toIdx < m.scheduled_load.size());
        m.scheduled_load[toIdx].inflow_connection_id = connId;
    }
    break;
    case ComponentType::waste_sink_type:
    {
        // NOTE: do nothing
    }
    break;
    default:
    {
        write_error_message("Model_AddConnection", "unhandled component type: " + ToString(toType));
        std::exit(1);
    }
    break;
    }
    return c;
}

bool SameConnection(Connection a, Connection b)
{
    // TODO: revisit: needs to have .FromId and .ToId and ports but not
    // others
    return a.from == b.from && a.from_subtype_index == b.from_subtype_index &&
           a.from_port == b.from_port && a.to == b.to && a.to_subtype_index == b.to_subtype_index &&
           a.to_port == b.to_port;
}

std::optional<Flow> ModelResults_GetFlowForConnection(Model const& m,
                                                      Connection conn,
                                                      double time,
                                                      std::vector<TimeAndFlows> timeAndFlows)
{
    for (size_t connId = 0; connId < m.connection.size(); ++connId)
    {
        if (SameConnection(m.connection[connId], conn))
        {
            Flow f = {};
            for (size_t i = 0; i < timeAndFlows.size(); ++i)
            {
                if (time >= timeAndFlows[i].time_s)
                {
                    f = timeAndFlows[i].flows[connId];
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

std::optional<flow_t> ModelResults_GetStoreState(Model const& m,
                                                 size_t compId,
                                                 double time,
                                                 std::vector<TimeAndFlows> timeAndFlows)
{
    if (compId >= m.component.component_type.size() ||
        m.component.component_type[compId] != ComponentType::store_type)
    {
        return {};
    }
    size_t storeIdx = m.component.subtype_index[compId];
    // TODO: update to also be able to give storage amounts between events
    // by looking at the inflow and outflows to storage and doing the
    // math...
    for (size_t i = 0; i < timeAndFlows.size(); ++i)
    {
        if (time == timeAndFlows[i].time_s && storeIdx < timeAndFlows[i].storage_amounts_J.size())
        {
            return timeAndFlows[i].storage_amounts_J[storeIdx];
        }
    }
    return {};
}

ScenarioOccurrenceStats
ModelResults_CalculateScenarioOccurrenceStats(size_t scenarioId,
                                              size_t occurrenceNumber,
                                              Model const& m,
                                              FlowDict const& flowDict,
                                              std::vector<TimeAndFlows> const& timeAndFlows)
{
    ScenarioOccurrenceStats sos {};
    sos.scenario_id = scenarioId;
    sos.occurrence_number = occurrenceNumber;
    double initialStorage_kJ = 0.0;
    double finalStorage_kJ = 0.0;
    if (timeAndFlows.size() > 0)
    {
        for (flow_t const& stored_J : timeAndFlows[0].storage_amounts_J)
        {
            initialStorage_kJ += static_cast<double>(stored_J) / J_per_kJ;
        }
    }
    double lastTime = timeAndFlows.size() > 0 ? timeAndFlows[0].time_s : 0.0;
    bool wasDown = false;
    double sedt_s = 0.0;
    size_t numTimeAndFlows = timeAndFlows.size();
    size_t lastEventIdx = numTimeAndFlows - 1;
    for (size_t eventIdx = 1; eventIdx < numTimeAndFlows; ++eventIdx)
    {
        double dt_s = timeAndFlows[eventIdx].time_s - lastTime;
        assert(dt_s > 0.0);
        lastTime = timeAndFlows[eventIdx].time_s;
        // TODO: in Simulation, ensure we ALWAYS have an event at final time
        // in order that scenario duration equals what we have here; this
        // is a good check.
        sos.duration_s += dt_s;
        bool allLoadsMet = true;
        std::map<size_t, bool> allLoadsMetByFlowType;
        size_t prevEventIdx = eventIdx - 1;
        for (size_t connId = 0; connId < timeAndFlows[prevEventIdx].flows.size(); ++connId)
        {
            size_t flowTypeId = m.connection[connId].flow_type_id;
            ComponentType fromType =
                m.component.component_type[m.connection[connId].from_component_id];
            ComponentType toType = m.component.component_type[m.connection[connId].to_component_id];
            Flow const& flow = timeAndFlows[prevEventIdx].flows[connId];
            double actualFlow_W = static_cast<double>(flow.actual_W);
            double requestedFlow_W = static_cast<double>(flow.requested_W);
            switch (fromType)
            {
            case ComponentType::constant_source_type:
            case ComponentType::schedule_based_source_type:
            {
                sos.inflow_kJ += (actualFlow_W / W_per_kW) * dt_s;
            }
            break;
            case ComponentType::environment_source_type:
            {
                sos.in_from_env_kJ += (actualFlow_W / W_per_kW) * dt_s;
            }
            break;
            default:
            {
            }
            break;
            }
            switch (toType)
            {
            case ComponentType::constant_load_type:
            case ComponentType::schedule_based_load_type:
            {
                double outflowAchieved_kJ = (actualFlow_W / W_per_kW) * dt_s;
                sos.outflow_achieved_kJ += outflowAchieved_kJ;
                double outflowRequest_kJ = (requestedFlow_W / W_per_kW) * dt_s;
                sos.outflow_request_kJ += outflowRequest_kJ;
                bool loadsMet = flow.actual_W == flow.requested_W;
                allLoadsMet = allLoadsMet && loadsMet;
                double loadNotServed_kJ = 0.0;
                if (!loadsMet)
                {
                    assert(flow.requested_W > flow.actual_W);
                    loadNotServed_kJ = ((requestedFlow_W - actualFlow_W) / W_per_kW) * dt_s;
                    sos.load_not_served_kJ += loadNotServed_kJ;
                }
                bool foundFlowTypeStats = false;
                for (StatsByFlowType& sbf : sos.flow_type_stats)
                {
                    if (sbf.flow_type_id == flowTypeId)
                    {
                        sbf.total_request_kJ += outflowRequest_kJ;
                        sbf.total_achieved_kJ += outflowAchieved_kJ;
                        if (!allLoadsMetByFlowType.contains(flowTypeId))
                        {
                            allLoadsMetByFlowType.insert({flowTypeId, true});
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
                        allLoadsMetByFlowType.insert({flowTypeId, true});
                    }
                    if (!loadsMet)
                    {
                        allLoadsMetByFlowType[flowTypeId] = false;
                    }
                    StatsByFlowType sbf {
                        .flow_type_id = flowTypeId,
                        // NOTE: placeholder; updated later if all loads
                        // for this flow are met
                        .uptime_s = 0.0,
                        .total_request_kJ = outflowRequest_kJ,
                        .total_achieved_kJ = outflowAchieved_kJ,
                    };
                    sos.flow_type_stats.push_back(std::move(sbf));
                }
                size_t compId = m.connection[connId].to_component_id;
                bool foundLoadAndFlowTypeStats = false;
                for (StatsByLoadAndFlowType& sblf : sos.load_and_flow_type_stats)
                {
                    if (sblf.component_id == compId && sblf.stats.flow_type_id == flowTypeId)
                    {
                        if (loadsMet)
                        {
                            sblf.stats.uptime_s += dt_s;
                        }
                        sblf.stats.total_request_kJ += outflowRequest_kJ;
                        sblf.stats.total_achieved_kJ += outflowAchieved_kJ;
                        foundLoadAndFlowTypeStats = true;
                        break;
                    }
                }
                if (!foundLoadAndFlowTypeStats)
                {
                    StatsByFlowType sbf {
                        .flow_type_id = flowTypeId,
                        .uptime_s = loadsMet ? dt_s : 0.0,
                        .total_request_kJ = outflowRequest_kJ,
                        .total_achieved_kJ = outflowAchieved_kJ,
                    };
                    StatsByLoadAndFlowType sblf {
                        .component_id = compId,
                        .stats = std::move(sbf),
                    };
                    sos.load_and_flow_type_stats.push_back(std::move(sblf));
                }
                bool foundLoadNotServedForComp = false;
                for (LoadNotServedForComp& lns : sos.load_not_served_for_components)
                {
                    if (lns.component_id == compId && lns.flow_type_id == flowTypeId)
                    {
                        foundLoadNotServedForComp = true;
                        lns.load_not_served_kJ += loadNotServed_kJ;
                        break;
                    }
                }
                if (!foundLoadNotServedForComp)
                {
                    LoadNotServedForComp lns {
                        .component_id = compId,
                        .flow_type_id = flowTypeId,
                        .load_not_served_kJ = loadNotServed_kJ,
                    };
                    sos.load_not_served_for_components.push_back(std::move(lns));
                }
            }
            break;
            case ComponentType::waste_sink_type:
            {
                sos.wasteflow_kJ += (actualFlow_W / W_per_kW) * dt_s;
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
            for (StatsByFlowType& sbft : sos.flow_type_stats)
            {
                if (sbft.flow_type_id == item.first)
                {
                    foundMatch = true;
                    if (item.second)
                    {
                        sbft.uptime_s += dt_s;
                    }
                    break;
                }
            }
            ignore(foundMatch);
            assert(foundMatch);
        }
        if (allLoadsMet)
        {
            sos.uptime_s += dt_s;
            if (wasDown && sedt_s > sos.max_SEDT_s)
            {
                sos.max_SEDT_s = sedt_s;
            }
            sedt_s = 0.0;
            wasDown = false;
        }
        else
        {
            sos.downtime_s += dt_s;
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
        for (size_t storeIdx = 0; storeIdx < timeAndFlows[eventIdx].storage_amounts_J.size();
             ++storeIdx)
        {
            double prevStored_J =
                static_cast<double>(timeAndFlows[prevEventIdx].storage_amounts_J[storeIdx]);
            double currentStored_J =
                static_cast<double>(timeAndFlows[eventIdx].storage_amounts_J[storeIdx]);
            double increaseInStorage_J = currentStored_J - prevStored_J;
            if (increaseInStorage_J > 0.0)
            {
                sos.storage_charge_kJ += increaseInStorage_J / J_per_kJ;
            }
            else
            {
                sos.storage_discharge_kJ += -1.0 * (increaseInStorage_J / J_per_kJ);
            }
            if (eventIdx == lastEventIdx)
            {
                double amount_J =
                    static_cast<double>(timeAndFlows[eventIdx].storage_amounts_J[storeIdx]);
                finalStorage_kJ += amount_J / J_per_kJ;
            }
        }
    }
    sos.change_in_storage_kJ = finalStorage_kJ - initialStorage_kJ;
    if (sedt_s > sos.max_SEDT_s)
    {
        sos.max_SEDT_s = sedt_s;
    }
    // calculate availability using reliability schedules
    std::vector<TimeState> relSch;
    for (size_t i = 0; i < m.reliability.size(); ++i)
    {
        relSch = TimeState_Combine(relSch, m.reliability[i].time_states);
    }
    TimeState_CountAndTimeFailureEvents(relSch,
                                        m.final_time_s,
                                        sos.event_count_by_failure_mode_id,
                                        sos.event_count_by_fragility_mode_id,
                                        sos.time_by_failure_mode_id_s,
                                        sos.time_by_fragility_mode_id_s);
    sos.availability_s = TimeState_CalcAvailability_s(relSch, m.final_time_s);
    std::map<size_t, std::vector<TimeState>> relSchByCompId;
    for (size_t i = 0; i < m.reliability.size(); ++i)
    {
        ScheduleBasedReliability const& sbr = m.reliability[i];
        relSchByCompId[sbr.component_id] = sbr.time_states;
    }
    for (size_t compId = 0; compId < m.component.tag.size(); ++compId)
    {
        if (!sos.event_count_by_comp_id_by_failure_mode_id.contains(compId))
        {
            sos.event_count_by_comp_id_by_failure_mode_id[compId] = std::map<size_t, size_t> {};
        }
        if (!sos.event_count_by_comp_id_by_fragility_mode_id.contains(compId))
        {
            sos.event_count_by_comp_id_by_fragility_mode_id[compId] = std::map<size_t, size_t> {};
        }
        if (!sos.time_by_comp_id_by_failure_mode_id_s.contains(compId))
        {
            sos.time_by_comp_id_by_failure_mode_id_s[compId] = std::map<size_t, double> {};
        }
        if (!sos.time_by_comp_id_by_fragility_mode_id_s.contains(compId))
        {
            sos.time_by_comp_id_by_fragility_mode_id_s[compId] = std::map<size_t, double> {};
        }
        if (relSchByCompId.contains(compId))
        {
            TimeState_CountAndTimeFailureEvents(
                relSchByCompId[compId],
                m.final_time_s,
                sos.event_count_by_comp_id_by_failure_mode_id[compId],
                sos.event_count_by_comp_id_by_fragility_mode_id[compId],
                sos.time_by_comp_id_by_failure_mode_id_s[compId],
                sos.time_by_comp_id_by_fragility_mode_id_s[compId]);
            sos.availability_by_comp_id_s[compId] =
                TimeState_CalcAvailability_s(relSchByCompId[compId], m.final_time_s);
        }
        else
        {
            // NOTE: if there is no reliability schedule,
            // availability is 100% and there are no failure times or events
            // to count/sum.
            sos.availability_by_comp_id_s[compId] = m.final_time_s;
        }
    }
    // TODO: extract this into a new function
    std::vector<std::string> flowTypeNames;
    flowTypeNames.reserve(sos.flow_type_stats.size());
    for (auto const& fts : sos.flow_type_stats)
    {
        flowTypeNames.push_back(flowDict.flow_type[fts.flow_type_id]);
    }
    std::vector<size_t> flowTypeNames_idx(flowTypeNames.size());
    std::iota(flowTypeNames_idx.begin(), flowTypeNames_idx.end(), 0);
    std::sort(flowTypeNames_idx.begin(),
              flowTypeNames_idx.end(),
              [&](size_t a, size_t b) -> bool { return flowTypeNames[a] < flowTypeNames[b]; });
    std::vector<StatsByFlowType> newFlowTypeStats;
    newFlowTypeStats.reserve(sos.flow_type_stats.size());
    for (size_t ftn_idx : flowTypeNames_idx)
    {
        newFlowTypeStats.push_back(std::move(sos.flow_type_stats[ftn_idx]));
    }
    sos.flow_type_stats = std::move(newFlowTypeStats);
    // TODO: extract common parts out to function
    std::vector<std::string> loadFlowTypeNames;
    loadFlowTypeNames.reserve(sos.load_and_flow_type_stats.size());
    for (auto const& lfts : sos.load_and_flow_type_stats)
    {
        std::string loadName = m.component.tag[lfts.component_id];
        std::string flowName = flowDict.flow_type[lfts.stats.flow_type_id];
        std::string sortTag = loadName + "/" + flowName;
        loadFlowTypeNames.push_back(std::move(sortTag));
    }
    std::vector<size_t> loadFlowTypeNames_idx(loadFlowTypeNames.size());
    std::iota(loadFlowTypeNames_idx.begin(), loadFlowTypeNames_idx.end(), 0);
    std::sort(loadFlowTypeNames_idx.begin(),
              loadFlowTypeNames_idx.end(),
              [&](size_t a, size_t b) -> bool
              { return loadFlowTypeNames[a] < loadFlowTypeNames[b]; });
    std::vector<StatsByLoadAndFlowType> newLoadAndFlowTypeStats;
    newLoadAndFlowTypeStats.reserve(sos.load_and_flow_type_stats.size());
    for (size_t lftn_idx : loadFlowTypeNames_idx)
    {
        newLoadAndFlowTypeStats.push_back(std::move(sos.load_and_flow_type_stats[lftn_idx]));
    }
    sos.load_and_flow_type_stats = std::move(newLoadAndFlowTypeStats);
    // TODO: extract common parts out to function
    std::vector<std::string> loadNotServedFlowTypeNames;
    loadNotServedFlowTypeNames.reserve(sos.load_and_flow_type_stats.size());
    for (LoadNotServedForComp const& lns : sos.load_not_served_for_components)
    {
        std::string loadName = m.component.tag[lns.component_id];
        std::string flowName = flowDict.flow_type[lns.flow_type_id];
        std::string sortTag = loadName + "/" + flowName;
        loadNotServedFlowTypeNames.push_back(std::move(sortTag));
    }
    std::vector<size_t> loadNotServedFlowTypeNames_idx(loadNotServedFlowTypeNames.size());
    std::iota(loadNotServedFlowTypeNames_idx.begin(), loadNotServedFlowTypeNames_idx.end(), 0);
    std::sort(loadNotServedFlowTypeNames_idx.begin(),
              loadNotServedFlowTypeNames_idx.end(),
              [&](size_t a, size_t b) -> bool
              { return loadNotServedFlowTypeNames[a] < loadNotServedFlowTypeNames[b]; });
    std::vector<LoadNotServedForComp> newLoadNotServedForComponents;
    newLoadNotServedForComponents.reserve(sos.load_not_served_for_components.size());
    for (size_t lns_idx : loadNotServedFlowTypeNames_idx)
    {
        newLoadNotServedForComponents.push_back(
            std::move(sos.load_not_served_for_components[lns_idx]));
    }
    sos.load_not_served_for_components = std::move(newLoadNotServedForComponents);
    return sos;
}

std::optional<TagAndPort> ParseTagAndPort(std::string const& s, std::string const& tableName)
{
    std::string tag = s.substr(0, s.find(":"));
    size_t opening = s.find("(");
    size_t closing = s.find(")");
    if (opening == std::string::npos || closing == std::string::npos)
    {
        std::cout << "[" << tableName << "] "
                  << "unable to parse connection string '" << s << "'" << std::endl;
        return {};
    }
    size_t count = closing - (opening + 1);
    std::string port = s.substr(opening + 1, count);
    TagAndPort tap {
        .tag = tag,
        .port = static_cast<size_t>(std::atoi(port.c_str())),
    };
    return tap;
}

Result ParseNetwork(FlowDict const& fd, Model& m, toml::table const& table)
{
    if (!table.contains("connections"))
    {
        std::cout << "[network] "
                  << "required key 'connections' missing" << std::endl;
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
                      << "'connections' at index " << i << " must be an array" << std::endl;
            return Result::Failure;
        }
        // TODO: std::vector<toml::value> itemAsArray = item.as_array();
        if (item.as_array().size() < 3)
        {
            std::cout << "[network] "
                      << "'connections' at index " << i << " must be an array of length >= 3"
                      << std::endl;
            return Result::Failure;
        }
        for (int idx = 0; idx < 3; ++idx)
        {
            if (!item.as_array()[idx].is_string())
            {
                std::cout << "[network] "
                          << "'connections' at index " << i << " and subindex " << idx
                          << " must be a string" << std::endl;
                return Result::Failure;
            }
        }
        std::string from = item.as_array()[0].as_string();
        std::optional<TagAndPort> maybeFromTap = ParseTagAndPort(from, "network");
        if (!maybeFromTap.has_value())
        {
            std::cout << "[network] "
                      << "unable to parse connection string at [" << i << "][0]" << std::endl;
            return Result::Failure;
        }
        TagAndPort fromTap = maybeFromTap.value();
        std::string to = item.as_array()[1].as_string();
        std::optional<TagAndPort> maybeToTap = ParseTagAndPort(to, "network");
        if (!maybeToTap.has_value())
        {
            std::cout << "[network] "
                      << "unable to parse connection string at [" << i << "][1]" << std::endl;
            return Result::Failure;
        }
        TagAndPort toTap = maybeToTap.value();
        std::string flow = item.as_array()[2].as_string();
        std::optional<size_t> maybeFlowTypeId = FlowDict_GetIdByTag(fd, flow);
        if (!maybeFlowTypeId.has_value())
        {
            std::cout << "[network] "
                      << "could not identify flow type '" << flow << "'" << std::endl;
            return Result::Failure;
        }
        size_t flowTypeId = maybeFlowTypeId.value();
        std::optional<size_t> maybeFromCompId = Model_FindCompIdByTag(m, fromTap.tag);
        if (!maybeFromCompId.has_value())
        {
            std::cout << "[network] "
                      << "could not find component id for tag '" << from << "'" << std::endl;
            return Result::Failure;
        }
        std::optional<size_t> maybeToCompId = Model_FindCompIdByTag(m, toTap.tag);
        if (!maybeToCompId.has_value())
        {
            std::cout << "[network] "
                      << "could not find component id for tag '" << to << "'" << std::endl;
            return Result::Failure;
        }
        size_t fromCompId = maybeFromCompId.value();
        size_t toCompId = maybeToCompId.value();
        if (fromTap.port >= m.component.outflow_type[fromCompId].size())
        {
            std::cout << "[network] "
                      << "port is unaddressable for "
                      << ToString(m.component.component_type[fromCompId]) << ": trying to address "
                      << fromTap.port << " but only " << m.component.outflow_type[fromCompId].size()
                      << " ports available" << std::endl;
            return Result::Failure;
        }
        if (m.component.outflow_type[fromCompId][fromTap.port] != flowTypeId)
        {
            std::ostringstream oss;
            oss << "mismatch of flow types: " << fromTap.tag
                << ":outflow=" << fd.flow_type[m.component.outflow_type[fromCompId][fromTap.port]]
                << "; connection: " << flow;
            write_error_message("network", oss.str());
            return Result::Failure;
        }
        if (toCompId >= m.component.inflow_type.size())
        {
            std::cout << "[network] toCompId overflows InflowTypes" << std::endl;
            return Result::Failure;
        }
        if (toTap.port >= m.component.inflow_type[toCompId].size())
        {
            if (toCompId >= m.component.component_type.size())
            {
                std::cout << "[network] component type not logged" << std::endl;
                return Result::Failure;
            }
            std::cout << "[network] port is unaddressable for "
                      << ToString(m.component.component_type[toCompId]) << ": trying to address "
                      << toTap.port << " but only " << m.component.inflow_type[toCompId].size()
                      << " ports available" << std::endl;
            return Result::Failure;
        }
        if (m.component.inflow_type[toCompId][toTap.port] != flowTypeId)
        {
            if (toCompId >= m.component.outflow_type.size())
            {
                std::cout << "[network] toCompId is beyond outflow types" << std::endl;
                return Result::Failure;
            }
            if (toTap.port >= m.component.outflow_type[toCompId].size())
            {
                std::cout << "[network] port is unaddressable"
                          << ":tag=" << fromTap.tag << "[" << fromTap.port << "] => " << toTap.tag
                          << "[" << toTap.port << "]:port=" << toTap.port
                          << ":availablePorts=" << m.component.outflow_type[toCompId].size()
                          << std::endl;
                return Result::Failure;
            }
            size_t typeId = m.component.outflow_type[toCompId][toTap.port];
            if (typeId >= fd.flow_type.size())
            {
                std::cout << "[network] port is unaddressable"
                          << ":port=" << toTap.port << ":flowTypeId=" << typeId
                          << ":availableFlowTypes=" << fd.flow_type.size() << std::endl;
                return Result::Failure;
            }
            std::cout << "[network] mismatch of flow types: " << toTap.tag
                      << ":inflow=" << fd.flow_type[m.component.outflow_type[toCompId][toTap.port]]
                      << "; connection: " << flow << std::endl;
            return Result::Failure;
        }
        Model_AddConnection(m, fromCompId, fromTap.port, toCompId, toTap.port, flowTypeId);
    }
    return Result::Success;
}

std::optional<size_t> Model_FindCompIdByTag(Model const& m, std::string const& tag)
{
    for (size_t i = 0; i < m.component.tag.size(); ++i)
    {
        if (m.component.tag[i] == tag)
        {
            return i;
        }
    }
    return {};
}

std::optional<size_t> FlowDict_GetIdByTag(FlowDict const& fd, std::string const& tag)
{
    for (size_t idx = 0; idx < fd.flow_type.size(); ++idx)
    {
        if (fd.flow_type[idx] == tag)
        {
            return idx;
        }
    }
    return {};
}

std::string ConnectionToString(ComponentDict const& cd, Connection const& c, bool compact)
{
    std::string fromTag = cd.tag[c.from_component_id];
    if (fromTag.empty() && c.from == ComponentType::waste_sink_type)
    {
        fromTag = "WASTE";
    }
    else if (fromTag.empty() && c.from == ComponentType::environment_source_type)
    {
        fromTag = "ENV";
    }
    std::string toTag = cd.tag[c.to_component_id];
    if (toTag.empty() && c.to == ComponentType::waste_sink_type)
    {
        toTag = "WASTE";
    }
    else if (toTag.empty() && c.to == ComponentType::environment_source_type)
    {
        toTag = "ENV";
    }
    std::ostringstream oss {};
    oss << fromTag << (compact ? "" : ("[" + std::to_string(c.from_component_id) + "]")) << ":OUT("
        << c.from_port << ")" << (compact ? "" : (": " + ToString(c.from))) << " => " << toTag
        << (compact ? "" : ("[" + std::to_string(c.to_component_id) + "]")) << ":IN(" << c.to_port
        << ")" << (compact ? "" : (": " + ToString(c.to)));
    return oss.str();
}

std::string
ConnectionToString(ComponentDict const& cd, FlowDict const& fd, Connection const& c, bool compact)
{
    std::ostringstream oss {};
    oss << ConnectionToString(cd, c, compact) << " [flow: " << fd.flow_type[c.flow_type_id] << "]";
    return oss.str();
}

std::string NodeConnectionToString(Model const& model,
                                   NodeConnection const& c,
                                   bool compact,
                                   bool aggregateGroups)
{
    if (!aggregateGroups)
    {
        return ConnectionToString(model.component, model.connection[c.connection_id], compact);
    }
    std::string fromTag = "";
    std::string toTag = "";
    std::string fromString = "";
    std::string toString = "";

    auto const& componentMap = model.component;
    if (c.from_component_id.index() == 0)
    {
        // component
        auto idx = std::get<0>(c.from_component_id).id;
        fromTag = componentMap.tag[idx];
        if (fromTag.empty() && c.from == ComponentType::waste_sink_type)
        {
            fromTag = "WASTE";
        }
        else if (fromTag.empty() && c.from == ComponentType::environment_source_type)
        {
            fromTag = "ENV";
        }
        fromString = std::to_string(idx);
    }
    else
    {
        // group
        fromTag = std::get<1>(c.from_component_id).id;
    }

    if (c.to_component_id.index() == 0)
    {
        auto idx = std::get<0>(c.to_component_id).id;
        toTag = componentMap.tag[idx];
        if (toTag.empty() && c.to == ComponentType::waste_sink_type)
        {
            toTag = "WASTE";
        }
        else if (toTag.empty() && c.to == ComponentType::environment_source_type)
        {
            toTag = "ENV";
        }
        toString = std::to_string(idx);
    }
    else
    {
        // group
        toTag = std::get<1>(c.to_component_id).id;
    }

    std::ostringstream oss {};
    oss << fromTag << (compact ? "" : ("[" + fromString + "]")) << ":OUT(" << c.from_port << ")";

    if (c.from_component_id.index() == 0)
    {
        oss << (compact ? "" : (": " + ToString(c.from)));
    }

    oss << " => " << toTag << (compact ? "" : ("[" + toString + "]")) << ":IN(" << c.to_port << ")";

    if (c.to_component_id.index() == 0)
    {
        oss << (compact ? "" : (": " + ToString(c.to)));
    }

    return oss.str();
}

std::string NodeConnectionToString(Model const& model,
                                   FlowDict const& fd,
                                   NodeConnection const& nodeConn,
                                   bool compact,
                                   bool aggregateGroups)
{
    std::ostringstream oss {};
    oss << NodeConnectionToString(model, nodeConn, compact, aggregateGroups)
        << " [flow: " << fd.flow_type[nodeConn.flow_type_id] << "]";
    return oss.str();
}

// TODO: extract connection printing
void Model_PrintConnections(Model const& m, FlowDict const& ft)
{
    for (size_t i = 0; i < m.connection.size(); ++i)
    {
        std::cout << i << ": " << ConnectionToString(m.component, ft, m.connection[i]) << std::endl;
    }
}

std::ostream& operator<<(std::ostream& os, Flow const& flow)
{
    os << "Flow{Requested_W=" << flow.requested_W << ", "
       << "Available_W=" << flow.available_W << ", "
       << "Actual_W=" << flow.actual_W << "}";
    return os;
}

double Interpolate1d(double x, double x0, double y0, double x1, double y1)
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

double LinearFragilityCurve_GetFailureFraction(LinearFragilityCurve lfc, double intensityLevel)
{
    return Interpolate1d(intensityLevel, lfc.lower_bound, 0.0, lfc.upper_bound, 1.0);
}

double TabularFragilityCurve_GetFailureFraction(TabularFragilityCurve tfc, double intensityLevel)
{
    size_t size = tfc.intensity.size();
    assert(size == tfc.failure_fraction.size());
    assert(size > 0);
    if (intensityLevel <= tfc.intensity[0])
    {
        return tfc.failure_fraction[0];
    }
    if (intensityLevel >= tfc.intensity[size - 1])
    {
        return tfc.failure_fraction[size - 1];
    }
    for (size_t i = 0; i < size; ++i)
    {
        if (intensityLevel == tfc.intensity[i])
        {
            return tfc.failure_fraction[i];
        }
        if ((i + 1) < size && intensityLevel > tfc.intensity[i] &&
            intensityLevel <= tfc.intensity[i + 1])
        {
            return Interpolate1d(intensityLevel,
                                 tfc.intensity[i],
                                 tfc.failure_fraction[i],
                                 tfc.intensity[i + 1],
                                 tfc.failure_fraction[i + 1]);
        }
    }
    return 0.0;
}

void ComponentDict_SetInitialAge(ComponentDict& cd, size_t id, double age_s)
{
    assert(id < cd.component_type.size());
    assert(cd.component_type.size() == cd.initial_age_s.size());
    cd.initial_age_s[id] = age_s;
}

void ComponentDict_SetReporting(ComponentDict& cd, size_t id, bool report)
{
    assert(id < cd.component_type.size());
    assert(cd.component_type.size() == cd.report.size());
    cd.report[id] = report;
}

void AddComponentToGroup(Model& model, size_t id, std::string group)
{
    // record group association of component
    model.component_to_group.insert({id, group});

    // insert component into group set
    auto& map = model.group_to_component;
    if (map.contains(group))
    {
        auto& components_in_group = map[group]; //
        components_in_group.insert(id);
    }
    else
    {
        map.insert({group, {id}});
    }
}
} // namespace erin
