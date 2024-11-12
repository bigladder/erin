// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <assert.h>
#include <iostream>
#include <optional>

#include "erin/scenario.h"
#include "erin/toml.h"
#include "erin/units.h"
#include "erin/validation.h"

namespace erin
{
std::optional<size_t> get_scenario_by_tag(ScenarioDict& sd, std::string const& tag)
{
    for (size_t i = 0; i < sd.tag.size(); ++i)
    {
        if (sd.tag[i] == tag)
        {
            return i;
        }
    }
    return {};
}

size_t register_scenario(ScenarioDict& sd, std::string const& tag)
{
    size_t id = sd.tag.size();
    for (size_t i = 0; i < id; ++i)
    {
        if (sd.tag[i] == tag)
        {
            assert(sd.duration.size() == sd.max_occurrence.size());
            assert(sd.duration.size() == sd.occurrence_distribution_id.size());
            assert(sd.duration.size() == sd.tag.size());
            assert(sd.duration.size() == sd.time_unit.size());
            return i;
        }
    }
    sd.tag.push_back(tag);
    sd.occurrence_distribution_id.push_back(0);
    sd.duration.push_back(0.0);
    sd.time_unit.push_back(TimeUnit::hour);
    sd.time_offset_in_seconds.push_back(0.0);
    sd.max_occurrence.push_back(0);
    assert(sd.duration.size() == sd.max_occurrence.size());
    assert(sd.duration.size() == sd.occurrence_distribution_id.size());
    assert(sd.duration.size() == sd.tag.size());
    assert(sd.duration.size() == sd.time_unit.size());
    return id;
}

size_t register_scenario(ScenarioDict& sd,
                                     std::string const& tag,
                                     size_t occurrenceDistId,
                                     double duration,
                                     TimeUnit timeUnit,
                                     std::optional<size_t> maxOccurrences,
                                     double timeOffset)
{
    size_t id = sd.tag.size();
    for (size_t i = 0; i < id; ++i)
    {
        if (sd.tag[i] == tag)
        {
            sd.occurrence_distribution_id[i] = occurrenceDistId;
            sd.duration[i] = duration;
            sd.time_unit[i] = timeUnit;
            sd.max_occurrence[i] = maxOccurrences;
            sd.time_offset_in_seconds[i] = time_to_seconds(timeOffset, timeUnit);
            assert(sd.duration.size() == sd.max_occurrence.size());
            assert(sd.duration.size() == sd.occurrence_distribution_id.size());
            assert(sd.duration.size() == sd.tag.size());
            assert(sd.duration.size() == sd.time_unit.size());
            return i;
        }
    }
    sd.tag.push_back(tag);
    sd.occurrence_distribution_id.push_back(occurrenceDistId);
    sd.duration.push_back(duration);
    sd.time_unit.push_back(timeUnit);
    sd.max_occurrence.push_back(maxOccurrences);
    assert(sd.duration.size() == sd.max_occurrence.size());
    assert(sd.duration.size() == sd.occurrence_distribution_id.size());
    assert(sd.duration.size() == sd.tag.size());
    assert(sd.duration.size() == sd.time_unit.size());
    return id;
}

std::optional<size_t> ParseSingleScenario(ScenarioDict& sd,
                                          DistributionSystem const& ds,
                                          toml::table const& table,
                                          std::string const& fullName,
                                          std::string const& tag)
{
    auto maybeOccurrenceDist = TOMLTable_parse_string(table, "occurrence_distribution", fullName);
    if (!maybeOccurrenceDist.has_value())
    {
        return {};
    }
    auto maybeTimeUnitStr =
        TOMLTable_parse_string_with_set_responses(table, ValidTimeUnits, "time_unit", fullName);
    if (!maybeTimeUnitStr.has_value())
    {
        return {};
    }
    auto maybeDuration = TOMLTable_parse_double(table, "duration", fullName);
    if (!maybeDuration.has_value())
    {
        return {};
    }
    std::optional<size_t> maxOccurrences = {};
    if (table.contains("max_occurrences"))
    {
        if (table.at("max_occurrences").is_string())
        {
            auto maxOccurrencesString = TOMLTable_parse_string(table, "max_occurrences", fullName);
            if (!maxOccurrencesString.has_value())
            {
                return {};
            }
            if (maxOccurrencesString.value() != "unlimited")
            {
                std::cout << "[" << fullName << "] max_occurrences must "
                          << "be a non-zero positive number or the string "
                          << "'unlimited' or the value -1 (unlimited); got '"
                          << maxOccurrencesString.value() << "'" << std::endl;
                return {};
            }
        }
        else
        {
            std::optional<int64_t> maxOccurrenceValue =
                TOMLTable_parse_integer(table, "max_occurrences", fullName);
            if (!maxOccurrenceValue.has_value())
            {
                return {};
            }
            if (maxOccurrenceValue.value() == -1)
            {
                // nothing to do; this means there is no maximum number of
                // occurrences.
            }
            else if (maxOccurrenceValue.value() > 0)
            {
                maxOccurrences = maxOccurrenceValue;
            }
        }
    }
    auto maybeTimeUnit = tag_to_time_unit(maybeTimeUnitStr.value());
    if (!maybeTimeUnit.has_value())
    {
        return {};
    }
    double timeOffset = 0.0;
    if (table.contains("time_offset"))
    {
        if (table.at("time_offset").is_integer())
        {
            timeOffset = static_cast<double>(table.at("time_offset").as_integer());
        }
        else if (table.at("time_offset").is_floating())
        {
            timeOffset = table.at("time_offset").as_floating();
        }
        else
        {
            return {};
        }
    }
    size_t id = register_scenario(sd,
                                              tag,
                                              ds.lookup_dist_by_tag(maybeOccurrenceDist.value()),
                                              maybeDuration.value(),
                                              maybeTimeUnit.value(),
                                              maxOccurrences,
                                              timeOffset);
    return id;
}

Result ParseScenarios(ScenarioDict& sd, DistributionSystem const& ds, toml::table const& table)
{
    bool ranAtLeastOnce = false;
    for (auto it = table.cbegin(); it != table.cend(); ++it)
    {
        std::string const& tag = it->first;
        std::string fullName = "scenarios." + tag;
        if (it->second.is_table())
        {
            auto maybeScenarioId =
                ParseSingleScenario(sd, ds, it->second.as_table(), fullName, tag);
            if (!maybeScenarioId.has_value())
            {
                return Result::failure;
            }
            ranAtLeastOnce = true;
        }
        else
        {
            std::cout << "[" << fullName << "] "
                      << "not a table" << std::endl;
            return Result::failure;
        }
    }
    if (!ranAtLeastOnce)
    {
        std::cout << "[scenarios] "
                  << "must define at least one scenario" << std::endl;
        return Result::failure;
    }
    return ranAtLeastOnce ? Result::success : Result::failure;
}

void Scenario_Print(ScenarioDict const& sd, DistributionSystem const& ds)
{
    for (size_t i = 0; i < sd.duration.size(); ++i)
    {
        std::cout << i << ": " << sd.tag[i] << std::endl;
        std::cout << "- duration: " << sd.duration[i] << " " << time_unit_to_tag(sd.time_unit[i])
                  << std::endl;
        auto maybeDist = ds.get_dist_by_id(sd.occurrence_distribution_id[i]);
        if (maybeDist.has_value())
        {
            Distribution d = maybeDist.value();
            std::cout << "- occurrence distribution: " << dist_type_to_tag(d.Type) << "["
                      << sd.occurrence_distribution_id[i] << "] -- " << d.Tag << std::endl;
        }
        std::cout << "- max occurrences: ";
        if (sd.max_occurrence[i].has_value())
        {
            std::cout << sd.max_occurrence[i].value() << std::endl;
        }
        else
        {
            std::cout << "no limit" << std::endl;
        }
    }
}

} // namespace erin
