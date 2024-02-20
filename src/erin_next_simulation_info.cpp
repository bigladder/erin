#include <iostream>
#include <unordered_set>
#include "erin_next/erin_next_simulation_info.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_validation.h"

namespace erin_next
{
	std::unordered_set<std::string> const RequiredSimulationInfoFields{
		"time_unit", "max_time",
	};

	// TODO: remove rate_unit and quantity_unit; match the user guide first
	std::unordered_map<std::string, std::string> const
	DefaultSimulationInfoFields{
		{ "rate_unit", "kW" },
		{ "quantity_unit", "kJ" },
	};

	std::unordered_set<std::string> const OptionalSimulationInfoFields{
		"fixed_random", "fixed_random_series", "random_seed"
	};

	std::optional<SimulationInfo>
	ParseSimulationInfo(std::unordered_map<toml::key,toml::value> const& table)
	{
		SimulationInfo si{};
		if (!TOMLTable_IsValid(
			table,
			RequiredSimulationInfoFields,
			OptionalSimulationInfoFields,
			DefaultSimulationInfoFields,
			"simulation_info"))
		{
			return {};
		}
		auto rawTimeUnit = TOMLTable_ParseStringWithSetResponses(
			table, ValidTimeUnits, "time_unit", "simulation_info");
		if (!rawTimeUnit.has_value()) return {};
		auto maybeTimeUnit = TagToTimeUnit(rawTimeUnit.value());
		if (!maybeTimeUnit.has_value()) return {};
		si.TheTimeUnit = maybeTimeUnit.value();
		auto rawMaxTime = TOMLTable_ParseDouble(
			table, "max_time", "simulation_info");
		if (!rawMaxTime.has_value())
		{
			return {};
		}
		si.MaxTime = rawMaxTime.value();
		// TODO: remove rate unit
		auto rawRateUnit = TOMLTable_ParseStringWithSetResponses(
			table, ValidRateUnits, "rate_unit", "simulation_info");
		if (rawRateUnit.has_value())
		{
			si.RateUnit = rawRateUnit.value();
		}
		else
		{
			si.RateUnit = DefaultSimulationInfoFields.at("rate_unit");
		}
		// TODO: remove quantity unit
		auto rawQuantityUnit = TOMLTable_ParseStringWithSetResponses(
			table, ValidQuantityUnits, "quantity_unit", "simulation_info");
		if (rawQuantityUnit.has_value())
		{
			si.QuantityUnit = rawQuantityUnit.value();
		}
		else
		{
			si.QuantityUnit = DefaultSimulationInfoFields.at("quantity_unit");
		}
		RandomType rtype = RandomType::RandomFromClock;
		if (table.contains("fixed_random"))
		{
			std::optional<double> maybeFixed = TOMLTable_ParseDouble(
				table, "fixed_random", "simulation_info");
			if (!maybeFixed.has_value())
			{
				return {};
			}
			rtype = RandomType::FixedRandom;
			si.FixedValue = maybeFixed.value();
		}
		else if (table.contains("fixed_random_series"))
		{
			std::optional<std::vector<double>> maybeSeries =
				TOMLTable_ParseArrayOfDouble(
					table, "fixed_random_series", "simulation_info");
			if (!maybeSeries)
			{
				return {};
			}
			rtype = RandomType::FixedSeries;
			si.Series = std::move(maybeSeries.value());
		}
		else if (table.contains("random_seed"))
		{
			std::optional<int> maybeSeed =
				TOMLTable_ParseInteger(table, "random_seed", "simulation_info");
			if (!maybeSeed)
			{
				return {};
			}
			rtype = RandomType::RandomFromSeed;
			si.Seed = maybeSeed.value() < 0
				? (-1 * maybeSeed.value())
				: maybeSeed.value();
		}
		si.TypeOfRandom = rtype;
		return si;
	}

	bool
	operator==(SimulationInfo const& a, SimulationInfo const& b)
	{
		return a.MaxTime == b.MaxTime
			&& a.QuantityUnit == b.QuantityUnit
			&& a.RateUnit == b.RateUnit
			&& a.TheTimeUnit == b.TheTimeUnit;
	}

	bool
	operator!=(SimulationInfo const& a, SimulationInfo const& b)
	{
		return !(a == b);
	}

	std::ostream&
	operator<<(std::ostream& os, SimulationInfo const& s)
	{
		os << "SimulationInfo{"
			<< "MaxTime=" << s.MaxTime << "; "
			<< "TimeUnit=" << TimeUnitToTag(s.TheTimeUnit) << "; "
			<< "QuantityUnit=\"" << s.QuantityUnit << "\"; "
			<< "RateUnit=\"" << s.RateUnit << "\"}";
		return os;
	}
}