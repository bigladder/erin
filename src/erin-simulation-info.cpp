// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.

#include <iostream>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include "erin/simulation-info.h"
#include "erin/toml.h"
#include "erin/units.h"
#include "erin/utils.h"
#include "erin/validation.h"

namespace erin
{
std::unordered_set<std::string> const RequiredSimulationInfoFields {
    "time_unit",
    "max_time",
};

// TODO: remove rate_unit and quantity_unit; match the user guide first
std::unordered_map<std::string, std::string> const DefaultSimulationInfoFields {
    {"rate_unit", "kW"},
    {"quantity_unit", "kJ"},
};

std::unordered_set<std::string> const OptionalSimulationInfoFields {
    "fixed_random", "fixed_random_series", "random_seed"};

// NOTE: pre-requisite, table already validated
// TODO: change this to use unordered_map<string, InputValue>
std::optional<SimulationInfo>
parse_simulation_info(std::unordered_map<std::string, InputValue> const& table)
{
    SimulationInfo si {};
    si.input_format_version = std::get<std::string>(table.at("input_format_version").value);
    if (si.input_format_version != current_input_version)
    {
        // TODO: replace with logger for warning
        write_warning_message("simulation_info", "input_format_version doesn't match current");
    }
    std::string rawTimeUnit = std::get<std::string>(table.at("time_unit").value);
    std::optional<TimeUnit> maybeTimeUnit = tag_to_time_unit(rawTimeUnit);
    if (!maybeTimeUnit.has_value())
    {
        // TODO: replace with logger for warning
        write_error_message("simulation_info", "unhandled time unit string '" + rawTimeUnit + "'");
        return {};
    }
    si.time_unit = maybeTimeUnit.value();
    double rawMaxTime = std::get<double>(table.at("max_time").value);
    si.max_time_s = rawMaxTime;
    std::string rawRateUnit = std::get<std::string>(table.at("rate_unit").value);
    auto maybeRateUnit = tag_to_power_unit(rawRateUnit);
    if (!maybeRateUnit.has_value())
    {
        // TODO: replace with logger for warning
        write_error_message("simulation_info", "unhandled rate unit '" + rawRateUnit + "'");
        return {};
    }
    si.rate_unit = maybeRateUnit.value();
    auto rawQuantityUnit = std::get<std::string>(table.at("quantity_unit").value);
    si.quantity_unit = rawQuantityUnit;
    RandomType rtype = RandomType::random_from_clock;
    if (table.contains("fixed_random"))
    {
        double fixedValue = std::get<double>(table.at("fixed_random").value);
        rtype = RandomType::fixed_random;
        si.fixed_value = fixedValue;
    }
    else if (table.contains("fixed_random_series"))
    {
        std::vector<double> maybeSeries =
            std::get<std::vector<double>>(table.at("fixed_random_series").value);
        rtype = RandomType::fixed_series;
        si.series = std::move(maybeSeries);
    }
    else if (table.contains("random_seed"))
    {
        int64_t maybeSeed = std::get<int64_t>(table.at("random_seed").value);
        rtype = RandomType::random_from_seed;
        si.seed = static_cast<int unsigned>(maybeSeed < 0 ? (-1 * maybeSeed) : maybeSeed);
    }
    si.type_of_random = rtype;
    return si;
}

bool operator==(SimulationInfo const& a, SimulationInfo const& b)
{
    return a.max_time_s == b.max_time_s && a.quantity_unit == b.quantity_unit && a.rate_unit == b.rate_unit &&
           a.time_unit == b.time_unit;
}

bool operator!=(SimulationInfo const& a, SimulationInfo const& b) { return !(a == b); }

std::ostream& operator<<(std::ostream& os, SimulationInfo const& s)
{
    os << "SimulationInfo{"
       << "MaxTime=" << s.max_time_s << "; "
       << "TimeUnit=\"" << time_unit_to_tag(s.time_unit) << "\"; "
       << "QuantityUnit=\"" << s.quantity_unit << "\"; "
       << "RateUnit=\"" << power_unit_to_string(s.rate_unit) << "\"}";
    return os;
}
} // namespace erin
