// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_TOML_H
#define ERIN_TOML_H

#include <optional>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../vendor/toml11/toml.hpp"

#include "erin/logging.h"
#include "erin/time_and_amount.h"
#include "erin/valdata.h"

namespace erin
{

std::unordered_map<std::string, InputValue>
TOMLTable_parse_with_validation(std::unordered_map<toml::key, toml::value> const& table,
                                ValidationInfo const& validation_info,
                                std::string const& table_name,
                                std::vector<std::string>& errors,
                                std::vector<std::string>& warnings);

std::optional<std::string>
TOMLTable_parse_string(std::unordered_map<toml::key, toml::value> const& table,
                       std::string const& field_name,
                       std::string const& table_name);

std::optional<std::string>
TOMLTable_parse_string_with_set_responses(std::unordered_map<toml::key, toml::value> const& table,
                                          std::unordered_set<std::string> const& allowed_responses,
                                          std::string const& field_name,
                                          std::string const& table_name);

std::optional<bool> TOML_parse_value_as_bool(toml::value const& v);

std::optional<double> TOML_parse_numeric_value_as_double(toml::value const& v);

std::optional<int> TOML_parse_numeric_value_as_integer(toml::value const& v);

std::optional<double>
TOMLTable_parse_double(std::unordered_map<toml::key, toml::value> const& table,
                       std::string const& field_name,
                       std::string const& table_name);

std::optional<int> TOMLTable_parse_integer(std::unordered_map<toml::key, toml::value> const& table,
                                           std::string const& field_name,
                                           std::string const& table_name);

std::optional<std::vector<TimeAndAmount>>
TOMLTable_parse_vector_of_time_rate_pairs(std::unordered_map<toml::key, toml::value> const& table,
                                          std::string const& field_name,
                                          std::string const& table_name,
                                          double time_mult,
                                          double rate_mult);

std::optional<std::vector<double>>
TOMLTable_parse_array_of_double(std::unordered_map<toml::key, toml::value> const& table,
                                std::string const& field_name,
                                std::string const& table_name);

std::optional<PairsVector>
TOMLTable_parse_array_of_pairs_of_double(std::unordered_map<toml::key, toml::value> const& table,
                                         std::string const& field_name,
                                         std::string const& table_name);

std::unordered_set<std::string> TOMLTable_parse_component_tags_in_use(toml::value const& data);

} // namespace erin

#endif
