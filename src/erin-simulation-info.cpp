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
ParseSimulationInfo(std::unordered_map<std::string, InputValue> const& table)
{
    SimulationInfo si {};
    si.InputFormatVersion = std::get<std::string>(table.at("input_format_version").value);
    if (si.InputFormatVersion != current_input_version)
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
    si.TheTimeUnit = maybeTimeUnit.value();
    double rawMaxTime = std::get<double>(table.at("max_time").value);
    si.MaxTime = rawMaxTime;
    std::string rawRateUnit = std::get<std::string>(table.at("rate_unit").value);
    auto maybeRateUnit = tag_to_power_unit(rawRateUnit);
    if (!maybeRateUnit.has_value())
    {
        // TODO: replace with logger for warning
        write_error_message("simulation_info", "unhandled rate unit '" + rawRateUnit + "'");
        return {};
    }
    si.RateUnit = maybeRateUnit.value();
    auto rawQuantityUnit = std::get<std::string>(table.at("quantity_unit").value);
    si.QuantityUnit = rawQuantityUnit;
    RandomType rtype = RandomType::random_from_clock;
    if (table.contains("fixed_random"))
    {
        double fixedValue = std::get<double>(table.at("fixed_random").value);
        rtype = RandomType::fixed_random;
        si.FixedValue = fixedValue;
    }
    else if (table.contains("fixed_random_series"))
    {
        std::vector<double> maybeSeries =
            std::get<std::vector<double>>(table.at("fixed_random_series").value);
        rtype = RandomType::fixed_series;
        si.Series = std::move(maybeSeries);
    }
    else if (table.contains("random_seed"))
    {
        int64_t maybeSeed = std::get<int64_t>(table.at("random_seed").value);
        rtype = RandomType::random_from_seed;
        si.Seed = static_cast<int unsigned>(maybeSeed < 0 ? (-1 * maybeSeed) : maybeSeed);
    }
    si.TypeOfRandom = rtype;
    return si;
}

bool operator==(SimulationInfo const& a, SimulationInfo const& b)
{
    return a.MaxTime == b.MaxTime && a.QuantityUnit == b.QuantityUnit && a.RateUnit == b.RateUnit &&
           a.TheTimeUnit == b.TheTimeUnit;
}

bool operator!=(SimulationInfo const& a, SimulationInfo const& b) { return !(a == b); }

std::ostream& operator<<(std::ostream& os, SimulationInfo const& s)
{
    os << "SimulationInfo{"
       << "MaxTime=" << s.MaxTime << "; "
       << "TimeUnit=\"" << time_unit_to_tag(s.TheTimeUnit) << "\"; "
       << "QuantityUnit=\"" << s.QuantityUnit << "\"; "
       << "RateUnit=\"" << power_unit_to_string(s.RateUnit) << "\"}";
    return os;
}
} // namespace erin
