/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_COMPONENT_H
#define ERIN_COMPONENT_H
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_simulation.h"
#include "erin_next/erin_next_result.h"
#include "../vendor/toml11/toml.hpp"
#include "erin_next/erin_next_validation.h"

namespace erin
{

    Result
    ParseSingleComponent(
        Simulation& s,
        toml::table const& table,
        std::string const& tag,
        ComponentValidationMap const& compValids
    );

    Result
    ParseComponents(
        Simulation& s,
        toml::table const& table,
        ComponentValidationMap const& compValids
    );

} // namespace erin

#endif
