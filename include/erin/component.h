// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_COMPONENT_H
#define ERIN_COMPONENT_H
#include <unordered_set>

#include "../vendor/toml11/toml.hpp"

#include "erin/erin.h"
#include "erin/result.h"
#include "erin/simulation.h"
#include "erin/validation.h"

namespace erin
{

Result parse_single_component(Simulation& s,
                            toml::table const& table,
                            std::string const& tag,
                            ComponentValidationMap const& comp_validation_info,
                            Log const& log);

Result parse_components(Simulation& s,
                       toml::table const& table,
                       ComponentValidationMap const& comp_validation_info,
                       std::unordered_set<std::string> const& component_tags_in_use,
                       Log const& log);

} // namespace erin

#endif
