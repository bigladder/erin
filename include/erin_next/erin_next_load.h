/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_LOAD_H
#define ERIN_LOAD_H

#include "erin_next/erin_next_time_and_amount.h"
#include "erin_next/erin_next_units.h"
#include "../vendor/toml11/toml.hpp"
#include "erin_next/erin_next_validation.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <ostream>

namespace erin
{
    struct Load
    {
        std::string Tag;
        std::vector<TimeAndAmount> TimeAndLoads;
    };

    std::optional<Load>
    ParseSingleLoadExplicit(
        std::unordered_map<std::string, InputValue> const& table,
        std::string const& tag
    );

    std::optional<Load>
    ParseSingleLoadFileLoad(
        std::unordered_map<std::string, InputValue> const& table,
        std::string const& tag
    );

    std::optional<std::vector<Load>>
    ParseLoads(
        toml::table const& table,
        ValidationInfo const& explicitValidation,
        ValidationInfo const& fileValidation,
        Log const& log
    );

    std::ostream&
    operator<<(std::ostream& os, Load const& load);

    int
    WritePackedLoads(
        const std::vector<Load>& loads,
        std::string const& loadsFilename
    );
} // namespace erin

#endif
