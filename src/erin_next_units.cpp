/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_utils.h"
#include <exception>
#include <sstream>
#include <exception>

namespace erin_next
{
	std::optional<PowerUnit>
	TagToPowerUnit(std::string const& tag)
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

	std::string
	PowerUnitToString(PowerUnit unit)
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
				WriteErrorMessage("units", "unhandled power unit");
				std::exit(1);
			}
			break;
		}
		return result;
	}

	double
	Power_ToWatt(double value, PowerUnit unit)
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

	std::optional<EnergyUnit>
	TagToEnergyUnit(std::string const& tag)
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

	std::string
	EnergyUnitToString(EnergyUnit unit)
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
				WriteErrorMessage("units", "unhandled energy unit");
				std::exit(1);
			}
			break;
		}
		return result;
	}

	double
	Energy_ToJoules(double value, EnergyUnit unit)
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
				WriteErrorMessage("units", "unhandled energy unit");
				std::exit(1);
			}
			break;
		}
		return result;
	}

	std::optional<TimeUnit>
	TagToTimeUnit(std::string const& tag)
	{

		if (tag == "s" || tag == "sec" || tag == "secs" || tag == "second"
			|| tag == "seconds")
		{
			return TimeUnit::Second;
		}
		if (tag == "min" || tag == "mins" || tag == "minute"
			|| tag == "minutes")
		{
			return TimeUnit::Minute;
		}
		if (tag == "h" || tag == "hr" || tag == "hrs" || tag == "hour"
			|| tag == "hours")
		{
			return TimeUnit::Hour;
		}
		if (tag == "d" || tag == "ds" || tag == "day" || tag == "days")
		{
			return TimeUnit::Day;
		}
		if (tag == "week" || tag == "weeks")
		{
			return TimeUnit::Week;
		}
		if (tag == "yr" || tag == "y" || tag == "yrs" || tag == "ys"
			|| tag == "year" || tag == "years")
		{
			return TimeUnit::Year;
		}
		return {};
	}

	std::string
	TimeUnitToTag(TimeUnit unit)
	{
		std::string result;
		switch (unit)
		{
			case (TimeUnit::Second):
			{
				result = "s";
			}
			break;
			case (TimeUnit::Minute):
			{
				result = "min";
			}
			break;
			case (TimeUnit::Hour):
			{
				result = "h";
			}
			break;
			case (TimeUnit::Day):
			{
				result = "d";
			}
			break;
			case (TimeUnit::Week):
			{
				result = "week";
			}
			break;
			case (TimeUnit::Year):
			{
				result = "yr";
			}
			break;
			default:
			{
				std::ostringstream oss{};
				oss << "unhandled TimeType '" << unit << "'" << std::endl;
				WriteErrorMessage("units", oss.str());
				std::exit(1);
			}
			break;
		}
		return result;
	}

	double
	Time_ToSeconds(double t, TimeUnit unit)
	{
		switch (unit)
		{
			case (TimeUnit::Second):
			{
				return t;
			}
			break;
			case (TimeUnit::Minute):
			{
				return t * static_cast<double>(seconds_per_minute);
			}
			break;
			case (TimeUnit::Hour):
			{
				return t * static_cast<double>(seconds_per_hour);
			}
			break;
			case (TimeUnit::Day):
			{
				return t * static_cast<double>(seconds_per_day);
			}
			break;
			case (TimeUnit::Week):
			{
				return t * static_cast<double>(seconds_per_week);
			}
			break;
			case (TimeUnit::Year):
			{
				return t * static_cast<double>(seconds_per_year);
			}
			break;
		}
		std::ostringstream oss{};
		oss << "unhandled time unit '" << TimeUnitToTag(unit) << "'"
			<< std::endl;
		throw new std::invalid_argument{oss.str()};
	}

	std::string
	SecondsToPrettyString(double time_s)
	{
		size_t years = static_cast<size_t>(time_s) / seconds_per_year;
		size_t hours = static_cast<size_t>(time_s - seconds_per_year * years)
			/ seconds_per_hour;
		size_t minutes =
			static_cast<size_t>(
				time_s - seconds_per_year * years - seconds_per_hour * hours
			)
			/ seconds_per_minute;
		size_t seconds = static_cast<size_t>(
			time_s - seconds_per_year * years - seconds_per_hour * hours
			- seconds_per_minute * minutes
		);
		std::ostringstream oss{};
		oss << years << " yr " << hours << " h " << minutes << " min "
			<< seconds << " s";
		return oss.str();
	}
}
