// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_VALIDATION_H
#define ERIN_VALIDATION_H

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "erin/valdata.h"

namespace erin
{

std::string const current_input_version = "0.2";

std::unordered_set<std::string> const ValidTimeUnits {"years",
                                                      "year",
                                                      "yr",
                                                      "weeks",
                                                      "week",
                                                      "d",
                                                      "days",
                                                      "day",
                                                      "hours",
                                                      "hour",
                                                      "h",
                                                      "minutes",
                                                      "minute",
                                                      "min",
                                                      "seconds",
                                                      "second",
                                                      "s"};

std::unordered_set<std::string> const ValidRateUnits {
    "W",
    "kW",
    "MW",
};

std::unordered_set<std::string> const ValidQuantityUnits {
    "J",
    "kJ",
    "MJ",
    "Wh",
    "kWh",
    "MWh",
};

std::string InputSection_toString(InputSection s);

std::optional<InputSection> String_toInputSection(std::string tag);

void UpdateValidationInfoByField(ValidationInfo& info, FieldInfo const& f);

// PUBLIC
InputValidationMap setup_global_validation_info();

} // namespace erin

#endif
