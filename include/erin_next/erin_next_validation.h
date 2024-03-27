/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_VALIDATION_H
#define ERIN_VALIDATION_H
#include "erin_next/erin_next_toml.h"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace erin
{

    std::unordered_set<std::string> const ValidTimeUnits{
        "years",
        "year",
        "yr",
        "weeks",
        "week",
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
        "s"
    };

    std::unordered_set<std::string> const ValidRateUnits{
        "W",
        "kW",
        "MW",
    };

    std::unordered_set<std::string> const ValidQuantityUnits{
        "J",
        "kJ",
        "MJ",
        "Wh",
        "kWh",
        "MWh",
    };

    std::string
    InputSection_toString(InputSection s);

    std::optional<InputSection>
    String_toInputSection(std::string tag);

    void
    UpdateValidationInfoByField(ValidationInfo& info, FieldInfo const& f);

    InputValidationMap
    SetupGlobalValidationInfo();

} // namespace erin

#endif
