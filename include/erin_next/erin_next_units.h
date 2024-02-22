/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_UNITS_H
#define ERIN_NEXT_UNITS_H
#include <string>
#include <optional>

namespace erin_next
{
	enum PowerUnit
	{
		Watt,
		KiloWatt,
		MegaWatt,
	};

	std::optional<PowerUnit>
	TagToPowerUnit(std::string const& tag);

	std::string
	PowerUnitToString(PowerUnit unit);

	double
	Power_ToWatt(double value, PowerUnit unit);

	constexpr double W_per_kW = 1'000.0;
	constexpr double J_per_kJ = 1'000.0;

	enum EnergyUnit
	{
		Joule,
		KiloJoule,
		MegaJoule,
		WattHour,
		KiloWattHour,
		MegaWattHour,
	};

	std::optional<EnergyUnit>
	TagToEnergyUnit(std::string const& tag);

	std::string
	EnergyUnitToString(EnergyUnit unit);

	double
	Energy_ToJoules(double value, EnergyUnit unit);

	enum TimeUnit
	{
		Second,
		Minute,
		Hour,
		Day,
		Week,
		Year,
	};

	std::optional<TimeUnit>
	TagToTimeUnit(std::string const& tag);

	std::string
	TimeUnitToTag(TimeUnit unit);

	double
	Time_ToSeconds(double t, TimeUnit unit);

	std::string
	SecondsToPrettyString(double time_s);
}

#endif