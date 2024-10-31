// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_COMPONENT_H
#define ERIN_COMPONENT_H
#include "erin/erin.h"
#include "erin/simulation.h"
#include "erin/result.h"
#include "../vendor/toml11/toml.hpp"
#include "erin/validation.h"
#include <unordered_set>

namespace erin
{

Result ParseSingleComponent(Simulation& s,
                            toml::table const& table,
                            std::string const& tag,
                            ComponentValidationMap const& compValids,
                            Log const& log);

Result ParseComponents(Simulation& s,
                       toml::table const& table,
                       ComponentValidationMap const& compValids,
                       std::unordered_set<std::string> const& componentTagsInUse,
                       Log const& log);

} // namespace erin

#endif
