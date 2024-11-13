// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_SIMULATION_H
#define ERIN_SIMULATION_H
#include <cstdlib>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../vendor/courier/include/courier/courier.h"
#include "../vendor/toml11/toml.hpp"

#include "erin/distribution.h"
#include "erin/erin.h"
#include "erin/load.h"
#include "erin/logging.h"
#include "erin/result.h"
#include "erin/scenario.h"
#include "erin/simulation-info.h"
#include "erin/validation.h"

namespace erin
{

struct Simulation
{
    FlowDict flow_type_map;
    ScenarioDict scenario_map;
    LoadDict load_map;
    SimulationInfo info;
    Model the_model;
    std::vector<LinearFragilityCurve> linear_fragility_curves;
    std::vector<TabularFragilityCurve> tabular_fragility_curves;
    IntensityDict intensities;
    ScenarioIntensityDict scenario_intensities;
    FragilityCurveDict fragility_curves;
    ComponentFragilityModeDict component_fragilities;
    FragilityModeDict fragility_modes;
    ComponentFailureModeDict component_failure_modes;
    FailureModeDict failure_modes;
};

std::string double_to_string(double value, unsigned int precision);

std::string FlowToString(flow_t value_W, unsigned int precision);

void initialize(Simulation& s);

size_t register_flow(Simulation& s, std::string const& flow_tag);

size_t register_scenario(Simulation& s, std::string const& scenario_tag);

size_t register_intensity(Simulation& s, std::string const& tag);

size_t register_intensity_level_for_scenario(Simulation& s,
                                                    size_t scenario_id,
                                                    size_t intensity_id,
                                                    double intensity_level);

size_t register_load_schedule(Simulation& s,
                                       std::string const& tag,
                                       std::vector<TimeAndAmount> const& load_schedule);

std::optional<size_t> get_load_id_by_tag(Simulation const& s, std::string const& tag);

void register_all_loads(Simulation& s, std::vector<Load> const& loads);

void print_components(Simulation const& s);

void print_fragility_curves(Simulation const& s);

void print_failure_modes(Simulation const& s);

void print_component_failure_modes(Simulation const& s);

void print_fragility_modes(Simulation const& s);

void print_component_fragility_modes(Simulation const& s);

void print_scenarios(Simulation const& s);

void print_loads(Simulation const& s);

size_t scenario_count(Simulation const& s);

Result parse_simulation_info(Simulation& s,
                                      toml::value const& v,
                                      ValidationInfo const& validation_info,
                                      Log const& log);

Result parse_loads(Simulation& s,
                             toml::value const& v,
                             ValidationInfo const& explicit_validation,
                             ValidationInfo const& file_based_validation);

size_t register_fragility_curve(Simulation& s, std::string const& tag);

size_t register_fragility_curve(Simulation& s,
                                         std::string const& tag,
                                         FragilityCurveType curve_type,
                                         size_t curve_index);

size_t register_failure_mode(Simulation& s,
                                      std::string const& tag,
                                      size_t failure_id,
                                      size_t repair_id);

size_t register_fragility_mode(Simulation& s,
                                        std::string const& tag,
                                        size_t fragility_curve_id,
                                        std::optional<size_t> maybe_repair_distribution_id);

Result parse_fragility_curves(Simulation& s, std::string const& v, Log const& log);

Result parse_fragility_modes(Simulation& s, toml::value const& v, Log const& log);

Result parse_components(Simulation& s,
                                  toml::value const& v,
                                  ComponentValidationMap const& component_validations,
                                  std::unordered_set<std::string> const& component_tags_in_use,
                                  Log const& log);

Result parse_distributions(Simulation& s, toml::value const& v, Log const& log);

Result parse_network(Simulation& s, toml::value const& v, Log const& log);

Result parse_scenarios(Simulation& s, toml::value const& v, Log const& log);

std::optional<Simulation>
read_from_toml(toml::value const& v,
                          InputValidationMap const& validation_info,
                          std::unordered_set<std::string> const& component_tags_in_use,
                          Log const& log = Log {});

void print(Simulation const& s);

void print_intensities(Simulation const& s);

Result
set_loads_for_scenario(std::vector<ScheduleBasedLoad>& loads, LoadDict load_map, size_t scenario_index);

Result
set_supply_for_scenario(std::vector<ScheduleBasedSource>& loads, LoadDict load_map, size_t scenario_index);

std::vector<double> determine_scenario_occurrence_times(Simulation& s, size_t scenIdx);

std::unordered_map<size_t, double> get_intensities_for_scenario(Simulation& s, size_t scenIdx);

std::vector<ScheduleBasedReliability> copy_reliabilities(Simulation const& s);

std::unordered_map<size_t, std::vector<TimeState>>
create_failure_schedules(std::vector<size_t> const& component_failure_mode_component_ids,
                       std::vector<size_t> const& component_failure_mode_failure_mode_ids,
                       std::vector<double> const& component_initial_ages_s,
                       ReliabilityCoordinator const& rc,
                       std::function<double()> const& random_funct,
                       DistributionSystem const& ds,
                       double scenario_duration_s,
                       double scenario_offset_s);

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
    Log const& log);

