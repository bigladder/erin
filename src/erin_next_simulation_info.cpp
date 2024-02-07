#include "erin_next/erin_next_simulation_info.h"
#include <iostream>
#include <unordered_set>

namespace erin_next
{
	std::unordered_set<std::string> const ValidTimeUnits{
		"hours", "minutes", "seconds",
	};

	std::unordered_set<std::string> const ValidRateUnits{
		"W", "kW", "MW",
	};

	std::unordered_set<std::string> const ValidQuantityUnits{
		"J", "kJ", "MJ",
	};

	std::unordered_set<std::string> const RequiredSimulationInfoFields{
		"time_unit", "max_time",
	};

	std::unordered_map<std::string, std::string> const
	DefaultSimulationInfoFields{
		{ "rate_unit", "kW" },
		{ "quantity_unit", "kJ" },
	};

	std::optional<SimulationInfo>
	ParseSimulationInfo(std::unordered_map<toml::key,toml::value> const& table)
	{
		SimulationInfo si{};
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			if (!RequiredSimulationInfoFields.contains(it->first)
				&& !DefaultSimulationInfoFields.contains(it->first))
			{
				std::cout << "[simulation_info] Unrecognized key '" << it->first
					<< "'" << std::endl;
				return {};
			}
		}
		for (auto it = RequiredSimulationInfoFields.cbegin();
			it != RequiredSimulationInfoFields.cend();
			++it)
		{
			if (!table.contains(*it))
			{
				std::cout << "[simulation_info] Missing required key '"
					<< *it << "'" << std::endl;
			}
		}
		if (table.contains("time_unit"))
		{
			std::string rawTimeUnit = table.at("time_unit").as_string();
			if (ValidTimeUnits.contains(rawTimeUnit))
			{
				si.TimeUnit = table.at("time_unit").as_string();
			}
			else
			{
				std::cout << "[simulation_info]: Invalid time_unit '"
					<< rawTimeUnit << "'" << std::endl;
				std::cout << "Valid units are: ";
				bool first = true;
				for (auto it = ValidTimeUnits.cbegin(); it != ValidTimeUnits.cend(); ++it)
				{
					std::cout << (first ? "" : ", ") << *it;
					first = false;
				}
				std::cout << std::endl;
				return {};
			}
		}
		else
		{
			return {};
		}
		if (table.contains("max_time"))
		{
			if (table.at("max_time").is_integer())
			{
				si.MaxTime = (double)table.at("max_time").as_integer();
			}
			else if (table.at("max_time").is_floating())
			{
				si.MaxTime = (double)table.at("max_time").as_floating();
			}
			else
			{
				std::cout << "[simulation_info] max_time value is not a number "
					<< "'" << table.at("max_time").as_string()
					<< "'" << std::endl;
				return {};
			}
		}
		else
		{
			return {};
		}
		std::string rawRateUnit;
		if (table.contains("rate_unit"))
		{
			rawRateUnit = table.at("rate_unit").as_string();
		}
		else
		{
			rawRateUnit = DefaultSimulationInfoFields.at("rate_unit");
		}
		if (ValidRateUnits.contains(rawRateUnit))
		{
			si.RateUnit = rawRateUnit;
		}
		else
		{
			std::cout << "[simulation_info] rate_unit value not valid, '"
				<< rawRateUnit << "'; valid options are ";
			bool first = true;
			for (auto it = ValidRateUnits.cbegin();
				it != ValidRateUnits.cend();
				++it)
			{
				std::cout << (first ? "" : ", ") << *it;
			}
			std::cout << std::endl;
			return {};
		}
		std::string rawQuantityUnit;
		if (table.contains("quantity_unit"))
		{
			rawQuantityUnit = table.at("quantity_unit").as_string();
		}
		else
		{
			rawQuantityUnit = DefaultSimulationInfoFields.at("quantity_unit");
		}
		if (ValidQuantityUnits.contains(rawQuantityUnit))
		{
			si.QuantityUnit = rawQuantityUnit;
		}
		else
		{
			std::cout << "[simulation_info] quantity_unit value not valid, '"
				<< rawQuantityUnit << "'; valid options are ";
			bool first = true;
			for (auto it = ValidQuantityUnits.cbegin();
				it != ValidQuantityUnits.cend();
				++it)
			{
				std::cout << (first ? "" : ", ") << *it;
			}
			std::cout << std::endl;
			return {};
		}
		return si;
	}

	bool
	operator==(SimulationInfo const& a, SimulationInfo const& b)
	{
		return a.MaxTime == b.MaxTime
			&& a.QuantityUnit == b.QuantityUnit
			&& a.RateUnit == b.RateUnit
			&& a.TimeUnit == b.TimeUnit;
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
			<< "MaxTime = " << s.MaxTime << "; "
			<< "TimeUnit = \"" << s.TimeUnit << "\"; "
			<< "QuantityUnit=\"" << s.QuantityUnit << "\"; "
			<< "RateUnit=\"" << s.RateUnit << "\";}";
		return os;
	}
}