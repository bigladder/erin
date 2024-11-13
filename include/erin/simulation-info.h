// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_SIMULATION_INFO_H
#define ERIN_SIMULATION_INFO_H

#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../vendor/toml11/toml.hpp"

#include "erin/random.h"
#include "erin/units.h"
#include "erin/valdata.h"

namespace erin
{

// TODO: consider what we're asking for in SimulationInfo. I think we
// should do the following: get rid of rate and quantity unit. TimeUnit
// is needed as it corresponds with max_time. Otherwise, more thought is
// needed for the rate units. Do we want to get into unit conversion for
// some large number of potential rate units? If so, these should be
// display units by flow type with, perhaps, an overall default. Units
// specified elsewhere would just be for conversion on reading in. Again,
// more thought is needed as we might need to consider ranges when using
// uint32_t...
struct SimulationInfo
{
    std::string input_format_version = "";
    PowerUnit rate_unit;
    // TODO: remove QuantityUnit; not in user guide; or does this set
    // defaults? if keep, use EnergyUnit
    std::string quantity_unit;
    TimeUnit time_unit;
    double max_time_s;
    RandomType type_of_random;
    int unsigned seed = 0;
    std::vector<double> series;
    double fixed_value = 0.0;
};

std::optional<SimulationInfo>
parse_simulation_info(std::unordered_map<std::string, InputValue> const& table);

bool operator==(SimulationInfo const& a, SimulationInfo const& b);

bool operator!=(SimulationInfo const& a, SimulationInfo const& b);

std::ostream& operator<<(std::ostream& os, SimulationInfo const& s);

} // namespace erin

#endif
