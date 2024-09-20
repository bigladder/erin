/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_simulation.h"
#include "erin/logging.h"
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_timestate.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_utils.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_random.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_toml.h"
#include "fmt/format.h"
#include <fmt/core.h>
#include <cassert>
#include <fstream>
#include <ios>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <cstdlib>

namespace erin
{
    void
    Simulation_Init(Simulation& s)
    {
        // NOTE: we register a 'null' flow. This allows users to 'opt-out'
        // of flow specification by passing empty strings. Effectively, this
        // allows any connections to occur which is nice for simple examples.
        Simulation_RegisterFlow(s, "");
    }

    size_t
    Simulation_RegisterFlow(Simulation& s, std::string const& flowTag)
    {
        size_t id = s.FlowTypeMap.Type.size();
        for (size_t i = 0; i < id; ++i)
        {
            if (s.FlowTypeMap.Type[i] == flowTag)
            {
                return i;
            }
        }
        s.FlowTypeMap.Type.push_back(flowTag);
        return id;
    }

    size_t
    Simulation_RegisterScenario(Simulation& s, std::string const& scenarioTag)
    {
        return ScenarioDict_RegisterScenario(s.ScenarioMap, scenarioTag);
    }

    size_t
    Simulation_RegisterIntensity(Simulation& s, std::string const& tag)
    {
        for (size_t i = 0; i < s.Intensities.Tags.size(); ++i)
        {
            if (s.Intensities.Tags[i] == tag)
            {
                return i;
            }
        }
        size_t id = s.Intensities.Tags.size();
        s.Intensities.Tags.push_back(tag);
        return id;
    }

    size_t
    Simulation_RegisterIntensityLevelForScenario(
        Simulation& s,
        size_t scenarioId,
        size_t intensityId,
        double intensityLevel
    )
    {
        for (size_t i = 0; i < s.ScenarioIntensities.IntensityIds.size(); ++i)
        {
            if (s.ScenarioIntensities.IntensityIds[i] == intensityId
                && s.ScenarioIntensities.ScenarioIds[i] == scenarioId)
            {
                s.ScenarioIntensities.IntensityLevels[i] = intensityLevel;
                return i;
            }
        }
        size_t id = s.ScenarioIntensities.IntensityIds.size();
        s.ScenarioIntensities.ScenarioIds.push_back(scenarioId);
        s.ScenarioIntensities.IntensityIds.push_back(intensityId);
        s.ScenarioIntensities.IntensityLevels.push_back(intensityLevel);
        return id;
    }

    size_t
    Simulation_RegisterLoadSchedule(
        Simulation& s,
        std::string const& tag,
        std::vector<TimeAndAmount> const& loadSchedule
    )
    {
        size_t id = s.LoadMap.Tags.size();
        assert(s.LoadMap.Tags.size() == s.LoadMap.Loads.size());
        for (size_t i = 0; i < id; ++i)
        {
            if (s.LoadMap.Tags[i] == tag)
            {
                s.LoadMap.Loads[i].clear();
                s.LoadMap.Loads[i] = loadSchedule;
                return i;
            }
        }
        s.LoadMap.Tags.push_back(tag);
        s.LoadMap.Loads.push_back(loadSchedule);
        return id;
    }

    std::optional<size_t>
    Simulation_GetLoadIdByTag(Simulation const& s, std::string const& tag)
    {
        for (size_t i = 0; i < s.LoadMap.Tags.size(); ++i)
        {
            if (s.LoadMap.Tags[i] == tag)
            {
                return i;
            }
        }
        return {};
    }

    void
    Simulation_RegisterAllLoads(Simulation& s, std::vector<Load> const& loads)
    {
        s.LoadMap.Tags.clear();
        s.LoadMap.Loads.clear();
        auto numLoads = loads.size();
        s.LoadMap.Tags.reserve(numLoads);
        s.LoadMap.Loads.reserve(numLoads);
        for (size_t i = 0; i < numLoads; ++i)
        {
            s.LoadMap.Tags.push_back(loads[i].Tag);
            s.LoadMap.Loads.push_back(loads[i].TimeAndLoads);
        }
    }

    void
    Simulation_PrintComponents(Simulation const& s)
    {
        Model const& m = s.TheModel;
        for (size_t compId = 0; compId < m.ComponentMap.CompType.size();
             ++compId)
        {
            assert(compId < m.ComponentMap.OutflowType.size());
            assert(compId < m.ComponentMap.InflowType.size());
            assert(compId < m.ComponentMap.CompType.size());
            assert(compId < m.ComponentMap.Tag.size());
            assert(compId < m.ComponentMap.Idx.size());
            std::vector<size_t> const& outflowTypes =
                m.ComponentMap.OutflowType[compId];
            std::vector<size_t> inflowTypes = m.ComponentMap.InflowType[compId];
            std::cout << compId << ": "
                      << ToString(m.ComponentMap.CompType[compId]);
            if (!m.ComponentMap.Tag[compId].empty())
            {
                std::cout << " -- " << m.ComponentMap.Tag[compId] << std::endl;
            }
            else
            {
                std::cout << std::endl;
            }
            for (size_t inportIdx = 0; inportIdx < inflowTypes.size();
                 ++inportIdx)
            {
                size_t inflowType = inflowTypes[inportIdx];
                if (inflowType < s.FlowTypeMap.Type.size()
                    && !s.FlowTypeMap.Type[inflowType].empty())
                {
                    std::cout << "- inport " << inportIdx << ": "
                              << s.FlowTypeMap.Type[inflowType] << std::endl;
                }
            }
            for (size_t outportIdx = 0; outportIdx < outflowTypes.size();
                 ++outportIdx)
            {
                size_t outflowType = outflowTypes[outportIdx];
                if (outflowType < s.FlowTypeMap.Type.size()
                    && !s.FlowTypeMap.Type[outflowType].empty())
                {
                    std::cout << "- outport " << outportIdx << ": "
                              << s.FlowTypeMap.Type[outflowType] << std::endl;
                }
            }
            size_t subtypeIdx = m.ComponentMap.Idx[compId];
            switch (m.ComponentMap.CompType[compId])
            {
                case ComponentType::ScheduleBasedLoadType:
                {
                    assert(subtypeIdx < m.ScheduledLoads.size());
                    ScheduleBasedLoad const& sbl = m.ScheduledLoads[subtypeIdx];
                    for (auto const& keyValue : sbl.ScenarioIdToLoadId)
                    {
                        size_t scenarioIdx = keyValue.first;
                        size_t loadIdx = keyValue.second;
                        assert(scenarioIdx < s.ScenarioMap.Tags.size());
                        assert(loadIdx < s.LoadMap.Tags.size());
                        std::cout << "-- for scenario: "
                                  << s.ScenarioMap.Tags[scenarioIdx]
                                  << ", use load: " << s.LoadMap.Tags[loadIdx]
                                  << std::endl;
                    }
                }
                break;
                case ComponentType::ScheduleBasedSourceType:
                {
                    assert(subtypeIdx < m.ScheduledSrcs.size());
                    ScheduleBasedSource const& sbs =
                        m.ScheduledSrcs[subtypeIdx];
                    for (auto const& keyValue : sbs.ScenarioIdToSourceId)
                    {
                        size_t scenarioIdx = keyValue.first;
                        size_t loadIdx = keyValue.second;
                        assert(scenarioIdx < s.ScenarioMap.Tags.size());
                        assert(loadIdx < s.LoadMap.Tags.size());
                        std::cout << "-- for scenario: "
                                  << s.ScenarioMap.Tags[scenarioIdx]
                                  << ", use supply: " << s.LoadMap.Tags[loadIdx]
                                  << std::endl;
                    }
                    std::cout << "-- max outflow (W): "
                              << (sbs.MaxOutflow_W == max_flow_W
                                      ? "unlimited"
                                      : std::to_string(sbs.MaxOutflow_W))
                              << std::endl;
                }
                break;
                case ComponentType::ConstantEfficiencyConverterType:
                {
                    assert(subtypeIdx < m.ConstEffConvs.size());
                    ConstantEfficiencyConverter const& cec =
                        m.ConstEffConvs[subtypeIdx];
                    std::cout << "-- efficiency: " << cec.Efficiency * 100.0
                              << "%" << std::endl;
                    std::cout << "-- max outflow (W): "
                              << (cec.MaxOutflow_W == max_flow_W
                                      ? "unlimited"
                                      : std::to_string(cec.MaxOutflow_W))
                              << std::endl;
                    std::cout << "-- max lossflow (W): "
                              << (cec.MaxLossflow_W == max_flow_W
                                      ? "unlimited"
                                      : std::to_string(cec.MaxLossflow_W))
                              << std::endl;
                }
                break;
                case ComponentType::VariableEfficiencyConverterType:
                {
                    assert(subtypeIdx < m.VarEffConvs.size());
                    VariableEfficiencyConverter const& vec =
                        m.VarEffConvs[subtypeIdx];
                    std::cout << "-- efficiencies by load fraction:"
                              << std::endl;
                    auto maxOutflow_W = static_cast<double>(vec.MaxOutflow_W);
                    for (size_t i = 0; i < vec.Efficiencies.size(); ++i)
                    {
                        std::cout << fmt::format(
                            "  -- {:5.3f}",
                            (vec.OutflowsForEfficiency_W[i] / maxOutflow_W)
                        );
                        std::cout << fmt::format(
                            ": {:5.2f}%", (vec.Efficiencies[i] * 100.0)
                        ) << std::endl;
                    }
                    std::cout << "-- max outflow (W): "
                              << (vec.MaxOutflow_W == max_flow_W
                                      ? "unlimited"
                                      : std::to_string(vec.MaxOutflow_W))
                              << std::endl;
                    std::cout << "-- max lossflow (W): "
                              << (vec.MaxLossflow_W == max_flow_W
                                      ? "unlimited"
                                      : std::to_string(vec.MaxLossflow_W))
                              << std::endl;
                }
                break;
                case ComponentType::MoverType:
                {
                    assert(subtypeIdx < m.Movers.size());
                    Mover const& mov = m.Movers[subtypeIdx];
                    std::cout << "-- cop: " << mov.COP << std::endl;
                    std::cout << "-- max outflow (W): "
                              << (mov.MaxOutflow_W == max_flow_W
                                      ? "unlimited"
                                      : std::to_string(mov.MaxOutflow_W))
                              << std::endl;
                }
                break;
                case ComponentType::VariableEfficiencyMoverType:
                {
                    assert(subtypeIdx < m.VarEffMovers.size());
                    VariableEfficiencyMover const& mov =
                        m.VarEffMovers[subtypeIdx];
                    std::cout << "-- cop by load fraction:" << std::endl;
                    auto maxOutflow_W = static_cast<double>(mov.MaxOutflow_W);
                    for (size_t i = 0; i < mov.COPs.size(); ++i)
                    {
                        std::cout << fmt::format(
                            " -- {:5.3f}",
                            (mov.OutflowsForCop_W[i] / maxOutflow_W)
                        );
                        std::cout << fmt::format(": {:5.2f}", (mov.COPs[i]))
                                  << std::endl;
                    }
                    std::cout << "-- max outflow (W): "
                              << (mov.MaxOutflow_W == max_flow_W
                                      ? "unlimited"
                                      : std::to_string(maxOutflow_W))
                              << std::endl;
                }
                break;
                case ComponentType::StoreType:
                {
                    assert(subtypeIdx < m.Stores.size());
                    Store const& store = m.Stores[subtypeIdx];
                    std::cout << "-- capacity (J): " << store.Capacity_J
                              << std::endl;
                    std::cout << "-- initial SOC: "
                              << (static_cast<double>(store.InitialStorage_J)
                                  / static_cast<double>(store.Capacity_J))
                              << std::endl;
                    std::cout << "-- initial capacity (J): " << store.Capacity_J
                              << std::endl;
                    std::cout << "-- SOC to start charging: "
                              << (static_cast<double>(store.ChargeAmount_J)
                                  / static_cast<double>(store.Capacity_J))
                              << std::endl;
                    std::cout
                        << "-- max charge rate (W): " << store.MaxChargeRate_W
                        << std::endl;
                    std::cout << "-- max discharge rate (W): "
                              << store.MaxDischargeRate_W << std::endl;
                    std::cout << "-- max outflow (W): "
                              << (store.MaxOutflow_W == max_flow_W
                                      ? "unlimited"
                                      : std::to_string(store.MaxOutflow_W))
                              << std::endl;
                    std::cout << "-- roundtrip efficiency: "
                              << store.RoundTripEfficiency * 100.0 << "%"
                              << std::endl;
                }
                break;
                case ComponentType::PassThroughType:
                {
                    assert(subtypeIdx < m.PassThroughs.size());
                    PassThrough const& pt = m.PassThroughs[subtypeIdx];
                    std::cout << "-- max outflow (W): "
                              << (pt.MaxOutflow_W == max_flow_W
                                      ? "unlimited"
                                      : std::to_string(pt.MaxOutflow_W))
                              << std::endl;
                }
                break;
                default:
                {
                }
                break;
            }
            for (size_t compFailModeIdx = 0;
                 compFailModeIdx < s.ComponentFailureModes.ComponentIds.size();
                 ++compFailModeIdx)
            {
                if (s.ComponentFailureModes.ComponentIds[compFailModeIdx]
                    == compId)
                {
                    size_t fmId =
                        s.ComponentFailureModes.FailureModeIds[compFailModeIdx];
                    std::cout
                        << "-- failure-mode: " << s.FailureModes.Tags[fmId]
                        << "[" << fmId << "]" << std::endl;
                }
            }
            for (size_t compFragIdx = 0;
                 compFragIdx < s.ComponentFragilities.ComponentIds.size();
                 ++compFragIdx)
            {
                if (s.ComponentFragilities.ComponentIds[compFragIdx] == compId)
                {
                    size_t fmId =
                        s.ComponentFragilities.FragilityModeIds[compFragIdx];
                    std::cout
                        << "-- fragility mode: " << s.FragilityModes.Tags[fmId]
                        << "[" << fmId << "]" << std::endl;
                }
            }
        }
    }

