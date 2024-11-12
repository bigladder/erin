// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_SCENARIO_H
#define ERIN_SCENARIO_H
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include "../vendor/toml11/toml.hpp"

#include "erin/distribution.h"
#include "erin/result.h"
#include "erin/units.h"

namespace erin
{
struct ScenarioDict
{
    std::vector<std::string> tag;
    std::vector<size_t> occurrence_distribution_id;
    // TODO: remove TimeUnits and pre-convert to make Durations in seconds
    std::vector<TimeUnit> time_unit;
    std::vector<double> duration;
    std::vector<double> time_offset_in_seconds;
    // NOTE: an entry of none means "no max occurrences"; will take as
    // many as fit in the max time of the simulation (see SimulationInfo)
    std::vector<std::optional<size_t>> max_occurrence;
};

std::optional<size_t> ScenarioDict_GetScenarioByTag(ScenarioDict& sd, std::string const& tag);

size_t ScenarioDict_RegisterScenario(ScenarioDict& sd, std::string const& tag);

size_t ScenarioDict_RegisterScenario(ScenarioDict& sd,
                                     std::string const& tag,
                                     size_t occurrenceDistId,
                                     double duration,
                                     TimeUnit timeUnit,
                                     std::optional<size_t> maxOccurrences,
                                     double timeOffset);

std::optional<size_t> ParseSingleScenario(ScenarioDict& sd,
                                          DistributionSystem const& ds,
                                          toml::table const& table,
                                          std::string const& fullName,
                                          std::string const& tag);

Result ParseScenarios(ScenarioDict& sd, DistributionSystem const& ds, toml::table const& table);

void Scenario_Print(ScenarioDict const& sd, DistributionSystem const& ds);
} // namespace erin

#endif
