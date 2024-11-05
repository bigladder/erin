// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_H
#define ERIN_H

#include <cassert>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../vendor/toml11/toml.hpp"

#include "erin/const.h"
#include "erin/distribution.h"
#include "erin/logging.h"
#include "erin/lookup_table.h"
#include "erin/reliability.h"
#include "erin/result.h"
#include "erin/time_and_amount.h"
#include "erin/timestate.h"
#include "erin/units.h"

namespace erin
{
// DATA
// NOTE: the maximum allowed flow
constexpr flow_t const max_flow_W = std::numeric_limits<flow_t>::max();

enum class FlowDirection
{
    inflow = 0,
    outflow = 1,
};

enum class ComponentType
{
    constant_load_type,
    schedule_based_load_type,
    constant_source_type,
    schedule_based_source_type,
    constant_efficiency_converter_type,
    variable_efficiency_converter_type,
    mux_type,
    store_type,
    pass_through_type,
    mover_type,
    variable_efficiency_mover_type,
    waste_sink_type,
    environment_source_type,
    switch_type,
};

// Holds the various flow types encountered
// NOTE: each entry added must be unique and the index into this
// vector is the flow type used through the rest of the simulation
// NOTE: if we end up wanting to have mass-flow and volumetric-flow,
// we may want to have an enum for FlowCategory or FundamentalFlowType
// with items of Power, Mass, and/or Volume. Each of those would have
// a base unit associated with them. The base unit for flow for Power is
// Watt, for example. For Mass, it might be kg/s (or g/s?).
struct FlowDict
{
    std::vector<std::string> flow_type;
};

struct LoadDict
{
    std::vector<std::string> tags;
    std::vector<std::vector<TimeAndAmount>> loads;
};

// TODO: enable this in the future. Idea is to return
// it via Model_Add*() calls and use it for all gets on
// model and simulation state.
// struct CompId
// {
//     // Index into the top-level component array
//     uint32_t ComponentIdx;
//     // Index into the array of the subtype
//     uint32_t SubtypeIdx;
// };

// NOTE: arrays in struct below indexed by size_t which we call ComponentId
struct ComponentDict
{
    std::vector<size_t> subtype_index;
    std::vector<ComponentType> component_type;
    std::vector<std::string> tag;
    std::vector<double> initial_age_s;
    // Component's inflow type by inport; result indexes FlowDict
    std::vector<std::vector<size_t>> inflow_type;
    // Component's outflow type by outport; result indexes FlowDict
    std::vector<std::vector<size_t>> outflow_type;
    // if true, all connected inflows should be reported in event log
    std::vector<bool> report;
};

struct FlowSummary
{
    double time_s = 0.0;
    flow_t inflow_W = 0;
    flow_t outflow_request_W = 0;
    flow_t outflow_achieved_W = 0;
    flow_t storage_discharge_W = 0;
    flow_t storage_charge_W = 0;
    flow_t wasteflow_W = 0;
    flow_t env_inflow_W = 0;
};

struct StatsByFlowType
{
    size_t flow_type_id;
    double uptime_s = 0.0;
    double total_request_kJ = 0.0;
    double total_achieved_kJ = 0.0;
};

struct StatsByLoadAndFlowType
{
    // indexes ComponentMap
    size_t component_id;
    StatsByFlowType stats;
};

struct LoadNotServedForComp
{
    size_t component_id;
    size_t flow_type_id;
    double load_not_served_kJ = 0.0;
};

struct ScenarioOccurrenceStats
{
    // Id of the scenario; indexes into Simulation.ScenarioMap
    size_t scenario_id = 0;
    // The occurrence of this scenario; 1st occurrence is 1, 2nd is 2, etc.
    size_t occurrence_number;
    double duration_s = 0.0;
    double inflow_kJ = 0.0;
    double outflow_request_kJ = 0.0;
    double outflow_achieved_kJ = 0.0;
    double storage_discharge_kJ = 0.0;
    double storage_charge_kJ = 0.0;
    double wasteflow_kJ = 0.0;
    double in_from_env_kJ = 0.0;
    double load_not_served_kJ = 0.0;
    // net change in storage finalStored_kJ - initialStored_kJ
    double change_in_storage_kJ = 0.0;
    double uptime_s = 0.0;
    double downtime_s = 0.0;
    double max_SEDT_s = 0.0;
    double availability_s = 0.0;
    std::map<size_t, double> availability_by_comp_id_s;
    // Event Counts
    std::map<size_t, size_t> event_count_by_failure_mode_id;
    std::map<size_t, std::map<size_t, size_t>> event_count_by_comp_id_by_failure_mode_id;
    std::map<size_t, size_t> event_count_by_fragility_mode_id;
    std::map<size_t, std::map<size_t, size_t>> event_count_by_comp_id_by_fragility_mode_id;
    // Failure/Fragility Times
    std::map<size_t, double> time_by_failure_mode_id_s;
    std::map<size_t, std::map<size_t, double>> time_by_comp_id_by_failure_mode_id_s;
    std::map<size_t, double> time_by_fragility_mode_id_s;
    std::map<size_t, std::map<size_t, double>> time_by_comp_id_by_fragility_mode_id_s;
    // Characteristics by Flow Type
    // NOTE: sorted in alphabetical order by flow type name
    std::vector<StatsByFlowType> flow_type_stats;
    // NOTE: sorted in alphabetical order by [componentTag, flowType]
    std::vector<StatsByLoadAndFlowType> load_and_flow_type_stats;
    // NOTE: sorted in alphabetical order by [componentTag, flowType]
    std::vector<LoadNotServedForComp> load_not_served_for_components;
};

struct ConstantLoad
{
    flow_t load_W;
    size_t inflow_connection_id;
};

struct ScheduleBasedLoad
{
    std::vector<TimeAndAmount> times_and_loads;
    size_t inflow_connection_id;
    std::map<size_t, size_t> scenario_id_to_load_id;
};

struct ScheduleBasedReliability
{
    std::vector<TimeState> time_states;
    size_t component_id;
};

struct ConstantSource
{
    flow_t available_W;
    size_t outflow_connection_id;
};

struct ScheduleBasedSource
{
    std::vector<TimeAndAmount> time_and_availables;
    size_t outflow_connection_id;
    size_t wasteflow_connection_id;
    std::map<size_t, size_t> scenario_id_to_source_id;
    flow_t max_outflow_W = max_flow_W;
};

struct ConstantEfficiencyConverter
{
    // NOTE: efficiency is a fraction in range (0.0, 1.0]
    double efficiency;
    size_t inflow_connection_id;
    size_t outflow_connection_id;
    std::optional<size_t> lossflow_connection_id;
    size_t wasteflow_connection_id;
    flow_t max_outflow_W = max_flow_W;
    flow_t max_lossflow_W = max_flow_W;
};

struct VariableEfficiencyConverter
{
    size_t inflow_connection_id;
    size_t outflow_connection_id;
    std::optional<size_t> lossflow_connection_id;
    size_t wasteflow_connection_id;
    flow_t max_outflow_W = max_flow_W;
    flow_t max_lossflow_W = max_flow_W;
    std::vector<double> outflows_for_efficiency_W;
    std::vector<double> inflows_for_efficiency_W;
    // Efficiencies corresponding to the outflows and inflows
    std::vector<double> efficiencies;
};

struct Mover
{
    // Coefficient of Performance
    double COP;
    size_t inflow_connection_id;
    size_t outflow_connection_id;
    size_t in_from_env_connection_id;
    size_t wasteflow_connection_id;
    flow_t max_outflow_W = max_flow_W;
};

struct VariableEfficiencyMover
{
    size_t inflow_connection_id;
    size_t outflow_connection_id;
    size_t in_from_env_connection_id;
    size_t wasteflow_connection_id;
    flow_t max_outflow_W = max_flow_W;
    std::vector<double> outflows_for_COP_W;
    std::vector<double> inflows_for_COP_W;
    // Coefficient of Performances -- indexed by above two vectors
    std::vector<double> COPs;
};

struct Connection
{
    ComponentType from = ComponentType::constant_source_type;
    // index into the specific component type's array
    size_t from_subtype_index = 0;
    size_t from_port = 0;
    // index into ComponentDict
    size_t from_component_id = 0;
    ComponentType to = ComponentType::constant_load_type;
    // index into the specific component type's array
    size_t to_subtype_index = 0;
    size_t to_port = 0;
    // index into ComponentDict
    size_t to_component_id = 0;
    size_t flow_type_id = 0;
    size_t result_id = 0;
};

template <typename T>
struct ID
{
    ID(T id_in) : id(id_in) {}
    T id;
};

struct ComponentID : ID<size_t>
{
    ComponentID(size_t id_in = 0) : ID(id_in) {}
};

struct GroupID : ID<std::string>
{
    GroupID(std::string id_in = "") : ID<std::string>(id_in) {}
};

struct NodeID : std::variant<ComponentID, GroupID>
{
    NodeID() : std::variant<ComponentID, GroupID>(0) {}
    NodeID(size_t id_in) : std::variant<ComponentID, GroupID>(id_in) {}
    NodeID(std::string id_in) : std::variant<ComponentID, GroupID>(id_in) {}

