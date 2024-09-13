/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_scenario.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_toml.h"
#include <iostream>
#include <optional>
#include <assert.h>

namespace erin
{
    std::optional<size_t>
    ScenarioDict_GetScenarioByTag(ScenarioDict& sd, std::string const& tag)
    {
        for (size_t i = 0; i < sd.Tags.size(); ++i)
        {
            if (sd.Tags[i] == tag)
            {
                return i;
            }
        }
        return {};
    }

    size_t
    ScenarioDict_RegisterScenario(ScenarioDict& sd, std::string const& tag)
    {
        size_t id = sd.Tags.size();
        for (size_t i = 0; i < id; ++i)
        {
            if (sd.Tags[i] == tag)
            {
                assert(sd.Durations.size() == sd.MaxOccurrences.size());
                assert(
                    sd.Durations.size() == sd.OccurrenceDistributionIds.size()
                );
                assert(sd.Durations.size() == sd.Tags.size());
                assert(sd.Durations.size() == sd.TimeUnits.size());
                return i;
            }
        }
        sd.Tags.push_back(tag);
        sd.OccurrenceDistributionIds.push_back(0);
        sd.Durations.push_back(0.0);
        sd.TimeUnits.push_back(TimeUnit::Hour);
        sd.TimeOffsetsInSeconds.push_back(0.0);
        sd.MaxOccurrences.push_back(0);
        assert(sd.Durations.size() == sd.MaxOccurrences.size());
        assert(sd.Durations.size() == sd.OccurrenceDistributionIds.size());
        assert(sd.Durations.size() == sd.Tags.size());
        assert(sd.Durations.size() == sd.TimeUnits.size());
        return id;
    }

    size_t
    ScenarioDict_RegisterScenario(
        ScenarioDict& sd,
        std::string const& tag,
        size_t occurrenceDistId,
        double duration,
        TimeUnit timeUnit,
        std::optional<size_t> maxOccurrences,
        double timeOffset
    )
    {
        size_t id = sd.Tags.size();
        for (size_t i = 0; i < id; ++i)
        {
            if (sd.Tags[i] == tag)
            {
                sd.OccurrenceDistributionIds[i] = occurrenceDistId;
                sd.Durations[i] = duration;
                sd.TimeUnits[i] = timeUnit;
                sd.MaxOccurrences[i] = maxOccurrences;
                sd.TimeOffsetsInSeconds[i] = Time_ToSeconds(timeOffset, timeUnit);
                assert(sd.Durations.size() == sd.MaxOccurrences.size());
                assert(
                    sd.Durations.size() == sd.OccurrenceDistributionIds.size()
                );
                assert(sd.Durations.size() == sd.Tags.size());
                assert(sd.Durations.size() == sd.TimeUnits.size());
                return i;
            }
        }
        sd.Tags.push_back(tag);
        sd.OccurrenceDistributionIds.push_back(occurrenceDistId);
        sd.Durations.push_back(duration);
        sd.TimeUnits.push_back(timeUnit);
        sd.MaxOccurrences.push_back(maxOccurrences);
        assert(sd.Durations.size() == sd.MaxOccurrences.size());
        assert(sd.Durations.size() == sd.OccurrenceDistributionIds.size());
        assert(sd.Durations.size() == sd.Tags.size());
        assert(sd.Durations.size() == sd.TimeUnits.size());
        return id;
    }

    std::optional<size_t>
    ParseSingleScenario(
        ScenarioDict& sd,
        DistributionSystem const& ds,
        toml::table const& table,
        std::string const& fullName,
        std::string const& tag
    )
    {
        auto maybeOccurrenceDist =
            TOMLTable_ParseString(table, "occurrence_distribution", fullName);
        if (!maybeOccurrenceDist.has_value())
        {
            return {};
        }
        auto maybeTimeUnitStr = TOMLTable_ParseStringWithSetResponses(
            table, ValidTimeUnits, "time_unit", fullName
        );
        if (!maybeTimeUnitStr.has_value())
        {
            return {};
        }
        auto maybeDuration = TOMLTable_ParseDouble(table, "duration", fullName);
        if (!maybeDuration.has_value())
        {
            return {};
        }
        std::optional<size_t> maxOccurrences = {};
        if (table.contains("max_occurrences"))
        {
            if (table.at("max_occurrences").is_string())
            {
                auto maxOccurrencesString =
                    TOMLTable_ParseString(table, "max_occurrences", fullName);
                if (!maxOccurrencesString.has_value())
                {
                    return {};
                }
                if (maxOccurrencesString.value() != "unlimited")
                {
                    std::cout << "[" << fullName << "] max_occurrences must "
                              << "be a non-zero positive number or the string "
                              << "'unlimited'; got '"
                              << maxOccurrencesString.value() << "'"
                              << std::endl;
                    return {};
                }
            }
            else
            {
                std::optional<int64_t> maxOccurrenceValue =
                    TOMLTable_ParseInteger(table, "max_occurrences", fullName);
                if (!maxOccurrenceValue.has_value())
                {
                    return {};
                }
                if (maxOccurrenceValue.value() > 0)
                {
                    maxOccurrences = maxOccurrenceValue;
                }
            }
        }
        auto maybeTimeUnit = TagToTimeUnit(maybeTimeUnitStr.value());
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
        size_t id = ScenarioDict_RegisterScenario(
            sd,
            tag,
            ds.lookup_dist_by_tag(maybeOccurrenceDist.value()),
            maybeDuration.value(),
            maybeTimeUnit.value(),
            maxOccurrences,
            timeOffset
        );
        return id;
    }

    Result
    ParseScenarios(
        ScenarioDict& sd,
        DistributionSystem const& ds,
        toml::table const& table
    )
    {
        bool ranAtLeastOnce = false;
        for (auto it = table.cbegin(); it != table.cend(); ++it)
        {
            std::string const& tag = it->first;
            std::string fullName = "scenarios." + tag;
            if (it->second.is_table())
            {
                auto maybeScenarioId = ParseSingleScenario(
                    sd, ds, it->second.as_table(), fullName, tag
                );
                if (!maybeScenarioId.has_value())
                {
                    return Result::Failure;
                }
                ranAtLeastOnce = true;
            }
            else
            {
                std::cout << "[" << fullName << "] " << "not a table"
                          << std::endl;
                return Result::Failure;
            }
        }
        if (!ranAtLeastOnce)
        {
            std::cout << "[scenarios] " << "must define at least one scenario"
                      << std::endl;
            return Result::Failure;
        }
        return ranAtLeastOnce ? Result::Success : Result::Failure;
    }

    void
    Scenario_Print(ScenarioDict const& sd, DistributionSystem const& ds)
    {
        for (size_t i = 0; i < sd.Durations.size(); ++i)
        {
            std::cout << i << ": " << sd.Tags[i] << std::endl;
            std::cout << "- duration: " << sd.Durations[i] << " "
                      << TimeUnitToTag(sd.TimeUnits[i]) << std::endl;
            auto maybeDist = ds.get_dist_by_id(sd.OccurrenceDistributionIds[i]);
            if (maybeDist.has_value())
            {
                Distribution d = maybeDist.value();
                std::cout << "- occurrence distribution: "
                          << dist_type_to_tag(d.Type) << "["
                          << sd.OccurrenceDistributionIds[i] << "] -- " << d.Tag
                          << std::endl;
            }
            std::cout << "- max occurrences: ";
            if (sd.MaxOccurrences[i].has_value())
            {
                std::cout << sd.MaxOccurrences[i].value() << std::endl;
            }
            else
            {
                std::cout << "no limit" << std::endl;
            }
        }
    }

} // namespace erin
