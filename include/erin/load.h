// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_LOAD_H
#define ERIN_LOAD_H

#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../vendor/toml11/toml.hpp"

#include "erin/logging.h"
#include "erin/time_and_amount.h"
#include "erin/units.h"
#include "erin/validation.h"

namespace erin
{

struct Load
{
    std::string tag;
    std::vector<TimeAndAmount> time_and_loads;
};

std::optional<std::vector<Load>> parse_loads(toml::table const& table,
                                             ValidationInfo const& explicit_validation,
                                             ValidationInfo const& file_validation,
                                             Log const& log);

std::ostream& operator<<(std::ostream& os, Load const& load);

int write_packed_loads(const std::vector<Load>& loads, std::string const& loads_filename);

} // namespace erin

#endif
