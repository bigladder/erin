// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <ios>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>

#include "erin/component.h"
#include "erin/erin.h"
#include "erin/logging.h"
#include "erin/random.h"
#include "erin/simulation.h"
#include "erin/timestate.h"
#include "erin/toml.h"
#include "erin/units.h"
#include "erin/utils.h"
#include "erin/validation.h"

namespace erin
{
void initialize(Simulation& s)
{
    // NOTE: we register a 'null' flow. This allows users to 'opt-out'
    // of flow specification by passing empty strings. Effectively, this
    // allows any connections to occur which is nice for simple examples.
    register_flow(s, "");
}

size_t register_flow(Simulation& s, std::string const& flowTag)
{
    size_t id = s.flow_type_map.flow_type.size();
    for (size_t i = 0; i < id; ++i)
    {
        if (s.flow_type_map.flow_type[i] == flowTag)
        {
            return i;
        }
    }
    s.flow_type_map.flow_type.push_back(flowTag);
    return id;
}

size_t register_scenario(Simulation& s, std::string const& scenarioTag)
{
    return register_scenario(s.scenario_map, scenarioTag);
}

size_t register_intensity(Simulation& s, std::string const& tag)
{
    for (size_t i = 0; i < s.intensities.tag.size(); ++i)
    {
        if (s.intensities.tag[i] == tag)
        {
            return i;
        }
    }
    size_t id = s.intensities.tag.size();
    s.intensities.tag.push_back(tag);
    return id;
}

size_t register_intensity_level_for_scenario(Simulation& s,
                                                    size_t scenarioId,
                                                    size_t intensityId,
                                                    double intensityLevel)
{
    for (size_t i = 0; i < s.scenario_intensities.intensity_id.size(); ++i)
    {
        if (s.scenario_intensities.intensity_id[i] == intensityId &&
            s.scenario_intensities.scenario_id[i] == scenarioId)
        {
            s.scenario_intensities.intensity_level[i] = intensityLevel;
            return i;
        }
    }
    size_t id = s.scenario_intensities.intensity_id.size();
    s.scenario_intensities.scenario_id.push_back(scenarioId);
    s.scenario_intensities.intensity_id.push_back(intensityId);
    s.scenario_intensities.intensity_level.push_back(intensityLevel);
    return id;
}

size_t register_load_schedule(Simulation& s,
                                       std::string const& tag,
                                       std::vector<TimeAndAmount> const& loadSchedule)
{
    size_t id = s.load_map.tags.size();
    assert(s.load_map.tags.size() == s.load_map.loads.size());
    for (size_t i = 0; i < id; ++i)
    {
        if (s.load_map.tags[i] == tag)
        {
            s.load_map.loads[i].clear();
            s.load_map.loads[i] = loadSchedule;
            return i;
        }
    }
    s.load_map.tags.push_back(tag);
    s.load_map.loads.push_back(loadSchedule);
    return id;
}

std::optional<size_t> get_load_id_by_tag(Simulation const& s, std::string const& tag)
{
    for (size_t i = 0; i < s.load_map.tags.size(); ++i)
    {
        if (s.load_map.tags[i] == tag)
        {
            return i;
        }
    }
    return {};
}

void register_all_loads(Simulation& s, std::vector<Load> const& loads)
{
    s.load_map.tags.clear();
    s.load_map.loads.clear();
    auto numLoads = loads.size();
    s.load_map.tags.reserve(numLoads);
    s.load_map.loads.reserve(numLoads);
    for (size_t i = 0; i < numLoads; ++i)
    {
        s.load_map.tags.push_back(loads[i].tag);
        s.load_map.loads.push_back(loads[i].time_and_loads);
    }
}

void print_components(Simulation const& s)
{
    Model const& m = s.the_model;
    for (size_t compId = 0; compId < m.component.component_type.size(); ++compId)
    {
        assert(compId < m.component.outflow_type.size());
        assert(compId < m.component.inflow_type.size());
        assert(compId < m.component.component_type.size());
        assert(compId < m.component.tag.size());
        assert(compId < m.component.subtype_index.size());
        std::vector<size_t> const& outflowTypes = m.component.outflow_type[compId];
        std::vector<size_t> inflowTypes = m.component.inflow_type[compId];
        std::cout << compId << ": " << ToString(m.component.component_type[compId]);
        if (!m.component.tag[compId].empty())
        {
            std::cout << " -- " << m.component.tag[compId] << std::endl;
        }
        else
        {
            std::cout << std::endl;
        }
        for (size_t inportIdx = 0; inportIdx < inflowTypes.size(); ++inportIdx)
        {
            size_t inflowType = inflowTypes[inportIdx];
            if (inflowType < s.flow_type_map.flow_type.size() &&
                !s.flow_type_map.flow_type[inflowType].empty())
            {
                std::cout << "- inport " << inportIdx << ": "
                          << s.flow_type_map.flow_type[inflowType] << std::endl;
            }
        }
        for (size_t outportIdx = 0; outportIdx < outflowTypes.size(); ++outportIdx)
        {
            size_t outflowType = outflowTypes[outportIdx];
            if (outflowType < s.flow_type_map.flow_type.size() &&
                !s.flow_type_map.flow_type[outflowType].empty())
            {
                std::cout << "- outport " << outportIdx << ": "
                          << s.flow_type_map.flow_type[outflowType] << std::endl;
            }
        }
        std::cout << "- report? " << (m.component.report[compId] ? "true" : "false") << std::endl;
        if (m.component_to_group.contains(compId))
        {
            std::cout << "- group: " << m.component_to_group.at(compId) << std::endl;
        }
        size_t subtypeIdx = m.component.subtype_index[compId];
        switch (m.component.component_type[compId])
        {
        case ComponentType::schedule_based_load_type:
        {
            assert(subtypeIdx < m.scheduled_load.size());
            ScheduleBasedLoad const& sbl = m.scheduled_load[subtypeIdx];
            std::cout << "-- inflow connection: " << sbl.inflow_connection_id << std::endl;
            for (auto const& keyValue : sbl.scenario_id_to_load_id)
            {
                size_t scenarioIdx = keyValue.first;
                size_t loadIdx = keyValue.second;
                assert(scenarioIdx < s.scenario_map.tag.size());
                assert(loadIdx < s.load_map.tags.size());
                std::cout << "-- for scenario: " << s.scenario_map.tag[scenarioIdx]
                          << ", use load: " << s.load_map.tags[loadIdx] << std::endl;
            }
        }
        break;
        case ComponentType::constant_load_type:
        {
            assert(subtypeIdx < m.constant_load.size());
            ConstantLoad const& cl = m.constant_load[subtypeIdx];
            std::cout << "-- constant request: " << cl.load_W << " W" << std::endl;
            std::cout << "-- inflow connection: " << cl.inflow_connection_id << std::endl;
        }
        break;
        case ComponentType::schedule_based_source_type:
        {
            assert(subtypeIdx < m.scheduled_source.size());
            ScheduleBasedSource const& sbs = m.scheduled_source[subtypeIdx];
            for (auto const& keyValue : sbs.scenario_id_to_source_id)
            {
                size_t scenarioIdx = keyValue.first;
                size_t loadIdx = keyValue.second;
                assert(scenarioIdx < s.scenario_map.tag.size());
                assert(loadIdx < s.load_map.tags.size());
                std::cout << "-- for scenario: " << s.scenario_map.tag[scenarioIdx]
                          << ", use supply: " << s.load_map.tags[loadIdx] << std::endl;
            }
            std::cout << "-- max outflow (W): "
                      << (sbs.max_outflow_W == max_flow_W ? "unlimited"
                                                          : std::to_string(sbs.max_outflow_W))
                      << std::endl;

            std::cout << "-- outflow connection: " << sbs.outflow_connection_id << std::endl;
        }
        break;
        case ComponentType::constant_efficiency_converter_type:
        {
            assert(subtypeIdx < m.constant_efficiency_converter.size());
            ConstantEfficiencyConverter const& cec = m.constant_efficiency_converter[subtypeIdx];
            std::cout << "-- efficiency: " << cec.efficiency * 100.0 << "%" << std::endl;
            std::cout << "-- max outflow (W): "
                      << (cec.max_outflow_W == max_flow_W ? "unlimited"
                                                          : std::to_string(cec.max_outflow_W))
                      << std::endl;
            std::cout << "-- max lossflow (W): "
                      << (cec.max_lossflow_W == max_flow_W ? "unlimited"
                                                           : std::to_string(cec.max_lossflow_W))
                      << std::endl;
            std::cout << "-- inflow connection: " << cec.inflow_connection_id << std::endl;
            std::cout << "-- outflow connection: " << cec.outflow_connection_id << std::endl;
            std::cout << "-- lossflow connection: "
                      << (cec.lossflow_connection_id.has_value()
                              ? std::to_string(cec.lossflow_connection_id.value())
                              : "NA")
                      << std::endl;
            std::cout << "-- wasteflow connection: " << cec.wasteflow_connection_id << std::endl;
        }
        break;
        case ComponentType::variable_efficiency_converter_type:
        {
            assert(subtypeIdx < m.variable_efficiency_converter.size());
            VariableEfficiencyConverter const& vec = m.variable_efficiency_converter[subtypeIdx];
            std::cout << "-- efficiencies by load fraction:" << std::endl;
            auto maxOutflow_W = static_cast<double>(vec.max_outflow_W);
            for (size_t i = 0; i < vec.efficiencies.size(); ++i)
            {
                std::cout << fmt::format("  -- {:5.3f}",
                                         (vec.outflows_for_efficiency_W[i] / maxOutflow_W));
                std::cout << fmt::format(": {:5.2f}%", (vec.efficiencies[i] * 100.0)) << std::endl;
            }
            std::cout << "-- max outflow (W): "
                      << (vec.max_outflow_W == max_flow_W ? "unlimited"
                                                          : std::to_string(vec.max_outflow_W))
                      << std::endl;
            std::cout << "-- max lossflow (W): "
                      << (vec.max_lossflow_W == max_flow_W ? "unlimited"
                                                           : std::to_string(vec.max_lossflow_W))
                      << std::endl;
            std::cout << "-- inflow connection: " << vec.inflow_connection_id << std::endl;
            std::cout << "-- outflow connection: " << vec.outflow_connection_id << std::endl;
            std::cout << "-- lossflow connection: "
                      << (vec.lossflow_connection_id.has_value()
                              ? std::to_string(vec.lossflow_connection_id.value())
                              : "NA")
                      << std::endl;
            std::cout << "-- wasteflow connection: " << vec.wasteflow_connection_id << std::endl;
        }
        break;
        case ComponentType::mover_type:
        {
            assert(subtypeIdx < m.mover.size());
            Mover const& mov = m.mover[subtypeIdx];
            std::cout << "-- cop: " << mov.COP << std::endl;
            std::cout << "-- max outflow (W): "
                      << (mov.max_outflow_W == max_flow_W ? "unlimited"
                                                          : std::to_string(mov.max_outflow_W))
                      << std::endl;
            std::cout << "-- inflow connection: " << mov.inflow_connection_id << std::endl;
            std::cout << "-- outflow connection: " << mov.outflow_connection_id << std::endl;
            std::cout << "-- envflow connection: " << mov.in_from_env_connection_id << std::endl;
            std::cout << "-- wasteflow connection: " << mov.wasteflow_connection_id << std::endl;
        }
        break;
        case ComponentType::variable_efficiency_mover_type:
        {
            assert(subtypeIdx < m.variable_efficiency_mover.size());
            VariableEfficiencyMover const& mov = m.variable_efficiency_mover[subtypeIdx];
            std::cout << "-- cop by load fraction:" << std::endl;
            auto maxOutflow_W = static_cast<double>(mov.max_outflow_W);
            for (size_t i = 0; i < mov.COPs.size(); ++i)
            {
                std::cout << fmt::format(" -- {:5.3f}", (mov.outflows_for_COP_W[i] / maxOutflow_W));
                std::cout << fmt::format(": {:5.2f}", (mov.COPs[i])) << std::endl;
            }
            std::cout << "-- max outflow (W): "
                      << (mov.max_outflow_W == max_flow_W ? "unlimited"
                                                          : std::to_string(maxOutflow_W))
                      << std::endl;
            std::cout << "-- inflow connection: " << mov.inflow_connection_id << std::endl;
            std::cout << "-- outflow connection: " << mov.outflow_connection_id << std::endl;
            std::cout << "-- envflow connection: " << mov.in_from_env_connection_id << std::endl;
            std::cout << "-- wasteflow connection: " << mov.wasteflow_connection_id << std::endl;
        }
        break;
        case ComponentType::store_type:
        {
            assert(subtypeIdx < m.store.size());
            Store const& store = m.store[subtypeIdx];
            std::cout << "-- capacity (J): " << store.capacity_J << std::endl;
            std::cout << "-- initial SOC: "
                      << (static_cast<double>(store.initial_storage_J) /
                          static_cast<double>(store.capacity_J))
                      << std::endl;
            std::cout << "-- initial capacity (J): " << store.initial_storage_J << std::endl;
            std::cout << "-- SOC to start charging: "
                      << (static_cast<double>(store.charge_amount_J) /
                          static_cast<double>(store.capacity_J))
                      << std::endl;
            std::cout << "-- max charge rate (W): " << store.max_charge_rate_W << std::endl;
            std::cout << "-- max discharge rate (W): " << store.max_discharge_rate_W << std::endl;
            std::cout << "-- max outflow (W): "
                      << (store.max_outflow_W == max_flow_W ? "unlimited"
                                                            : std::to_string(store.max_outflow_W))
                      << std::endl;
            std::cout << "-- roundtrip efficiency: " << store.roundtrip_efficiency * 100.0 << "%"
                      << std::endl;
            std::cout << "-- inflow connection: "
                      << (store.inflow_connection_id.has_value()
                              ? std::to_string(store.inflow_connection_id.value())
                              : "NA")
                      << std::endl;
            std::cout << "-- outflow connection: " << store.outflow_connection_id << std::endl;
        }
        break;
        case ComponentType::pass_through_type:
        {
            assert(subtypeIdx < m.pass_through.size());
            PassThrough const& pt = m.pass_through[subtypeIdx];
            std::cout << "-- max outflow (W): "
                      << (pt.max_outflow_W == max_flow_W ? "unlimited"
                                                         : std::to_string(pt.max_outflow_W))
                      << std::endl;
            std::cout << "-- inflow connection: " << pt.inflow_connection_id << std::endl;
            std::cout << "-- outflow connection: " << pt.outflow_connection_id << std::endl;
        }
        break;
        case ComponentType::mux_type:
        {
            assert(subtypeIdx < m.mux.size());
            Mux const& mux = m.mux[subtypeIdx];
            assert(mux.inflow_connection_ids.size() == mux.number_of_inports);
            assert(mux.outflow_connection_ids.size() == mux.number_of_outports);
            for (size_t i = 0; i < mux.number_of_inports; ++i)
            {
                std::cout << "-- inflow connection " << i << ": " << mux.inflow_connection_ids[i]
                          << std::endl;
            }
            for (size_t i = 0; i < mux.number_of_outports; ++i)
            {
                std::cout << "-- outflow connection " << i << ": " << mux.outflow_connection_ids[i]
                          << std::endl;
            }
        }
        break;
        case ComponentType::constant_source_type:
        {
            assert(subtypeIdx < m.constant_source.size());
            ConstantSource const& cs = m.constant_source[subtypeIdx];
            std::cout << "-- outflow connection: " << cs.outflow_connection_id << std::endl;
        }
        break;
        case ComponentType::switch_type:
        {
            assert(subtypeIdx < m.transfer_switch.size());
            Switch const& sw = m.transfer_switch[subtypeIdx];
            std::cout << "-- primary inflow connection: " << sw.inflow_connection_id_primary
                      << std::endl;
            std::cout << "-- secondary inflow connection: " << sw.inflow_connection_id_secondary
                      << std::endl;
            std::cout << "-- outflow connection: " << sw.outflow_connection_id << std::endl;
        }
        break;
        default:
        {
        }
        break;
        }
        for (size_t compFailModeIdx = 0;
             compFailModeIdx < s.component_failure_modes.component_id.size();
             ++compFailModeIdx)
        {
            if (s.component_failure_modes.component_id[compFailModeIdx] == compId)
            {
                size_t fmId = s.component_failure_modes.failure_mode_id[compFailModeIdx];
                std::cout << "-- failure-mode: " << s.failure_modes.tag[fmId] << "[" << fmId << "]"
                          << std::endl;
            }
        }
        for (size_t compFragIdx = 0; compFragIdx < s.component_fragilities.component_id.size();
             ++compFragIdx)
        {
            if (s.component_fragilities.component_id[compFragIdx] == compId)
            {
                size_t fmId = s.component_fragilities.fragility_mode_id[compFragIdx];
                std::cout << "-- fragility mode: " << s.fragility_modes.tag[fmId] << "[" << fmId
                          << "]" << std::endl;
            }
        }
    }
}

void print_fragility_curves(Simulation const& s)
{
    assert(s.fragility_curves.curve_id.size() == s.fragility_curves.curve_type.size());
    assert(s.fragility_curves.curve_id.size() == s.fragility_curves.tag.size());
    for (size_t i = 0; i < s.fragility_curves.curve_id.size(); ++i)
    {
        std::cout << i << ": " << fragility_curve_type_to_tag(s.fragility_curves.curve_type[i])
                  << " -- " << s.fragility_curves.tag[i] << std::endl;
        size_t idx = s.fragility_curves.curve_id[i];
        switch (s.fragility_curves.curve_type[i])
        {
        case (FragilityCurveType::linear):
        {
            std::cout << "-- lower bound: " << s.linear_fragility_curves[idx].lower_bound
                      << std::endl;
            std::cout << "-- upper bound: " << s.linear_fragility_curves[idx].upper_bound
                      << std::endl;
            size_t intensityId = s.linear_fragility_curves[idx].vulnerability_id;
            std::cout << "-- vulnerable to: " << s.intensities.tag[intensityId] << "["
                      << intensityId << "]" << std::endl;
        }
        break;
        case (FragilityCurveType::tabular):
        {
            size_t size = s.tabular_fragility_curves[idx].intensity.size();
            size_t intensityId = s.tabular_fragility_curves[idx].vulnerability_id;
            if (size > 0)
            {
                std::cout << "-- intensity from " << s.tabular_fragility_curves[idx].intensity[0]
                          << " to " << s.tabular_fragility_curves[idx].intensity[size - 1]
                          << std::endl;
                std::cout << "-- vulnerable to: " << s.intensities.tag[intensityId] << "["
                          << intensityId << "]" << std::endl;
            }
        }
        break;
        default:
        {
            std::cout << "unhandled fragility curve type" << std::endl;
            std::exit(1);
        }
        break;
        }
    }
}

void print_failure_modes(Simulation const& s)
{
    for (size_t i = 0; i < s.failure_modes.tag.size(); ++i)
    {
        auto maybeFailureDist =
            s.the_model.dist_sys.get_dist_by_id(s.failure_modes.failure_distribution_id[i]);
        auto maybeRepairDist =
            s.the_model.dist_sys.get_dist_by_id(s.failure_modes.repair_distribution_id[i]);
        std::cout << i << ": " << s.failure_modes.tag[i] << std::endl;
        if (maybeFailureDist.has_value())
        {
            Distribution const& failureDist = maybeFailureDist.value();
            std::cout << "-- failure distribution: " << failureDist.Tag << ", "
                      << dist_type_to_tag(failureDist.Type) << "["
                      << s.failure_modes.failure_distribution_id[i] << "]" << std::endl;
        }
        else
        {
            std::cout << "-- ERROR! Problem finding failure distribution "
                      << " with id = " << s.failure_modes.failure_distribution_id[i] << std::endl;
        }
        if (maybeRepairDist.has_value())
        {
            Distribution const& repairDist = maybeRepairDist.value();
            std::cout << "-- repair distribution: " << repairDist.Tag << ", "
                      << dist_type_to_tag(repairDist.Type) << "["
                      << s.failure_modes.repair_distribution_id[i] << "]" << std::endl;
        }
        else
        {
            std::cout << "-- ERROR! Problem finding repair distribution "
                      << " with id = " << s.failure_modes.repair_distribution_id[i] << std::endl;
        }
    }
}

void print_component_failure_modes(Simulation const& s)
{
    for (size_t i = 0; i < s.component_failure_modes.component_id.size(); ++i)
    {
        size_t compId = s.component_failure_modes.component_id[i];
        size_t fmId = s.component_failure_modes.failure_mode_id[i];
        std::cout << "[" << i << "]: component=" << s.the_model.component.tag[compId] << "["
                  << compId << "]; failure mode=" << s.failure_modes.tag[fmId] << "[" << fmId << "]"
                  << std::endl;
    }
}

void print_fragility_modes(Simulation const& s)
{
    for (size_t i = 0; i < s.fragility_modes.tag.size(); ++i)
    {
        std::cout << i << ": " << s.fragility_modes.tag[i] << std::endl;
        std::cout << "-- fragility curve: "
                  << s.fragility_curves.tag[s.fragility_modes.fragility_curve_id[i]] << "["
                  << s.fragility_modes.fragility_curve_id[i] << "]" << std::endl;
        if (s.fragility_modes.repair_distribution_id[i].has_value())
        {
            std::optional<Distribution> maybeDist = s.the_model.dist_sys.get_dist_by_id(
                s.fragility_modes.repair_distribution_id[i].value());
            if (maybeDist.has_value())
            {
                Distribution const& d = maybeDist.value();
                std::cout << "-- repair dist: " << d.Tag << "["
                          << s.fragility_modes.repair_distribution_id[i].value() << "]"
                          << std::endl;
            }
        }
    }
}

void print_component_fragility_modes(Simulation const& s)
{
    for (size_t i = 0; i < s.component_fragilities.component_id.size(); ++i)
    {
        size_t compId = s.component_fragilities.component_id[i];
        size_t fmId = s.component_fragilities.fragility_mode_id[i];
        std::cout << "[" << i << "]: component=" << s.the_model.component.tag[compId] << "["
                  << s.component_fragilities.component_id[i]
                  << "]; fragility mode=" << s.fragility_modes.tag[fmId] << "[" << fmId << "]"
                  << std::endl;
    }
}

void print_scenarios(Simulation const& s)
{
    for (size_t i = 0; i < s.scenario_map.tag.size(); ++i)
    {
        std::cout << i << ": " << s.scenario_map.tag[i] << std::endl;
        std::cout << "- duration: " << s.scenario_map.duration[i] << " "
                  << time_unit_to_tag(s.scenario_map.time_unit[i]) << std::endl;
        std::cout << "- offset: "
                  << time_in_seconds_to_desired_unit(s.scenario_map.time_offset_in_seconds[i],
                                                     TimeUnit::hour)
                  << " " << time_unit_to_tag(TimeUnit::hour) << std::endl;
        auto maybeDist =
            s.the_model.dist_sys.get_dist_by_id(s.scenario_map.occurrence_distribution_id[i]);
        if (maybeDist.has_value())
        {
            Distribution const& d = maybeDist.value();
            std::cout << "- occurrence distribution: " << dist_type_to_tag(d.Type) << "["
                      << s.scenario_map.occurrence_distribution_id[i] << "] -- " << d.Tag
                      << std::endl;
        }
        std::cout << "- max occurrences: ";
        if (s.scenario_map.max_occurrence[i].has_value())
        {
            std::cout << s.scenario_map.max_occurrence[i].value() << std::endl;
        }
        else
        {
            std::cout << "no limit" << std::endl;
        }
        bool printedHeader = false;
        for (size_t siIdx = 0; siIdx < s.scenario_intensities.intensity_id.size(); ++siIdx)
        {
            if (s.scenario_intensities.scenario_id[siIdx] == i)
            {
                if (!printedHeader)
                {
                    std::cout << "- intensities:" << std::endl;
                    printedHeader = true;
                }
                auto intId = s.scenario_intensities.intensity_id[siIdx];
                auto const& intTag = s.intensities.tag[intId];
                std::cout << "-- " << intTag << "[" << intId
                          << "]: " << s.scenario_intensities.intensity_level[siIdx] << std::endl;
            }
        }
    }
}

void print_loads(Simulation const& s)
{
    for (size_t i = 0; i < s.load_map.tags.size(); ++i)
    {
        std::cout << i << ": " << s.load_map.tags[i] << std::endl;
        std::cout << "- load entries: " << s.load_map.loads[i].size() << std::endl;
        if (!s.load_map.loads[i].empty())
        {
            // TODO: add time units
            std::cout << "- initial time: " << s.load_map.loads[i][0].Time_s << std::endl;
            // TODO: add time units
            std::cout << "- final time  : "
                      << s.load_map.loads[i][s.load_map.loads[i].size() - 1].Time_s << std::endl;
            // TODO: add max rate
            // TODO: add min rate
            // TODO: add average rate
        }
    }
    /*
    std::cout << "Loads:" << std::endl;
    for (size_t i = 0; i < loads.size(); ++i)
    {
        std::cout << i << ": " << loads[i] << std::endl;
    }
    */
}

size_t scenario_count(Simulation const& s) { return s.scenario_map.tag.size(); }

Result parse_simulation_info(Simulation& s,
                                      toml::value const& v,
                                      ValidationInfo const& validationInfo,
                                      Log const& log)
{
    if (!v.contains("simulation_info"))
    {
        Log_error(log, "simulation_info", "Required section [simulation_info] not found");
        return Result::failure;
    }
    toml::value const& simInfoValue = v.at("simulation_info");
    if (!simInfoValue.is_table())
    {
        Log_error(log, "simulation_info", "Required section [simulation_info] is not a table");
        return Result::failure;
    }
    toml::table const& simInfoTable = simInfoValue.as_table();
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::unordered_map<std::string, InputValue> inputs = TOMLTable_parse_with_validation(
        simInfoTable, validationInfo, "simulation_info", errors, warnings);
    if (!warnings.empty())
    {
        for (auto const& w : warnings)
        {
            Log_warning(log, w);
        }
    }
    if (!errors.empty())
    {
        for (auto const& err : errors)
        {
            Log_error(log, err);
        }
        return Result::failure;
    }
    auto maybeSimInfo = ParseSimulationInfo(inputs);
    if (!maybeSimInfo.has_value())
    {
        return Result::failure;
    }
    s.info = std::move(maybeSimInfo.value());
    return Result::success;
}

Result Simulation_ParseLoads(Simulation& s,
                             toml::value const& v,
                             ValidationInfo const& explicitValidation,
                             ValidationInfo const& fileValidation,
                             Log const& log)
{
    toml::value const& loadTable = v.at("loads");
    auto maybeLoads = parse_loads(loadTable.as_table(), explicitValidation, fileValidation, log);
    if (!maybeLoads.has_value())
    {
        return Result::failure;
    }
    std::vector<Load> loads = std::move(maybeLoads.value());
    register_all_loads(s, loads);
    return Result::success;
}

// TODO: change this to a std::optional<size_t> GetFragilityCurveByTag()
// if it returns !*.has_value(), register with the bogus data explicitly.
size_t register_fragility_curve(Simulation& s, std::string const& tag)
{
    return register_fragility_curve(s, tag, FragilityCurveType::linear, 0);
}

size_t register_fragility_curve(Simulation& s,
                                         std::string const& tag,
                                         FragilityCurveType curveType,
                                         size_t curveIdx)
{
    for (size_t i = 0; i < s.fragility_curves.tag.size(); ++i)
    {
        if (s.fragility_curves.tag[i] == tag)
        {
            s.fragility_curves.curve_id[i] = curveIdx;
            s.fragility_curves.curve_type[i] = curveType;
            return i;
        }
    }
    size_t id = s.fragility_curves.tag.size();
    s.fragility_curves.tag.push_back(tag);
    s.fragility_curves.curve_id.push_back(curveIdx);
    s.fragility_curves.curve_type.push_back(curveType);
    return id;
}

size_t register_failure_mode(Simulation& s,
                                      std::string const& tag,
                                      size_t failureId,
                                      size_t repairId)
{
    size_t size = s.failure_modes.tag.size();
    for (size_t i = 0; i < size; ++i)
    {
        if (s.failure_modes.tag[i] == tag)
        {
            s.failure_modes.failure_distribution_id[i] = failureId;
            s.failure_modes.repair_distribution_id[i] = repairId;
            return i;
        }
    }
    size_t result = size;
    s.failure_modes.tag.push_back(tag);
    s.failure_modes.failure_distribution_id.push_back(failureId);
    s.failure_modes.repair_distribution_id.push_back(repairId);
    return result;
}

size_t register_fragility_mode(Simulation& s,
                                        std::string const& tag,
                                        size_t fragilityCurveId,
                                        std::optional<size_t> maybeRepairDistId)
{
    size_t size = s.fragility_modes.tag.size();
    for (size_t i = 0; i < size; ++i)
    {
        if (s.fragility_modes.tag[i] == tag)
        {
            s.fragility_modes.fragility_curve_id[i] = fragilityCurveId;
            s.fragility_modes.repair_distribution_id[i] = maybeRepairDistId;
            return i;
        }
    }
    size_t result = size;
    s.fragility_modes.tag.push_back(tag);
    s.fragility_modes.fragility_curve_id.push_back(fragilityCurveId);
    s.fragility_modes.repair_distribution_id.push_back(maybeRepairDistId);
    return result;
}

std::optional<size_t>
Parse_VulnerableTo(Simulation const& s, toml::table const& fcData, std::string const& tableFullName)
{
    if (!fcData.contains("vulnerable_to"))
    {
        write_error_message(tableFullName, "missing required field 'vulnerable_to'");
        return {};
    }
    if (!fcData.at("vulnerable_to").is_string())
    {
        write_error_message(tableFullName, "field 'vulnerable_to' not a string");
        return {};
    }
    std::string const& vulnerStr = fcData.at("vulnerable_to").as_string();
    std::optional<size_t> maybeIntId = get_intensity_id_by_tag(s.intensities, vulnerStr);
    if (!maybeIntId.has_value())
    {
        write_error_message(tableFullName,
                            "could not find referenced intensity '" + vulnerStr +
                                "' for 'vulnerable_to'");
        return {};
    }
    return maybeIntId;
}

Result Simulation_ParseLinearFragilityCurve(Simulation& s,
                                            std::string const& fcName,
                                            std::string const& tableFullName,
                                            toml::table const& fcData)
{
    if (!fcData.contains("lower_bound"))
    {
        std::cout << "[" << tableFullName << "] "
                  << "missing required field 'lower_bound'" << std::endl;
        return Result::failure;
    }
    if (!(fcData.at("lower_bound").is_floating() || fcData.at("lower_bound").is_integer()))
    {
        std::cout << "[" << tableFullName << "] "
                  << "field 'lower_bound' not a number" << std::endl;
        return Result::failure;
    }
    std::optional<double> maybeLowerBound =
        TOMLTable_parse_double(fcData, "lower_bound", tableFullName);
    if (!maybeLowerBound.has_value())
    {
        std::cout << "[" << tableFullName << "] "
                  << "field 'lower_bound' has no value" << std::endl;
        return Result::failure;
    }
    double lowerBound = maybeLowerBound.value();
    if (!fcData.contains("upper_bound"))
    {
        std::cout << "[" << tableFullName << "] "
                  << "missing required field 'upper_bound'" << std::endl;
        return Result::failure;
    }
    if (!(fcData.at("upper_bound").is_floating() || fcData.at("upper_bound").is_integer()))
    {
        std::cout << "[" << tableFullName << "] "
                  << "field 'upper_bound' not a number" << std::endl;
        return Result::failure;
    }
    std::optional<double> maybeUpperBound =
        TOMLTable_parse_double(fcData, "upper_bound", tableFullName);
    if (!maybeUpperBound.has_value())
    {
        std::cout << "[" << tableFullName << "] "
                  << "field 'upper_bound' has no value" << std::endl;
        return Result::failure;
    }
    double upperBound = maybeUpperBound.value();
    std::optional<size_t> maybeIntId = Parse_VulnerableTo(s, fcData, tableFullName);
    if (!maybeIntId.has_value())
    {
        return Result::failure;
    }
    size_t intensityId = maybeIntId.value();
    LinearFragilityCurve lfc {};
    lfc.lower_bound = lowerBound;
    lfc.upper_bound = upperBound;
    lfc.vulnerability_id = intensityId;
    size_t idx = s.linear_fragility_curves.size();
    s.linear_fragility_curves.push_back(std::move(lfc));
    register_fragility_curve(s, fcName, FragilityCurveType::linear, idx);
    return Result::success;
}

Result Simulation_ParseFragilityCurves(Simulation& s, toml::value const& v, Log const& log)
{
    if (v.contains("fragility_curve"))
    {
        if (!v.at("fragility_curve").is_table())
        {
            Log_error(log, "fragility_curve", "must be a table");
            return Result::failure;
        }
        for (auto const& pair : v.at("fragility_curve").as_table())
        {
            std::string const& fcName = pair.first;
            std::string tableFullName = "fragility_curve." + fcName;
            if (!pair.second.is_table())
            {
                Log_error(log, tableFullName, "not a table");
                return Result::failure;
            }
            toml::table const& fcData = pair.second.as_table();
            if (!fcData.contains("type"))
            {
                Log_error(log, tableFullName, "does not contain required value 'type'");
                return Result::failure;
            }
            std::string const& typeStr = fcData.at("type").as_string();
            std::optional<FragilityCurveType> maybeFct = tag_to_fragility_curve_type(typeStr);
            if (!maybeFct.has_value())
            {
                Log_error(log, tableFullName, "could not interpret type as string");
                return Result::failure;
            }
            FragilityCurveType fct = maybeFct.value();
            switch (fct)
            {
            case (FragilityCurveType::linear):
            {
                if (Simulation_ParseLinearFragilityCurve(s, fcName, tableFullName, fcData) ==
                    Result::failure)
                {
                    return Result::failure;
                }
            }
            break;
            case (FragilityCurveType::tabular):
            {
                std::optional<size_t> maybeIntId = Parse_VulnerableTo(s, fcData, tableFullName);
                if (!maybeIntId.has_value())
                {
                    return Result::failure;
                }
                size_t intensityId = maybeIntId.value();
                auto maybePairs = TOMLTable_parse_array_of_pairs_of_double(
                    fcData, "intensity_failure_pairs", tableFullName);
                if (!maybePairs.has_value())
                {
                    return Result::failure;
                }
                PairsVector pv = maybePairs.value();
                TabularFragilityCurve tfc {};
                tfc.vulnerability_id = intensityId;
                tfc.intensity = std::move(pv.firsts);
                tfc.failure_fraction = std::move(pv.seconds);
                size_t subtypeIdx = s.tabular_fragility_curves.size();
                s.tabular_fragility_curves.push_back(std::move(tfc));
                register_fragility_curve(
                    s, fcName, FragilityCurveType::tabular, subtypeIdx);
            }
            break;
            default:
            {
                Log_error(log, tableFullName, "unhandled fragility curve type '" + typeStr + "'");
                std::exit(1);
            }
            break;
            }
        }
    }
    return Result::success;
}

bool Simulation_IsFailureModeNameUnique(Simulation& s, std::string const& name)
{
    for (std::string const& tag : s.failure_modes.tag)
    {
        if (tag == name)
        {
            return false;
        }
    }
    return true;
}

bool Simulation_IsFragilityModeNameUnique(Simulation& s, std::string const& name)
{
    for (size_t i = 0; i < s.fragility_modes.tag.size(); ++i)
    {
        if (s.fragility_modes.tag[i] == name)
        {
            return false;
        }
    }
    return true;
}

bool Simulation_IsFailureNameUnique(Simulation& s, std::string const& name)
{
    return Simulation_IsFailureModeNameUnique(s, name) &&
           Simulation_IsFragilityModeNameUnique(s, name);
}

Result Simulation_ParseFailureModes(Simulation& s, toml::value const& v, Log const& log)
{
    if (v.contains("failure_mode"))
    {
        if (!v.at("failure_mode").is_table())
        {
            Log_error(log, "failure_mode", "failure_mode section must be a table");
            return Result::failure;
        }
        toml::table const& fmTable = v.at("failure_mode").as_table();
        for (auto const& pair : fmTable)
        {
            std::string const& fmName = pair.first;
            std::string const fullName = "failue_mode." + fmName;
            if (!Simulation_IsFragilityModeNameUnique(s, fmName))
            {
                Log_error(log,
                          fmName,
                          "failure mode name must be unique within both "
                          "failure_mode and fragility_mode names");
                return Result::failure;
            }
            if (!pair.second.is_table())
            {
                Log_error(log, fullName, "value must be a table");
                return Result::failure;
            }
            toml::table const& fmValueTable = pair.second.as_table();
            if (!fmValueTable.contains("failure_dist"))
            {
                Log_error(log, fullName, "missing required field 'failure_dist'");
                return Result::failure;
            }
            auto maybeFailureDistTag =
                TOMLTable_parse_string(fmValueTable, "failure_dist", fullName);
            if (!maybeFailureDistTag.has_value())
            {
                Log_error(log, fullName, "could not parse 'failure_dist' as string");
                return Result::failure;
            }
            std::string const& failureDistTag = maybeFailureDistTag.value();
            if (!fmValueTable.contains("repair_dist"))
            {
                Log_error(log, fullName, "missing required field 'repair_dist'");
                return Result::failure;
            }
            auto maybeRepairDistTag = TOMLTable_parse_string(fmValueTable, "repair_dist", fullName);
            if (!maybeRepairDistTag.has_value())
            {
                Log_error(log, fullName, "could not parse 'repair_dist' as string");
                return Result::failure;
            }
            std::string const& repairDistTag = maybeRepairDistTag.value();
            size_t failureId = s.the_model.dist_sys.lookup_dist_by_tag(failureDistTag);
            size_t repairId = s.the_model.dist_sys.lookup_dist_by_tag(repairDistTag);
            register_failure_mode(s, fmName, failureId, repairId);
        }
    }
    return Result::success;
}

Result Simulation_ParseFragilityModes(Simulation& s, toml::value const& v, Log const& log)
{
    if (v.contains("fragility_mode"))
    {
        if (!v.at("fragility_mode").is_table())
        {
            Log_error(log, "fragility_mode must be a table");
            return Result::failure;
        }
        toml::table const& fmTable = v.at("fragility_mode").as_table();
        for (auto const& pair : fmTable)
        {
            std::string const& fmName = pair.first;
            std::string const fullName = "fragility_mode." + fmName;
            if (!Simulation_IsFailureModeNameUnique(s, fmName))
            {
                Log_error(log,
                          fullName,
                          "fragility mode name must be unique within both "
                          "failure_mode and fragility_mode names");
                return Result::failure;
            }
            if (!pair.second.is_table())
            {
                Log_error(log, fullName, "fragility_mode section must be a table");
                return Result::failure;
            }
            toml::table const& fmValueTable = pair.second.as_table();
            if (!fmValueTable.contains("fragility_curve"))
            {
                Log_error(log, fullName, "missing required field 'fragility_curve'");
                return Result::failure;
            }
            if (!fmValueTable.at("fragility_curve").is_string())
            {
                Log_error(log, fullName, "'fragility_curve' field must be a string");
                return Result::failure;
            }
            std::string const& fcTag = fmValueTable.at("fragility_curve").as_string();
            size_t fcId = register_fragility_curve(s, fcTag);
            std::optional<size_t> maybeRepairDistId = {};
            if (fmValueTable.contains("repair_dist"))
            {
                if (!fmValueTable.at("repair_dist").is_string())
                {
                    Log_error(log, fullName, "field 'repair_dist' must be a string");
                    return Result::failure;
                }
                std::string const& repairDistTag = fmValueTable.at("repair_dist").as_string();
                maybeRepairDistId = s.the_model.dist_sys.lookup_dist_by_tag(repairDistTag);
            }
            register_fragility_mode(s, fmName, fcId, maybeRepairDistId);
        }
    }
    return Result::success;
}

Result Simulation_ParseComponents(Simulation& s,
                                  toml::value const& v,
                                  ComponentValidationMap const& compValidations,
                                  std::unordered_set<std::string> const& componentTagsInUse,
                                  Log const& log)
{
    if (v.contains("components") && v.at("components").is_table())
    {
        return parse_components(
            s, v.at("components").as_table(), compValidations, componentTagsInUse, log);
    }
    Log_error(log, "required field 'components' not found");
    return Result::failure;
}

Result Simulation_ParseDistributions(Simulation& s,
                                     toml::value const& v,
                                     DistributionValidationMap const& dvm,
                                     Log const& log)
{
    if (v.contains("dist") && v.at("dist").is_table())
    {
        // TODO: have ParseDistributions return a Result
        return ParseDistributions(s.the_model.dist_sys, v.at("dist").as_table(), dvm, log);
    }
    Log_error(log, "required field 'dist' not found");
    return Result::failure;
}

Result Simulation_ParseNetwork(Simulation& s, toml::value const& v, Log const& log)
{
    std::string const n = "network";
    if (v.contains(n) && v.at(n).is_table())
    {
        return ParseNetwork(s.flow_type_map, s.the_model, v.at(n).as_table());
    }
    Log_error(log, fmt::format("required field '{}' not found", n));
    return Result::failure;
}

Result Simulation_ParseScenarios(Simulation& s, toml::value const& v, Log const& log)
{
    if (v.contains("scenarios") && v.at("scenarios").is_table())
    {
        auto result =
            parse_scenarios(s.scenario_map, s.the_model.dist_sys, v.at("scenarios").as_table());
        if (result == Result::success)
        {
            for (auto const& pair : v.at("scenarios").as_table())
            {
                std::string const& scenarioName = pair.first;
                std::optional<size_t> maybeScenarioId =
                    get_scenario_by_tag(s.scenario_map, scenarioName);
                if (!maybeScenarioId.has_value())
                {
                    Log_error(log,
                              "scenarios",
                              fmt::format("could not find scenario id for '{}'", scenarioName));
                    return Result::failure;
                }
                size_t scenarioId = maybeScenarioId.value();
                std::string fullName = "scenarios." + scenarioName;
                if (!pair.second.is_table())
                {
                    Log_error(log, fullName, "must be a table");
                    return Result::failure;
                }
                toml::table const& data = pair.second.as_table();
                if (data.contains("intensity"))
                {
                    if (!data.at("intensity").is_table())
                    {
                        Log_error(log, fullName + ".intensity", "must be a table");
                        return Result::failure;
                    }
                    for (auto const& p : data.at("intensity").as_table())
                    {
                        std::string const& intensityTag = p.first;
                        if (!(p.second.is_integer() || p.second.is_floating()))
                        {
                            Log_error(
                                log, fullName + ".intensity." + intensityTag, "must be a number");
                            return Result::failure;
                        }
                        std::optional<double> maybeValue =
                            TOML_parse_numeric_value_as_double(p.second);
                        if (!maybeValue.has_value())
                        {
                            Log_error(
                                log, fullName + ".intensity." + intensityTag, "must be a number");
                            return Result::failure;
                        }
                        double value = maybeValue.value();
                        size_t intensityId = register_intensity(s, intensityTag);
                        register_intensity_level_for_scenario(
                            s, scenarioId, intensityId, value);
                    }
                }
            }
        }
        return result;
    }
    Log_error(log, "required field 'scenarios' not found or not a table");
    return Result::failure;
}

std::optional<Simulation>
Simulation_read_from_toml(toml::value const& v,
                          InputValidationMap const& validationInfo,
                          std::unordered_set<std::string> const& componentTagsInUse,
                          Log const& log)
{
    Simulation s = {};
    initialize(s);
    auto simInfoResult = parse_simulation_info(s, v, validationInfo.simulation_info, log);
    if (simInfoResult == Result::failure)
    {
        Log_error(log, "simulation_info", "problem parsing...");
        return {};
    }
    auto loadsResult = Simulation_ParseLoads(
        s, v, validationInfo.load_explicit, validationInfo.load_file_based, log);
    if (loadsResult == Result::failure)
    {
        Log_error(log, "loads", "problem parsing...");
        return {};
    }
    auto compResult =
        Simulation_ParseComponents(s, v, validationInfo.component, componentTagsInUse, log);
    if (compResult == Result::failure)
    {
        Log_error(log, "components", "problem parsing...");
        return {};
    }
    auto distResult = Simulation_ParseDistributions(s, v, validationInfo.distribution, log);
    if (distResult == Result::failure)
    {
        Log_error(log, "dist", "problem parsing...");
        return {};
    }
    if (Simulation_ParseFailureModes(s, v, log) == Result::failure)
    {
        Log_error(log, "failure_mode", "problem parsing...");
        return {};
    }
    if (Simulation_ParseFragilityModes(s, v, log) == Result::failure)
    {
        Log_error(log, "fragility_mode", "problem parsing...");
        return {};
    }
    if (Simulation_ParseNetwork(s, v, log) == Result::failure)
    {
        Log_error(log, "network", "problem parsing...");
        return {};
    }
    if (Simulation_ParseScenarios(s, v, log) == Result::failure)
    {
        Log_error(log, "scenarios", "problem parsing...");
        return {};
    }
    if (Simulation_ParseFragilityCurves(s, v, log) == Result::failure)
    {
        Log_error(log, "fragility_curve", "problem parsing...");
        return {};
    }
    return s;
}

static void Simulation_PrintGroups(Simulation const& s)
{
    if (s.the_model.group_to_component.size() == 0)
    {
        return;
    }
    std::vector<std::string> groupTags {};
    groupTags.reserve(s.the_model.group_to_component.size());
    for (auto const& p : s.the_model.group_to_component)
    {
        groupTags.push_back(p.first);
    }
    std::set<size_t> groupedComponents {};
    std::sort(groupTags.begin(), groupTags.end());
    for (std::string const& gTag : groupTags)
    {
        size_t count = s.the_model.group_to_component.at(gTag).size();
        std::cout << "- GROUP: " << gTag << " (count: " << count << ")" << std::endl;
        for (size_t compId : s.the_model.group_to_component.at(gTag))
        {
            groupedComponents.insert(compId);
            std::cout << "-- " << s.the_model.component.tag[compId] << std::endl;
        }
    }
    size_t numUngrouped = s.the_model.component.tag.size() - groupedComponents.size();
    std::cout << "- UNGROUPED (count: " << numUngrouped << ")" << std::endl;
    for (size_t compId = 0; compId < s.the_model.component.tag.size(); ++compId)
    {
        if (groupedComponents.contains(compId))
        {
            continue;
        }
        if (s.the_model.component.component_type[compId] == ComponentType::environment_source_type)
        {
            std::cout << "-- ENV[" << compId << "]" << std::endl;
        }
        else if (s.the_model.component.component_type[compId] == ComponentType::waste_sink_type)
        {
            std::cout << "-- WASTE[" << compId << "]" << std::endl;
        }
        else
        {
            std::cout << "-- " << s.the_model.component.tag[compId] << std::endl;
        }
    }
}

void Simulation_print(Simulation const& s)
{
    std::cout << "-----------------" << std::endl;
    std::cout << s.info << std::endl;
    std::cout << "\nLoads:" << std::endl;
    print_loads(s);
    std::cout << "\nComponents:" << std::endl;
    print_components(s);
    std::cout << "\nGroups:" << std::endl;
    Simulation_PrintGroups(s);
    std::cout << "\nDistributions:" << std::endl;
    s.the_model.dist_sys.print_distributions();
    std::cout << "\nFailure Modes:" << std::endl;
    print_failure_modes(s);
    std::cout << "\nComponent/Failure Modes:" << std::endl;
    print_component_failure_modes(s);
    std::cout << "\nFragility Curves:" << std::endl;
    print_fragility_curves(s);
    std::cout << "\nFragility Modes:" << std::endl;
    print_fragility_modes(s);
    std::cout << "\nComponent/Fragility Modes:" << std::endl;
    print_component_fragility_modes(s);
    std::cout << "\nConnections:" << std::endl;
    Model_PrintConnections(s.the_model, s.flow_type_map);
    std::cout << "\nScenarios:" << std::endl;
    print_scenarios(s);
    std::cout << "\nIntensities:" << std::endl;
    Simulation_PrintIntensities(s);
}

void Simulation_PrintIntensities(Simulation const& s)
{
    for (size_t i = 0; i < s.intensities.tag.size(); ++i)
    {
        std::cout << i << ": " << s.intensities.tag[i] << std::endl;
    }
}

void WriteEventFileHeader(std::ofstream& out,
                          Model const& model,
                          FlowDict const& fd,
                          std::vector<size_t> const& nodeConnOrder,
                          std::vector<size_t> const& storeOrder,
                          std::vector<size_t> const& compOrder,
                          TimeUnit outputTimeUnit,
                          std::vector<NodeConnection> const& nodeConnections,
                          bool aggregateGroups)
{
    ComponentDict const& compMap = model.component;
    out << "scenario id,"
        << "scenario start time (P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]),"
        << "elapsed ("
        << (outputTimeUnit == TimeUnit::hour ? "hours" : time_unit_to_tag(outputTimeUnit)) << ")";

    for (std::string const& prefix : std::vector<std::string> {"", "REQUEST:", "AVAILABLE:"})
    {
        for (auto& iOrdNodeConn : nodeConnOrder)
        {
            NodeConnection const& nodeConn = nodeConnections[iOrdNodeConn];
            out << "," << prefix
                << NodeConnectionToString(model, fd, nodeConn, true, aggregateGroups) << " (kW)";
        }
    }

    for (std::pair<std::string, std::string> const& prePostFix :
         std::vector<std::pair<std::string, std::string>> {{"Stored: ", " (kJ)"}, {"SOC: ", ""}})
    {
        for (size_t storeIdx : storeOrder)
        {
            for (size_t compId = 0; compId < compMap.tag.size(); ++compId)
            {
                if (compMap.component_type[compId] == ComponentType::store_type &&
                    compMap.subtype_index[compId] == storeIdx)
                {
                    std::string tag(compMap.tag[compId]);
                    if (aggregateGroups && model.component_to_group.contains(compId))
                    {
                        tag = model.component_to_group.at(compId) + "(" + tag + ")";
                    }
                    out << "," << prePostFix.first << tag << prePostFix.second;
                }
            }
        }
    }
    // op-state: <component-name>
    for (size_t compId : compOrder)
    {
        if (!model.component.tag[compId].empty())
        {
            std::string compName = model.component.tag[compId];
            if (aggregateGroups && model.component_to_group.contains(compId))
            {
                auto& group = model.component_to_group.at(compId);
                compName = group + "(" + compName + ")";
            }
            out << ",op-state: " << compName;
        }
    }
    out << std::endl;
}

std::vector<size_t> CalculateConnectionOrder(Simulation const& s)
{
    // TODO: need to enforce connections are unique
    size_t const numConns = s.the_model.connection.size();
    std::vector<size_t> result;
    std::vector<std::string> originalConnTags;
    std::vector<std::string> connTags;
    result.reserve(numConns);
    originalConnTags.reserve(numConns);
    connTags.reserve(numConns);
    for (auto const& conn : s.the_model.connection)
    {
        std::string connTag = ConnectionToString(s.the_model.component, conn, true);
        originalConnTags.push_back(connTag);
        connTags.push_back(connTag);
    }
    std::sort(connTags.begin(), connTags.end());
    for (auto const& connTag : connTags)
    {
        for (size_t connId = 0; connId < s.the_model.connection.size(); ++connId)
        {
            if (connTag == originalConnTags[connId])
            {
                result.push_back(connId);
                break;
            }
        }
    }
    assert(result.size() == numConns);
    return result;
}

std::vector<size_t> CalculateScenarioOrder(Simulation const& s)
{
    std::vector<size_t> result;
    std::vector<std::string> scenarioTags(s.scenario_map.tag);
    size_t numScenarios = s.scenario_map.tag.size();
    std::sort(scenarioTags.begin(), scenarioTags.end());
    result.reserve(numScenarios);
    for (std::string const& tag : scenarioTags)
    {
        for (size_t scenarioId = 0; scenarioId < s.scenario_map.tag.size(); ++scenarioId)
        {
            if (tag == s.scenario_map.tag[scenarioId])
            {
                result.push_back(scenarioId);
                break;
            }
        }
    }
    assert(result.size() == numScenarios);
    return result;
}

std::vector<size_t> CalculateComponentOrder(Simulation const& s)
{
    size_t const numComps = s.the_model.component.tag.size();
    std::vector<size_t> result;
    result.reserve(numComps);
    std::vector<std::string> compTags(s.the_model.component.tag);
    std::sort(compTags.begin(), compTags.end());
    for (auto const& t : compTags)
    {
        for (size_t compId = 0; compId < numComps; ++compId)
        {
            if (s.the_model.component.tag[compId] == t)
            {
                result.push_back(compId);
                break;
            }
        }
    }
    assert(result.size() == numComps);
    return result;
}

std::vector<size_t> RemoveNonReportingIds(std::vector<size_t> const& idOrder,
                                          std::unordered_set<size_t> const& idsToReport)
{
    std::vector<size_t> result {};
    result.reserve(idsToReport.size());
    for (size_t const& id : idOrder)
    {
        if (idsToReport.contains(id))
        {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<size_t> CalculateStoreOrder(Simulation const& s,
                                        std::unordered_set<size_t> const& compsToReport)
{
    std::vector<size_t> result {};
    std::vector<std::string> storeTags {};
    storeTags.reserve(s.the_model.store.size());
    size_t const numComps = s.the_model.component.component_type.size();
    size_t const numStores = s.the_model.store.size();
    std::vector<size_t> reportedStoreIdxs {};
    reportedStoreIdxs.reserve(s.the_model.store.size());
    for (size_t storeId = 0; storeId < numStores; ++storeId)
    {
        for (size_t compId = 0; compId < numComps; ++compId)
        {
            if (!compsToReport.contains(compId))
            {
                continue;
            }
            ComponentType type = s.the_model.component.component_type[compId];
            size_t idx = s.the_model.component.subtype_index[compId];
            if (type == ComponentType::store_type && idx == storeId)
            {
                reportedStoreIdxs.push_back(idx);
                storeTags.push_back(s.the_model.component.tag[compId]);
                break;
            }
        }
    }
    assert(storeTags.size() <= numStores);
    std::vector<std::string> originalStoreTags(storeTags);
    std::sort(storeTags.begin(), storeTags.end());
    for (auto const& tag : storeTags)
    {
        for (size_t i = 0; i < reportedStoreIdxs.size(); ++i)
        {
            if (tag == originalStoreTags[i])
            {
                result.push_back(reportedStoreIdxs[i]);
            }
        }
    }
    assert(result.size() <= numStores);
    return result;
}

std::vector<size_t> CalculateFailModeOrder(Simulation const& s)
{
    size_t const numFailModes = s.failure_modes.tag.size();
    std::vector<size_t> result;
    std::vector<std::string> failTags(s.failure_modes.tag);
    std::sort(failTags.begin(), failTags.end());
    for (auto const& t : failTags)
    {
        for (size_t i = 0; i < numFailModes; ++i)
        {
            if (s.failure_modes.tag[i] == t)
            {
                result.push_back(i);
                break;
            }
        }
    }
    assert(result.size() == numFailModes);
    return result;
}

std::vector<size_t> CalculateFragilModeOrder(Simulation const& s)
{
    size_t const numFragModes = s.fragility_modes.tag.size();
    std::vector<size_t> result;
    std::vector<std::string> fragTags(s.fragility_modes.tag);
    std::sort(fragTags.begin(), fragTags.end());
    for (auto const& t : fragTags)
    {
        for (size_t i = 0; i < numFragModes; ++i)
        {
            if (s.fragility_modes.tag[i] == t)
            {
                result.push_back(i);
                break;
            }
        }
    }
    assert(result.size() == numFragModes);
    return result;
}

std::string FlowInWattsToString(flow_t value_W, unsigned int precision)
{
    bool const printDebugInfo = false;
    if (value_W == max_flow_W)
    {
        if (printDebugInfo)
        {
            std::cout << "Found infinity:" << std::endl;
            std::cout << "- value_W   : " << fmt::format("{}", value_W) << std::endl;
            std::cout << "- max_flow_W: " << fmt::format("{}", max_flow_W) << std::endl;
        }
        return "inf";
    }
    double value_kW = static_cast<double>(value_W) / W_per_kW;
    return double_to_string(value_kW, precision);
}

std::vector<size_t> CalculateNodeConnectionOrder(Simulation const& s,
                                                 std::vector<NodeConnection> nodeConnections,
                                                 bool aggregateGroups = true)
{
    size_t const nNodeConns = nodeConnections.size();
    std::vector<std::string> originalNodeConnTags = {};
    std::vector<std::string> nodeConnTags = {};

    originalNodeConnTags.reserve(nNodeConns);
    nodeConnTags.reserve(nNodeConns);
    for (auto const& nodeConn : nodeConnections)
    {
        std::string nodeConnTag =
            NodeConnectionToString(s.the_model, nodeConn, true, aggregateGroups);
        originalNodeConnTags.push_back(nodeConnTag);
        nodeConnTags.push_back(nodeConnTag);
    }
    std::sort(nodeConnTags.begin(), nodeConnTags.end());

    std::vector<size_t> result;
    result.reserve(nNodeConns);
    for (auto const& nodeConnTag : nodeConnTags)
    {
        for (size_t nodeConnId = 0; nodeConnId < nodeConnTags.size(); ++nodeConnId)
        {
            if (nodeConnTag == originalNodeConnTags[nodeConnId])
            {
                result.push_back(nodeConnId);
                break;
            }
        }
    }
    assert(result.size() == nNodeConns);
    return result;
}

std::vector<NodeConnection> GetNodeConnections(Simulation& s, bool aggregateGroups)
{
    auto& connections = s.the_model.connection;

    std::vector<NodeConnection> nodeConnections = {};

    s.the_model.number_of_group_ports_to = {};
    s.the_model.number_of_group_ports_from = {};

    for (const auto& [key, value] : s.the_model.group_to_component)
    {
        s.the_model.number_of_group_ports_to.insert({key, 0});
        s.the_model.number_of_group_ports_from.insert({key, 0});
    }

    auto connOrder = CalculateConnectionOrder(s);

    auto nConn = connections.size();
    for (size_t iOrdConn = 0; iOrdConn < nConn; ++iOrdConn)
    {
        auto& iConn = connOrder[iOrdConn];
        auto const& connection = connections[iConn];
        bool fromIsGroup = false;
        bool toIsGroup = false;

        if (aggregateGroups)
        {
            fromIsGroup = s.the_model.component_to_group.contains(connection.from_component_id);
            toIsGroup = s.the_model.component_to_group.contains(connection.to_component_id);
        }

        NodeConnection nodeConn;
        nodeConn.connection_id = iConn;
        nodeConn.from_component_id = connection.from_component_id;
        nodeConn.from_port = connection.from_port;
        nodeConn.from_subtype_index = connection.from_subtype_index;
        nodeConn.from = connection.from;

        nodeConn.to_component_id = connection.to_component_id;
        nodeConn.to_port = connection.to_port;
        nodeConn.to_subtype_index = connection.to_subtype_index;
        nodeConn.to = connection.to;

        nodeConn.flow_type_id = connection.flow_type_id;

        if (fromIsGroup && toIsGroup)
        {
            auto groupFrom = s.the_model.component_to_group[connection.from_component_id];
            auto groupTo = s.the_model.component_to_group[connection.to_component_id];
            if (groupFrom == groupTo)
            {
                continue;
            }
        }
        if (fromIsGroup)
        {
            auto groupFrom = s.the_model.component_to_group[connection.from_component_id];
            nodeConn.from_component_id = groupFrom;
            auto& nPorts = s.the_model.number_of_group_ports_from[groupFrom];
            nodeConn.from_port = nPorts;
            nPorts++;
        }
        if (toIsGroup)
        {
            auto groupTo = s.the_model.component_to_group[connection.to_component_id];
            nodeConn.to_component_id = groupTo;
            auto& nPorts = s.the_model.number_of_group_ports_to[groupTo];
            nodeConn.to_port = nPorts;
            nPorts++;
        }
        bool newConn = true;
        for (auto& nodeConn0 : nodeConnections)
        {
            if (nodeConn0 == nodeConn)
            {
                newConn = false;
                nodeConn0.original_connection_id.push_back(iConn);
                break;
            }
        }
        if (newConn)
        {
            nodeConn.original_connection_id = {iConn};
            nodeConnections.push_back(nodeConn);
        }
    }

    return nodeConnections;
}

void AggregateGroups(std::vector<TimeAndFlows>& results,
                     std::vector<NodeConnection> const& nodeConnections)
{
    auto num_events = results.size();
    if (num_events == 0)
    {
        return;
    }

    auto nNodeConn = nodeConnections.size();
    auto nResult = results.size();
    std::vector<TimeAndFlows> newResults(nResult);

    for (size_t iResult = 0; iResult < nResult; ++iResult)
    {
        newResults[iResult].time_s = results[iResult].time_s;

        auto& origFlows = results[iResult].flows;
        auto& newFlows = newResults[iResult].flows;
        newFlows.resize(nNodeConn);

        for (size_t iNodeConn = 0; iNodeConn < nNodeConn; ++iNodeConn)
        {
            auto& nodeConn = nodeConnections[iNodeConn];
            for (auto const& iConn : nodeConn.original_connection_id)
            {
                newFlows[iNodeConn] += origFlows[iConn];
            }
        }

        newResults[iResult].storage_amounts_J = results[iResult].storage_amounts_J;
    }
    results = newResults;
}

void WriteResultsToEventFile(std::ofstream& out,
                             std::vector<TimeAndFlows> results,
                             Simulation const& s,
                             std::string const& scenarioTag,
                             std::string const& scenarioStartTimeTag,
                             std::vector<size_t> const& nodeConnOrder,
                             std::vector<size_t> const& storeOrder,
                             std::vector<size_t> const& compOrder,
                             TimeUnit outputTimeUnit)
{
    // TODO: pass in desired precision
    unsigned int precision = 1;
    unsigned int storePrecision = 3;
    Model const& m = s.the_model;
    std::map<size_t, std::vector<TimeState>> relSchByCompId;
    for (size_t i = 0; i < m.reliability.size(); ++i)
    {
        size_t compId = m.reliability[i].component_id;
        relSchByCompId[compId] = m.reliability[i].time_states;
    }

    for (auto const& r : results)
    {
        assert(r.flows.size() >= nodeConnOrder.size());
        out << scenarioTag << "," << scenarioStartTimeTag << ",";
        out << time_in_seconds_to_desired_unit(r.time_s, outputTimeUnit);

        for (size_t const& i : nodeConnOrder)
        {
            out << "," << FlowInWattsToString(r.flows[i].actual_W, precision);
        }
        for (size_t const& i : nodeConnOrder)
        {
            out << "," << FlowInWattsToString(r.flows[i].requested_W, precision);
        }
        for (size_t const& i : nodeConnOrder)
        {
            out << "," << FlowInWattsToString(r.flows[i].available_W, precision);
        }
        // NOTE: Amounts in kJ
        for (size_t i : storeOrder)
        {
            double store_J = static_cast<double>(r.storage_amounts_J[i]);
            double store_kJ = store_J / J_per_kJ;
            out << "," << std::fixed << std::setprecision(storePrecision) << store_kJ;
        }
        // NOTE: Store state in SOC
        for (size_t i : storeOrder)
        {
            double soc = 0.0;
            if (m.store[i].capacity_J > 0)
            {
                soc = static_cast<double>(r.storage_amounts_J[i]) /
                      static_cast<double>(m.store[i].capacity_J);
            }
            out << "," << std::fixed << std::setprecision(storePrecision) << soc;
        }
        for (size_t i : compOrder)
        {
            if (!m.component.tag[i].empty())
            {
                if (relSchByCompId.contains(i))
                {
                    TimeState ts = get_active_time_state(relSchByCompId[i], r.time_s);
                    if (ts.state)
                    {
                        out << ",available";
                    }
                    else
                    {
                        // lookup the failure and fragility modes
                        std::vector<size_t> failModes;
                        failModes.reserve(ts.failure_mode_causes.size());
                        std::vector<size_t> fragModes;
                        fragModes.reserve(ts.fragility_mode_causes.size());
                        for (size_t fm : ts.failure_mode_causes)
                        {
                            failModes.push_back(fm);
                        }
                        for (size_t fm : ts.fragility_mode_causes)
                        {
                            fragModes.push_back(fm);
                        }
                        std::sort(failModes.begin(), failModes.end());
                        std::sort(fragModes.begin(), fragModes.end());
                        std::vector<std::string> fmTags;
                        fmTags.reserve(ts.failure_mode_causes.size() +
                                       ts.fragility_mode_causes.size());
                        for (auto const& failModeId : failModes)
                        {
                            fmTags.push_back(s.failure_modes.tag[failModeId]);
                        }
                        for (auto const& fragModeId : fragModes)
                        {
                            fmTags.push_back(s.fragility_modes.tag[fragModeId]);
                        }
                        bool first = true;
                        std::ostringstream oss {};
                        for (std::string const& tag : fmTags)
                        {
                            oss << (first ? "" : " | ") << tag;
                            first = false;
                        }
                        out << "," << oss.str();
                    }
                }
                else
                {
                    out << ",available";
                }
            }
        }
        out << std::endl;
    }
}

Result
SetLoadsForScenario(std::vector<ScheduleBasedLoad>& loads, LoadDict loadMap, size_t scenarioIdx)
{
    for (size_t sblIdx = 0; sblIdx < loads.size(); ++sblIdx)
    {
        if (loads[sblIdx].scenario_id_to_load_id.contains(scenarioIdx))
        {
            auto loadId = loads[sblIdx].scenario_id_to_load_id.at(scenarioIdx);
            std::vector<TimeAndAmount> schedule {};
            size_t numEntries = loadMap.loads[loadId].size();
            schedule.reserve(numEntries);
            for (size_t i = 0; i < numEntries; ++i)
            {
                TimeAndAmount tal {};
                tal.Time_s = loadMap.loads[loadId][i].Time_s;
                tal.Amount_W = loadMap.loads[loadId][i].Amount_W;
                schedule.push_back(std::move(tal));
            }
            loads[sblIdx].times_and_loads = std::move(schedule);
        }
        else
        {
            std::cout << "ERROR:"
                      << "Unhandled scenario id in ScenarioIdToLoadId" << std::endl;
            return Result::failure;
        }
    }
    return Result::success;
}

Result
SetSupplyForScenario(std::vector<ScheduleBasedSource>& loads, LoadDict loadMap, size_t scenarioIdx)
{
    for (size_t sblIdx = 0; sblIdx < loads.size(); ++sblIdx)
    {
        if (loads[sblIdx].scenario_id_to_source_id.contains(scenarioIdx))
        {
            auto loadId = loads[sblIdx].scenario_id_to_source_id.at(scenarioIdx);
            std::vector<TimeAndAmount> schedule {};
            size_t numEntries = loadMap.loads[loadId].size();
            schedule.reserve(numEntries);
            for (size_t i = 0; i < numEntries; ++i)
            {
                TimeAndAmount tal {};
                tal.Time_s = loadMap.loads[loadId][i].Time_s;
                tal.Amount_W = loadMap.loads[loadId][i].Amount_W;
                schedule.push_back(std::move(tal));
            }
            loads[sblIdx].time_and_availables = std::move(schedule);
        }
        else
        {
            std::cout << "ERROR:"
                      << "Unhandled scenario id in ScenarioIdToSourceId" << std::endl;
            return Result::failure;
        }
    }
    return Result::success;
}

std::vector<double> DetermineScenarioOccurrenceTimes(Simulation& s, size_t scenIdx)
{
    std::vector<double> occurrenceTimes_s;
    auto const& maybeMaxOccurrences = s.scenario_map.max_occurrence[scenIdx];
    size_t maxOccurrence = maybeMaxOccurrences.has_value() ? maybeMaxOccurrences.value() : 1'000;
    auto const distId = s.scenario_map.occurrence_distribution_id[scenIdx];
    double scenarioStartTime_s = 0.0;
    double maxTime_s = time_to_seconds(s.info.MaxTime, s.info.TheTimeUnit);
    for (size_t i = 0; i < maxOccurrence; ++i)
    {
        scenarioStartTime_s += s.the_model.dist_sys.next_time_advance(distId);
        if (scenarioStartTime_s > maxTime_s)
        {
            break;
        }
        occurrenceTimes_s.push_back(scenarioStartTime_s);
    }
    return occurrenceTimes_s;
}

std::unordered_map<size_t, double> GetIntensitiesForScenario(Simulation& s, size_t scenIdx)
{
    std::unordered_map<size_t, double> intensityIdToAmount;
    size_t numIntensities = 0;
    for (size_t i = 0; i < s.scenario_intensities.scenario_id.size(); ++i)
    {
        if (s.scenario_intensities.scenario_id[i] == scenIdx)
        {
            numIntensities++;
        }
    }
    if (numIntensities == 0)
    {
        return intensityIdToAmount;
    }
    intensityIdToAmount.reserve(numIntensities);
    for (size_t i = 0; i < s.scenario_intensities.scenario_id.size(); ++i)
    {
        if (s.scenario_intensities.scenario_id[i] == scenIdx)
        {
            auto intensityId = s.scenario_intensities.intensity_id[i];
            intensityIdToAmount[intensityId] = s.scenario_intensities.intensity_level[i];
        }
    }
    return intensityIdToAmount;
}

std::vector<ScheduleBasedReliability> CopyReliabilities(Simulation const& s)
{
    std::vector<ScheduleBasedReliability> originalReliabilities;
    originalReliabilities.reserve(s.the_model.reliability.size());
    for (size_t sbrIdx = 0; sbrIdx < s.the_model.reliability.size(); ++sbrIdx)
    {
        ScheduleBasedReliability const& sbrSrc = s.the_model.reliability[sbrIdx];
        ScheduleBasedReliability sbrCopy {};
        sbrCopy.component_id = sbrSrc.component_id;
        sbrCopy.time_states.reserve(sbrSrc.time_states.size());
        for (size_t tsIdx = 0; tsIdx < sbrSrc.time_states.size(); ++tsIdx)
        {
            TimeState const& tsSrc = sbrSrc.time_states[tsIdx];
            TimeState tsCopy {};
            tsCopy.time = tsSrc.time;
            tsCopy.state = tsSrc.state;
            sbrCopy.time_states.push_back(std::move(tsCopy));
        }
        originalReliabilities.push_back(std::move(sbrCopy));
    }
    return originalReliabilities;
}

std::vector<std::string> ReliabilitiesToStrings(std::vector<ScheduleBasedReliability> const& sbrs)
{
    std::vector<std::string> result {};
    result.push_back(fmt::format("ScheduleBasedReliability vector size: {}", sbrs.size()));
    for (ScheduleBasedReliability const& sbr : sbrs)
    {
        std::ostringstream oss {};
        bool isFirst = true;
        for (TimeState const& ts : sbr.time_states)
        {
            if (isFirst)
            {
                isFirst = false;
            }
            else
            {
                oss << ",";
            }
            oss << "{" << ts.time << "," << ts.state << "}";
        }
        result.push_back(
            fmt::format("- {{ComponentId: {},TimeStates=[{}]}}", sbr.component_id, oss.str()));
    }
    return result;
}

void PrintReliabilities(std::vector<ScheduleBasedReliability> const& sbrs)
{
    for (std::string const& s : ReliabilitiesToStrings(sbrs))
    {
        std::cout << s << std::endl;
    }
}

std::vector<ScheduleBasedReliability> ApplyReliabilitiesAndFragilities(
    std::function<double()>& randFn,
    std::vector<size_t> const& componentFailureModeComponentIds,
    std::vector<double> const& componentInitialAges_s,
    std::vector<std::string> const& componentTags,
    std::vector<size_t> const& componentFragilityComponentIds,
    std::vector<size_t> const& componentFragilityFragilityModeIds,
    std::vector<size_t> const& fragilityModeFragilityCurveIds,
    std::vector<std::optional<size_t>> const& fragilityModeRepairDistIds,
    std::vector<std::string> const& fragilityModeTags,
    std::vector<size_t> const& fragilityCurveCurveIds,
    std::vector<FragilityCurveType> const& fragilityCurveCurveTypes,
    std::vector<LinearFragilityCurve> linearFragilityCurves,
    std::vector<TabularFragilityCurve> tabularFragilityCurves,
    DistributionSystem const& ds,
    double startTime_s,
    double endTime_s,
    std::unordered_map<size_t, double> const& intensityIdToAmount,
    std::unordered_map<size_t, std::vector<TimeState>> const& relSchByCompId,
    bool verbose,
    Log const& log)
{
    std::vector<ScheduleBasedReliability> result;
    result.reserve(componentFailureModeComponentIds.size());
    std::unordered_set<size_t> reliabilitiesAdded;
    reliabilitiesAdded.reserve(componentFailureModeComponentIds.size());
    for (size_t cfmIdx = 0; cfmIdx < componentFailureModeComponentIds.size(); ++cfmIdx)
    {
        auto const& compId = componentFailureModeComponentIds[cfmIdx];
        // NOTE: there should be a reliability schedule for each entry in
        // ComponentFailureModes. However, since it is possible to have
        // more than one failure mode on one component (and those have
        // already been combined by this point), we need to check if we've
        // already added this reliability schedule
        if (reliabilitiesAdded.contains(compId))
        {
            continue;
        }
        std::vector<TimeState> const& sch = relSchByCompId.at(compId);
        double initialAge_s = componentInitialAges_s[compId];
        if (verbose)
        {
            Log_info(log, fmt::format("component: {}", componentTags[compId]));
            Log_info(log, fmt::format("initial age (h): {}", (initialAge_s / seconds_per_hour)));
        }
        std::vector<TimeState> clipped =
            clip(sch, startTime_s + initialAge_s, endTime_s + initialAge_s, true);
        // NOTE: Reliabilities have not yet been assigned so we can
        // just push_back()
        ScheduleBasedReliability sbr {};
        sbr.component_id = compId;
        sbr.time_states = std::move(clipped);
        result.push_back(std::move(sbr));
        reliabilitiesAdded.insert(compId);
    }
    if (intensityIdToAmount.size() > 0)
    {
        if (verbose)
        {
            Log_info(log, "... Applying fragilities");
        }
        // NOTE: if there are no components having fragility modes,
        // there is nothing to do.
        for (size_t cfmIdx = 0; cfmIdx < componentFragilityComponentIds.size(); ++cfmIdx)
        {
            size_t fmId = componentFragilityFragilityModeIds[cfmIdx];
            size_t fcId = fragilityModeFragilityCurveIds[fmId];
            std::optional<size_t> repairId = fragilityModeRepairDistIds[fmId];
            FragilityCurveType curveType = fragilityCurveCurveTypes[fcId];
            size_t fcIdx = fragilityCurveCurveIds[fcId];
            bool isFailed = false;
            double failureFrac = 0.0;
            switch (curveType)
            {
            case (FragilityCurveType::linear):
            {
                LinearFragilityCurve lfc = linearFragilityCurves[fcIdx];
                size_t vulnerId = lfc.vulnerability_id;
                if (intensityIdToAmount.contains(vulnerId))
                {
                    double level = intensityIdToAmount.at(vulnerId);
                    failureFrac = LinearFragilityCurve_GetFailureFraction(lfc, level);
                }
            }
            break;
            case (FragilityCurveType::tabular):
            {
                TabularFragilityCurve tfc = tabularFragilityCurves[fcIdx];
                size_t vulnerId = tfc.vulnerability_id;
                if (intensityIdToAmount.contains(vulnerId))
                {
                    double level = intensityIdToAmount.at(vulnerId);
                    failureFrac = TabularFragilityCurve_GetFailureFraction(tfc, level);
                }
            }
            break;
            default:
            {
                Log_error(log, "fragility_curve", "unhandled fragility curve type");
                std::exit(1);
            }
            break;
            }
            if (failureFrac >= 1.0)
            {
                isFailed = true;
            }
            else if (failureFrac <= 0.0)
            {
                isFailed = false;
            }
            else
            {
                isFailed = randFn() <= failureFrac;
            }
            // NOTE: if we are not failed, there is nothing to do
            if (isFailed)
            {
                // now we have to find the affected component
                // and assign/update a reliability schedule for it
                // including any repair distribution if we have
                // one.
                size_t compId = componentFragilityComponentIds[cfmIdx];
                if (verbose)
                {
                    Log_debug(log,
                              "fragility_curve",
                              fmt::format("component failed: {}; cause: {}",
                                          componentTags[compId],
                                          fragilityModeTags[fmId]));
                }
                // does the component have a reliability signal?
                bool hasReliabilityAlready = false;
                size_t reliabilityId = 0;
                for (size_t rIdx = 0; rIdx < result.size(); ++rIdx)
                {
                    if (result[rIdx].component_id == compId)
                    {
                        hasReliabilityAlready = true;
                        reliabilityId = rIdx;
                        break;
                    }
                }
                std::vector<TimeState> newTimeStates;
                TimeState ts {};
                ts.state = false;
                ts.time = 0.0;
                ts.fragility_mode_causes.insert(fmId);
                newTimeStates.push_back(std::move(ts));
                if (repairId.has_value())
                {
                    size_t repId = repairId.value();
                    double randValue = randFn();
                    if (verbose)
                    {
                        Log_info(log,
                                 fmt::format("randValue for next time advance is: {}", randValue));
                    }
                    double repairTime_s = ds.next_time_advance(repId, randValue);
                    TimeState repairTime {};
                    repairTime.time = repairTime_s;
                    repairTime.state = true;
                    newTimeStates.push_back(std::move(repairTime));
                }
                if (hasReliabilityAlready)
                {
                    auto const& currentSch = result[reliabilityId].time_states;
                    std::vector<TimeState> combined = combine(currentSch, newTimeStates);
                    result[reliabilityId].time_states = std::move(combined);
                }
                else
                {
                    ScheduleBasedReliability sbr {};
                    sbr.component_id = compId;
                    sbr.time_states = newTimeStates;
                    result.push_back(std::move(sbr));
                }
            }
        }
    }
    return result;
}

// TODO: change this to write all data in a columnar format
// and THEN sort them and iterate through them to write the
// header and rows in order. Otherwise, we separate the header
// names from data calculations.
void WriteStatisticsToFile(Simulation const& s,
                           std::string const& statsFilePath,
                           std::vector<ScenarioOccurrenceStats> const& occurrenceStats,
                           std::vector<size_t> const& compOrder,
                           std::vector<size_t> const& failOrder,
                           std::vector<size_t> const& fragOrder)
{
    std::ofstream stats;
    stats.open(statsFilePath);
    if (!stats.good())
    {
        std::cout << "Could not open '" << statsFilePath << "' for writing." << std::endl;
        return;
    }
    stats << "scenario id,"
          << "occurrence number,"
          << "duration (h),"
          << "total source (kJ),"
          << "total load (kJ),"
          << "total storage (kJ),"
          << "total waste (kJ),"
          << "energy balance (source-(load+storage+waste)) (kJ),"
          << "site efficiency,"
          << "uptime (h),"
          << "downtime (h),"
          << "load not served (kJ),"
          << "energy robustness [ER],"
          << "energy availability [EA],"
          << "max single event downtime [MaxSEDT] (h),"
          << "global availability";
    if (occurrenceStats.size() > 0)
    {
        for (auto const& statsByFlow : occurrenceStats[0].flow_type_stats)
        {
            std::string const& flowType = s.flow_type_map.flow_type[statsByFlow.flow_type_id];
            stats << ",energy robustness [ER] for " << flowType;
            stats << ",energy availability [EA] for " << flowType;
        }
        for (auto const& statsByFlowLoad : occurrenceStats[0].load_and_flow_type_stats)
        {
            std::string const& flowType =
                s.flow_type_map.flow_type[statsByFlowLoad.stats.flow_type_id];
            std::string const& tag = s.the_model.component.tag[statsByFlowLoad.component_id];
            stats << ",energy robustness [ER] for " << tag << " [flow: " << flowType << "]";
            stats << ",energy availability [EA] for " << tag << " [flow: " << flowType << "]";
        }
        for (auto const& lnsByComp : occurrenceStats[0].load_not_served_for_components)
        {
            std::string const& flowType = s.flow_type_map.flow_type[lnsByComp.flow_type_id];
            std::string const& tag = s.the_model.component.tag[lnsByComp.component_id];
            stats << ",load not served (kJ) for " << tag << " [flow: " << flowType << "]";
        }
    }
    std::set<size_t> componentsToSkip;
    for (size_t i : compOrder)
    {
        if (s.the_model.component.tag[i].empty())
        {
            componentsToSkip.insert(i);
        }
        else
        {
            stats << ",availability: " << s.the_model.component.tag[i];
        }
    }
    for (size_t i : failOrder)
    {
        stats << ",global count: " << s.failure_modes.tag[i];
    }
    for (size_t i : fragOrder)
    {
        stats << ",global count: " << s.fragility_modes.tag[i];
    }
    for (size_t i : failOrder)
    {
        stats << ",global time fraction: " << s.failure_modes.tag[i];
    }
    for (size_t i : fragOrder)
    {
        stats << ",global time fraction: " << s.fragility_modes.tag[i];
    }
    std::map<size_t, std::set<size_t>> failModeIdsByCompId;
    std::map<size_t, std::set<size_t>> fragModeIdsByCompId;
    for (size_t compId : compOrder)
    {
        failModeIdsByCompId[compId] = std::set<size_t> {};
        fragModeIdsByCompId[compId] = std::set<size_t> {};
        for (auto const& occ : occurrenceStats)
        {
            if (occ.event_count_by_comp_id_by_failure_mode_id.contains(compId))
            {
                for (auto const& p : occ.event_count_by_comp_id_by_failure_mode_id.at(compId))
                {
                    failModeIdsByCompId[compId].insert(p.first);
                }
            }
            if (occ.event_count_by_comp_id_by_fragility_mode_id.contains(compId))
            {
                for (auto const& p : occ.event_count_by_comp_id_by_fragility_mode_id.at(compId))
                {
                    fragModeIdsByCompId[compId].insert(p.first);
                }
            }
        }
        for (size_t failModeId : failOrder)
        {
            if (failModeIdsByCompId[compId].contains(failModeId))
            {
                stats << ",count: " << s.the_model.component.tag[compId] << " / "
                      << s.failure_modes.tag[failModeId];
            }
        }
        for (size_t fragModeId : fragOrder)
        {
            if (fragModeIdsByCompId[compId].contains(fragModeId))
            {
                stats << ",count: " << s.the_model.component.tag[compId] << " / "
                      << s.fragility_modes.tag[fragModeId];
            }
        }
    }
    for (size_t compId : compOrder)
    {
        for (size_t failModeId : failOrder)
        {
            if (failModeIdsByCompId[compId].contains(failModeId))
            {
                stats << ",time fraction: " << s.the_model.component.tag[compId] << " / "
                      << s.failure_modes.tag[failModeId];
            }
        }
        for (size_t fragModeId : fragOrder)
        {
            if (fragModeIdsByCompId[compId].contains(fragModeId))
            {
                stats << ",time fraction: " << s.the_model.component.tag[compId] << " / "
                      << s.fragility_modes.tag[fragModeId];
            }
        }
    }
    stats << std::endl;
    for (auto const& os : occurrenceStats)
    {
        double stored_kJ = os.storage_charge_kJ - os.storage_discharge_kJ;
        double balance = os.inflow_kJ + os.in_from_env_kJ -
                         (os.outflow_achieved_kJ + stored_kJ + os.wasteflow_kJ);
        double efficiency = (os.inflow_kJ + os.storage_discharge_kJ) > 0.0
                                ? ((os.outflow_achieved_kJ + os.storage_charge_kJ) /
                                   (os.inflow_kJ + os.storage_discharge_kJ))
                                : 0.0;
        double ER =
            os.outflow_request_kJ > 0.0 ? (os.outflow_achieved_kJ / os.outflow_request_kJ) : 1.0;
        double EA = os.duration_s > 0.0 ? (os.uptime_s / os.duration_s) : 1.0;
        stats << s.scenario_map.tag[os.scenario_id];
        stats << "," << os.occurrence_number;
        stats << "," << (os.duration_s / seconds_per_hour);
        stats << "," << double_to_string(os.inflow_kJ + os.in_from_env_kJ, 0);
        // TODO(mok): break out InFromEnv from Inflow and list separately
        stats << "," << double_to_string(os.outflow_achieved_kJ, 0);
        stats << "," << double_to_string(stored_kJ, 0);
        stats << "," << double_to_string(os.wasteflow_kJ, 0);
        stats << "," << double_to_string(balance, 6);
        stats << "," << efficiency;
        stats << "," << (os.uptime_s / seconds_per_hour);
        stats << "," << (os.downtime_s / seconds_per_hour);
        stats << "," << os.load_not_served_kJ;
        stats << "," << ER;
        stats << "," << EA;
        stats << "," << (os.max_SEDT_s / seconds_per_hour);
        double global_availability =
            (os.duration_s > 0.0) ? (os.availability_s / os.duration_s) : 0.0;
        if (global_availability < 0.0)
        {
            global_availability = 0.0;
        }
        stats << "," << global_availability;
        // NOTE: written in alphabetical order by flowtype name
        for (auto const& statsByFlow : os.flow_type_stats)
        {

            double ER_by_flow = statsByFlow.total_request_kJ > 0.0
                                    ? (statsByFlow.total_achieved_kJ / statsByFlow.total_request_kJ)
                                    : 0.0;
            double EA_by_flow = os.duration_s > 0.0 ? (statsByFlow.uptime_s / os.duration_s) : 0.0;
            stats << "," << ER_by_flow;
            stats << "," << EA_by_flow;
        }
        for (auto const& statsByFlowLoad : os.load_and_flow_type_stats)
        {
            double ER_by_load = statsByFlowLoad.stats.total_request_kJ > 0.0
                                    ? (statsByFlowLoad.stats.total_achieved_kJ /
                                       statsByFlowLoad.stats.total_request_kJ)
                                    : 0.0;
            double EA_by_load =
                os.duration_s > 0.0 ? (statsByFlowLoad.stats.uptime_s / os.duration_s) : 0.0;
            stats << "," << ER_by_load;
            stats << "," << EA_by_load;
        }
        for (auto const& lnsByComp : os.load_not_served_for_components)
        {
            stats << "," << lnsByComp.load_not_served_kJ;
        }
        for (size_t i : compOrder)
        {
            if (!componentsToSkip.contains(i))
            {
                double availability =
                    os.duration_s > 0.0 ? os.availability_by_comp_id_s.at(i) / os.duration_s : 1.0;
                stats << "," << availability;
            }
        }
        for (size_t i : failOrder)
        {
            size_t eventCount = os.event_count_by_failure_mode_id.contains(i)
                                    ? os.event_count_by_failure_mode_id.at(i)
                                    : 0;
            stats << "," << eventCount;
        }
        for (size_t i : fragOrder)
        {
            size_t eventCount = os.event_count_by_fragility_mode_id.contains(i)
                                    ? os.event_count_by_fragility_mode_id.at(i)
                                    : 0;
            stats << "," << eventCount;
        }
        for (size_t i : failOrder)
        {
            double time_s =
                os.time_by_failure_mode_id_s.contains(i) ? os.time_by_failure_mode_id_s.at(i) : 0.0;
            stats << "," << (os.duration_s > 0.0 ? time_s / os.duration_s : 0.0);
        }
        for (size_t i : fragOrder)
        {
            double time_s = os.time_by_fragility_mode_id_s.contains(i)
                                ? os.time_by_fragility_mode_id_s.at(i)
                                : 0.0;
            stats << "," << (os.duration_s > 0.0 ? time_s / os.duration_s : 0.0);
        }
        for (size_t compId : compOrder)
        {
            for (size_t i : failOrder)
            {
                if (failModeIdsByCompId[compId].contains(i))
                {
                    if (os.event_count_by_comp_id_by_failure_mode_id.contains(compId) &&
                        os.event_count_by_comp_id_by_failure_mode_id.at(compId).contains(i))
                    {
                        stats << ","
                              << os.event_count_by_comp_id_by_failure_mode_id.at(compId).at(i);
                    }
                    else
                    {
                        stats << ",0";
                    }
                }
            }
            for (size_t i : fragOrder)
            {
                if (fragModeIdsByCompId[compId].contains(i))
                {
                    if (os.event_count_by_comp_id_by_fragility_mode_id.contains(compId) &&
                        os.event_count_by_comp_id_by_fragility_mode_id.at(compId).contains(i))
                    {
                        stats << ","
                              << os.event_count_by_comp_id_by_fragility_mode_id.at(compId).at(i);
                    }
                    else
                    {
                        stats << ",0";
                    }
                }
            }
        }
        for (size_t compId : compOrder)
        {
            for (size_t i : failOrder)
            {
                if (failModeIdsByCompId[compId].contains(i))
                {
                    if (os.time_by_comp_id_by_failure_mode_id_s.contains(compId) &&
                        os.time_by_comp_id_by_failure_mode_id_s.at(compId).contains(i))
                    {
                        double t = os.time_by_comp_id_by_failure_mode_id_s.at(compId).at(i);
                        stats << "," << (os.duration_s > 0.0 ? t / os.duration_s : 0.0);
                    }
                    else
                    {
                        stats << ",0";
                    }
                }
            }
            for (size_t i : fragOrder)
            {
                if (fragModeIdsByCompId[compId].contains(i))
                {
                    if (os.time_by_comp_id_by_fragility_mode_id_s.contains(compId) &&
                        os.time_by_comp_id_by_fragility_mode_id_s.at(compId).contains(i))
                    {
                        double t = os.time_by_comp_id_by_fragility_mode_id_s.at(compId).at(i);
                        stats << "," << (os.duration_s > 0.0 ? t / os.duration_s : 0.0);
                    }
                    else
                    {
                        stats << ",0";
                    }
                }
            }
        }
        stats << std::endl;
    }
    stats.close();
}

std::vector<TimeAndFlows> ApplyUniformTimeStep(std::vector<TimeAndFlows> const& results,
                                               double const time_step_h)
{
    auto num_events = results.size();
    if ((num_events == 0) || (time_step_h <= 0.0))
    {
        return results;
    }

    auto taf = results.front();
    auto num_stored = taf.storage_amounts_J.size();

    std::vector<TimeAndFlows> modified_results = {taf};

    double t_prev_report_s = 0.0;
    double T_report_s = 3600.0 * time_step_h;

    for (auto& next_taf : results)
    {

        double t_next_report_s = t_prev_report_s + T_report_s;
        while (t_next_report_s <= next_taf.time_s)
        {

            auto mod_taf = taf;
            if (t_next_report_s == next_taf.time_s)
                mod_taf.flows = next_taf.flows;

            mod_taf.time_s = t_next_report_s;
            double dt_orig_s = next_taf.time_s - taf.time_s;
            if (dt_orig_s > 0.0)
            {
                double dt_s = t_next_report_s - taf.time_s;
                double time_frac = dt_s / dt_orig_s;
                for (std::size_t i = 0; i < num_stored; ++i)
                {
                    mod_taf.storage_amounts_J[i] =
                        static_cast<flow_t>((1. - time_frac) * taf.storage_amounts_J[i] +
                                            time_frac * next_taf.storage_amounts_J[i]);
                }
            }
            modified_results.push_back(mod_taf);

            t_prev_report_s = t_next_report_s;
            t_next_report_s += T_report_s;
        }
        taf = next_taf;
    }
    return modified_results;
}

std::unordered_map<size_t, std::vector<TimeState>>
CreateFailureSchedules(std::vector<size_t> const& componentFailureModeComponentIds,
                       std::vector<size_t> const& componentFailureModeFailureModeIds,
                       std::vector<double> const& componentInitialAges_s,
                       ReliabilityCoordinator const& rc,
                       std::function<double()> const& randFn,
                       DistributionSystem const& ds,
                       double scenarioDuration_s,
                       double scenarioOffset_s)
{
    std::unordered_map<size_t, std::vector<TimeState>> relSchByCompFailId;
    relSchByCompFailId.reserve(componentFailureModeComponentIds.size());
    std::unordered_set<size_t> componentsWithFailures;
    componentsWithFailures.reserve(componentFailureModeComponentIds.size());
    for (size_t compFailId = 0; compFailId < componentFailureModeComponentIds.size(); ++compFailId)
    {
        size_t fmId = componentFailureModeFailureModeIds[compFailId];
        size_t compId = componentFailureModeComponentIds[compFailId];
        componentsWithFailures.insert(compId);
        double age_s = componentInitialAges_s[compId];
        // NOTE: Fix. ERIN is like the movie Groundhog's Day --
        // each "year" is repeated over and over again until the
        // max time limit is reached. As such, if we have
        // 1,000 years of simulation, we need to generate 1,000
        // unique 1-year reliability schedules. The exact duration
        // will be (scenarioStartMonth + scenarioStartDay
        // + scenarioDuration) - Jan 1 at 00:00:00. We could also
        // just build out the longest duration needed by any
        // scenario and just use that for all of them and clip
        // to the correct time...
        // BUT s.Info.MaxTime should just set the number of
        // "Groundhog Days"...
        // Question: does scenario need start month/day? Could just be
        // of length duration... is it needed to match up with initial
        // age? Yes. So what we need for each scenario is a time
        // offset. So the reliability schedule duration will be
        // (scenarioOffset + scenarioDuration). Offset will be from
        // the time the age is assessed.
        double endTime_s = age_s + scenarioOffset_s + scenarioDuration_s;
        std::vector<TimeState> relSch = rc.make_schedule_for_link(fmId, randFn, ds, endTime_s);
        for (auto& ts : relSch)
        {
            if (!ts.state)
            {
                ts.failure_mode_causes.insert(fmId);
            }
        }
        relSchByCompFailId.insert({compFailId, std::move(relSch)});
    }
    // NOTE: combine reliability curves so they are per component
    std::unordered_map<size_t, std::vector<TimeState>> relSchByCompId;
    relSchByCompId.reserve(componentsWithFailures.size());
    for (auto const& pair : relSchByCompFailId)
    {
        size_t compId = componentFailureModeComponentIds[pair.first];
        if (relSchByCompId.contains(compId))
        {
            std::vector<TimeState> combined = combine(pair.second, relSchByCompId.at(compId));
            relSchByCompId[compId] = std::move(combined);
        }
        else
        {
            relSchByCompId[compId] = pair.second;
        }
    }
    return relSchByCompId;
}

void WriteReliabilityCurves(std::string const& scenarioName,
                            size_t scenarioOccurrence,
                            Simulation const& s)
{
    std::string fname = fmt::format("{}-{:03}.csv", scenarioName, scenarioOccurrence);
    std::ofstream out;
    out.open(fname);
    if (!out.good())
    {
        std::cout << "Could not open '" << fname << "' for writing." << std::endl;
        return;
    }
    size_t maxRow = 0;
    for (size_t i = 0; i < s.the_model.reliability.size(); ++i)
    {
        ScheduleBasedReliability const& sbr = s.the_model.reliability[i];
        if (sbr.time_states.size() > maxRow)
        {
            maxRow = sbr.time_states.size();
        }
    }
    for (size_t row = 0; row < maxRow; row++)
    {
        if (row == 0)
        {
            for (size_t i = 0; i < s.the_model.reliability.size(); ++i)
            {
                ScheduleBasedReliability const& sbr = s.the_model.reliability[i];
                if (i > 0)
                {
                    out << ",";
                }
                std::string const& compTag = s.the_model.component.tag[sbr.component_id];
                out << "time (h)," << compTag << " state,causes";
            }
            out << "\n";
        }
        for (size_t i = 0; i < s.the_model.reliability.size(); ++i)
        {
            ScheduleBasedReliability const& sbr = s.the_model.reliability[i];
            if (i > 0)
            {
                out << ",";
            }
            if (row < sbr.time_states.size())
            {
                std::vector<std::string> causes;
                for (size_t fmId : sbr.time_states[row].failure_mode_causes)
                {
                    causes.push_back(s.failure_modes.tag[fmId]);
                }
                for (size_t fmId : sbr.time_states[row].fragility_mode_causes)
                {
                    causes.push_back(s.fragility_modes.tag[fmId]);
                }
                std::string causeStr = "";
                for (std::string const& cause : causes)
                {
                    causeStr += (causeStr.size() == 0) ? cause : fmt::format(" | {}", cause);
                }
                out << time_in_seconds_to_desired_unit(sbr.time_states[row].time, TimeUnit::hour)
                    << "," << sbr.time_states[row].state << "," << causeStr;
            }
            else
            {
                out << ",,";
            }
        }
        out << "\n";
    }
    out.close();
}

std::unordered_set<size_t> CalculateComponentsToReport(std::vector<bool> const& reportFlags)
{
    std::unordered_set<size_t> compsToReport {};
    compsToReport.reserve(reportFlags.size());
    for (size_t id = 0; id < reportFlags.size(); ++id)
    {
        if (reportFlags[id])
        {
            compsToReport.insert(id);
        }
    }
    return compsToReport;
}

std::unordered_set<size_t>
CalculateConnectionsToReport(std::vector<Connection> const& conns,
                             std::unordered_set<size_t> const& compsToReport)
{
    std::unordered_set<size_t> connsToReport {};
    connsToReport.reserve(conns.size());
    for (size_t id = 0; id < conns.size(); ++id)
    {
        Connection const& c = conns[id];
        if (compsToReport.contains(c.from_component_id) ||
            compsToReport.contains(c.to_component_id))
        {
            connsToReport.insert(id);
        }
    }
    return connsToReport;
}

void Simulation_run(Simulation& s,
                    Log& log,
                    std::string const& eventsFilename,
                    std::string const& statsFilename,
                    double time_step_h /*-1.0*/,
                    bool aggregateGroups,
                    bool saveReliabilityCurves,
                    bool verbose)
{
    // TODO: wrap into input options struct and pass in
    bool const checkNetwork = false;
    if (checkNetwork)
    {
        std::vector<std::string> issues = check_network(s.the_model);
        if (issues.size() > 0)
        {
            Log_warning(log, "network connection", "start list of issues");
            for (std::string const& issue : issues)
            {
                Log_warning(log, "network connection", issue);
            }
            Log_warning(log, "network connection", "end list of issues");
        }
        assert(issues.size() == 0);
    }
    // TODO: turn the following into parameters
    TimeUnit outputTimeUnit = TimeUnit::hour;

    FixedRandom fixedRandom;
    FixedSeries fixedSeries;
    Random fullRandom;
    switch (s.info.TypeOfRandom)
    {
    case (RandomType::fixed_random):
    {
        fixedRandom.fixed_value = s.info.FixedValue;
        s.the_model.random_function = fixedRandom;
    }
    break;
    case (RandomType::fixed_series):
    {
        fixedSeries.index = 0;
        fixedSeries.series = s.info.Series;
        s.the_model.random_function = fixedSeries;
    }
    break;
    case (RandomType::random_from_seed):
    {
        fullRandom = create_random_with_seed(s.info.Seed);
        s.the_model.random_function = fullRandom;
    }
    break;
    case (RandomType::random_from_clock):
    {
        fullRandom = create_random();
        s.the_model.random_function = fullRandom;
    }
    break;
    default:
    {
        Log_error(log, "RandomType", "unhandled random type");
        std::exit(1);
    }
    break;
    }
    // TODO: expose proper options
    // TODO: check the components and network:
    // -- that all components are hooked up to something
    // -- that no port is double linked
    // -- that all connections have the correct flows
    // -- that required ports are linked
    // -- check that we have a proper acyclic graph?
    // NOTE: set up reliability manager
    // TODO: remove duplication of data here
    for (size_t fmIdx = 0; fmIdx < s.failure_modes.failure_distribution_id.size(); ++fmIdx)
    {
        s.the_model.rel_coord.add_failure_mode(s.failure_modes.tag[fmIdx],
                                               s.failure_modes.failure_distribution_id[fmIdx],
                                               s.failure_modes.repair_distribution_id[fmIdx]);
    }
    for (size_t compFailId = 0; compFailId < s.component_failure_modes.component_id.size();
         ++compFailId)
    {
        s.the_model.rel_coord.link_component_with_failure_mode(
            s.component_failure_modes.component_id[compFailId],
            s.component_failure_modes.failure_mode_id[compFailId]);
    }
    // TODO: generate a data structure to hold all results.
    // TODO: set random function for Model based on SimInfo
    // IDEA: split out file writing into separate thread? See
    // https://codetrips.com/2020/07/26/modern-c-writing-a-thread-safe-queue/comment-page-1/
    // IDEA: use Apache Arrow for memory tables? Output parquet as primary
    // output format (instead of CSV)?
    // NOW, we want to do a simulation for each scenario
    std::ofstream out;
    out.open(eventsFilename);
    if (!out.good())
    {
        Log_warning(
            log, "file I/O", fmt::format("Could not open '{}' for writing", eventsFilename));
        return;
    }

    // TODO: need to account for WASTE component
    // if a component is not reported and connected to WASTE, the
    // WASTE should also be not reported
    std::unordered_set<size_t> compsToReport =
        CalculateComponentsToReport(s.the_model.component.report);
    std::unordered_set<size_t> connsToReport =
        CalculateConnectionsToReport(s.the_model.connection, compsToReport);

    std::vector<size_t> scenarioOrder = CalculateScenarioOrder(s);
    std::vector<size_t> connOrder = CalculateConnectionOrder(s);
    std::vector<size_t> storeOrderForEvents = CalculateStoreOrder(s, compsToReport);
    std::vector<size_t> compOrder = CalculateComponentOrder(s);
    std::vector<size_t> failOrder = CalculateFailModeOrder(s);
    std::vector<size_t> fragOrder = CalculateFragilModeOrder(s);
    std::vector<size_t> compOrderForEvents = RemoveNonReportingIds(compOrder, compsToReport);

    auto nodeConnections = GetNodeConnections(s, aggregateGroups);
    std::vector<size_t> nodeConnOrder =
        CalculateNodeConnectionOrder(s, nodeConnections, aggregateGroups);
    std::unordered_set<size_t> nodeConnsToReport {};
    nodeConnsToReport.reserve(connsToReport.size());
    for (size_t id = 0; id < nodeConnections.size(); ++id)
    {
        NodeConnection const& nc = nodeConnections[id];
        if (connsToReport.contains(nc.connection_id))
        {
            nodeConnsToReport.insert(id);
        }
    }
    std::vector<size_t> nodeConnOrderForEvents =
        RemoveNonReportingIds(nodeConnOrder, nodeConnsToReport);

    WriteEventFileHeader(out,
                         s.the_model,
                         s.flow_type_map,
                         nodeConnOrderForEvents,
                         storeOrderForEvents,
                         compOrderForEvents,
                         outputTimeUnit,
                         nodeConnections,
                         aggregateGroups);
    std::vector<ScenarioOccurrenceStats> occurrenceStats;
    for (size_t scenIdx : scenarioOrder)
    {
        double scenarioDuration_s =
            time_to_seconds(s.scenario_map.duration[scenIdx], s.scenario_map.time_unit[scenIdx]);
        double scenarioOffset_s = s.scenario_map.time_offset_in_seconds[scenIdx];
        std::string const& scenarioTag = s.scenario_map.tag[scenIdx];
        if (verbose)
        {
            Log_info(log, "Scenario", scenarioTag);
        }
        // for this scenario, ensure all schedule-based components
        // have the right schedule set for this scenario
        if (SetLoadsForScenario(s.the_model.scheduled_load, s.load_map, scenIdx) == Result::failure)
        {
            Log_warning(log, "", "Issue setting schedule loads");
            return;
        }
        if (SetSupplyForScenario(s.the_model.scheduled_source, s.load_map, scenIdx) ==
            Result::failure)
        {
            Log_warning(log, "", "Issue setting schedule sources");
            return;
        }
        // TODO: implement load substitution for schedule-based sources
        // for (size_t sbsIdx = 0; sbsIdx < s.Model.ScheduleSrcs.size();
        // ++sbsIdx) {/* ... */}
        std::vector<double> occurrenceTimes_s = DetermineScenarioOccurrenceTimes(s, scenIdx);
        if (verbose)
        {
            Log_debug(log,
                      fmt::format("Calculated {} occurrence times for {}",
                                  occurrenceTimes_s.size(),
                                  s.scenario_map.tag[scenIdx]));
        }
        // TODO: initialize total scenario stats (i.e.,
        // over all occurrences)
        std::unordered_map<size_t, double> intensityIdToAmount =
            GetIntensitiesForScenario(s, scenIdx);
        for (size_t occIdx = 0; occIdx < occurrenceTimes_s.size(); ++occIdx)
        {
            if (verbose)
            {
                Log_debug(log, fmt::format("... Occurrence #{}", occIdx + 1));
            }
            std::unordered_map<size_t, std::vector<TimeState>> relSchByCompId =
                CreateFailureSchedules(s.component_failure_modes.component_id,
                                       s.component_failure_modes.failure_mode_id,
                                       s.the_model.component.initial_age_s,
                                       s.the_model.rel_coord,
                                       s.the_model.random_function,
                                       s.the_model.dist_sys,
                                       scenarioDuration_s,
                                       scenarioOffset_s);
            if (verbose)
            {
                Log_info(log, "Generating reliability schedules");
                for (auto const& pair : relSchByCompId)
                {
                    std::string const& tag = s.the_model.component.tag[pair.first];
                    Log_info(log, fmt::format("Schedule for {}[{}]", tag, pair.first));
                    for (auto const& ts : pair.second)
                    {
                        Log_debug(log, fmt::format("- {}", to_string(ts)));
                    }
                }
            }
            double t = occurrenceTimes_s[occIdx];
            double tEnd = t + scenarioDuration_s;
            if (verbose)
            {
                Log_info(
                    log,
                    fmt::format("Occurrence #{} at {}", occIdx + 1, seconds_to_pretty_string(t)));
                Log_info(
                    log,
                    fmt::format("Scenario start time: {} h",
                                time_in_seconds_to_hours(static_cast<uint64_t>(scenarioOffset_s))));
                Log_info(log,
                         fmt::format(
                             "Scenario end time: {} h",
                             time_in_seconds_to_hours(static_cast<uint64_t>(scenarioOffset_s) +
                                                      static_cast<uint64_t>(scenarioDuration_s))));
            }
            s.the_model.reliability.clear();
            s.the_model.reliability =
                ApplyReliabilitiesAndFragilities(s.the_model.random_function,
                                                 s.component_failure_modes.component_id,
                                                 s.the_model.component.initial_age_s,
                                                 s.the_model.component.tag,
                                                 s.component_fragilities.component_id,
                                                 s.component_fragilities.fragility_mode_id,
                                                 s.fragility_modes.fragility_curve_id,
                                                 s.fragility_modes.repair_distribution_id,
                                                 s.fragility_modes.tag,
                                                 s.fragility_curves.curve_id,
                                                 s.fragility_curves.curve_type,
                                                 s.linear_fragility_curves,
                                                 s.tabular_fragility_curves,
                                                 s.the_model.dist_sys,
                                                 scenarioOffset_s,
                                                 scenarioOffset_s + scenarioDuration_s,
                                                 intensityIdToAmount,
                                                 relSchByCompId,
                                                 verbose,
                                                 log);
            if (verbose)
            {
                Log_info(log, fmt::format("Reliabilities for Scenario: {}", scenarioTag));
                Log_info(log, fmt::format("Occurrence #{}", occIdx + 1));
                for (std::string const& reliabilityStrings :
                     ReliabilitiesToStrings(s.the_model.reliability))
                {
                    Log_info(log, reliabilityStrings);
                }
            }
            if (saveReliabilityCurves)
            {
                if (verbose)
                {
                    Log_debug(log, "Writing reliability curves...");
                }
                WriteReliabilityCurves(s.scenario_map.tag[scenIdx], occIdx, s);
                if (verbose)
                {
                    Log_debug(log, "Reliability curves written");
                }
            }
            std::string scenarioStartTimeTag =
                time_to_ISO8601_period(static_cast<uint64_t>(std::llround(t)));
            if (verbose)
            {
                Log_info(log,
                         fmt::format("Running {} from {} for {} {}",
                                     s.scenario_map.tag[scenIdx],
                                     scenarioStartTimeTag,
                                     s.scenario_map.duration[scenIdx],
                                     time_unit_to_tag(s.scenario_map.time_unit[scenIdx])));
                Log_info(log,
                         fmt::format("time: {} to {}",
                                     seconds_to_pretty_string(t),
                                     seconds_to_pretty_string(tEnd)));
            }
            s.the_model.final_time_s = scenarioDuration_s;
            // TODO: add an optional verbosity flag to SimInfo
            // -- use that to set things like the print flag below

            auto results = Simulate(s.the_model, verbose, true, log);
            {
                auto* output_results = &results;
                std::vector<TimeAndFlows> modified_results0 = {};
                if (time_step_h > 0.0)
                {
                    modified_results0 = ApplyUniformTimeStep(results, time_step_h);
                    output_results = &modified_results0;
                }

                std::vector<TimeAndFlows> modified_results1 = *output_results;
                AggregateGroups(modified_results1, nodeConnections);
                output_results = &modified_results1;

                // TODO: investigate putting output on another thread
                WriteResultsToEventFile(out,
                                        *output_results,
                                        s,
                                        scenarioTag,
                                        scenarioStartTimeTag,
                                        nodeConnOrderForEvents,
                                        storeOrderForEvents,
                                        compOrderForEvents,
                                        outputTimeUnit);
            }
            ScenarioOccurrenceStats sos = ModelResults_CalculateScenarioOccurrenceStats(
                scenIdx, occIdx + 1, s.the_model, s.flow_type_map, results);
            occurrenceStats.push_back(std::move(sos));
        }
        if (verbose)
        {
            Log_info(log, fmt::format("Scenario {} finished", scenarioTag));
        }
        // TODO: merge per-occurrence stats with global for the current
        // scenario
    }
    out.close();
    WriteStatisticsToFile(s, statsFilename, occurrenceStats, compOrder, failOrder, fragOrder);
}
} // namespace erin
