/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_scenario.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_toml.h"
#include <iostream>
#include <optional>

namespace erin_next
{
	std::optional<size_t>
	ParseSingleScenario(
		ScenarioDict& sd,
		DistributionSystem const& ds,
		toml::table const& table,
		std::string const& tag)
	{
		auto maybeOccurrenceDist =
			TOMLTable_ParseString(table, "occurrence_distribution", tag);
		if (!maybeOccurrenceDist.has_value()) return {};
		auto maybeTimeUnit = TOMLTable_ParseStringWithSetResponses(
			table, ValidTimeUnits, "time_unit", tag);
		if (!maybeTimeUnit.has_value()) return {};
		auto maybeNetworkTag = TOMLTable_ParseString(
			table, "network", tag);
		if (!maybeNetworkTag.has_value()) return {};
		auto maybeDuration = TOMLTable_ParseDouble(
			table, "duration", tag);
		if (!maybeDuration.has_value()) return {};
		std::optional<size_t> maxOccurrences = {};
		if (table.contains("max_occurrences"))
		{
			maxOccurrences =
				TOMLTable_ParseInteger(table, "max_occurrences", tag);
			if (!maxOccurrences.has_value()) return {};
		}
		size_t id = sd.TimeUnits.size();
		sd.OccurrenceDistributionIds.push_back(
			ds.lookup_dist_by_tag(maybeOccurrenceDist.value()));
		sd.TimeUnits.push_back(std::move(maybeTimeUnit.value()));
		// TODO: decide whether we're going to keep handling multiple networks.
		// If yes, this is a bug and this procedure will need to take in
		// a reference object to look up the network id by tag. If no, we can
		// eliminate this line and this field from the data structure.
		sd.NetworkIds.push_back(0);
		sd.Durations.push_back(maybeDuration.value());
		sd.MaxOccurrences.push_back(maxOccurrences);
		return id;
	}

	std::vector<size_t>
	ParseScenarios(
		ScenarioDict& sd,
		DistributionSystem const& ds,
		toml::table const& table)
	{
		std::vector<size_t> scenarioIds = {};
		scenarioIds.reserve(table.size());
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			std::string const& tag = it->first;
			std::string fullName = "scenarios." + tag;
			if (it->second.is_table())
			{
				auto maybeScenarioId =
					ParseSingleScenario(
						sd, ds, it->second.as_table(), fullName);
				if (!maybeScenarioId.has_value()) break;
				scenarioIds.push_back(maybeScenarioId.value());
			}
			else
			{
				std::cout << "[" << fullName << "] "
					<< "not a table" << std::endl;
				break;
			}
		}
		return scenarioIds;
	}

	void
	Scenario_Print(ScenarioDict const& sd, DistributionSystem const& ds)
	{
		for (size_t i=0; i < sd.Durations.size(); ++i)
		{
			std::cout << i << ": " << sd.Tags[i] << std::endl;
			std::cout << "- duration: " << sd.Durations[i]
				<< " " << sd.TimeUnits[i] << std::endl;
			auto maybeDist = ds.get_dist_by_id(sd.OccurrenceDistributionIds[i]);
			if (maybeDist.has_value())
			{
				Distribution d = maybeDist.value();
				std::cout << "- occurrence distribution: "
					<< dist_type_to_tag(d.Type)
					<< "[" << sd.OccurrenceDistributionIds[i] << "] -- "
					<< d.Tag << std::endl;
			}
			std::cout << "- max occurrences: ";
			if (sd.MaxOccurrences[i].has_value())
			{
				std:: cout << sd.MaxOccurrences[i].value() << std::endl;
			}
			else
			{
				std::cout << "no limit" << std::endl;
			}
		}
	}

}