/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_TOML_H
#define ERIN_NEXT_TOML_H
#include "../vendor/toml11/toml.hpp"
#include "erin_next/erin_next_time_and_load.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <optional>
#include <stdint.h>

namespace erin_next
{
	bool
	TOMLTable_IsValid(
		std::unordered_map<toml::key, toml::value> const& table,
		std::unordered_set<std::string> const& requiredFields,
		std::unordered_set<std::string> const& optionalFields,
		std::unordered_map<std::string, std::string> const& defaults,
		std::string const& tableName);

	std::optional<std::string>
	TOMLTable_ParseString(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName);

	std::optional<std::string>
	TOMLTable_ParseStringWithSetResponses(
		std::unordered_map<toml::key, toml::value> const& table,
		std::unordered_set<std::string> const& allowedResponses,
		std::string const& fieldName,
		std::string const& tableName);

	std::optional<double>
	TOML_ParseNumericValueAsDouble(toml::value const& v);

	std::optional<int>
	TOML_ParseNumericValueAsInteger(toml::value const& v);

	std::optional<double>
	TOMLTable_ParseDouble(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName);

	std::optional<std::vector<TimeAndLoad>>
	TOMLTable_ParseVectorOfTimeRatePairs(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName);
}

#endif // ERIN_NEXT_TOML_H