    bool operator==(NodeID nodeID) const
    {
        bool same = false;
        auto i = index();
        auto i0 = nodeID.index();
        if (i == i0)
        {
            if (i == 0)
            {
                if (std::get<0>(*this).id == std::get<0>(nodeID).id)
                {
                    same = true;
                }
            }
            else
            {
                if (std::get<1>(*this).id == std::get<1>(nodeID).id)
                {
                    same = true;
                }
            }
        }
        return same;
    }
};

struct NodeConnection
{
    size_t connection_id = 0;
    ComponentType from = ComponentType::constant_source_type;
    ComponentType to = ComponentType::constant_load_type;
    // index into the specific component type's array
    size_t from_subtype_index = 0;
    size_t from_port = 0;
    // index into ComponentDict
    NodeID from_component_id;
    // index into the specific component type's array
    size_t to_subtype_index = 0;
    size_t to_port = 0;
    // index into ComponentDict
    NodeID to_component_id;
    size_t flow_type_id = 0;

    std::vector<size_t> original_connection_id = {};

    bool operator==(NodeConnection const& node_connection) const
    {
        bool from_is_same = (node_connection.from_component_id == from_component_id) &&
                            (node_connection.from_port == from_port);
        bool to_is_same = (node_connection.to_component_id == to_component_id) &&
                          (node_connection.to_port == to_port);
        return from_is_same && to_is_same;
    }
};

struct Mux
{
    size_t number_of_inports;
    size_t number_of_outports;
    std::vector<size_t> inflow_connection_ids;
    std::vector<size_t> outflow_connection_ids;
    std::vector<flow_t> max_outflows_W;
};

struct Store
{
    flow_t capacity_J;
    flow_t max_charge_rate_W;
    flow_t max_discharge_rate_W;
    // amount at or below which we request charge
    flow_t charge_amount_J;
    flow_t initial_storage_J;
    std::optional<size_t> inflow_connection_id = {};
    size_t outflow_connection_id;
    std::optional<size_t> wasteflow_connection_id = {};
    double roundtrip_efficiency = 1.0;
    flow_t max_outflow_W = max_flow_W;
};

struct PassThrough
{
    size_t inflow_connection_id = 0;
    size_t outflow_connection_id = 0;
    flow_t max_outflow_W = max_flow_W;
};

// TODO[mok]: need to rethink this. This adds a branch with an add.
// Probably a horrible performance issue. Use double but convert to
// unsigned int when finalize flows?
inline flow_t safe_add(flow_t a, flow_t b) { return (b > (max_flow_W - a)) ? max_flow_W : a + b; }

struct Flow
{
    flow_t requested_W = 0;
    flow_t available_W = 0;
    flow_t actual_W = 0;

