/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_TOML_H
#define ERIN_NEXT_TOML_H
#include "../vendor/toml11/toml.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <optional>

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
	TOMLTable_ParseDouble(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName);
}

#endif // ERIN_NEXT_TOML_H