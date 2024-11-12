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

std::optional<size_t> get_scenario_by_tag(ScenarioDict& sd, std::string const& tag);

size_t register_scenario(ScenarioDict& sd, std::string const& tag);

size_t register_scenario(ScenarioDict& sd,
                                     std::string const& tag,
                                     size_t occurrence_distribution_id,
                                     double duration,
                                     TimeUnit time_unit,
                                     std::optional<size_t> maximum_occurrences,
                                     double time_offset);

std::optional<size_t> parse_single_scenario(ScenarioDict& sd,
                                          DistributionSystem const& ds,
                                          toml::table const& table,
                                          std::string const& full_name,
                                          std::string const& tag);

Result parse_scenarios(ScenarioDict& sd, DistributionSystem const& ds, toml::table const& table);

void scenario_print(ScenarioDict const& sd, DistributionSystem const& ds);
} // namespace erin

#endif