std::vector<TimeAndFlows> ApplyUniformTimeStep(std::vector<TimeAndFlows> const& results,
                                               double const time_step_h);

void AggregateGroups(Model& model, std::vector<TimeAndFlows> const& results);

void Simulation_run(Simulation& s,
                    Log& log,
                    std::string const& eventsFilename,
                    std::string const& statsFilename = "stats.csv",
                    double time_step_h = -1.0,
                    bool aggregateGroups = true,
                    bool saveReliabilityCurves = false,
                    bool verbose = false);

bool Simulation_IsFailureNameUnique(Simulation& s, std::string const& name);

bool Simulation_IsFailureModeNameUnique(Simulation& s, std::string const& name);

bool Simulation_IsFragilityModeNameUnique(Simulation& s, std::string const& name);

Result Simulation_ParseFailureModes(Simulation& s, toml::value const& v, Log const& log);

Result Simulation_ParseLinearFragilityCurve(Simulation& s,
                                            std::string const& fcName,
                                            std::string const& tableFullName,
                                            toml::table const& fcData);

std::optional<size_t> Parse_VulnerableTo(Simulation const& s,
                                         toml::table const& fcData,
                                         std::string const& tableFullName);

std::vector<size_t> CalculateScenarioOrder(Simulation const& s);

std::vector<size_t> CalculateComponentOrder(Simulation const& s);

std::vector<size_t> CalculateStoreOrder(Simulation const& s,
                                        std::unordered_set<std::string> const& compsToReport);

std::vector<size_t> CalculateFailModeOrder(Simulation const& s);

std::vector<size_t> CalculateFragilModeOrder(Simulation const& s);

void WriteEventFileHeader(std::ofstream& out,
                          Model const& m,
                          FlowDict const& fd,
                          std::vector<size_t> const& connOrder,
                          std::vector<size_t> const& storeOrder,
                          std::vector<size_t> const& compOrder,
                          TimeUnit outputTimeUnit,
                          std::vector<NodeConnection> const& nodeConnections,
                          bool aggregateGroups);

void WriteResultsToEventFile(std::ofstream& out,
                             std::vector<TimeAndFlows> results,
                             Simulation const& s,
                             std::string const& scenarioTag,
                             std::string const& scenarioStartTimeTag,
                             std::vector<size_t> const& connOrder,
                             TimeUnit outputTimeUnit = TimeUnit::hour);

void WriteStatisticsToFile(Simulation const& s,
                           std::string const& statsFilePath,
                           std::vector<ScenarioOccurrenceStats> const& occurrenceStats,
                           std::vector<size_t> const& compOrder);

} // namespace erin

#endif
