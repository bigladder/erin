/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_toml.h"
#include <iostream>

namespace erin_next
{
	std::unordered_set<std::string> RequiredLoadFields{
		"time_unit", "rate_unit", "time_rate_pairs",
	};

	std::unordered_set<std::string> OptionalLoadFields{};

	std::unordered_map<std::string, std::string> DefaultLoadFields{};

	std::optional<Load>
	ParseSingleLoad(toml::table const& table, std::string const& tag)
	{
		std::string tableFullName = "loads." + tag;
		if (!TOMLTable_IsValid(
			table, RequiredLoadFields, OptionalLoadFields, DefaultLoadFields,
			tableFullName))
		{
			std::cout << "[" << tableFullName << "] "
				<< "is not valid" << std::endl;
			// TODO: print out why it is not valid? I think TOMLTable_IsValid(.)
			// already does that but confirm...
			return {};
		}
		auto timeUnit = TOMLTable_ParseStringWithSetResponses(
			table, ValidTimeUnits, "time_unit", tableFullName);
		auto rateUnit = TOMLTable_ParseStringWithSetResponses(
			table, ValidRateUnits, "rate_unit", tableFullName);
		auto timeRatePairs = TOMLTable_ParseVectorOfTimeRatePairs(
			table, "time_rate_pairs", tableFullName);
		if (!timeUnit.has_value()
			|| !rateUnit.has_value()
			|| !timeRatePairs.has_value())
		{
			return {};
		}
		Load load{};
		load.RateUnit = rateUnit.value();
		load.Tag = tag;
		load.TimeUnit = timeUnit.value();
		load.TimeAndLoads = std::move(timeRatePairs.value());
		return load;
	}

	std::optional<std::vector<Load>>
	ParseLoads(toml::table const& table)
	{
		std::vector<Load> loads{};
		loads.reserve(table.size());
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			if (it->second.is_table())
			{
				toml::table const& singleLoad = it->second.as_table();
				auto load = ParseSingleLoad(singleLoad, it->first);
				if (load.has_value())
				{
					loads.push_back(std::move(load.value()));
				}
				else
				{
					// TODO: report error?
					return {};
				}
			}
			else
			{
				std::cout << "[loads." << it->first << "] "
					<< "is not a table" << std::endl;
				return {};
			}
		}
		return loads;
	}

	std::ostream&
	operator<<(std::ostream& os, Load const& load)
	{
		os << "Load{"
			<< "Tag=\"" << load.Tag << "\"; "
			<< "TimeUnit=\"" << load.TimeUnit << "\"; "
			<< "RateUnit=\"" << load.RateUnit << "\"; "
			<< "TimeAndLoads=[";
		for (auto it = load.TimeAndLoads.cbegin();
			it != load.TimeAndLoads.cend();
			++it)
		{
			os << (it == load.TimeAndLoads.cbegin() ? "" : ", ")
				<< *it;
		}
		os << "]}";
		return os;
	}
}