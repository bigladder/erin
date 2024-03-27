#include "erin_next/erin_next_simulation_info.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_utils.h"
#include "erin_next/erin_next_validation.h"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <tuple>

namespace erin
{
	std::unordered_set<std::string> const RequiredSimulationInfoFields{
		"time_unit",
		"max_time",
	};

	// TODO: remove rate_unit and quantity_unit; match the user guide first
	std::unordered_map<std::string, std::string> const
		DefaultSimulationInfoFields{
			{"rate_unit", "kW"},
			{"quantity_unit", "kJ"},
		};

	std::unordered_set<std::string> const OptionalSimulationInfoFields{
		"fixed_random",
		"fixed_random_series",
		"random_seed"
	};

	// NOTE: pre-requisite, table already validated
	// TODO: change this to use unordered_map<string, InputValue>
	std::optional<SimulationInfo>
	ParseSimulationInfo(std::unordered_map<std::string, InputValue> const& table
	)
	{
		SimulationInfo si{};
		std::string rawTimeUnit =
			std::get<std::string>(table.at("time_unit").Value);
		std::optional<TimeUnit> maybeTimeUnit = TagToTimeUnit(rawTimeUnit);
		if (!maybeTimeUnit.has_value())
		{
			WriteErrorMessage(
				"simulation_info",
				"unhandled time unit string '" + rawTimeUnit + "'"
			);
			return {};
		}
		si.TheTimeUnit = maybeTimeUnit.value();
		double rawMaxTime = std::get<double>(table.at("max_time").Value);
		si.MaxTime = rawMaxTime;
		std::string rawRateUnit =
			std::get<std::string>(table.at("rate_unit").Value);
		auto maybeRateUnit = TagToPowerUnit(rawRateUnit);
		if (!maybeRateUnit.has_value())
		{
			WriteErrorMessage(
				"simulation_info", "unhandled rate unit '" + rawRateUnit + "'"
			);
			return {};
		}
		si.RateUnit = maybeRateUnit.value();
		auto rawQuantityUnit =
			std::get<std::string>(table.at("quantity_unit").Value);
		si.QuantityUnit = rawQuantityUnit;
		RandomType rtype = RandomType::RandomFromClock;
		if (table.contains("fixed_random"))
		{
			double fixedValue =
				std::get<double>(table.at("fixed_random").Value);
			rtype = RandomType::FixedRandom;
			si.FixedValue = fixedValue;
		}
		else if (table.contains("fixed_random_series"))
		{
			std::vector<double> maybeSeries = std::get<std::vector<double>>(
				table.at("fixed_random_series").Value
			);
			rtype = RandomType::FixedSeries;
			si.Series = std::move(maybeSeries);
		}
		else if (table.contains("random_seed"))
		{
			int64_t maybeSeed =
				std::get<int64_t>(table.at("random_seed").Value);
			rtype = RandomType::RandomFromSeed;
			si.Seed = static_cast<int unsigned>(
				maybeSeed < 0 ? (-1 * maybeSeed) : maybeSeed
			);
		}
		si.TypeOfRandom = rtype;
		return si;
	}

	bool
	operator==(SimulationInfo const& a, SimulationInfo const& b)
	{
		return a.MaxTime == b.MaxTime && a.QuantityUnit == b.QuantityUnit
			&& a.RateUnit == b.RateUnit && a.TheTimeUnit == b.TheTimeUnit;
	}

	bool
	operator!=(SimulationInfo const& a, SimulationInfo const& b)
	{
		return !(a == b);
	}

	std::ostream&
	operator<<(std::ostream& os, SimulationInfo const& s)
	{
		os << "SimulationInfo{" << "MaxTime=" << s.MaxTime << "; "
		   << "TimeUnit=\"" << TimeUnitToTag(s.TheTimeUnit) << "\"; "
		   << "QuantityUnit=\"" << s.QuantityUnit << "\"; " << "RateUnit=\""
		   << PowerUnitToString(s.RateUnit) << "\"}";
		return os;
	}
} // namespace erin