    Flow operator+(Flow const& flow) const
    {
        return Flow {
            .requested_W = requested_W + flow.requested_W,
            .available_W = safe_add(available_W, flow.available_W),
            .actual_W = actual_W + flow.actual_W,
        };
    }

    Flow operator+=(Flow const& flow) { return *this = *this + flow; }
};

struct TimeAndFlows
{
    // TODO: change to Time_s
    double time_s = 0.0;
    std::vector<Flow> flows;
    std::vector<flow_t> storage_amounts_J;
};

typedef std::unordered_map<std::string, std::set<std::size_t>> GroupToComponentMap;

typedef std::unordered_map<std::size_t, std::string> ComponentToGroupMap;

struct Switch
{
    size_t inflow_connection_id_primary;
    size_t inflow_connection_id_secondary;
    size_t outflow_connection_id;
    flow_t max_outflow_W;
};

struct Model
{
    ComponentDict component;
    std::vector<ConstantSource> constant_source;
    std::vector<ScheduleBasedSource> scheduled_source;
    std::vector<ConstantLoad> constant_load;
    std::vector<ScheduleBasedLoad> scheduled_load;
    std::vector<ConstantEfficiencyConverter> constant_efficiency_converter;
    std::vector<VariableEfficiencyConverter> variable_efficiency_converter;
    std::vector<Mux> mux;
    std::vector<Store> store;
    std::vector<PassThrough> pass_through;
    std::vector<Mover> mover;
    std::vector<VariableEfficiencyMover> variable_efficiency_mover;
    std::vector<Switch> transfer_switch;
    std::vector<Connection> connection;
    std::vector<ScheduleBasedReliability> reliability;
    DistributionSystem dist_sys {};
    ReliabilityCoordinator rel_coord {};
    std::function<double()> random_function;
    double final_time_s = 0.0;
    GroupToComponentMap group_to_component;
    ComponentToGroupMap component_to_group;
    std::unordered_map<std::string, size_t> number_of_group_ports_to;
    std::unordered_map<std::string, size_t> number_of_group_ports_from;
};

struct ComponentIdAndWasteConnection
{
    size_t id;
    Connection waste_connection_id;
};

struct ComponentIdAndWasteAndEnvironmentConnection
{
    size_t id;
    Connection waste_connection_id;
    Connection environment_connection_id;
};

enum class SwitchState
{
    primary = 0,
    secondary = 1,
};

struct SimulationState
{
    std::set<size_t> active_connections_back {};
    std::set<size_t> active_connections_front {};
    // a set of component id that are unavailable
    std::set<size_t> unavailable_components {};
    std::vector<flow_t> storage_amounts_J {};
    std::vector<double> storage_next_event_times {};
    std::vector<Flow> flows {};
    std::vector<size_t> schedule_based_load_index {};
    std::vector<size_t> schedule_based_source_index {};
    std::vector<SwitchState> switch_states {};
};

struct TagAndPort
{
    std::string tag;
    size_t port;
};

enum class FragilityResult
{
    is_failed,
    has_survived,
};

enum class FragilityCurveType
{
    linear,
    tabular,
};

struct LinearFragilityCurve
{
    // indexes IntensityDict
    size_t vulnerability_id = 0;
    double lower_bound = 0.0;
    double upper_bound = 1.0;
};

struct TabularFragilityCurve
{
    size_t vulnerability_id = 0;
    std::vector<double> intensity;
    std::vector<double> failure_fraction;
};

struct IntensityDict
{
    std::vector<std::string> tag {};
};

struct ScenarioIntensityDict
{
    std::vector<size_t> scenario_id;
    std::vector<size_t> intensity_id;
    std::vector<double> intensity_level;
};

struct FragilityCurveDict
{
    std::vector<std::string> tag {};
    std::vector<FragilityCurveType> curve_type {};
    std::vector<size_t> curve_id {};
};

// TODO: should we call these "tables" instead of dict?
// more remeniscent of databases...
struct ComponentFragilityModeDict
{
    std::vector<size_t> component_id;
    std::vector<size_t> fragility_mode_id;
};

struct FragilityModeDict
{
    std::vector<std::string> tag {};
    std::vector<size_t> fragility_curve_id {};
    std::vector<std::optional<size_t>> repair_distribution_id {};
};

struct ComponentFailureModeDict
{
    // index into ComponentDict
    std::vector<size_t> component_id;
    // index into FailureModeDict
    std::vector<size_t> failure_mode_id;
};

struct FailureModeDict
{
    std::vector<std::string> tag;
    std::vector<size_t> failure_distribution_id;
    std::vector<size_t> repair_distribution_id;
};

// FUNCTIONS
void add_connection_issue(std::vector<std::string>& issues,
                          std::string component_tag,
                          size_t component_id,
                          size_t component_port,
                          size_t component_subtype_index,
                          ComponentType component_type,
                          Connection const& connection,
                          size_t connection_index,
                          FlowDirection flow_direction);

std::vector<std::string> Model_check_network(Model const& m);

inline flow_t safe_add(flow_t a, flow_t b);

std::vector<TimeAndAmount> ConvertToTimeAndAmounts(std::vector<std::vector<double>> const& input,
                                                   double timeToSeconds = 1.0,
                                                   double rateToWatts = 1.0);

std::optional<FragilityCurveType> TagToFragilityCurveType(std::string const& tag);

std::string FragilityCurveTypeToTag(FragilityCurveType fctype);

std::optional<size_t> GetIntensityIdByTag(IntensityDict intenseDict, std::string const& tag);

size_t Component_AddComponentReturningId(ComponentDict& c, ComponentType ct, size_t idx);

size_t Component_AddComponentReturningId(ComponentDict& c,
                                         ComponentType ct,
                                         size_t idx,
                                         std::vector<size_t> inflowType,
                                         std::vector<size_t> outflowType,
                                         std::string const& tag,
                                         double initialAge_s,
                                         bool report = true);

void Helper_AddIfNotAdded(std::vector<size_t>& items, size_t item);

SwitchState SimulationState_GetSwitchState(SimulationState const& ss, size_t const& switchIdx);

void SimulationState_SetSwitchState(SimulationState& ss,
                                    size_t const& switchIdx,
                                    SwitchState newState);

void SimulationState_AddActiveConnectionBack(SimulationState& ss, size_t connIdx);

void SimulationState_AddActiveConnectionForward(SimulationState& ss, size_t connIdx);

size_t CountActiveConnections(SimulationState const& ss);

void ActivateConnectionsForConstantLoads(Model const& m, SimulationState& ss);

void ActivateConnectionsForConstantSources(Model const& m, SimulationState& ss);

void ActivateConnectionsForScheduleBasedLoads(Model const& m, SimulationState& ss, double t);

void ActivateConnectionsForScheduleBasedSources(Model const& m, SimulationState& ss, double t);

void ActivateConnectionsForStores(Model& m, SimulationState& ss, double t);

void ActivateConnectionsForReliability(Model& m, SimulationState& ss, double time, bool verbose);

double GetNextTime(double nextTime, size_t count, std::function<double(size_t)> f);

double EarliestNextEvent(Model const& m, SimulationState const& ss, double t);

std::optional<size_t>
FindOutflowConnection(Model const& m, ComponentType ct, size_t compId, size_t outflowPort);

void UpdateConstantEfficiencyLossflowAndWasteflow(Model const& m,
                                                  SimulationState& ss,
                                                  size_t compIdx);

void UpdateVariableEfficiencyLossflowAndWasteflow(Model const& m,
                                                  SimulationState& ss,
                                                  size_t compIdx);

void RunMuxPostFinalization(Model const& m, SimulationState& ss, size_t compIdx);

void RunActiveConnections(Model& m, SimulationState& ss, double t);

void RunConnectionsPostFinalization(Model& model, SimulationState& ss, double t);

void RunPassthroughBackward(Model& m, SimulationState& ss, size_t connIdx, size_t compIdx);

void RunConnectionsBackward(Model& model, SimulationState& ss);

void RunPassthroughForward(Model& m, SimulationState& ss, size_t connIdx, size_t compIdx);

void RunConnectionsForward(Model& model, SimulationState& ss);

flow_t FinalizeFlowValue(flow_t requested, flow_t available);

void FinalizeFlows(SimulationState& ss);

double NextEvent(ScheduleBasedLoad const& sb, size_t sbIdx, SimulationState const& ss);

double NextEvent(ScheduleBasedSource const& sb, size_t sbIdx, SimulationState const& ss);

double NextEvent(ScheduleBasedReliability const& sbr, double t);

double NextStorageEvent(SimulationState const& ss, size_t storeIdx, double t);

void UpdateStoresPerElapsedTime(Model const& m, SimulationState& ss, double elapsedTime);

// TODO: change name to `std::string ComponentTypeToString(ComponentType);`
std::string ToString(ComponentType ct);

std::optional<ComponentType> TagToComponentType(std::string const& tag);

std::vector<std::string> FlowsToStrings(Model const& m, SimulationState const& ss, double t);

void LogFlows(Log const& log, Model const& m, SimulationState const& ss, double t);

void PrintFlows(Model const& m, SimulationState const& ss, double t);

FlowSummary SummarizeFlows(Model const& m, SimulationState const& ss, double t);

bool PrintFlowSummary(FlowSummary s);

void PrintModelState(Model& model, SimulationState& ss);

std::vector<Flow> CopyFlows(std::vector<Flow> flows);

std::vector<flow_t> CopyStorageStates(SimulationState& ss);

std::vector<TimeAndFlows>
Simulate(Model& m, bool verbose = true, bool enableSwitchLogic = true, Log const& log = Log {});

void Model_SetComponentToRepaired(Model const& m, SimulationState& ss, size_t compId);

void Model_SetComponentToFailed(Model const& m, SimulationState& ss, size_t compId);

size_t Model_AddSwitch(Model& m, size_t flowTypeId, std::string const& tag);

size_t Model_AddConstantLoad(Model& m, flow_t load);

size_t Model_AddConstantLoad(
    Model& m, flow_t load, size_t inflowTypeId, std::string const& tag, bool report);

size_t Model_AddScheduleBasedLoad(Model& m, double* times, flow_t* loads, size_t numItems);

size_t Model_AddScheduleBasedLoad(Model& m, std::vector<TimeAndAmount> const& timesAndLoads);

size_t Model_AddScheduleBasedLoad(Model& m,
                                  std::vector<TimeAndAmount> const& timesAndLoads,
                                  std::map<size_t, size_t> const& scenarioIdToLoadId);

size_t Model_AddScheduleBasedLoad(Model& m,
                                  std::vector<TimeAndAmount> const& timesAndLoads,
                                  std::map<size_t, size_t> const& scenarioIdToLoadId,
                                  size_t inflowTypeId,
                                  std::string const& tag);

size_t Model_AddConstantSource(Model& m, flow_t available);

size_t
Model_AddConstantSource(Model& m, flow_t available, size_t outflowTypeId, std::string const& tag);

ComponentIdAndWasteConnection Model_AddScheduleBasedSource(Model& m,
                                                           std::vector<TimeAndAmount> const& xs);

ComponentIdAndWasteConnection
Model_AddScheduleBasedSource(Model& m,
                             std::vector<TimeAndAmount> const& xs,
                             std::map<size_t, size_t> const& scenarioIdToSourceId,
                             size_t outflowId,
                             std::string const& tag,
                             double initialAge_s);

size_t Model_AddMux(Model& m, size_t numInports, size_t numOutports);

size_t Model_AddMux(
    Model& m, size_t numInports, size_t numOutports, size_t flowId, std::string const& tag);

size_t Model_AddStore(Model& m,
                      flow_t capacity,
                      flow_t maxCharge,
                      flow_t maxDischarge,
                      flow_t nochargeAmount,
                      flow_t initialStorage);

size_t Model_AddStore(Model& m,
                      flow_t capacity,
                      flow_t maxCharge,
                      flow_t maxDischarge,
                      flow_t chargeAmount,
                      flow_t initialStorage,
                      size_t flowId,
                      std::string const& tag);

ComponentIdAndWasteConnection Model_AddStoreWithWasteflow(Model& m,
                                                          flow_t capacity,
                                                          flow_t maxCharge,
                                                          flow_t maxDischarge,
                                                          flow_t chargeAmount,
                                                          flow_t initialStorage,
                                                          size_t flowId,
                                                          double roundtripEfficiency,
                                                          std::string const& tag);

ComponentIdAndWasteConnection
Model_AddConstantEfficiencyConverter(Model& m, flow_t eff_numerator, flow_t eff_denominator);

ComponentIdAndWasteConnection Model_AddConstantEfficiencyConverter(Model& m, double efficiency);

ComponentIdAndWasteConnection Model_AddConstantEfficiencyConverter(Model& m,
                                                                   double efficiency,
                                                                   size_t inflowId,
                                                                   size_t outflowId,
                                                                   size_t lossflowId,
                                                                   std::string const& tag,
                                                                   bool report = true);

ComponentIdAndWasteConnection
Model_AddVariableEfficiencyConverter(Model& m,
                                     std::vector<double>&& outflows_W,
                                     std::vector<double>&& efficiency_by_outflows,
                                     size_t inflowId,
                                     size_t outflowId,
                                     size_t lossflowId,
                                     std::string const& tag,
                                     bool report = true);

size_t Model_AddPassThrough(Model& m);

size_t Model_AddPassThrough(Model& m, size_t flowId, std::string const& tag);

Connection Model_AddConnection(Model& m, size_t from, size_t fromPort, size_t to, size_t toPort);

Connection Model_AddConnection(Model& m,
                               size_t fromId,
                               size_t fromPort,
                               size_t toId,
                               size_t toPort,
                               size_t flowId,
                               bool checkIntegrity = false);

bool SameConnection(Connection a, Connection b);

std::optional<Flow> ModelResults_GetFlowForConnection(Model const& m,
                                                      Connection conn,
                                                      double time,
                                                      std::vector<TimeAndFlows> timeAndFlows);

std::optional<flow_t> ModelResults_GetStoreState(Model const& m,
                                                 size_t compId,
                                                 double time,
                                                 std::vector<TimeAndFlows> timeAndFlows);

ScenarioOccurrenceStats
ModelResults_CalculateScenarioOccurrenceStats(size_t scenarioId,
                                              size_t occurrenceNumber,
                                              Model const& m,
                                              FlowDict const& flowDict,
                                              std::vector<TimeAndFlows> const& timeAndFlows);

void Debug_PrintNumberOfPasses(bool onlyGrandTotal = false);

void Debug_ResetNumberOfPasses(bool resetAll = false);

void Model_SetupSimulationState(Model& m, SimulationState& ss);

void RunConstantEfficiencyConverterBackward(Model const& m,
                                            SimulationState& ss,
                                            size_t connIdx,
                                            size_t compIdx);

void RunVariableEfficiencyConverterBackward(Model const& m,
                                            SimulationState& ss,
                                            size_t connIdx,
                                            size_t compIdx);

void RunMoverBackward(Model const& m, SimulationState& ss, size_t outflowConnIdx, size_t moverIdx);

void UpdateEnvironmentFlowForMover(Model const& m, SimulationState& ss, size_t moverIdx);

void UpdateEnvironmentFlowForVariableEfficiencyMover(Model const& m,
                                                     SimulationState& ss,
                                                     size_t moverIdx);

void Mux_RequestInflowsIntelligently(SimulationState& ss,
                                     std::vector<size_t> const& inflowConns,
                                     flow_t remainingRequest);

void Mux_BalanceRequestFlows(SimulationState& ss,
                             std::vector<size_t> const& inflowConns,
                             flow_t remainingRequest_W,
                             bool logNewActivity);

void RunMuxBackward(Model& model, SimulationState& ss, size_t compIdx);

void BalanceMuxRequests(Model& model, SimulationState& ss, size_t muxIdx, bool isUnavailable);

void RunStoreBackward(Model& model, SimulationState& ss, size_t connIdx, size_t compIdx);

void RunConstantEfficiencyConverterForward(Model const& model,
                                           SimulationState& ss,
                                           size_t connIdx,
                                           size_t compIdx);

void RunVariableEfficiencyConverterForward(Model const& model,
                                           SimulationState& ss,
                                           size_t connIdx,
                                           size_t compIdx);

void RunMoverForward(Model const& model, SimulationState& ss, size_t connIdx, size_t compIdx);

void RunMuxForward(Model& model, SimulationState& ss, size_t compIdx);

void RunStoreForward(Model& model, SimulationState& ss, size_t connIdx, size_t compIdx);

void RunStorePostFinalization(Model& model, SimulationState& ss, double t, size_t compIdx);

void RunScheduleBasedSourceBackward(Model& model,
                                    SimulationState& ss,
                                    size_t connIdx,
                                    size_t compIdx);

size_t Model_NumberOfComponents(Model const& m);

size_t Model_AddFixedReliabilityDistribution(Model& m, double dt);

ComponentIdAndWasteAndEnvironmentConnection Model_AddMover(Model& m, double cop);

ComponentIdAndWasteAndEnvironmentConnection Model_AddMover(Model& m,
                                                           double cop,
                                                           size_t inflowTypeId,
                                                           size_t outflowTypeId,
                                                           std::string const& tag,
                                                           bool report = true);

ComponentIdAndWasteAndEnvironmentConnection
Model_AddVariableEfficiencyMover(Model& m,
                                 std::vector<double>&& outflowsForCop_W,
                                 std::vector<double>&& copByOutflow,
                                 size_t inflowTypeId,
                                 size_t outflowTypeId,
                                 std::string const& tag,
                                 bool report = true);

size_t
Model_AddFailureModeToComponent(Model& m, size_t compId, size_t failureDistId, size_t repairDistId);

void UpdateScheduleBasedLoadNextEvent(Model const& m, SimulationState& ss, double time);

void UpdateScheduleBasedSourceNextEvent(Model const& model, SimulationState& ss, double time);

std::optional<TagAndPort> ParseTagAndPort(std::string const& s, std::string const& tableName);

Result ParseNetwork(FlowDict const& ft, Model& model, toml::table const& table);

std::optional<size_t> Model_FindCompIdByTag(Model const& m, std::string const& tag);

std::optional<size_t> FlowDict_GetIdByTag(FlowDict const& ft, std::string const& tag);

void Model_PrintConnections(Model const& m, FlowDict const& ft);

std::ostream& operator<<(std::ostream& os, Flow const& flow);

std::string ConnectionToString(ComponentDict const& cd, Connection const& c, bool compact = false);

std::string ConnectionToString(ComponentDict const& cd,
                               FlowDict const& fd,
                               Connection const& c,
                               bool compact = false);

std::string NodeConnectionToString(Model const& model,
                                   NodeConnection const& c,
                                   bool compact = false,
                                   bool aggregateGroups = true);
std::string NodeConnectionToString(Model const& model,
                                   FlowDict const& fd,
                                   NodeConnection const& c,
                                   bool compact = false,
                                   bool aggregateGroups = true);

double Interpolate1d(double x, double x0, double y0, double x1, double y1);

double LinearFragilityCurve_GetFailureFraction(LinearFragilityCurve lfc, double intensityLevel);

double TabularFragilityCurve_GetFailureFraction(TabularFragilityCurve tfc, double intensityLevel);

void ComponentDict_SetInitialAge(ComponentDict& cd, size_t id, double age_s);

void ComponentDict_SetReporting(ComponentDict& cd, size_t id, bool report);

void AddComponentToGroup(Model& model, size_t id, std::string group);

} // namespace erin

#endif
