// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <cmath>
#include <sstream>

#include <fmt/core.h>

#include "erin/utils.h"

namespace erin
{

Months_days_elapsed day_of_year_to_months_days_elapsed(uint64_t day_of_year)
{
    uint64_t const doy = day_of_year % days_per_year;
    for (size_t i = 0; i < num_months; ++i)
    {
        if (doy < day_of_year_to_month[i])
        {
            // elapsed_months_of_days = the number of days (since year
            // start) represented by the elapsed months
            uint64_t elapsed_months_of_days {0};
            if (i > 0)
            {
                elapsed_months_of_days = day_of_year_to_month[i - 1];
            }
            return Months_days_elapsed {static_cast<flow_t>(i),
                                        static_cast<flow_t>(doy - elapsed_months_of_days)};
        }
    }
    std::ostringstream oss;
    oss << "impossible condition error: should not get here\n"
        << "doy: " << doy << "\n"
        << "day_of_year: " << day_of_year << "\n"
        << "days_per_year: " << days_per_year << std::endl;
    write_error_message("impossible condition", oss.str());
    std::exit(1);
}

std::string time_to_ISO8601_period(uint64_t time_seconds)
{
    uint64_t const seconds = time_seconds % seconds_per_minute;
    std::lldiv_t const minute_div = std::lldiv(time_seconds, seconds_per_minute);
    uint64_t const minutes = minute_div.quot % minutes_per_hour;
    std::lldiv_t const hour_div = std::lldiv(time_seconds, seconds_per_hour);
    uint64_t const hours = hour_div.quot % hours_per_day;
    std::lldiv_t const day_of_year_div = std::lldiv(time_seconds, seconds_per_day);
    uint64_t const day_of_year = day_of_year_div.quot % days_per_year;
    auto const month_days = day_of_year_to_months_days_elapsed(day_of_year);
    int const days = static_cast<int>(month_days.days);
    int const months = static_cast<int>(month_days.months);
    std::lldiv_t const year_div = std::lldiv(time_seconds, seconds_per_year);
    uint64_t const years = year_div.quot;
    std::ostringstream oss;
    oss << "P" << std::right << std::setfill('0') << std::setw(4) << years << "-" << std::right
        << std::setfill('0') << std::setw(2) << months << "-" << std::right << std::setfill('0')
        << std::setw(2) << days << "T" << std::right << std::setfill('0') << std::setw(2) << hours
        << ":" << std::right << std::setfill('0') << std::setw(2) << minutes << ":" << std::right
        << std::setfill('0') << std::setw(2) << seconds;
    return oss.str();
}

double time_in_seconds_to_hours(uint64_t time_seconds)
{
    return static_cast<double>(time_seconds) / static_cast<double>(seconds_per_hour);
}

void write_tagged_category_message(std::string const& category,
                                std::string const& tag,
                                std::string const& message)
{
    std::cerr << WriteTaggedCategoryToString(category, tag, message) << std::endl;
}

void write_warning_message(std::string const& tag, std::string const& message)
{
    std::cerr << WriteWarningToString(tag, message) << std::endl;
}

void write_error_message(std::string const& tag, std::string const& message)
{
    std::cerr << WriteErrorToString(tag, message) << std::endl;
}

std::string WriteTaggedCategoryToString(std::string const& category,
                                        std::string const& tag,
                                        std::string const& message)
{
    std::ostringstream oss;
    if (!tag.empty())
    {
        oss << "[" << category << ": " << tag << "] ";
    }
    else
    {
        oss << "[" << category << "] ";
    }
    oss << message << std::endl;
    return oss.str();
}

std::string WriteWarningToString(std::string const& tag, std::string const& message)
{
    return WriteTaggedCategoryToString("WARNING", tag, message);
}

std::string WriteErrorToString(std::string const& tag, std::string const& message)
{
    return WriteTaggedCategoryToString("ERROR", tag, message);
}

// TODO: fix, this is slow! Almost 25% of benchmark occurs here...
std::string double_to_string(double value, unsigned int precision)
{
    assert(precision <= 6);
    // NOTE: a small epsilon is added to the value to fix a bug where
    // for example, 1.505 at a fixed precision of 2 does not round to 1.51
    constexpr double eps = 1e-8;
    std::string proposed = fmt::format("{:.{}f}", value + eps, static_cast<int>(precision));
    int end_idx = static_cast<int>(proposed.size());
    bool has_decimal = false;
    for (char const& ch : proposed)
    {
        if (ch == '.')
        {
            has_decimal = true;
            break;
        }
    }
    if (has_decimal)
    {
        while (proposed[end_idx - 1] == '0' && end_idx > 0)
        {
            --end_idx;
        }
    }
    if (proposed[end_idx - 1] == '.')
    {
        --end_idx;
    }
    return proposed.substr(0, end_idx);
}

} // namespace erin