    void
    Simulation_PrintFragilityCurves(Simulation const& s)
    {
        assert(
            s.FragilityCurves.CurveId.size()
            == s.FragilityCurves.CurveTypes.size()
        );
        assert(
            s.FragilityCurves.CurveId.size() == s.FragilityCurves.Tags.size()
        );
        for (size_t i = 0; i < s.FragilityCurves.CurveId.size(); ++i)
        {
            std::cout << i << ": "
                      << FragilityCurveTypeToTag(s.FragilityCurves.CurveTypes[i]
                         )
                      << " -- " << s.FragilityCurves.Tags[i] << std::endl;
            size_t idx = s.FragilityCurves.CurveId[i];
            switch (s.FragilityCurves.CurveTypes[i])
            {
                case (FragilityCurveType::Linear):
                {
                    std::cout << "-- lower bound: "
                              << s.LinearFragilityCurves[idx].LowerBound
                              << std::endl;
                    std::cout << "-- upper bound: "
                              << s.LinearFragilityCurves[idx].UpperBound
                              << std::endl;
                    size_t intensityId =
                        s.LinearFragilityCurves[idx].VulnerabilityId;
                    std::cout << "-- vulnerable to: "
                              << s.Intensities.Tags[intensityId] << "["
                              << intensityId << "]" << std::endl;
                }
                break;
                case (FragilityCurveType::Tabular):
                {
                    size_t size =
                        s.TabularFragilityCurves[idx].Intensities.size();
                    size_t intensityId =
                        s.TabularFragilityCurves[idx].VulnerabilityId;
                    if (size > 0)
                    {
                        std::cout
                            << "-- intensity from "
                            << s.TabularFragilityCurves[idx].Intensities[0]
                            << " to "
                            << s.TabularFragilityCurves[idx]
                                   .Intensities[size - 1]
                            << std::endl;
                        std::cout << "-- vulnerable to: "
                                  << s.Intensities.Tags[intensityId] << "["
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

    void
    Simulation_PrintFailureModes(Simulation const& s)
    {
        for (size_t i = 0; i < s.FailureModes.Tags.size(); ++i)
        {
            auto maybeFailureDist = s.TheModel.DistSys.get_dist_by_id(
                s.FailureModes.FailureDistIds[i]
            );
            auto maybeRepairDist = s.TheModel.DistSys.get_dist_by_id(
                s.FailureModes.RepairDistIds[i]
            );
            std::cout << i << ": " << s.FailureModes.Tags[i] << std::endl;
            if (maybeFailureDist.has_value())
            {
                Distribution const& failureDist = maybeFailureDist.value();
                std::cout << "-- failure distribution: " << failureDist.Tag
                          << ", " << dist_type_to_tag(failureDist.Type) << "["
                          << s.FailureModes.FailureDistIds[i] << "]"
                          << std::endl;
            }
            else
            {
                std::cout << "-- ERROR! Problem finding failure distribution "
                          << " with id = " << s.FailureModes.FailureDistIds[i]
                          << std::endl;
            }
            if (maybeRepairDist.has_value())
            {
                Distribution const& repairDist = maybeRepairDist.value();
                std::cout << "-- repair distribution: " << repairDist.Tag
                          << ", " << dist_type_to_tag(repairDist.Type) << "["
                          << s.FailureModes.RepairDistIds[i] << "]"
                          << std::endl;
            }
            else
            {
                std::cout << "-- ERROR! Problem finding repair distribution "
                          << " with id = " << s.FailureModes.RepairDistIds[i]
                          << std::endl;
            }
        }
    }

    void
    Simulation_PrintComponentFailureModes(Simulation const& s)
    {
        for (size_t i = 0; i < s.ComponentFailureModes.ComponentIds.size(); ++i)
        {
            size_t compId = s.ComponentFailureModes.ComponentIds[i];
            size_t fmId = s.ComponentFailureModes.FailureModeIds[i];
            std::cout << "[" << i
                      << "]: component=" << s.TheModel.ComponentMap.Tag[compId]
                      << "[" << compId
                      << "]; failure mode=" << s.FailureModes.Tags[fmId] << "["
                      << fmId << "]" << std::endl;
        }
    }

    void
    Simulation_PrintFragilityModes(Simulation const& s)
    {
        for (size_t i = 0; i < s.FragilityModes.Tags.size(); ++i)
        {
            std::cout << i << ": " << s.FragilityModes.Tags[i] << std::endl;
            std::cout
                << "-- fragility curve: "
                << s.FragilityCurves.Tags[s.FragilityModes.FragilityCurveId[i]]
                << "[" << s.FragilityModes.FragilityCurveId[i] << "]"
                << std::endl;
            if (s.FragilityModes.RepairDistIds[i].has_value())
            {
                std::optional<Distribution> maybeDist =
                    s.TheModel.DistSys.get_dist_by_id(
                        s.FragilityModes.RepairDistIds[i].value()
                    );
                if (maybeDist.has_value())
                {
                    Distribution const& d = maybeDist.value();
                    std::cout << "-- repair dist: " << d.Tag << "["
                              << s.FragilityModes.RepairDistIds[i].value()
                              << "]" << std::endl;
                }
            }
        }
    }

    void
    Simulation_PrintComponentFragilityModes(Simulation const& s)
    {
        for (size_t i = 0; i < s.ComponentFragilities.ComponentIds.size(); ++i)
        {
            size_t compId = s.ComponentFragilities.ComponentIds[i];
            size_t fmId = s.ComponentFragilities.FragilityModeIds[i];
            std::cout << "[" << i
                      << "]: component=" << s.TheModel.ComponentMap.Tag[compId]
                      << "[" << s.ComponentFragilities.ComponentIds[i]
                      << "]; fragility mode=" << s.FragilityModes.Tags[fmId]
                      << "[" << fmId << "]" << std::endl;
        }
    }

    void
    Simulation_PrintScenarios(Simulation const& s)
    {
        for (size_t i = 0; i < s.ScenarioMap.Tags.size(); ++i)
        {
            std::cout << i << ": " << s.ScenarioMap.Tags[i] << std::endl;
            std::cout << "- duration: " << s.ScenarioMap.Durations[i] << " "
                      << TimeUnitToTag(s.ScenarioMap.TimeUnits[i]) << std::endl;
            std::cout << "- offset: "
                      << TimeInSecondsToDesiredUnit(
                             s.ScenarioMap.TimeOffsetsInSeconds[i],
                             TimeUnit::Hour
                         )
                      << " " << TimeUnitToTag(TimeUnit::Hour) << std::endl;
            auto maybeDist = s.TheModel.DistSys.get_dist_by_id(
                s.ScenarioMap.OccurrenceDistributionIds[i]
            );
            if (maybeDist.has_value())
            {
                Distribution const& d = maybeDist.value();
                std::cout << "- occurrence distribution: "
                          << dist_type_to_tag(d.Type) << "["
                          << s.ScenarioMap.OccurrenceDistributionIds[i]
                          << "] -- " << d.Tag << std::endl;
            }
            std::cout << "- max occurrences: ";
            if (s.ScenarioMap.MaxOccurrences[i].has_value())
            {
                std::cout << s.ScenarioMap.MaxOccurrences[i].value()
                          << std::endl;
            }
            else
            {
                std::cout << "no limit" << std::endl;
            }
            bool printedHeader = false;
            for (size_t siIdx = 0;
                 siIdx < s.ScenarioIntensities.IntensityIds.size();
                 ++siIdx)
            {
                if (s.ScenarioIntensities.ScenarioIds[siIdx] == i)
                {
                    if (!printedHeader)
                    {
                        std::cout << "- intensities:" << std::endl;
                        printedHeader = true;
                    }
                    auto intId = s.ScenarioIntensities.IntensityIds[siIdx];
                    auto const& intTag = s.Intensities.Tags[intId];
                    std::cout
                        << "-- " << intTag << "[" << intId
                        << "]: " << s.ScenarioIntensities.IntensityLevels[siIdx]
                        << std::endl;
                }
            }
        }
    }

    void
    Simulation_PrintLoads(Simulation const& s)
    {
        for (size_t i = 0; i < s.LoadMap.Tags.size(); ++i)
        {
            std::cout << i << ": " << s.LoadMap.Tags[i] << std::endl;
            std::cout << "- load entries: " << s.LoadMap.Loads[i].size()
                      << std::endl;
            if (!s.LoadMap.Loads[i].empty())
            {
                // TODO: add time units
                std::cout << "- initial time: " << s.LoadMap.Loads[i][0].Time_s
                          << std::endl;
                // TODO: add time units
                std::cout
                    << "- final time  : "
                    << s.LoadMap.Loads[i][s.LoadMap.Loads[i].size() - 1].Time_s
                    << std::endl;
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

    size_t
    Simulation_ScenarioCount(Simulation const& s)
    {
        return s.ScenarioMap.Tags.size();
    }

    Result
    Simulation_ParseSimulationInfo(
        Simulation& s,
        toml::value const& v,
        ValidationInfo const& validationInfo
    )
    {
        if (!v.contains("simulation_info"))
        {
            WriteErrorMessage(
                "simulation_info",
                "Required section [simulation_info] not found"
            );
            return Result::Failure;
        }
        toml::value const& simInfoValue = v.at("simulation_info");
        if (!simInfoValue.is_table())
        {
            WriteErrorMessage(
                "simulation_info",
                "Required section [simulation_info] is not a table"
            );
            return Result::Failure;
        }
        toml::table const& simInfoTable = simInfoValue.as_table();
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::unordered_map<std::string, InputValue> inputs =
            TOMLTable_ParseWithValidation(
                simInfoTable,
                validationInfo,
                "simulation_info",
                errors,
                warnings
            );
        if (warnings.size() > 0)
        {
            std::cout << "WARNINGS:" << std::endl;
            for (auto const& w : warnings)
            {
                std::cerr << w << std::endl;
            }
        }
        if (errors.size() > 0)
        {
            std::cout << "ERRORS:" << std::endl;
            for (auto const& err : errors)
            {
                std::cerr << err << std::endl;
            }
            return Result::Failure;
        }
        auto maybeSimInfo = ParseSimulationInfo(inputs);
        if (!maybeSimInfo.has_value())
        {
            return Result::Failure;
        }
        s.Info = std::move(maybeSimInfo.value());
        return Result::Success;
    }

    Result
    Simulation_ParseLoads(
        Simulation& s,
        toml::value const& v,
        ValidationInfo const& explicitValidation,
        ValidationInfo const& fileValidation
    )
    {
        toml::value const& loadTable = v.at("loads");
        auto maybeLoads = ParseLoads(
            loadTable.as_table(), explicitValidation, fileValidation
        );
        if (!maybeLoads.has_value())
        {
            return Result::Failure;
        }
        std::vector<Load> loads = std::move(maybeLoads.value());
        Simulation_RegisterAllLoads(s, loads);
        return Result::Success;
    }

    // TODO: change this to a std::optional<size_t> GetFragilityCurveByTag()
    // if it returns !*.has_value(), register with the bogus data explicitly.
    size_t
    Simulation_RegisterFragilityCurve(Simulation& s, std::string const& tag)
    {
        return Simulation_RegisterFragilityCurve(
            s, tag, FragilityCurveType::Linear, 0
        );
    }

    size_t
    Simulation_RegisterFragilityCurve(
        Simulation& s,
        std::string const& tag,
        FragilityCurveType curveType,
        size_t curveIdx
    )
    {
        for (size_t i = 0; i < s.FragilityCurves.Tags.size(); ++i)
        {
            if (s.FragilityCurves.Tags[i] == tag)
            {
                s.FragilityCurves.CurveId[i] = curveIdx;
                s.FragilityCurves.CurveTypes[i] = curveType;
                return i;
            }
        }
        size_t id = s.FragilityCurves.Tags.size();
        s.FragilityCurves.Tags.push_back(tag);
        s.FragilityCurves.CurveId.push_back(curveIdx);
        s.FragilityCurves.CurveTypes.push_back(curveType);
        return id;
    }

    size_t
    Simulation_RegisterFailureMode(
        Simulation& s,
        std::string const& tag,
        size_t failureId,
        size_t repairId
    )
    {
        size_t size = s.FailureModes.Tags.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (s.FailureModes.Tags[i] == tag)
            {
                s.FailureModes.FailureDistIds[i] = failureId;
                s.FailureModes.RepairDistIds[i] = repairId;
                return i;
            }
        }
        size_t result = size;
        s.FailureModes.Tags.push_back(tag);
        s.FailureModes.FailureDistIds.push_back(failureId);
        s.FailureModes.RepairDistIds.push_back(repairId);
        return result;
    }

    size_t
    Simulation_RegisterFragilityMode(
        Simulation& s,
        std::string const& tag,
        size_t fragilityCurveId,
        std::optional<size_t> maybeRepairDistId
    )
    {
        size_t size = s.FragilityModes.Tags.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (s.FragilityModes.Tags[i] == tag)
            {
                s.FragilityModes.FragilityCurveId[i] = fragilityCurveId;
                s.FragilityModes.RepairDistIds[i] = maybeRepairDistId;
                return i;
            }
        }
        size_t result = size;
        s.FragilityModes.Tags.push_back(tag);
        s.FragilityModes.FragilityCurveId.push_back(fragilityCurveId);
        s.FragilityModes.RepairDistIds.push_back(maybeRepairDistId);
        return result;
    }

    std::optional<size_t>
    Parse_VulnerableTo(
        Simulation const& s,
        toml::table const& fcData,
        std::string const& tableFullName
    )
    {
        if (!fcData.contains("vulnerable_to"))
        {
            WriteErrorMessage(
                tableFullName, "missing required field 'vulnerable_to'"
            );
            return {};
        }
        if (!fcData.at("vulnerable_to").is_string())
        {
            WriteErrorMessage(
                tableFullName, "field 'vulnerable_to' not a string"
            );
            return {};
        }
        std::string const& vulnerStr = fcData.at("vulnerable_to").as_string();
        std::optional<size_t> maybeIntId =
            GetIntensityIdByTag(s.Intensities, vulnerStr);
        if (!maybeIntId.has_value())
        {
            WriteErrorMessage(
                tableFullName,
                "could not find referenced intensity '" + vulnerStr
                    + "' for 'vulnerable_to'"
            );
            return {};
        }
        return maybeIntId;
    }

    Result
    Simulation_ParseLinearFragilityCurve(
        Simulation& s,
        std::string const& fcName,
        std::string const& tableFullName,
        toml::table const& fcData
    )
    {
        if (!fcData.contains("lower_bound"))
        {
            std::cout << "[" << tableFullName << "] "
                      << "missing required field 'lower_bound'" << std::endl;
            return Result::Failure;
        }
        if (!(fcData.at("lower_bound").is_floating()
              || fcData.at("lower_bound").is_integer()))
        {
            std::cout << "[" << tableFullName << "] "
                      << "field 'lower_bound' not a number" << std::endl;
            return Result::Failure;
        }
        std::optional<double> maybeLowerBound =
            TOMLTable_ParseDouble(fcData, "lower_bound", tableFullName);
        if (!maybeLowerBound.has_value())
        {
            std::cout << "[" << tableFullName << "] "
                      << "field 'lower_bound' has no value" << std::endl;
            return Result::Failure;
        }
        double lowerBound = maybeLowerBound.value();
        if (!fcData.contains("upper_bound"))
        {
            std::cout << "[" << tableFullName << "] "
                      << "missing required field 'upper_bound'" << std::endl;
            return Result::Failure;
        }
        if (!(fcData.at("upper_bound").is_floating()
              || fcData.at("upper_bound").is_integer()))
        {
            std::cout << "[" << tableFullName << "] "
                      << "field 'upper_bound' not a number" << std::endl;
            return Result::Failure;
        }
        std::optional<double> maybeUpperBound =
            TOMLTable_ParseDouble(fcData, "upper_bound", tableFullName);
        if (!maybeUpperBound.has_value())
        {
            std::cout << "[" << tableFullName << "] "
                      << "field 'upper_bound' has no value" << std::endl;
            return Result::Failure;
        }
        double upperBound = maybeUpperBound.value();
        std::optional<size_t> maybeIntId =
            Parse_VulnerableTo(s, fcData, tableFullName);
        if (!maybeIntId.has_value())
        {
            return Result::Failure;
        }
        size_t intensityId = maybeIntId.value();
        LinearFragilityCurve lfc{};
        lfc.LowerBound = lowerBound;
        lfc.UpperBound = upperBound;
        lfc.VulnerabilityId = intensityId;
        size_t idx = s.LinearFragilityCurves.size();
        s.LinearFragilityCurves.push_back(std::move(lfc));
        Simulation_RegisterFragilityCurve(
            s, fcName, FragilityCurveType::Linear, idx
        );
        return Result::Success;
    }

    Result
    Simulation_ParseFragilityCurves(Simulation& s, toml::value const& v)
    {
        if (v.contains("fragility_curve"))
        {
            if (!v.at("fragility_curve").is_table())
            {
                std::cout << "[fragility_curve] not a table" << std::endl;
                return Result::Failure;
            }
            for (auto const& pair : v.at("fragility_curve").as_table())
            {
                std::string const& fcName = pair.first;
                std::string tableFullName = "fragility_curve." + fcName;
                if (!pair.second.is_table())
                {
                    std::cout << "[" << tableFullName << "] not a table"
                              << std::endl;
                    return Result::Failure;
                }
                toml::table const& fcData = pair.second.as_table();
                if (!fcData.contains("type"))
                {
                    std::cout << "[" << tableFullName << "] "
                              << "does not contain required value 'type'"
                              << std::endl;
                    return Result::Failure;
                }
                std::string const& typeStr = fcData.at("type").as_string();
                std::optional<FragilityCurveType> maybeFct =
                    TagToFragilityCurveType(typeStr);
                if (!maybeFct.has_value())
                {
                    std::cout << "[" << tableFullName << "] "
                              << "could not interpret type as string"
                              << std::endl;
                    return Result::Failure;
                }
                FragilityCurveType fct = maybeFct.value();
                switch (fct)
                {
                    case (FragilityCurveType::Linear):
                    {
                        if (Simulation_ParseLinearFragilityCurve(
                                s, fcName, tableFullName, fcData
                            )
                            == Result::Failure)
                        {
                            return Result::Failure;
                        }
                    }
                    break;
                    case (FragilityCurveType::Tabular):
                    {
                        std::optional<size_t> maybeIntId =
                            Parse_VulnerableTo(s, fcData, tableFullName);
                        if (!maybeIntId.has_value())
                        {
                            return Result::Failure;
                        }
                        size_t intensityId = maybeIntId.value();
                        auto maybePairs = TOMLTable_ParseArrayOfPairsOfDouble(
                            fcData, "intensity_failure_pairs", tableFullName
                        );
                        if (!maybePairs.has_value())
                        {
                            return Result::Failure;
                        }
                        PairsVector pv = maybePairs.value();
                        TabularFragilityCurve tfc{};
                        tfc.VulnerabilityId = intensityId;
                        tfc.Intensities = std::move(pv.Firsts);
                        tfc.FailureFractions = std::move(pv.Seconds);
                        size_t subtypeIdx = s.TabularFragilityCurves.size();
                        s.TabularFragilityCurves.push_back(std::move(tfc));
                        Simulation_RegisterFragilityCurve(
                            s, fcName, FragilityCurveType::Tabular, subtypeIdx
                        );
                    }
                    break;
                    default:
                    {
                        WriteErrorMessage(
                            tableFullName,
                            "unhandled fragility curve type '" + typeStr + "'"
                        );
                        std::exit(1);
                    }
                    break;
                }
            }
        }
        return Result::Success;
    }

    bool
    Simulation_IsFailureModeNameUnique(Simulation& s, std::string const& name)
    {
        for (std::string const& tag : s.FailureModes.Tags)
        {
            if (tag == name)
            {
                return false;
            }
        }
        return true;
    }

    bool
    Simulation_IsFragilityModeNameUnique(Simulation& s, std::string const& name)
    {
        for (size_t i = 0; i < s.FragilityModes.Tags.size(); ++i)
        {
            if (s.FragilityModes.Tags[i] == name)
            {
                return false;
            }
        }
        return true;
    }

    bool
    Simulation_IsFailureNameUnique(Simulation& s, std::string const& name)
    {
        return Simulation_IsFailureModeNameUnique(s, name)
            && Simulation_IsFragilityModeNameUnique(s, name);
    }

    Result
    Simulation_ParseFailureModes(Simulation& s, toml::value const& v)
    {
        if (v.contains("failure_mode"))
        {
            if (!v.at("failure_mode").is_table())
            {
                WriteErrorMessage(
                    "failure_mode", "failure_mode section must be a table"
                );
                return Result::Failure;
            }
            toml::table const& fmTable = v.at("failure_mode").as_table();
            for (auto const& pair : fmTable)
            {
                std::string const& fmName = pair.first;
                std::string const fullName = "failue_mode." + fmName;
                if (!Simulation_IsFragilityModeNameUnique(s, fmName))
                {
                    WriteErrorMessage(
                        fmName,
                        "failure mode name must be unique within both "
                        "failure_mode and fragility_mode names"
                    );
                    return Result::Failure;
                }
                if (!pair.second.is_table())
                {
                    WriteErrorMessage(fullName, "value must be a table");
                    return Result::Failure;
                }
                toml::table const& fmValueTable = pair.second.as_table();
                if (!fmValueTable.contains("failure_dist"))
                {
                    WriteErrorMessage(
                        fullName, "missing required field 'failure_dist'"
                    );
                    return Result::Failure;
                }
                auto maybeFailureDistTag = TOMLTable_ParseString(
                    fmValueTable, "failure_dist", fullName
                );
                if (!maybeFailureDistTag.has_value())
                {
                    WriteErrorMessage(
                        fullName, "could not parse 'failure_dist' as string"
                    );
                    return Result::Failure;
                }
                std::string const& failureDistTag = maybeFailureDistTag.value();
                if (!fmValueTable.contains("repair_dist"))
                {
                    WriteErrorMessage(
                        fullName, "missing required field 'repair_dist'"
                    );
                    return Result::Failure;
                }
                auto maybeRepairDistTag = TOMLTable_ParseString(
                    fmValueTable, "repair_dist", fullName
                );
                if (!maybeRepairDistTag.has_value())
                {
                    WriteErrorMessage(
                        fullName, "could not parse 'repair_dist' as string"
                    );
                    return Result::Failure;
                }
                std::string const& repairDistTag = maybeRepairDistTag.value();
                size_t failureId =
                    s.TheModel.DistSys.lookup_dist_by_tag(failureDistTag);
                size_t repairId =
                    s.TheModel.DistSys.lookup_dist_by_tag(repairDistTag);
                Simulation_RegisterFailureMode(s, fmName, failureId, repairId);
            }
        }
        return Result::Success;
    }

    Result
    Simulation_ParseFragilityModes(Simulation& s, toml::value const& v)
    {
        if (v.contains("fragility_mode"))
        {
            if (!v.at("fragility_mode").is_table())
            {
                return Result::Failure;
            }
            toml::table const& fmTable = v.at("fragility_mode").as_table();
            for (auto const& pair : fmTable)
            {
                std::string const& fmName = pair.first;
                std::string const fullName = "fragility_mode." + fmName;
                if (!Simulation_IsFailureModeNameUnique(s, fmName))
                {
                    WriteErrorMessage(
                        fullName,
                        "fragility mode name must be unique within both "
                        "failure_mode and fragility_mode names"
                    );
                    return Result::Failure;
                }
                if (!pair.second.is_table())
                {
                    WriteErrorMessage(
                        fullName, "fragility_mode section must be a table"
                    );
                    return Result::Failure;
                }
                toml::table const& fmValueTable = pair.second.as_table();
                if (!fmValueTable.contains("fragility_curve"))
                {
                    WriteErrorMessage(
                        fullName, "missing required field 'fragility_curve'"
                    );
                    return Result::Failure;
                }
                if (!fmValueTable.at("fragility_curve").is_string())
                {
                    WriteErrorMessage(
                        fullName, "'fragility_curve' field must be a string"
                    );
                    return Result::Failure;
                }
                std::string const& fcTag =
                    fmValueTable.at("fragility_curve").as_string();
                size_t fcId = Simulation_RegisterFragilityCurve(s, fcTag);
                std::optional<size_t> maybeRepairDistId = {};
                if (fmValueTable.contains("repair_dist"))
                {
                    if (!fmValueTable.at("repair_dist").is_string())
                    {
                        WriteErrorMessage(
                            fullName, "field 'repair_dist' must be a string"
                        );
                        return Result::Failure;
                    }
                    std::string const& repairDistTag =
                        fmValueTable.at("repair_dist").as_string();
                    maybeRepairDistId =
                        s.TheModel.DistSys.lookup_dist_by_tag(repairDistTag);
                }
                Simulation_RegisterFragilityMode(
                    s, fmName, fcId, maybeRepairDistId
                );
            }
        }
        return Result::Success;
    }

    Result
    Simulation_ParseComponents(
        Simulation& s,
        toml::value const& v,
        ComponentValidationMap const& compValidations,
        std::unordered_set<std::string> const& componentTagsInUse
    )
    {
        if (v.contains("components") && v.at("components").is_table())
        {
            return ParseComponents(
                s,
                v.at("components").as_table(),
                compValidations,
                componentTagsInUse
            );
        }
        WriteErrorMessage("<top>", "required field 'components' not found");
        return Result::Failure;
    }

    Result
    Simulation_ParseDistributions(
        Simulation& s,
        toml::value const& v,
        DistributionValidationMap const& dvm
    )
    {
        if (v.contains("dist") && v.at("dist").is_table())
        {
            // TODO: have ParseDistributions return a Result
            return ParseDistributions(
                s.TheModel.DistSys, v.at("dist").as_table(), dvm
            );
        }
        std::cout << "required field 'dist' not found" << std::endl;
        return Result::Failure;
    }

    Result
    Simulation_ParseNetwork(Simulation& s, toml::value const& v)
    {
        std::string const n = "network";
        if (v.contains(n) && v.at(n).is_table())
        {
            return ParseNetwork(s.FlowTypeMap, s.TheModel, v.at(n).as_table());
        }
        else
        {
            std::cout << "required field '" << n << "' not found" << std::endl;
            return Result::Failure;
        }
    }

    Result
    Simulation_ParseScenarios(Simulation& s, toml::value const& v)
    {
        if (v.contains("scenarios") && v.at("scenarios").is_table())
        {
            auto result = ParseScenarios(
                s.ScenarioMap, s.TheModel.DistSys, v.at("scenarios").as_table()
            );
            if (result == Result::Success)
            {
                for (auto const& pair : v.at("scenarios").as_table())
                {
                    std::string const& scenarioName = pair.first;
                    std::optional<size_t> maybeScenarioId =
                        ScenarioDict_GetScenarioByTag(
                            s.ScenarioMap, scenarioName
                        );
                    if (!maybeScenarioId.has_value())
                    {
                        std::cout << "[scenarios] "
                                  << "could not find scenario id for '"
                                  << scenarioName << "'" << std::endl;
                        return Result::Failure;
                    }
                    size_t scenarioId = maybeScenarioId.value();
                    std::string fullName = "scenarios." + scenarioName;
                    if (!pair.second.is_table())
                    {
                        std::cout << "[" << fullName << "] "
                                  << "must be a table" << std::endl;
                    }
                    toml::table const& data = pair.second.as_table();
                    if (data.contains("intensity"))
                    {
                        if (!data.at("intensity").is_table())
                        {
                            std::cout << "[" << fullName << ".intensity] "
                                      << "must be a table" << std::endl;
                            return Result::Failure;
                        }
                        for (auto const& p : data.at("intensity").as_table())
                        {
                            std::string const& intensityTag = p.first;
                            if (!(p.second.is_integer()
                                  || p.second.is_floating()))
                            {
                                std::cout << "[" << fullName << ".intensity."
                                          << intensityTag << "] "
                                          << "must be a number" << std::endl;
                                return Result::Failure;
                            }
                            std::optional<double> maybeValue =
                                TOML_ParseNumericValueAsDouble(p.second);
                            if (!maybeValue.has_value())
                            {
                                std::cout << "[" << fullName << ".intensity."
                                          << intensityTag << "] "
                                          << "must be a number" << std::endl;
                                return Result::Failure;
                            }
                            double value = maybeValue.value();
                            size_t intensityId =
                                Simulation_RegisterIntensity(s, intensityTag);
                            Simulation_RegisterIntensityLevelForScenario(
                                s, scenarioId, intensityId, value
                            );
                        }
                    }
                }
            }
            return result;
        }
        std::cout << "required field 'scenarios' not found or not a table"
                  << std::endl;
        return Result::Failure;
    }

    std::optional<Simulation>
    Simulation_ReadFromToml(
        toml::value const& v,
        InputValidationMap const& validationInfo,
        std::unordered_set<std::string> const& componentTagsInUse
    )
    {
        Simulation s = {};
        Simulation_Init(s);
        auto simInfoResult =
            Simulation_ParseSimulationInfo(s, v, validationInfo.SimulationInfo);
        if (simInfoResult == Result::Failure)
        {
            WriteErrorMessage("simulation_info", "problem parsing...");
            return {};
        }
        auto loadsResult = Simulation_ParseLoads(
            s,
            v,
            validationInfo.Load_01Explicit,
            validationInfo.Load_02FileBased
        );
        if (loadsResult == Result::Failure)
        {
            WriteErrorMessage("loads", "problem parsing...");
            return {};
        }
        auto compResult = Simulation_ParseComponents(
            s, v, validationInfo.Comp, componentTagsInUse
        );
        if (compResult == Result::Failure)
        {
            WriteErrorMessage("components", "problem parsing...");
            return {};
        }
        auto distResult =
            Simulation_ParseDistributions(s, v, validationInfo.Dist);
        if (distResult == Result::Failure)
        {
            WriteErrorMessage("dist", "problem parsing...");
            return {};
        }
        if (Simulation_ParseFailureModes(s, v) == Result::Failure)
        {
            WriteErrorMessage("failure_mode", "problem parsing...");
            return {};
        }
        if (Simulation_ParseFragilityModes(s, v) == Result::Failure)
        {
            WriteErrorMessage("fragility_mode", "problem parsing...");
            return {};
        }
        if (Simulation_ParseNetwork(s, v) == Result::Failure)
        {
            WriteErrorMessage("network", "problem parsing...");
            return {};
        }
        if (Simulation_ParseScenarios(s, v) == Result::Failure)
        {
            WriteErrorMessage("scenarios", "problem parsing...");
            return {};
        }
        if (Simulation_ParseFragilityCurves(s, v) == Result::Failure)
        {
            WriteErrorMessage("fragility_curve", "problem parsing...");
            return {};
        }
        return s;
    }

    void
    Simulation_Print(Simulation const& s)
    {
        std::cout << "-----------------" << std::endl;
        std::cout << s.Info << std::endl;
        std::cout << "\nLoads:" << std::endl;
        Simulation_PrintLoads(s);
        std::cout << "\nComponents:" << std::endl;
        Simulation_PrintComponents(s);
        std::cout << "\nDistributions:" << std::endl;
        s.TheModel.DistSys.print_distributions();
        std::cout << "\nFailure Modes:" << std::endl;
        Simulation_PrintFailureModes(s);
        std::cout << "\nComponent/Failure Modes:" << std::endl;
        Simulation_PrintComponentFailureModes(s);
        std::cout << "\nFragility Curves:" << std::endl;
        Simulation_PrintFragilityCurves(s);
        std::cout << "\nFragility Modes:" << std::endl;
        Simulation_PrintFragilityModes(s);
        std::cout << "\nComponent/Fragility Modes:" << std::endl;
        Simulation_PrintComponentFragilityModes(s);
        std::cout << "\nConnections:" << std::endl;
        Model_PrintConnections(s.TheModel, s.FlowTypeMap);
        std::cout << "\nScenarios:" << std::endl;
        Simulation_PrintScenarios(s);
        std::cout << "\nIntensities:" << std::endl;
        Simulation_PrintIntensities(s);
    }

    void
    Simulation_PrintIntensities(Simulation const& s)
    {
        for (size_t i = 0; i < s.Intensities.Tags.size(); ++i)
        {
            std::cout << i << ": " << s.Intensities.Tags[i] << std::endl;
        }
    }

    void
    WriteEventFileHeader(
        std::ofstream& out,
        Model const& model,
        FlowDict const& fd,
        std::vector<size_t> const& nodeConnOrder,
        std::vector<size_t> const& storeOrder,
        std::vector<size_t> const& compOrder,
        TimeUnit outputTimeUnit,
        std::vector<NodeConnection> nodeConnections,
        bool aggregateGroups
    )
    {
        ComponentDict const& compMap = model.ComponentMap;
        out << "scenario id,"
            << "scenario start time (P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]),"
            << "elapsed ("
            << (outputTimeUnit == TimeUnit::Hour
                    ? "hours"
                    : TimeUnitToTag(outputTimeUnit))
            << ")";

        // auto nNodeConn = nodeConnections.size();
        for (std::string const& prefix :
             std::vector<std::string>{"", "REQUEST:", "AVAILABLE:"})
        {
            for (auto& iOrdNodeConn : nodeConnOrder)
            {
                auto const& nodeConn = nodeConnections[iOrdNodeConn];
                out << "," << prefix
                    << NodeConnectionToString(
                           model, fd, nodeConn, true, aggregateGroups
                       )
                    << " (kW)";
            }
        }

        for (std::pair<std::string, std::string> const& prePostFix :
             std::vector<std::pair<std::string, std::string>>{
                 {"Stored: ", " (kJ)"}, {"SOC: ", ""}
             })
        {
            for (size_t storeIdx : storeOrder)
            {
                for (size_t compId = 0; compId < compMap.Tag.size(); ++compId)
                {
                    if (compMap.CompType[compId] == ComponentType::StoreType
                        && compMap.Idx[compId] == storeIdx)
                    {
                        auto tag = compMap.Tag[compId];
                        if (aggregateGroups
                            && model.ComponentToGroup.contains(compId))
                        {
                            tag = model.ComponentToGroup.at(compId) + "(" + tag
                                + ")";
                        }
                        out << "," << prePostFix.first << tag
                            << prePostFix.second;
                    }
                }
            }
        }
        // op-state: <component-name>
        for (size_t compId : compOrder)
        {
            if (!model.ComponentMap.Tag[compId].empty())
            {
                std::string compName = model.ComponentMap.Tag[compId];
                if (aggregateGroups && model.ComponentToGroup.contains(compId))
                {
                    auto& group = model.ComponentToGroup.at(compId);
                    compName = group + "(" + compName + ")";
                }
                out << ",op-state: " << compName;
            }
        }
        out << std::endl;
    }

    std::vector<size_t>
    CalculateConnectionOrder(Simulation const& s)
    {
        // TODO: need to enforce connections are unique
        size_t const numConns = s.TheModel.Connections.size();
        std::vector<size_t> result;
        std::vector<std::string> originalConnTags;
        std::vector<std::string> connTags;
        result.reserve(numConns);
        originalConnTags.reserve(numConns);
        connTags.reserve(numConns);
        for (auto const& conn : s.TheModel.Connections)
        {
            std::string connTag =
                ConnectionToString(s.TheModel.ComponentMap, conn, true);
            originalConnTags.push_back(connTag);
            connTags.push_back(connTag);
        }
        std::sort(connTags.begin(), connTags.end());
        for (auto const& connTag : connTags)
        {
            for (size_t connId = 0; connId < s.TheModel.Connections.size();
                 ++connId)
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

    std::vector<size_t>
    CalculateScenarioOrder(Simulation const& s)
    {
        std::vector<size_t> result;
        std::vector<std::string> scenarioTags(s.ScenarioMap.Tags);
        size_t numScenarios = s.ScenarioMap.Tags.size();
        std::sort(scenarioTags.begin(), scenarioTags.end());
        result.reserve(numScenarios);
        for (std::string const& tag : scenarioTags)
        {
            for (size_t scenarioId = 0; scenarioId < s.ScenarioMap.Tags.size();
                 ++scenarioId)
            {
                if (tag == s.ScenarioMap.Tags[scenarioId])
                {
                    result.push_back(scenarioId);
                    break;
                }
            }
        }
        assert(result.size() == numScenarios);
        return result;
    }

    std::vector<size_t>
    CalculateComponentOrder(Simulation const& s)
    {
        size_t const numComps = s.TheModel.ComponentMap.Tag.size();
        std::vector<size_t> result;
        result.reserve(numComps);
        std::vector<std::string> compTags(s.TheModel.ComponentMap.Tag);
        std::sort(compTags.begin(), compTags.end());
        for (auto const& t : compTags)
        {
            for (size_t compId = 0; compId < numComps; ++compId)
            {
                if (s.TheModel.ComponentMap.Tag[compId] == t)
                {
                    result.push_back(compId);
                    break;
                }
            }
        }
        assert(result.size() == numComps);
        return result;
    }

    std::vector<size_t>
    CalculateStoreOrder(Simulation const& s)
    {
        std::vector<size_t> result;
        std::vector<std::string> storeTags;
        storeTags.reserve(s.TheModel.Stores.size());
        size_t const numComps = s.TheModel.ComponentMap.CompType.size();
        size_t const numStores = s.TheModel.Stores.size();
        for (size_t storeId = 0; storeId < numStores; ++storeId)
        {
            for (size_t compId = 0; compId < numComps; ++compId)
            {
                ComponentType type = s.TheModel.ComponentMap.CompType[compId];
                size_t idx = s.TheModel.ComponentMap.Idx[compId];
                if (type == ComponentType::StoreType && idx == storeId)
                {
                    storeTags.push_back(s.TheModel.ComponentMap.Tag[compId]);
                    break;
                }
            }
        }
        assert(storeTags.size() == numStores);
        std::vector<std::string> originalStoreTags(storeTags);
        std::sort(storeTags.begin(), storeTags.end());
        for (auto const& tag : storeTags)
        {
            for (size_t storeId = 0; storeId < numStores; ++storeId)
            {
                if (tag == originalStoreTags[storeId])
                {
                    result.push_back(storeId);
                }
            }
        }
        assert(result.size() == numStores);
        return result;
    }

    std::vector<size_t>
    CalculateFailModeOrder(Simulation const& s)
    {
        size_t const numFailModes = s.FailureModes.Tags.size();
        std::vector<size_t> result;
        std::vector<std::string> failTags(s.FailureModes.Tags);
        std::sort(failTags.begin(), failTags.end());
        for (auto const& t : failTags)
        {
            for (size_t i = 0; i < numFailModes; ++i)
            {
                if (s.FailureModes.Tags[i] == t)
                {
                    result.push_back(i);
                    break;
                }
            }
        }
        assert(result.size() == numFailModes);
        return result;
    }

    std::vector<size_t>
    CalculateFragilModeOrder(Simulation const& s)
    {
        size_t const numFragModes = s.FragilityModes.Tags.size();
        std::vector<size_t> result;
        std::vector<std::string> fragTags(s.FragilityModes.Tags);
        std::sort(fragTags.begin(), fragTags.end());
        for (auto const& t : fragTags)
        {
            for (size_t i = 0; i < numFragModes; ++i)
            {
                if (s.FragilityModes.Tags[i] == t)
                {
                    result.push_back(i);
                    break;
                }
            }
        }
        assert(result.size() == numFragModes);
        return result;
    }

    std::string
    FlowInWattsToString(flow_t value_W, unsigned int precision)
    {
        bool const printDebugInfo = false;
        if (value_W == max_flow_W)
        {
            if (printDebugInfo)
            {
                std::cout << "Found infinity:" << std::endl;
                std::cout << "- value_W   : " << fmt::format("{}", value_W)
                          << std::endl;
                std::cout << "- max_flow_W: " << fmt::format("{}", max_flow_W)
                          << std::endl;
            }
            return "inf";
        }
        double value_kW = static_cast<double>(value_W) / W_per_kW;
        return DoubleToString(value_kW, precision);
    }

    std::vector<size_t>
    CalculateNodeConnectionOrder(
        Simulation const& s,
        std::vector<NodeConnection> nodeConnections,
        bool aggregateGroups = true
    )
    {
        size_t const nNodeConns = nodeConnections.size();
        std::vector<std::string> originalNodeConnTags = {};
        std::vector<std::string> nodeConnTags = {};

        originalNodeConnTags.reserve(nNodeConns);
        nodeConnTags.reserve(nNodeConns);
        for (auto const& nodeConn : nodeConnections)
        {
            std::string nodeConnTag = NodeConnectionToString(
                s.TheModel, nodeConn, true, aggregateGroups
            );
            originalNodeConnTags.push_back(nodeConnTag);
            nodeConnTags.push_back(nodeConnTag);
        }
        std::sort(nodeConnTags.begin(), nodeConnTags.end());

        std::vector<size_t> result;
        result.reserve(nNodeConns);
        for (auto const& nodeConnTag : nodeConnTags)
        {
            for (size_t nodeConnId = 0; nodeConnId < nodeConnTags.size();
                 ++nodeConnId)
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

    std::vector<NodeConnection>
    GetNodeConnections(Simulation& s, bool aggregateGroups)
    {
        auto& connections = s.TheModel.Connections;

        std::vector<NodeConnection> nodeConnections = {};

        s.TheModel.nGroupPortsTo = {};
        s.TheModel.nGroupPortsFrom = {};

        for (const auto& [key, value] : s.TheModel.GroupToComponents)
        {
            s.TheModel.nGroupPortsTo.insert({key, 0});
            s.TheModel.nGroupPortsFrom.insert({key, 0});
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
                fromIsGroup =
                    s.TheModel.ComponentToGroup.contains(connection.FromId);
                toIsGroup =
                    s.TheModel.ComponentToGroup.contains(connection.ToId);
            }

            NodeConnection nodeConn;
            nodeConn.FromId = connection.FromId;
            nodeConn.FromPort = connection.FromPort;
            nodeConn.FromIdx = connection.FromIdx;
            nodeConn.From = connection.From;

            nodeConn.ToId = connection.ToId;
            nodeConn.ToPort = connection.ToPort;
            nodeConn.ToIdx = connection.ToIdx;
            nodeConn.To = connection.To;

            nodeConn.FlowTypeId = connection.FlowTypeId;

            if (fromIsGroup && toIsGroup)
            {
                auto groupFrom = s.TheModel.ComponentToGroup[connection.FromId];
                auto groupTo = s.TheModel.ComponentToGroup[connection.ToId];
                if (groupFrom == groupTo)
                    continue;
            }
            if (fromIsGroup)
            {
                auto groupFrom = s.TheModel.ComponentToGroup[connection.FromId];
                nodeConn.FromId = groupFrom;
                auto& nPorts = s.TheModel.nGroupPortsFrom[groupFrom];
                nodeConn.FromPort = nPorts;
                nPorts++;
            }
            if (toIsGroup)
            {
                auto groupTo = s.TheModel.ComponentToGroup[connection.ToId];
                nodeConn.ToId = groupTo;
                auto& nPorts = s.TheModel.nGroupPortsTo[groupTo];
                nodeConn.ToPort = nPorts;
                nPorts++;
            }
            bool newConn = true;
            for (auto& nodeConn0 : nodeConnections)
            {
                if (nodeConn0 == nodeConn)
                {
                    newConn = false;
                    nodeConn0.origConnId.push_back(iConn);
                    break;
                }
            }
            if (newConn)
            {
                nodeConn.origConnId = {iConn};
                nodeConnections.push_back(nodeConn);
            }
        }

        return nodeConnections;
    }

    void
    AggregateGroups(
        std::vector<TimeAndFlows>& results,
        std::vector<NodeConnection> const& nodeConnections
    )
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
            newResults[iResult].Time = results[iResult].Time;

            auto& origFlows = results[iResult].Flows;
            auto& newFlows = newResults[iResult].Flows;
            newFlows.resize(nNodeConn);

            for (size_t iNodeConn = 0; iNodeConn < nNodeConn; ++iNodeConn)
            {
                auto& nodeConn = nodeConnections[iNodeConn];
                for (auto& iConn : nodeConn.origConnId)
                {
                    newFlows[iNodeConn] += origFlows[iConn];
                    break;
                }
            }

            newResults[iResult].StorageAmounts_J =
                results[iResult].StorageAmounts_J;
        }
        results = newResults;
    }

    void
    WriteResultsToEventFile(
        std::ofstream& out,
        std::vector<TimeAndFlows> results,
        Simulation const& s,
        std::string const& scenarioTag,
        std::string const& scenarioStartTimeTag,
        std::vector<size_t> const& nodeConnOrder,
        std::vector<size_t> const& storeOrder,
        std::vector<size_t> const& compOrder,
        TimeUnit outputTimeUnit
    )
    {
        // TODO: pass in desired precision
        unsigned int precision = 1;
        unsigned int storePrecision = 3;
        Model const& m = s.TheModel;
        std::map<size_t, std::vector<TimeState>> relSchByCompId;
        for (size_t i = 0; i < m.Reliabilities.size(); ++i)
        {
            size_t compId = m.Reliabilities[i].ComponentId;
            relSchByCompId[compId] = m.Reliabilities[i].TimeStates;
        }

        for (auto const& r : results)
        {
            assert(r.Flows.size() == nodeConnOrder.size());
            out << scenarioTag << "," << scenarioStartTimeTag << ",";
            out << TimeInSecondsToDesiredUnit(r.Time, outputTimeUnit);

            for (size_t const& i : nodeConnOrder)
            {
                out << ","
                    << FlowInWattsToString(r.Flows[i].Actual_W, precision);
            }
            for (size_t const& i : nodeConnOrder)
            {
                out << ","
                    << FlowInWattsToString(r.Flows[i].Requested_W, precision);
            }
            for (size_t const& i : nodeConnOrder)
            {
                out << ","
                    << FlowInWattsToString(r.Flows[i].Available_W, precision);
            }
            // NOTE: Amounts in kJ
            for (size_t i : storeOrder)
            {
                double store_J = static_cast<double>(r.StorageAmounts_J[i]);
                double store_kJ = store_J / J_per_kJ;
                out << "," << std::fixed << std::setprecision(storePrecision)
                    << store_kJ;
            }
            // NOTE: Store state in SOC
            for (size_t i : storeOrder)
            {
                double soc = 0.0;
                if (m.Stores[i].Capacity_J > 0)
                {
                    soc = static_cast<double>(r.StorageAmounts_J[i])
                        / static_cast<double>(m.Stores[i].Capacity_J);
                }
                out << "," << std::fixed << std::setprecision(storePrecision)
                    << soc;
            }
            for (size_t i : compOrder)
            {
                if (!m.ComponentMap.Tag[i].empty())
                {
                    if (relSchByCompId.contains(i))
                    {
                        TimeState ts = TimeState_GetActiveTimeState(
                            relSchByCompId[i], r.Time
                        );
                        if (ts.state)
                        {
                            out << ",available";
                        }
                        else
                        {
                            // lookup the failure and fragility modes
                            std::vector<size_t> failModes;
                            failModes.reserve(ts.failureModeCauses.size());
                            std::vector<size_t> fragModes;
                            fragModes.reserve(ts.fragilityModeCauses.size());
                            for (size_t fm : ts.failureModeCauses)
                            {
                                failModes.push_back(fm);
                            }
                            for (size_t fm : ts.fragilityModeCauses)
                            {
                                fragModes.push_back(fm);
                            }
                            std::sort(failModes.begin(), failModes.end());
                            std::sort(fragModes.begin(), fragModes.end());
                            std::vector<std::string> fmTags;
                            fmTags.reserve(
                                ts.failureModeCauses.size()
                                + ts.fragilityModeCauses.size()
                            );
                            for (auto const& failModeId : failModes)
                            {
                                fmTags.push_back(s.FailureModes.Tags[failModeId]
                                );
                            }
                            for (auto const& fragModeId : fragModes)
                            {
                                fmTags.push_back(
                                    s.FragilityModes.Tags[fragModeId]
                                );
                            }
                            bool first = true;
                            std::ostringstream oss{};
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
    SetLoadsForScenario(
        std::vector<ScheduleBasedLoad>& loads,
        LoadDict loadMap,
        size_t scenarioIdx
    )
    {
        for (size_t sblIdx = 0; sblIdx < loads.size(); ++sblIdx)
        {
            if (loads[sblIdx].ScenarioIdToLoadId.contains(scenarioIdx))
            {
                auto loadId = loads[sblIdx].ScenarioIdToLoadId.at(scenarioIdx);
                std::vector<TimeAndAmount> schedule{};
                size_t numEntries = loadMap.Loads[loadId].size();
                schedule.reserve(numEntries);
                for (size_t i = 0; i < numEntries; ++i)
                {
                    TimeAndAmount tal{};
                    tal.Time_s = loadMap.Loads[loadId][i].Time_s;
                    tal.Amount_W = loadMap.Loads[loadId][i].Amount_W;
                    schedule.push_back(std::move(tal));
                }
                loads[sblIdx].TimesAndLoads = std::move(schedule);
            }
            else
            {
                std::cout << "ERROR:"
                          << "Unhandled scenario id in ScenarioIdToLoadId"
                          << std::endl;
                return Result::Failure;
            }
        }
        return Result::Success;
    }

    Result
    SetSupplyForScenario(
        std::vector<ScheduleBasedSource>& loads,
        LoadDict loadMap,
        size_t scenarioIdx
    )
    {
        for (size_t sblIdx = 0; sblIdx < loads.size(); ++sblIdx)
        {
            if (loads[sblIdx].ScenarioIdToSourceId.contains(scenarioIdx))
            {
                auto loadId =
                    loads[sblIdx].ScenarioIdToSourceId.at(scenarioIdx);
                std::vector<TimeAndAmount> schedule{};
                size_t numEntries = loadMap.Loads[loadId].size();
                schedule.reserve(numEntries);
                for (size_t i = 0; i < numEntries; ++i)
                {
                    TimeAndAmount tal{};
                    tal.Time_s = loadMap.Loads[loadId][i].Time_s;
                    tal.Amount_W = loadMap.Loads[loadId][i].Amount_W;
                    schedule.push_back(std::move(tal));
                }
                loads[sblIdx].TimeAndAvails = std::move(schedule);
            }
            else
            {
                std::cout << "ERROR:"
                          << "Unhandled scenario id in ScenarioIdToSourceId"
                          << std::endl;
                return Result::Failure;
            }
        }
        return Result::Success;
    }

    std::vector<double>
    DetermineScenarioOccurrenceTimes(
        Simulation& s,
        size_t scenIdx,
        bool isVerbose
    )
    {
        std::vector<double> occurrenceTimes_s;
        auto const& maybeMaxOccurrences = s.ScenarioMap.MaxOccurrences[scenIdx];
        size_t maxOccurrence = maybeMaxOccurrences.has_value()
            ? maybeMaxOccurrences.value()
            : 1'000;
        auto const distId = s.ScenarioMap.OccurrenceDistributionIds[scenIdx];
        double scenarioStartTime_s = 0.0;
        double maxTime_s = Time_ToSeconds(s.Info.MaxTime, s.Info.TheTimeUnit);
        for (size_t i = 0; i < maxOccurrence; ++i)
        {
            scenarioStartTime_s += s.TheModel.DistSys.next_time_advance(distId);
            if (scenarioStartTime_s > maxTime_s)
            {
                break;
            }
            occurrenceTimes_s.push_back(scenarioStartTime_s);
        }
        if (isVerbose)
        {
            std::cout << "Occurrences: " << occurrenceTimes_s.size()
                      << std::endl;
            for (size_t i = 0; i < occurrenceTimes_s.size(); ++i)
            {
                std::cout << "-- "
                          << SecondsToPrettyString(occurrenceTimes_s[i])
                          << std::endl;
            }
        }
        return occurrenceTimes_s;
    }

    std::unordered_map<size_t, double>
    GetIntensitiesForScenario(Simulation& s, size_t scenIdx)
    {
        std::unordered_map<size_t, double> intensityIdToAmount;
        size_t numIntensities = 0;
        for (size_t i = 0; i < s.ScenarioIntensities.ScenarioIds.size(); ++i)
        {
            if (s.ScenarioIntensities.ScenarioIds[i] == scenIdx)
            {
                numIntensities++;
            }
        }
        if (numIntensities == 0)
        {
            return intensityIdToAmount;
        }
        intensityIdToAmount.reserve(numIntensities);
        for (size_t i = 0; i < s.ScenarioIntensities.ScenarioIds.size(); ++i)
        {
            if (s.ScenarioIntensities.ScenarioIds[i] == scenIdx)
            {
                auto intensityId = s.ScenarioIntensities.IntensityIds[i];
                intensityIdToAmount[intensityId] =
                    s.ScenarioIntensities.IntensityLevels[i];
            }
        }
        return intensityIdToAmount;
    }

    std::vector<ScheduleBasedReliability>
    CopyReliabilities(Simulation const& s)
    {
        std::vector<ScheduleBasedReliability> originalReliabilities;
        originalReliabilities.reserve(s.TheModel.Reliabilities.size());
        for (size_t sbrIdx = 0; sbrIdx < s.TheModel.Reliabilities.size();
             ++sbrIdx)
        {
            ScheduleBasedReliability const& sbrSrc =
                s.TheModel.Reliabilities[sbrIdx];
            ScheduleBasedReliability sbrCopy{};
            sbrCopy.ComponentId = sbrSrc.ComponentId;
            sbrCopy.TimeStates.reserve(sbrSrc.TimeStates.size());
            for (size_t tsIdx = 0; tsIdx < sbrSrc.TimeStates.size(); ++tsIdx)
            {
                TimeState const& tsSrc = sbrSrc.TimeStates[tsIdx];
                TimeState tsCopy{};
                tsCopy.time = tsSrc.time;
                tsCopy.state = tsSrc.state;
                sbrCopy.TimeStates.push_back(std::move(tsCopy));
            }
            originalReliabilities.push_back(std::move(sbrCopy));
        }
        return originalReliabilities;
    }

    std::vector<std::string>
    ReliabilitiesToStrings(std::vector<ScheduleBasedReliability> const& sbrs)
    {
        std::vector<std::string> result{};
        result.push_back(
            fmt::format("ScheduleBasedReliability vector size: {}",
                        sbrs.size()));
        for (ScheduleBasedReliability const& sbr : sbrs)
        {
            std::ostringstream oss{};
            bool isFirst = true;
            for (TimeState const& ts : sbr.TimeStates)
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
                fmt::format("- {{ComponentId: {},TimeStates=[{}]}}",
                            sbr.ComponentId, oss.str()));
        }
        return result;
    }

    void
    PrintReliabilities(std::vector<ScheduleBasedReliability> const& sbrs)
    {
        for (std::string const& s : ReliabilitiesToStrings(sbrs))
        {
            std::cout << s << std::endl;
        }
    }

    std::vector<ScheduleBasedReliability>
    ApplyReliabilitiesAndFragilities(
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
        std::unordered_map<size_t, std::vector<TimeState>> const&
            relSchByCompId,
        bool verbose,
        Log const& log
    )
    {
        std::vector<ScheduleBasedReliability> result;
        result.reserve(componentFailureModeComponentIds.size());
        std::unordered_set<size_t> reliabilitiesAdded;
        reliabilitiesAdded.reserve(componentFailureModeComponentIds.size());
        for (size_t cfmIdx = 0;
             cfmIdx < componentFailureModeComponentIds.size();
             ++cfmIdx)
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
                Log_Info(log, fmt::format("component: {}", componentTags[compId]));
                Log_Info(log, fmt::format("initial age (h): {}", (initialAge_s / seconds_per_hour)));
            }
            std::vector<TimeState> clip = TimeState_Clip(
                sch, startTime_s + initialAge_s, endTime_s + initialAge_s, true
            );
            // NOTE: Reliabilities have not yet been assigned so we can
            // just push_back()
            ScheduleBasedReliability sbr{};
            sbr.ComponentId = compId;
            sbr.TimeStates = std::move(clip);
            result.push_back(std::move(sbr));
            reliabilitiesAdded.insert(compId);
        }
        if (intensityIdToAmount.size() > 0)
        {
            if (verbose)
            {
                Log_Info(log, "... Applying fragilities");
            }
            // NOTE: if there are no components having fragility modes,
            // there is nothing to do.
            for (size_t cfmIdx = 0;
                 cfmIdx < componentFragilityComponentIds.size();
                 ++cfmIdx)
            {
                size_t fmId = componentFragilityFragilityModeIds[cfmIdx];
                size_t fcId = fragilityModeFragilityCurveIds[fmId];
                std::optional<size_t> repairId =
                    fragilityModeRepairDistIds[fmId];
                FragilityCurveType curveType = fragilityCurveCurveTypes[fcId];
                size_t fcIdx = fragilityCurveCurveIds[fcId];
                bool isFailed = false;
                double failureFrac = 0.0;
                switch (curveType)
                {
                    case (FragilityCurveType::Linear):
                    {
                        LinearFragilityCurve lfc = linearFragilityCurves[fcIdx];
                        size_t vulnerId = lfc.VulnerabilityId;
                        if (intensityIdToAmount.contains(vulnerId))
                        {
                            double level = intensityIdToAmount.at(vulnerId);
                            failureFrac =
                                LinearFragilityCurve_GetFailureFraction(
                                    lfc, level
                                );
                        }
                    }
                    break;
                    case (FragilityCurveType::Tabular):
                    {
                        TabularFragilityCurve tfc =
                            tabularFragilityCurves[fcIdx];
                        size_t vulnerId = tfc.VulnerabilityId;
                        if (intensityIdToAmount.contains(vulnerId))
                        {
                            double level = intensityIdToAmount.at(vulnerId);
                            failureFrac =
                                TabularFragilityCurve_GetFailureFraction(
                                    tfc, level
                                );
                        }
                    }
                    break;
                    default:
                    {
                        Log_Error(log, "fragility_curve", "unhandled fragility curve type");
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
                        Log_Debug(log, "fragility_curve",
                                  fmt::format("component failed: {}; cause: {}", componentTags[compId], fragilityModeTags[fmId]));
                    }
                    // does the component have a reliability signal?
                    bool hasReliabilityAlready = false;
                    size_t reliabilityId = 0;
                    for (size_t rIdx = 0; rIdx < result.size(); ++rIdx)
                    {
                        if (result[rIdx].ComponentId == compId)
                        {
                            hasReliabilityAlready = true;
                            reliabilityId = rIdx;
                            break;
                        }
                    }
                    std::vector<TimeState> newTimeStates;
                    TimeState ts{};
                    ts.state = false;
                    ts.time = 0.0;
                    ts.fragilityModeCauses.insert(fmId);
                    newTimeStates.push_back(std::move(ts));
                    if (repairId.has_value())
                    {
                        size_t repId = repairId.value();
                        double randValue = randFn();
                        if (verbose)
                        {
                            Log_Info(log, fmt::format("randValue for next time advance is: {}", randValue));
                        }
                        double repairTime_s =
                            ds.next_time_advance(repId, randValue);
                        TimeState repairTime{};
                        repairTime.time = repairTime_s;
                        repairTime.state = true;
                        newTimeStates.push_back(std::move(repairTime));
                    }
                    if (hasReliabilityAlready)
                    {
                        auto const& currentSch =
                            result[reliabilityId].TimeStates;
                        std::vector<TimeState> combined =
                            TimeState_Combine(currentSch, newTimeStates);
                        result[reliabilityId].TimeStates = std::move(combined);
                    }
                    else
                    {
                        ScheduleBasedReliability sbr{};
                        sbr.ComponentId = compId;
                        sbr.TimeStates = newTimeStates;
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
    void
    WriteStatisticsToFile(
        Simulation const& s,
        std::string const& statsFilePath,
        std::vector<ScenarioOccurrenceStats> const& occurrenceStats,
        std::vector<size_t> const& compOrder,
        std::vector<size_t> const& failOrder,
        std::vector<size_t> const& fragOrder
    )
    {
        std::ofstream stats;
        stats.open(statsFilePath);
        if (!stats.good())
        {
            std::cout << "Could not open '" << statsFilePath << "' for writing."
                      << std::endl;
            return;
        }
        stats << "scenario id," << "occurrence number," << "duration (h),"
              << "total source (kJ)," << "total load (kJ),"
              << "total storage (kJ)," << "total waste (kJ),"
              << "energy balance (source-(load+storage+waste)) (kJ),"
              << "site efficiency," << "uptime (h)," << "downtime (h),"
              << "load not served (kJ)," << "energy robustness [ER],"
              << "energy availability [EA],"
              << "max single event downtime [MaxSEDT] (h),"
              << "global availability";
        if (occurrenceStats.size() > 0)
        {
            for (auto const& statsByFlow : occurrenceStats[0].FlowTypeStats)
            {
                std::string const& flowType =
                    s.FlowTypeMap.Type[statsByFlow.FlowTypeId];
                stats << ",energy robustness [ER] for " << flowType;
                stats << ",energy availability [EA] for " << flowType;
            }
            for (auto const& statsByFlowLoad :
                 occurrenceStats[0].LoadAndFlowTypeStats)
            {
                std::string const& flowType =
                    s.FlowTypeMap.Type[statsByFlowLoad.Stats.FlowTypeId];
                std::string const& tag =
                    s.TheModel.ComponentMap.Tag[statsByFlowLoad.ComponentId];
                stats << ",energy robustness [ER] for " << tag
                      << " [flow: " << flowType << "]";
                stats << ",energy availability [EA] for " << tag
                      << " [flow: " << flowType << "]";
            }
            for (auto const& lnsByComp :
                 occurrenceStats[0].LoadNotServedForComponents)
            {
                std::string const& flowType =
                    s.FlowTypeMap.Type[lnsByComp.FlowTypeId];
                std::string const& tag =
                    s.TheModel.ComponentMap.Tag[lnsByComp.ComponentId];
                stats << ",load not served (kJ) for " << tag
                      << " [flow: " << flowType << "]";
            }
        }
        std::set<size_t> componentsToSkip;
        for (size_t i : compOrder)
        {
            if (s.TheModel.ComponentMap.Tag[i].empty())
            {
                componentsToSkip.insert(i);
            }
            else
            {
                stats << ",availability: " << s.TheModel.ComponentMap.Tag[i];
            }
        }
        for (size_t i : failOrder)
        {
            stats << ",global count: " << s.FailureModes.Tags[i];
        }
        for (size_t i : fragOrder)
        {
            stats << ",global count: " << s.FragilityModes.Tags[i];
        }
        for (size_t i : failOrder)
        {
            stats << ",global time fraction: " << s.FailureModes.Tags[i];
        }
        for (size_t i : fragOrder)
        {
            stats << ",global time fraction: " << s.FragilityModes.Tags[i];
        }
        std::map<size_t, std::set<size_t>> failModeIdsByCompId;
        std::map<size_t, std::set<size_t>> fragModeIdsByCompId;
        for (size_t compId : compOrder)
        {
            failModeIdsByCompId[compId] = std::set<size_t>{};
            fragModeIdsByCompId[compId] = std::set<size_t>{};
            for (auto const& occ : occurrenceStats)
            {
                if (occ.EventCountByCompIdByFailureModeId.contains(compId))
                {
                    for (auto const& p :
                         occ.EventCountByCompIdByFailureModeId.at(compId))
                    {
                        failModeIdsByCompId[compId].insert(p.first);
                    }
                }
                if (occ.EventCountByCompIdByFragilityModeId.contains(compId))
                {
                    for (auto const& p :
                         occ.EventCountByCompIdByFragilityModeId.at(compId))
                    {
                        fragModeIdsByCompId[compId].insert(p.first);
                    }
                }
            }
            for (size_t failModeId : failOrder)
            {
                if (failModeIdsByCompId[compId].contains(failModeId))
                {
                    stats << ",count: " << s.TheModel.ComponentMap.Tag[compId]
                          << " / " << s.FailureModes.Tags[failModeId];
                }
            }
            for (size_t fragModeId : fragOrder)
            {
                if (fragModeIdsByCompId[compId].contains(fragModeId))
                {
                    stats << ",count: " << s.TheModel.ComponentMap.Tag[compId]
                          << " / " << s.FragilityModes.Tags[fragModeId];
                }
            }
        }
        for (size_t compId : compOrder)
        {
            for (size_t failModeId : failOrder)
            {
                if (failModeIdsByCompId[compId].contains(failModeId))
                {
                    stats << ",time fraction: "
                          << s.TheModel.ComponentMap.Tag[compId] << " / "
                          << s.FailureModes.Tags[failModeId];
                }
            }
            for (size_t fragModeId : fragOrder)
            {
                if (fragModeIdsByCompId[compId].contains(fragModeId))
                {
                    stats << ",time fraction: "
                          << s.TheModel.ComponentMap.Tag[compId] << " / "
                          << s.FragilityModes.Tags[fragModeId];
                }
            }
        }
        stats << std::endl;
        for (auto const& os : occurrenceStats)
        {
            double stored_kJ = os.StorageCharge_kJ - os.StorageDischarge_kJ;
            double balance = os.Inflow_kJ + os.InFromEnv_kJ
                - (os.OutflowAchieved_kJ + stored_kJ + os.Wasteflow_kJ);
            double efficiency = (os.Inflow_kJ + os.StorageDischarge_kJ) > 0.0
                ? ((os.OutflowAchieved_kJ + os.StorageCharge_kJ)
                   / (os.Inflow_kJ + os.StorageDischarge_kJ))
                : 0.0;
            double ER = os.OutflowRequest_kJ > 0.0
                ? (os.OutflowAchieved_kJ / os.OutflowRequest_kJ)
                : 1.0;
            double EA =
                os.Duration_s > 0.0 ? (os.Uptime_s / os.Duration_s) : 1.0;
            stats << s.ScenarioMap.Tags[os.Id];
            stats << "," << os.OccurrenceNumber;
            stats << "," << (os.Duration_s / seconds_per_hour);
            stats << "," << DoubleToString(os.Inflow_kJ + os.InFromEnv_kJ, 0);
            // TODO(mok): break out InFromEnv from Inflow and list separately
            stats << "," << DoubleToString(os.OutflowAchieved_kJ, 0);
            stats << "," << DoubleToString(stored_kJ, 0);
            stats << "," << DoubleToString(os.Wasteflow_kJ, 0);
            stats << "," << DoubleToString(balance, 6);
            stats << "," << efficiency;
            stats << "," << (os.Uptime_s / seconds_per_hour);
            stats << "," << (os.Downtime_s / seconds_per_hour);
            stats << "," << os.LoadNotServed_kJ;
            stats << "," << ER;
            stats << "," << EA;
            stats << "," << (os.MaxSEDT_s / seconds_per_hour);
            stats << ","
                  << ((os.Duration_s > 0.0)
                          ? (os.Availability_s / os.Duration_s)
                          : 0.0);
            // NOTE: written in alphabetical order by flowtype name
            for (auto const& statsByFlow : os.FlowTypeStats)
            {

                double ER_by_flow = statsByFlow.TotalRequest_kJ > 0.0
                    ? (statsByFlow.TotalAchieved_kJ
                       / statsByFlow.TotalRequest_kJ)
                    : 0.0;
                double EA_by_flow = os.Duration_s > 0.0
                    ? (statsByFlow.Uptime_s / os.Duration_s)
                    : 0.0;
                stats << "," << ER_by_flow;
                stats << "," << EA_by_flow;
            }
            for (auto const& statsByFlowLoad : os.LoadAndFlowTypeStats)
            {
                double ER_by_load = statsByFlowLoad.Stats.TotalRequest_kJ > 0.0
                    ? (statsByFlowLoad.Stats.TotalAchieved_kJ
                       / statsByFlowLoad.Stats.TotalRequest_kJ)
                    : 0.0;
                double EA_by_load = os.Duration_s > 0.0
                    ? (statsByFlowLoad.Stats.Uptime_s / os.Duration_s)
                    : 0.0;
                stats << "," << ER_by_load;
                stats << "," << EA_by_load;
            }
            for (auto const& lnsByComp : os.LoadNotServedForComponents)
            {
                stats << "," << lnsByComp.LoadNotServed_kJ;
            }
            for (size_t i : compOrder)
            {
                if (!componentsToSkip.contains(i))
                {
                    double availability = os.Duration_s > 0.0
                        ? os.AvailabilityByCompId_s.at(i) / os.Duration_s
                        : 1.0;
                    stats << "," << availability;
                }
            }
            for (size_t i : failOrder)
            {
                size_t eventCount = os.EventCountByFailureModeId.contains(i)
                    ? os.EventCountByFailureModeId.at(i)
                    : 0;
                stats << "," << eventCount;
            }
            for (size_t i : fragOrder)
            {
                size_t eventCount = os.EventCountByFragilityModeId.contains(i)
                    ? os.EventCountByFragilityModeId.at(i)
                    : 0;
                stats << "," << eventCount;
            }
            for (size_t i : failOrder)
            {
                double time_s = os.TimeByFailureModeId_s.contains(i)
                    ? os.TimeByFailureModeId_s.at(i)
                    : 0.0;
                stats << ","
                      << (os.Duration_s > 0.0 ? time_s / os.Duration_s : 0.0);
            }
            for (size_t i : fragOrder)
            {
                double time_s = os.TimeByFragilityModeId_s.contains(i)
                    ? os.TimeByFragilityModeId_s.at(i)
                    : 0.0;
                stats << ","
                      << (os.Duration_s > 0.0 ? time_s / os.Duration_s : 0.0);
            }
            for (size_t compId : compOrder)
            {
                for (size_t i : failOrder)
                {
                    if (failModeIdsByCompId[compId].contains(i))
                    {
                        if (os.EventCountByCompIdByFailureModeId.contains(compId
                            )
                            && os.EventCountByCompIdByFailureModeId.at(compId)
                                   .contains(i))
                        {
                            stats << ","
                                  << os.EventCountByCompIdByFailureModeId
                                         .at(compId)
                                         .at(i);
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
                        if (os.EventCountByCompIdByFragilityModeId.contains(
                                compId
                            )
                            && os.EventCountByCompIdByFragilityModeId.at(compId)
                                   .contains(i))
                        {
                            stats << ","
                                  << os.EventCountByCompIdByFragilityModeId
                                         .at(compId)
                                         .at(i);
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
                        if (os.TimeByCompIdByFailureModeId_s.contains(compId)
                            && os.TimeByCompIdByFailureModeId_s.at(compId)
                                   .contains(i))
                        {
                            double t =
                                os.TimeByCompIdByFailureModeId_s.at(compId).at(i
                                );
                            stats << ","
                                  << (os.Duration_s > 0.0 ? t / os.Duration_s
                                                          : 0.0);
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
                        if (os.TimeByCompIdByFragilityModeId_s.contains(compId)
                            && os.TimeByCompIdByFragilityModeId_s.at(compId)
                                   .contains(i))
                        {
                            double t =
                                os.TimeByCompIdByFragilityModeId_s.at(compId)
                                    .at(i);
                            stats << ","
                                  << (os.Duration_s > 0.0 ? t / os.Duration_s
                                                          : 0.0);
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

    std::vector<TimeAndFlows>
    ApplyUniformTimeStep(
        std::vector<TimeAndFlows> const& results,
        double const time_step_h
    )
    {
        auto num_events = results.size();
        if ((num_events == 0) || (time_step_h <= 0.0))
        {
            return results;
        }

        auto taf = results.front();
        auto num_stored = taf.StorageAmounts_J.size();

        std::vector<TimeAndFlows> modified_results = {taf};

        double t_prev_report_s = 0.0;
        double T_report_s = 3600.0 * time_step_h;

        for (auto& next_taf : results)
        {

            double t_next_report_s = t_prev_report_s + T_report_s;
            while (t_next_report_s <= next_taf.Time)
            {

                auto mod_taf = taf;
                if (t_next_report_s == next_taf.Time)
                    mod_taf.Flows = next_taf.Flows;

                mod_taf.Time = t_next_report_s;
                double dt_orig_s = next_taf.Time - taf.Time;
                if (dt_orig_s > 0.0)
                {
                    double dt_s = t_next_report_s - taf.Time;
                    double time_frac = dt_s / dt_orig_s;
                    for (std::size_t i = 0; i < num_stored; ++i)
                    {
                        mod_taf.StorageAmounts_J[i] = static_cast<flow_t>(
                            (1. - time_frac) * taf.StorageAmounts_J[i]
                            + time_frac * next_taf.StorageAmounts_J[i]
                        );
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
    CreateFailureSchedules(
        std::vector<size_t> const& componentFailureModeComponentIds,
        std::vector<size_t> const& componentFailureModeFailureModeIds,
        std::vector<double> const& componentInitialAges_s,
        ReliabilityCoordinator const& rc,
        std::function<double()> const& randFn,
        DistributionSystem const& ds,
        double scenarioDuration_s,
        double scenarioOffset_s
    )
    {
        std::unordered_map<size_t, std::vector<TimeState>> relSchByCompFailId;
        relSchByCompFailId.reserve(componentFailureModeComponentIds.size());
        std::unordered_set<size_t> componentsWithFailures;
        componentsWithFailures.reserve(componentFailureModeComponentIds.size());
        for (size_t compFailId = 0;
             compFailId < componentFailureModeComponentIds.size();
             ++compFailId)
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
            std::vector<TimeState> relSch =
                rc.make_schedule_for_link(fmId, randFn, ds, endTime_s);
            for (auto& ts : relSch)
            {
                if (!ts.state)
                {
                    ts.failureModeCauses.insert(fmId);
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
                std::vector<TimeState> combined =
                    TimeState_Combine(pair.second, relSchByCompId.at(compId));
                relSchByCompId[compId] = std::move(combined);
            }
            else
            {
                relSchByCompId[compId] = pair.second;
            }
        }
        return relSchByCompId;
    }

    void
    WriteReliabilityCurves(
        std::string const& scenarioName,
        size_t scenarioOccurrence,
        Simulation const& s
    )
    {
        std::string fname =
            fmt::format("{}-{:03}.csv", scenarioName, scenarioOccurrence);
        std::ofstream out;
        out.open(fname);
        if (!out.good())
        {
            std::cout << "Could not open '" << fname << "' for writing."
                      << std::endl;
            return;
        }
        size_t maxRow = 0;
        for (size_t i = 0; i < s.TheModel.Reliabilities.size(); ++i)
        {
            ScheduleBasedReliability const& sbr = s.TheModel.Reliabilities[i];
            if (sbr.TimeStates.size() > maxRow)
            {
                maxRow = sbr.TimeStates.size();
            }
        }
        for (size_t row = 0; row < maxRow; row++)
        {
            if (row == 0)
            {
                for (size_t i = 0; i < s.TheModel.Reliabilities.size(); ++i)
                {
                    ScheduleBasedReliability const& sbr =
                        s.TheModel.Reliabilities[i];
                    if (i > 0)
                    {
                        out << ",";
                    }
                    std::string const& compTag =
                        s.TheModel.ComponentMap.Tag[sbr.ComponentId];
                    out << "time (h)," << compTag << " state,causes";
                }
                out << "\n";
            }
            for (size_t i = 0; i < s.TheModel.Reliabilities.size(); ++i)
            {
                ScheduleBasedReliability const& sbr =
                    s.TheModel.Reliabilities[i];
                if (i > 0)
                {
                    out << ",";
                }
                if (row < sbr.TimeStates.size())
                {
                    std::vector<std::string> causes;
                    for (size_t fmId : sbr.TimeStates[row].failureModeCauses)
                    {
                        causes.push_back(s.FailureModes.Tags[fmId]);
                    }
                    for (size_t fmId : sbr.TimeStates[row].fragilityModeCauses)
                    {
                        causes.push_back(s.FragilityModes.Tags[fmId]);
                    }
                    std::string causeStr = "";
                    for (std::string const& cause : causes)
                    {
                        causeStr += (causeStr.size() == 0)
                            ? cause
                            : fmt::format(" | {}", cause);
                    }
                    out << TimeInSecondsToDesiredUnit(
                        sbr.TimeStates[row].time, TimeUnit::Hour
                    ) << ","
                        << sbr.TimeStates[row].state << "," << causeStr;
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

    void
    Simulation_Run(
        Simulation& s,
        Log& log,
        std::string const& eventsFilename,
        std::string const& statsFilename,
        double time_step_h /*-1.0*/,
        bool aggregateGroups,
        bool saveReliabilityCurves,
        bool verbose
    )
    {
        // TODO: wrap into input options struct and pass in
        bool const checkNetwork = false;
        if (checkNetwork)
        {
            std::vector<std::string> issues = Model_CheckNetwork(s.TheModel);
            if (issues.size() > 0)
            {
                Log_Warning(log, "network connection", "start list of issues");
                for (std::string const& issue : issues)
                {
                    Log_Warning(log, "network connection", issue);
                }
                Log_Warning(log, "network connection", "end list of issues");
            }
            assert(issues.size() == 0);
        }
        // TODO: turn the following into parameters
        TimeUnit outputTimeUnit = TimeUnit::Hour;

        FixedRandom fixedRandom;
        FixedSeries fixedSeries;
        Random fullRandom;
        switch (s.Info.TypeOfRandom)
        {
            case (RandomType::FixedRandom):
            {
                fixedRandom.FixedValue = s.Info.FixedValue;
                s.TheModel.RandFn = fixedRandom;
            }
            break;
            case (RandomType::FixedSeries):
            {
                fixedSeries.Idx = 0;
                fixedSeries.Series = s.Info.Series;
                s.TheModel.RandFn = fixedSeries;
            }
            break;
            case (RandomType::RandomFromSeed):
            {
                fullRandom = CreateRandomWithSeed(s.Info.Seed);
                s.TheModel.RandFn = fullRandom;
            }
            break;
            case (RandomType::RandomFromClock):
            {
                fullRandom = CreateRandom();
                s.TheModel.RandFn = fullRandom;
            }
            break;
            default:
            {
                Log_Error(log, "RandomType", "unhandled random type");
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
        for (size_t fmIdx = 0; fmIdx < s.FailureModes.FailureDistIds.size();
             ++fmIdx)
        {
            s.TheModel.Rel.add_failure_mode(
                s.FailureModes.Tags[fmIdx],
                s.FailureModes.FailureDistIds[fmIdx],
                s.FailureModes.RepairDistIds[fmIdx]
            );
        }
        for (size_t compFailId = 0;
             compFailId < s.ComponentFailureModes.ComponentIds.size();
             ++compFailId)
        {
            s.TheModel.Rel.link_component_with_failure_mode(
                s.ComponentFailureModes.ComponentIds[compFailId],
                s.ComponentFailureModes.FailureModeIds[compFailId]
            );
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
            Log_Warning(log, "file I/O", fmt::format("Could not open '{}' for writing", eventsFilename));
            return;
        }

        std::vector<size_t> scenarioOrder = CalculateScenarioOrder(s);
        std::vector<size_t> connOrder = CalculateConnectionOrder(s);
        std::vector<size_t> storeOrder = CalculateStoreOrder(s);
        std::vector<size_t> compOrder = CalculateComponentOrder(s);
        std::vector<size_t> failOrder = CalculateFailModeOrder(s);
        std::vector<size_t> fragOrder = CalculateFragilModeOrder(s);

        auto nodeConnections = GetNodeConnections(s, aggregateGroups);
        std::vector<size_t> nodeConnOrder =
            CalculateNodeConnectionOrder(s, nodeConnections, aggregateGroups);

        WriteEventFileHeader(
            out,
            s.TheModel,
            s.FlowTypeMap,
            nodeConnOrder,
            storeOrder,
            compOrder,
            outputTimeUnit,
            nodeConnections,
            aggregateGroups
        );
        std::vector<ScenarioOccurrenceStats> occurrenceStats;
        for (size_t scenIdx : scenarioOrder)
        {
            double scenarioDuration_s = Time_ToSeconds(
                s.ScenarioMap.Durations[scenIdx],
                s.ScenarioMap.TimeUnits[scenIdx]
            );
            double scenarioOffset_s =
                s.ScenarioMap.TimeOffsetsInSeconds[scenIdx];
            std::string const& scenarioTag = s.ScenarioMap.Tags[scenIdx];
            if (verbose)
            {
                Log_Info(log, "Scenario", scenarioTag);
            }
            // for this scenario, ensure all schedule-based components
            // have the right schedule set for this scenario
            if (SetLoadsForScenario(
                    s.TheModel.ScheduledLoads, s.LoadMap, scenIdx
                )
                == Result::Failure)
            {
                Log_Warning(log, "", "Issue setting schedule loads");
                return;
            }
            if (SetSupplyForScenario(
                    s.TheModel.ScheduledSrcs, s.LoadMap, scenIdx
                )
                == Result::Failure)
            {
                Log_Warning(log, "", "Issue setting schedule sources");
                return;
            }
            // TODO: implement load substitution for schedule-based sources
            // for (size_t sbsIdx = 0; sbsIdx < s.Model.ScheduleSrcs.size();
            // ++sbsIdx) {/* ... */}
            std::vector<double> occurrenceTimes_s =
                DetermineScenarioOccurrenceTimes(s, scenIdx, verbose);
            if (verbose)
            {
                Log_Debug(
                    log,
                    fmt::format("Calculated {} occurrence times for {}",
                        occurrenceTimes_s.size(),
                        s.ScenarioMap.Tags[scenIdx]));
            }
            // TODO: initialize total scenario stats (i.e.,
            // over all occurrences)
            std::unordered_map<size_t, double> intensityIdToAmount =
                GetIntensitiesForScenario(s, scenIdx);
            for (size_t occIdx = 0; occIdx < occurrenceTimes_s.size(); ++occIdx)
            {
                if (verbose)
                {
                    Log_Debug(log, fmt::format("... Occurrence #{}", occIdx));
                }
                std::unordered_map<size_t, std::vector<TimeState>>
                    relSchByCompId = CreateFailureSchedules(
                        s.ComponentFailureModes.ComponentIds,
                        s.ComponentFailureModes.FailureModeIds,
                        s.TheModel.ComponentMap.InitialAges_s,
                        s.TheModel.Rel,
                        s.TheModel.RandFn,
                        s.TheModel.DistSys,
                        scenarioDuration_s,
                        scenarioOffset_s
                    );
                if (verbose)
                {
                    Log_Info(log, "Generating reliability schedules");
                    for (auto const& pair : relSchByCompId)
                    {
                        std::string const& tag =
                            s.TheModel.ComponentMap.Tag[pair.first];
                        Log_Info(log, fmt::format("Schedule for {}[{}]", tag, pair.first));
                        for (auto const& ts : pair.second)
                        {
                            Log_Debug(log, fmt::format("- {}", TimeState_ToString(ts)));
                        }
                    }
                }
                double t = occurrenceTimes_s[occIdx];
                double tEnd = t + scenarioDuration_s;
                if (verbose)
                {
                    Log_Info(log, fmt::format("Occurrence #{} at {}",
                                                 occIdx + 1,
                                                 SecondsToPrettyString(t)));
                    Log_Info(log,
                                fmt::format(
                                    "Scenario start time: {} h",
                                    TimeInSecondsToHours(
                                        static_cast<uint64_t>(scenarioOffset_s)
                                    )));
                    Log_Info(log,
                                fmt::format("Scenario end time: {} h",
                                    TimeInSecondsToHours(
                                        static_cast<uint64_t>(scenarioOffset_s)
                                        + static_cast<uint64_t>(scenarioDuration_s)
                                    )));
                }
                s.TheModel.Reliabilities.clear();
                s.TheModel.Reliabilities = ApplyReliabilitiesAndFragilities(
                    s.TheModel.RandFn,
                    s.ComponentFailureModes.ComponentIds,
                    s.TheModel.ComponentMap.InitialAges_s,
                    s.TheModel.ComponentMap.Tag,
                    s.ComponentFragilities.ComponentIds,
                    s.ComponentFragilities.FragilityModeIds,
                    s.FragilityModes.FragilityCurveId,
                    s.FragilityModes.RepairDistIds,
                    s.FragilityModes.Tags,
                    s.FragilityCurves.CurveId,
                    s.FragilityCurves.CurveTypes,
                    s.LinearFragilityCurves,
                    s.TabularFragilityCurves,
                    s.TheModel.DistSys,
                    scenarioOffset_s,
                    scenarioOffset_s + scenarioDuration_s,
                    intensityIdToAmount,
                    relSchByCompId,
                    verbose,
                    log
                );
                if (verbose)
                {
                    Log_Info(log, fmt::format("Reliabilities for Scenario: {}", scenarioTag));
                    Log_Info(log, fmt::format("Occurrence #{}", occIdx + 1));
                    for (std::string const& s : ReliabilitiesToStrings(s.TheModel.Reliabilities))
                    {
                        Log_Info(log, s);
                    }
                }
                if (saveReliabilityCurves)
                {
                    if (verbose)
                    {
                        Log_Debug(log, "Writing reliability curves...");
                    }
                    WriteReliabilityCurves(
                        s.ScenarioMap.Tags[scenIdx], occIdx, s
                    );
                    if (verbose)
                    {
                        Log_Debug(log, "Reliability curves written");
                    }
                }
                std::string scenarioStartTimeTag =
                    TimeToISO8601Period(static_cast<uint64_t>(std::llround(t)));
                if (verbose)
                {
                    Log_Info(log,
                        fmt::format("Running {} from {} for {} {}",
                            s.ScenarioMap.Tags[scenIdx],
                            scenarioStartTimeTag,
                            s.ScenarioMap.Durations[scenIdx],
                            TimeUnitToTag(s.ScenarioMap.TimeUnits[scenIdx])));
                    Log_Info(log,
                        fmt::format("time: {} to {}",
                            SecondsToPrettyString(t),
                            SecondsToPrettyString(tEnd)));
                }
                s.TheModel.FinalTime = scenarioDuration_s;
                // TODO: add an optional verbosity flag to SimInfo
                // -- use that to set things like the print flag below

                auto results = Simulate(s.TheModel, verbose);
                {
                    auto* output_results = &results;
                    std::vector<TimeAndFlows> modified_results0 = {};
                    if (time_step_h > 0.0)
                    {
                        modified_results0 =
                            ApplyUniformTimeStep(results, time_step_h);
                        output_results = &modified_results0;
                    }

                    std::vector<TimeAndFlows> modified_results1 =
                        *output_results;
                    AggregateGroups(modified_results1, nodeConnections);
                    output_results = &modified_results1;

                    // TODO: investigate putting output on another thread
                    WriteResultsToEventFile(
                        out,
                        *output_results,
                        s,
                        scenarioTag,
                        scenarioStartTimeTag,
                        nodeConnOrder,
                        storeOrder,
                        compOrder,
                        outputTimeUnit
                    );
                }
                ScenarioOccurrenceStats sos =
                    ModelResults_CalculateScenarioOccurrenceStats(
                        scenIdx, occIdx + 1, s.TheModel, s.FlowTypeMap, results
                    );
                occurrenceStats.push_back(std::move(sos));
            }
            if (verbose)
            {
                Log_Info(log, fmt::format("Scenario {} finished", scenarioTag));
            }
            // TODO: merge per-occurrence stats with global for the current
            // scenario
        }
        out.close();
        WriteStatisticsToFile(
            s, statsFilename, occurrenceStats, compOrder, failOrder, fragOrder
        );
    }
} // namespace erin
