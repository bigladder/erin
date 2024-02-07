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
	TOMLTable_ParseDouble(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName)
	{
		if (table.contains(fieldName))
		{
			if (table.at(fieldName).is_integer())
			{
				return (double)table.at(fieldName).as_integer();
			}
			else if (table.at(fieldName).is_floating())
			{
				return (double)table.at(fieldName).as_floating();
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

}