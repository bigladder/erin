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

double Power_ToWatt(double value, PowerUnit unit);

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

std::optional<EnergyUnit> TagToEnergyUnit(std::string const& tag);

std::string EnergyUnitToString(EnergyUnit unit);

double Energy_ToJoules(double value, EnergyUnit unit);

// PUBLIC
enum class TimeUnit
{
    Second,
    Minute,
    Hour,
    Day,
    Week,
    Year,
};

std::optional<TimeUnit> TagToTimeUnit(std::string const& tag);

std::string TimeUnitToTag(TimeUnit unit);

double Time_ToSeconds(double t, TimeUnit unit);

std::string SecondsToPrettyString(double time_s);

// PUBLIC
double TimeInSecondsToDesiredUnit(double time_s, TimeUnit unit);

} // namespace erin

#endif
