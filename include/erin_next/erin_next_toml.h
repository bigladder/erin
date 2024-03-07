/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_TOML_H
#define ERIN_NEXT_TOML_H
#include "erin_next/erin_next_time_and_amount.h"
#include "erin_next/erin_next_validation.h"
#include "../vendor/toml11/toml.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <stdint.h>
#include <optional>

namespace erin_next
{
	struct PairsVector
	{
		std::vector<double> Firsts;
		std::vector<double> Seconds;
	};

	std::unordered_map<std::string, toml::value>
	TOMLTable_ParseWithValidation(
		std::unordered_map<toml::key, toml::value> const& table,
		ValidationInfo const& validationInfo,
		std::string const& tableName,
		std::vector<std::string>& errors,
		std::vector<std::string>& warnings
	);

	// TODO: create struct to hold if tag/name is deprecated
	// struct TagWithDeprecation {string Name; bool IsDeprecated;}
	// TODO: add aliases std::unordered_map<std::string, std::vector<TagWithDeprecation>>
	// TODO: add in std::unordered_map<std::string, TomlType> to check types
	//       are correct
	// TODO: take in std::vector<std::string> to hold error messages
	bool
	TOMLTable_IsValid(
		std::unordered_map<toml::key, toml::value> const& table,
		std::unordered_set<std::string> const& requiredFields,
		std::unordered_set<std::string> const& optionalFields,
		std::unordered_map<std::string, std::string> const& defaults,
		std::string const& tableName,
		bool doPrint = false
	);

	std::optional<std::string>
	TOMLTable_ParseString(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName
	);

	std::optional<std::string>
	TOMLTable_ParseStringWithSetResponses(
		std::unordered_map<toml::key, toml::value> const& table,
		std::unordered_set<std::string> const& allowedResponses,
		std::string const& fieldName,
		std::string const& tableName
	);

	std::optional<double>
	TOML_ParseNumericValueAsDouble(toml::value const& v);

	std::optional<int>
	TOML_ParseNumericValueAsInteger(toml::value const& v);

	std::optional<double>
	TOMLTable_ParseDouble(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName
	);

	std::optional<int>
	TOMLTable_ParseInteger(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName
	);

	std::optional<std::vector<TimeAndAmount>>
	TOMLTable_ParseVectorOfTimeRatePairs(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName,
		double timeMult,
		double rateMult
	);

	std::optional<std::vector<double>>
	TOMLTable_ParseArrayOfDouble(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName
	);

	std::optional<PairsVector>
	TOMLTable_ParseArrayOfPairsOfDouble(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName
	);
}

#endif
