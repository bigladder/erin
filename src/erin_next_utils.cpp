/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_utils.h"
#include <cmath>
#include <sstream>

namespace erin
{

    Months_days_elapsed
    DayOfYearToMonthsDaysElapsed(uint64_t day_of_year)
    {
        uint64_t const doy = day_of_year % days_per_year;
        for (size_t i = 0; i < num_months; ++i)
        {
            if (doy < day_of_year_to_month[i])
            {
                // elapsed_months_of_days = the number of days (since year
                // start) represented by the elapsed months
                uint64_t elapsed_months_of_days{0};
                if (i > 0)
                {
                    elapsed_months_of_days = day_of_year_to_month[i - 1];
                }
                return Months_days_elapsed{
                    static_cast<flow_t>(i),
                    static_cast<flow_t>(doy - elapsed_months_of_days)
                };
            }
        }
        std::ostringstream oss;
        oss << "impossible condition error: should not get here\n"
            << "doy: " << doy << "\n"
            << "day_of_year: " << day_of_year << "\n"
            << "days_per_year: " << days_per_year << std::endl;
        WriteErrorMessage("impossible condition", oss.str());
        std::exit(1);
    }

    std::string
    TimeToISO8601Period(uint64_t time_seconds)
    {
        uint64_t const seconds = time_seconds % seconds_per_minute;
        std::lldiv_t const minute_div =
            std::lldiv(time_seconds, seconds_per_minute);
        uint64_t const minutes = minute_div.quot % minutes_per_hour;
        std::lldiv_t const hour_div =
            std::lldiv(time_seconds, seconds_per_hour);
        uint64_t const hours = hour_div.quot % hours_per_day;
        std::lldiv_t const day_of_year_div =
            std::lldiv(time_seconds, seconds_per_day);
        uint64_t const day_of_year = day_of_year_div.quot % days_per_year;
        auto const month_days = DayOfYearToMonthsDaysElapsed(day_of_year);
        int const days = static_cast<int>(month_days.days);
        int const months = static_cast<int>(month_days.months);
        std::lldiv_t const year_div =
            std::lldiv(time_seconds, seconds_per_year);
        uint64_t const years = year_div.quot;
        std::ostringstream oss;
        oss << "P" << std::right << std::setfill('0') << std::setw(4) << years
            << "-" << std::right << std::setfill('0') << std::setw(2) << months
            << "-" << std::right << std::setfill('0') << std::setw(2) << days
            << "T" << std::right << std::setfill('0') << std::setw(2) << hours
            << ":" << std::right << std::setfill('0') << std::setw(2) << minutes
            << ":" << std::right << std::setfill('0') << std::setw(2)
            << seconds;
        return oss.str();
    }

    double
    TimeInSecondsToHours(uint64_t time_seconds)
    {
        return static_cast<double>(time_seconds)
            / static_cast<double>(seconds_per_hour);
    }

    void
    WriteTaggedCategoryMessage(
        std::string const& category,
        std::string const& tag,
        std::string const& message
    )
    {
        std::cerr << WriteTaggedCategoryToString(category, tag, message)
                  << std::endl;
    }

    void
    WriteWarningMessage(std::string const& tag, std::string const& message)
    {
        std::cerr << WriteWarningToString(tag, message) << std::endl;
    }

    void
    WriteErrorMessage(std::string const& tag, std::string const& message)
    {
        std::cerr << WriteErrorToString(tag, message) << std::endl;
    }

    std::string
    WriteTaggedCategoryToString(
        std::string const& category,
        std::string const& tag,
        std::string const& message
    )
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

    std::string
    WriteWarningToString(std::string const& tag, std::string const& message)
    {
        return WriteTaggedCategoryToString("WARNING", tag, message);
    }

    std::string
    WriteErrorToString(std::string const& tag, std::string const& message)
    {
        return WriteTaggedCategoryToString("ERROR", tag, message);
    }

} // namespace erin
