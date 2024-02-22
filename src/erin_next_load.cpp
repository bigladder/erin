/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_csv.h"
#include "erin_next/erin_next_utils.h"
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
		TimeUnit timeUnit = TimeUnit::Second;
		PowerUnit rateUnit = PowerUnit::Watt;
		std::vector<TimeAndAmount> timeRatePairs;
		if (TOMLTable_IsValid(
			table, RequiredLoadFields, OptionalLoadFields, DefaultLoadFields,
			tableFullName, false))
		{
			auto maybeTimeUnitStr = TOMLTable_ParseStringWithSetResponses(
				table, ValidTimeUnits, "time_unit", tableFullName);
			if (!maybeTimeUnitStr.has_value())
			{
				WriteErrorMessage(tableFullName,
					"missing required field 'time_unit'");
				return {};
			}
			else
			{
				auto maybe = TagToTimeUnit(maybeTimeUnitStr.value());
				if (!maybe.has_value())
				{
					WriteErrorMessage(tableFullName,
						"Unhandled 'time_unit': " + maybeTimeUnitStr.value());
					return {};
				}
				timeUnit = maybe.value();
			}
			auto maybeRateUnitStr = TOMLTable_ParseStringWithSetResponses(
				table, ValidRateUnits, "rate_unit", tableFullName);
			if (!maybeRateUnitStr.has_value())
			{
				WriteErrorMessage(tableFullName,
					"missing required field 'rate_unit'");
				return {};
			}
			else
			{
				auto maybe = TagToPowerUnit(maybeRateUnitStr.value());
				if (!maybe.has_value())
				{
					WriteErrorMessage(tableFullName,
						"Unhandled 'rate_unit': " + maybeRateUnitStr.value());
					return {};
				}
				rateUnit = maybe.value();
			}
			auto maybeTimeRatePairs = TOMLTable_ParseVectorOfTimeRatePairs(
				table, "time_rate_pairs", tableFullName,
				Time_ToSeconds(1.0, timeUnit),
				Power_ToWatt(1.0, rateUnit));
			if (!maybeTimeRatePairs.has_value())
			{
				WriteErrorMessage(tableFullName,
					"unable to parse time/rate pairs");
				return {};
			}
			timeRatePairs = std::move(maybeTimeRatePairs.value());
		}
		else if (TOMLTable_IsValid(
			table, RequiredLoadFieldsCsv, OptionalLoadFields,
			DefaultLoadFields, tableFullName, false))
		{
			auto maybeCsvFileName = TOMLTable_ParseString(
				table, "csv_file", tableFullName);
			if (!maybeCsvFileName.has_value())
			{
				WriteErrorMessage(tableFullName,
					"csv_file is not a valid string");
				return {};
			}
			std::string csvFileName{maybeCsvFileName.value()};
			std::ifstream inputDataFile{};
			inputDataFile.open(csvFileName);
			if (!inputDataFile.good())
			{
				WriteErrorMessage(tableFullName,
					"unable to load input csv file '" + csvFileName + "'");
				return {};
			}
			auto header = read_row(inputDataFile);
			if (header.size() == 2)
			{
				std::string const& timeUnitStr = header[0];
				std::string const& rateUnitStr = header[1];
				auto maybeTimeUnit = TagToTimeUnit(timeUnitStr);
				if (!maybeTimeUnit.has_value())
				{
					WriteErrorMessage(tableFullName,
						"unhandled time unit: " + timeUnitStr);
					return {};
				}
				timeUnit = maybeTimeUnit.value();
				auto maybeRateUnit = TagToPowerUnit(rateUnitStr);
				if (!maybeRateUnit.has_value())
				{
					WriteErrorMessage(tableFullName,
						"unhandled rate unit: " + rateUnitStr);
					return {};
				}
				rateUnit = maybeRateUnit.value();
			}
			else
			{
				WriteErrorMessage(tableFullName,
					"csv file '" + csvFileName + "'"
					" -- header must have 2 columns: time unit "
					"and rate unit");
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
					WriteErrorMessage(tableFullName,
						"csv file '" + csvFileName + "'"
						" row: " + std::to_string(rowIdx)
						+ "; must have 2 columns; "
						"found: " + std::to_string(pair.size()));
					return {};
				}
				TimeAndAmount ta{};
				ta.Time_s = Time_ToSeconds(std::stod(pair[0]), timeUnit);
				ta.Amount_W = static_cast<uint32_t>(
					Power_ToWatt(std::stod(pair[1]), rateUnit));
				trps.push_back(ta);
			}
			inputDataFile.close();
			timeRatePairs = trps;
		}
		else
		{
			WriteErrorMessage(tableFullName, "is not valid");
			TOMLTable_IsValid(
				table, RequiredLoadFields, OptionalLoadFields,
				DefaultLoadFields, tableFullName, true);
			TOMLTable_IsValid(
				table, RequiredLoadFieldsCsv, OptionalLoadFields,
				DefaultLoadFields, tableFullName, true);
			return {};
		}
		Load load{};
		load.Tag = tag;
		load.TimeAndLoads = std::move(timeRatePairs);
		return load;
	}

	std::optional<std::vector<Load>>
	ParseLoads(toml::table const& table)
	{
		std::vector<Load> loads{};
		loads.reserve(table.size());
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			std::string tableName = "loads." + it->first;
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
					WriteErrorMessage(tableName,
						"load did not have value");
					return {};
				}
			}
			else
			{
				WriteErrorMessage(tableName, "is not a table");
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