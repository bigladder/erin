// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include "erin/units.h"
#include "erin/utils.h"
#include <exception>
#include <sstream>
#include <exception>

namespace erin
{
std::optional<PowerUnit> tag_to_power_unit(std::string const& tag)
{
    if (tag == "W")
    {
        return PowerUnit::Watt;
    }
    if (tag == "kW")
    {
        return PowerUnit::KiloWatt;
    }
    if (tag == "MW")
    {
        return PowerUnit::MegaWatt;
    }
    return {};
}

std::string power_unit_to_string(PowerUnit unit)
{
    std::string result;
    switch (unit)
    {
    case (PowerUnit::Watt):
    {
        result = "W";
    }
    break;
    case (PowerUnit::KiloWatt):
    {
        result = "kW";
    }
    break;
    case (PowerUnit::MegaWatt):
    {
        result = "MW";
    }
    break;
    default:
    {
        write_error_message("units", "unhandled power unit");
        std::exit(1);
    }
    break;
    }
    return result;
}

double power_to_watts(double value, PowerUnit unit)
{
    double result = 0.0;
    switch (unit)
    {
    case (PowerUnit::Watt):
    {
        result = value;
    }
    break;
    case (PowerUnit::KiloWatt):
    {
        result = value * 1'000.0;
    }
    break;
    case (PowerUnit::MegaWatt):
    {
        result = value * 1'000'000.0;
    }
    break;
    default:
    {
        std::cout << "unhandled power unit" << std::endl;
        std::exit(1);
    }
    break;
    }
    return result;
}

std::optional<EnergyUnit> tag_to_energy_unit(std::string const& tag)
{
    if (tag == "J")
    {
        return EnergyUnit::Joule;
    }
    if (tag == "kJ")
    {
        return EnergyUnit::KiloJoule;
    }
    if (tag == "MJ")
    {
        return EnergyUnit::MegaJoule;
    }
    if (tag == "Wh")
    {
        return EnergyUnit::WattHour;
    }
    if (tag == "kWh")
    {
        return EnergyUnit::KiloWattHour;
    }
    if (tag == "MWh")
    {
        return EnergyUnit::MegaWattHour;
    }
    return {};
}

std::string energy_unit_to_string(EnergyUnit unit)
{
    std::string result;
    switch (unit)
    {
    case (EnergyUnit::Joule):
    {
        result = "J";
    }
    break;
    case (EnergyUnit::KiloJoule):
    {
        result = "kJ";
    }
    break;
    case (EnergyUnit::MegaJoule):
    {
        result = "MJ";
    }
    break;
    case (EnergyUnit::WattHour):
    {
        result = "Wh";
    }
    break;
    case (EnergyUnit::KiloWattHour):
    {
        result = "kWh";
    }
    break;
    case (EnergyUnit::MegaWattHour):
    {
        result = "MWh";
    }
    break;
    default:
    {
        write_error_message("units", "unhandled energy unit");
        std::exit(1);
    }
    break;
    }
    return result;
}

double energy_to_joules(double value, EnergyUnit unit)
{
    double result = 0.0;
    switch (unit)
    {
    case (EnergyUnit::Joule):
    {
        result = value;
    }
    break;
    case (EnergyUnit::KiloJoule):
    {
        result = value * 1'000.0;
    }
    break;
    case (EnergyUnit::MegaJoule):
    {
        result = value * 1'000'000.0;
    }
    break;
    case (EnergyUnit::WattHour):
    {
        result = value * 3'600.0;
    }
    break;
    case (EnergyUnit::KiloWattHour):
    {
        result = value * 3'600'000.0;
    }
    break;
    case (EnergyUnit::MegaWattHour):
    {
        result = value * 3'600'000'000.0;
    }
    break;
    default:
    {
        write_error_message("units", "unhandled energy unit");
        std::exit(1);
    }
    break;
    }
    return result;
}

std::optional<TimeUnit> tag_to_time_unit(std::string const& tag)
{

    if (tag == "s" || tag == "sec" || tag == "secs" || tag == "second" || tag == "seconds")
    {
        return TimeUnit::second;
    }
    if (tag == "min" || tag == "mins" || tag == "minute" || tag == "minutes")
    {
        return TimeUnit::minute;
    }
    if (tag == "h" || tag == "hr" || tag == "hrs" || tag == "hour" || tag == "hours")
    {
        return TimeUnit::hour;
    }
    if (tag == "d" || tag == "ds" || tag == "day" || tag == "days")
    {
        return TimeUnit::day;
    }
    if (tag == "week" || tag == "weeks")
    {
        return TimeUnit::week;
    }
    if (tag == "yr" || tag == "y" || tag == "yrs" || tag == "ys" || tag == "year" || tag == "years")
    {
        return TimeUnit::year;
    }
    return {};
}

std::string time_unit_to_tag(TimeUnit unit)
{
    std::string result;
    switch (unit)
    {
    case (TimeUnit::second):
    {
        result = "s";
    }
    break;
    case (TimeUnit::minute):
    {
        result = "min";
    }
    break;
    case (TimeUnit::hour):
    {
        result = "h";
    }
    break;
    case (TimeUnit::day):
    {
        result = "d";
    }
    break;
    case (TimeUnit::week):
    {
        result = "week";
    }
    break;
    case (TimeUnit::year):
    {
        result = "yr";
    }
    break;
    default:
    {
        std::ostringstream oss {};
        oss << "unhandled TimeType '" << time_unit_to_tag(unit) << "'" << std::endl;
        write_error_message("units", oss.str());
        std::exit(1);
    }
    break;
    }
    return result;
}

double time_to_seconds(double t, TimeUnit unit)
{
    switch (unit)
    {
    case (TimeUnit::second):
    {
        return t;
    }
    break;
    case (TimeUnit::minute):
    {
        return t * static_cast<double>(seconds_per_minute);
    }
    break;
    case (TimeUnit::hour):
    {
        return t * static_cast<double>(seconds_per_hour);
    }
    break;
    case (TimeUnit::day):
    {
        return t * static_cast<double>(seconds_per_day);
    }
    break;
    case (TimeUnit::week):
    {
        return t * static_cast<double>(seconds_per_week);
    }
    break;
    case (TimeUnit::year):
    {
        return t * static_cast<double>(seconds_per_year);
    }
    break;
    }
    std::ostringstream oss {};
    oss << "unhandled time unit '" << time_unit_to_tag(unit) << "'" << std::endl;
    throw new std::invalid_argument {oss.str()};
}

std::string seconds_to_pretty_string(double time_s)
{
    size_t years = static_cast<size_t>(time_s) / seconds_per_year;
    size_t hours = static_cast<size_t>(time_s - seconds_per_year * years) / seconds_per_hour;
    size_t minutes =
        static_cast<size_t>(time_s - seconds_per_year * years - seconds_per_hour * hours) /
        seconds_per_minute;
    size_t seconds = static_cast<size_t>(time_s - seconds_per_year * years -
                                         seconds_per_hour * hours - seconds_per_minute * minutes);
    std::ostringstream oss {};
    oss << years << " yr " << hours << " h " << minutes << " min " << seconds << " s";
    return oss.str();
}

double time_in_seconds_to_desired_unit(double time_s, TimeUnit unit)
{
    if (unit == TimeUnit::second)
    {
        return time_s;
    }
    if (unit == TimeUnit::minute)
    {
        return time_s / static_cast<double>(seconds_per_minute);
    }
    if (unit == TimeUnit::hour)
    {
        return time_s / static_cast<double>(seconds_per_hour);
    }
    if (unit == TimeUnit::day)
    {
        return time_s / static_cast<double>(seconds_per_day);
    }
    if (unit == TimeUnit::week)
    {
        return time_s / static_cast<double>(seconds_per_week);
    }
    if (unit == TimeUnit::year)
    {
        return time_s / static_cast<double>(seconds_per_year);
    }
    write_error_message("WriteResultsToEventFile", "unhandled time unit");
    std::exit(1);
}

} // namespace erin
