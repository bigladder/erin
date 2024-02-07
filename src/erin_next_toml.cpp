#include <iostream>
#include "erin_next/erin_next_toml.h"

namespace erin_next
{
	bool
	TOMLTable_IsValid(
		std::unordered_map<toml::key, toml::value> const& table,
		std::unordered_set<std::string> const& requiredFields,
		std::unordered_set<std::string> const& optionalFields,
		std::unordered_map<std::string, std::string> const& defaults,
		std::string const& tableName)
	{
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			if (!requiredFields.contains(it->first)
				&& !optionalFields.contains(it->first)
				&& !defaults.contains(it->first))
			{
				std::cout << "[" << tableName << "] "
					<< "Unrecognized key '" << it->first << "'" << std::endl;
				return false;
			}
		}
		for (auto it=requiredFields.cbegin(); it != requiredFields.cend(); ++it)
		{
			if (!table.contains(*it))
			{
				std::cout << "[" << tableName << "] "
					<< "Missing required key '" << *it << "'" << std::endl;
				return false;
			}
		}
		return true;
	}

	std::optional<std::string>
	TOMLTable_ParseString(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName)
	{
		if (table.contains(fieldName))
		{
			std::string rawField = table.at(fieldName).as_string();
			return rawField;
		}
		std::cout << "[" << tableName << "] does not contain expected field '"
			<< fieldName << "'" << std::endl;
		return {};
	}

	std::optional<std::string>
	TOMLTable_ParseStringWithSetResponses(
		std::unordered_map<toml::key, toml::value> const& table,
		std::unordered_set<std::string> const& allowedResponses,
		std::string const& fieldName,
		std::string const& tableName)
	{
		auto field = TOMLTable_ParseString(table, fieldName, tableName);
		if (field.has_value())
		{
			if (allowedResponses.contains(field.value()))
			{
				return field;
			}
			else
			{
				std::cout << "[" << tableName << "] Invalid value for field '"
					<< fieldName << "' = '" << field.value() << "'"
					<< std::endl;
				std::cout << "Valid values: ";
				bool first = true;
				for (auto it = allowedResponses.cbegin(); it != allowedResponses.cend(); ++it)
				{
					std::cout << (first ? "" : ", ") << *it;
					first = false;
				}
				std::cout << std::endl;
			}
		}
		return field;
	}

	std::optional<double>
	TOML_ParseNumericValueAsDouble(toml::value const& v)
	{
		if (v.is_integer())
		{
			return (double)v.as_integer();
		}
		else if (v.is_floating())
		{
			return (double)v.as_floating();
		}
		return {};
	}

	std::optional<int>
	TOML_ParseNumericValueAsInteger(toml::value const& v)
	{
		if (v.is_integer())
		{
			return (int)v.as_integer();
		}
		else if (v.is_floating())
		{
			return (int)v.as_floating();
		}
		return {};
	}

	std::optional<double>
	TOMLTable_ParseDouble(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName)
	{
		if (table.contains(fieldName))
		{
			auto v = TOML_ParseNumericValueAsDouble(table.at(fieldName));
			if (v.has_value())
			{
				return v.value();
			}
			else
			{
				std::cout << "[" << tableName << "] "
					<< fieldName << " value is not a number "
					<< "'" << table.at(fieldName).as_string()
					<< "'" << std::endl;
			}
		}
		return {};
	}

	std::optional<std::vector<TimeAndLoad>>
	TOMLTable_ParseVectorOfTimeRatePairs(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName)
	{
		std::vector<TimeAndLoad> timeAndLoads{};
		if (!table.contains(fieldName) || !table.at(fieldName).is_array())
		{
			std::cout << "[" << tableName << "] "
				<< fieldName << " not present or not an array"
				<< std::endl;
			return {};
		}
		std::vector<toml::value> const& trs = table.at(fieldName).as_array();
		timeAndLoads.reserve(trs.size());
		for (size_t i = 0; i < trs.size(); ++i)
		{
			toml::value const& tr = trs.at(i);
			if (tr.is_array())
			{
				std::vector<toml::value> const& t_and_r = tr.as_array();
				if (t_and_r.size() == 2)
				{
					std::optional<double> t =
						TOML_ParseNumericValueAsDouble(t_and_r.at(0));
					// TODO: need to convert rate to a base unit before
					// conversion to integer in case we have decimal values
					std::optional<int> r =
						TOML_ParseNumericValueAsInteger(t_and_r.at(1));
					if (t.has_value() && r.has_value() && r.value() >= 0)
					{
						timeAndLoads.emplace_back(
							t.value(), (uint32_t)r.value());
					}
					else
					{
						return {};
					}
				}
				else
				{
					std::cout << "[" << tableName << "] "
						<< "time/rate pair was not of length 2"
						<< std::endl;
					return {};
				}
			}
			else
			{
				return {};
			}
		}
		return timeAndLoads;
	}

}