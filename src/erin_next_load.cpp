/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_csv.h"
#include <iostream>
#include <fstream>
#include <string>

namespace erin_next
{
	std::unordered_set<std::string> RequiredLoadFields{
		"time_unit", "rate_unit", "time_rate_pairs",
	};

	std::unordered_set<std::string> RequiredLoadFieldsCsv{"csv_file"};

	std::unordered_set<std::string> OptionalLoadFields{};

	std::unordered_map<std::string, std::string> DefaultLoadFields{};

	std::optional<Load>
	ParseSingleLoad(toml::table const& table, std::string const& tag)
	{
		std::string tableFullName = "loads." + tag;
		std::optional<std::string> timeUnit{};
		std::optional<std::string> rateUnit{};
		std::optional<std::vector<TimeAndAmount>> timeRatePairs{};
		if (TOMLTable_IsValid(
			table, RequiredLoadFields, OptionalLoadFields, DefaultLoadFields,
			tableFullName, false))
		{
			timeUnit = TOMLTable_ParseStringWithSetResponses(
				table, ValidTimeUnits, "time_unit", tableFullName);
			rateUnit = TOMLTable_ParseStringWithSetResponses(
				table, ValidRateUnits, "rate_unit", tableFullName);
			timeRatePairs = TOMLTable_ParseVectorOfTimeRatePairs(
				table, "time_rate_pairs", tableFullName);
		}
		else if (TOMLTable_IsValid(
			table, RequiredLoadFieldsCsv, OptionalLoadFields,
			DefaultLoadFields, tableFullName, false))
		{
			auto maybeCsvFileName = TOMLTable_ParseString(
				table, "csv_file", tableFullName);
			if (!maybeCsvFileName.has_value())
			{
				std::cout << "[" << tableFullName << "] "
					<< "csv_file is not a valid string" << std::endl;
				return {};
			}
			std::string csvFileName{maybeCsvFileName.value()};
			std::ifstream inputDataFile{};
			inputDataFile.open(csvFileName);
			if (!inputDataFile.good())
			{
				std::cout << "[" << tableFullName << "] "
					<< "unable to load input csv file '"
					<< csvFileName << "'"
					<< std::endl;
				return {};
			}
			auto header = read_row(inputDataFile);
			if (header.size() == 2)
			{
				timeUnit = header[0];
				rateUnit = header[1];
			}
			else
			{
				std::cout << "[" << tableFullName << "] "
					<< "csv file '"
					<< csvFileName << "'"
					<< " -- header must have 2 columns: time unit "
					<< "and rate unit" << std::endl;
				return {};
			}
			uint32_t rowIdx = 1;
			std::vector<TimeAndAmount> trps{};
			while (inputDataFile.is_open() && inputDataFile.good())
			{
				std::vector<std::string> pair = read_row(inputDataFile);
				if (pair.size() == 0)
				{
					break;
				}
				++rowIdx;
				if (pair.size() != 2)
				{
					std::cout << "[" << tableFullName << "] "
						<< "csv file '"
						<< csvFileName << "'"
						<< " row: " << rowIdx << "; must have 2 columns; "
						<< "found: " << pair.size() << std::endl;
					return {};
				}
				TimeAndAmount ta{};
				ta.Time = std::strtod(pair[0].c_str(), nullptr);
				ta.Amount = static_cast<uint32_t>(
					std::strtod(pair[1].c_str(), nullptr));
				trps.push_back(ta);
			}
			inputDataFile.close();
			timeRatePairs = trps;
		}
		else
		{
			std::cout << "[" << tableFullName << "] "
				<< "is not valid" << std::endl;
			TOMLTable_IsValid(
				table, RequiredLoadFields, OptionalLoadFields,
				DefaultLoadFields, tableFullName, true);
			TOMLTable_IsValid(
				table, RequiredLoadFieldsCsv, OptionalLoadFields,
				DefaultLoadFields, tableFullName, true);
			return {};
		}
		if (!timeUnit.has_value()
			|| !rateUnit.has_value()
			|| !timeRatePairs.has_value())
		{
			return {};
		}
		Load load{};
		load.RateUnit = rateUnit.value();
		load.Tag = tag;
		auto maybeTimeUnit = TagToTimeUnit(timeUnit.value());
		if (!maybeTimeUnit.has_value())
		{
			std::cout << "[" << tableFullName << "] "
				<< "could not interpret time unit tag '"
				<< timeUnit.value() << std::endl;
			return {};
		}
		load.TheTimeUnit = maybeTimeUnit.value();
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
			<< "TimeUnit=\"" << load.TheTimeUnit << "\"; "
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