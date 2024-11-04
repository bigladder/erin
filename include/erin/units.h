// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_UNITS_H
#define ERIN_UNITS_H
#include <string>
#include <optional>

namespace erin
{

enum class PowerUnit
{
    Watt,
    KiloWatt,
    MegaWatt,
};

std::optional<PowerUnit> tag_to_power_unit(std::string const& tag);

std::string power_unit_to_string(PowerUnit unit);

double power_to_watts(double value, PowerUnit unit);

constexpr double W_per_kW = 1'000.0;
constexpr double J_per_kJ = 1'000.0;

enum class EnergyUnit
{
    Joule,
    KiloJoule,
    MegaJoule,
    WattHour,
    KiloWattHour,
    MegaWattHour,
};

std::optional<EnergyUnit> tag_to_energy_unit(std::string const& tag);

std::string energy_unit_to_string(EnergyUnit unit);

double energy_to_joules(double value, EnergyUnit unit);

enum class TimeUnit
{
    Second,
    Minute,
    Hour,
    Day,
    Week,
    Year,
};

std::optional<TimeUnit> tag_to_time_unit(std::string const& tag);

std::string time_unit_to_tag(TimeUnit unit);

double time_to_seconds(double t, TimeUnit unit);

std::string seconds_to_pretty_string(double time_s);

double time_in_seconds_to_desired_unit(double time_s, TimeUnit unit);

} // namespace erin

#endif
