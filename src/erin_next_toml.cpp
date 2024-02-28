#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_utils.h"
#include <iostream>

namespace erin_next
{
	// TODO: pass in a mutable vector of error strings we can push to
	// in order to decouple printing from here.
	bool
	TOMLTable_IsValid(
		std::unordered_map<toml::key, toml::value> const& table,
		std::unordered_set<std::string> const& requiredFields,
		std::unordered_set<std::string> const& optionalFields,
		std::unordered_map<std::string, std::string> const& defaults,
		std::string const& tableName,
		bool doPrint)
	{
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			if (!requiredFields.contains(it->first)
				&& !optionalFields.contains(it->first)
				&& !defaults.contains(it->first))
			{
				if (doPrint)
				{
					std::cout << "[" << tableName << "] "
						<< "Unrecognized key '" << it->first
						<< "'" << std::endl;
				}
				return false;
			}
		}
		for (auto it=requiredFields.cbegin(); it != requiredFields.cend(); ++it)
		{
			if (!table.contains(*it))
			{
				if (doPrint)
				{
					std::cout << "[" << tableName << "] "
						<< "Missing required key '" << *it << "'" << std::endl;
				}
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
				return v;
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

	std::optional<int>
	TOMLTable_ParseInteger(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName)
	{
		if (table.contains(fieldName))
		{
			auto v = TOML_ParseNumericValueAsInteger(table.at(fieldName));
			if (v.has_value())
			{
				return v;
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

	std::optional<std::vector<TimeAndAmount>>
	TOMLTable_ParseVectorOfTimeRatePairs(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName,
		double timeMult,
		double rateMult)
	{
		std::vector<TimeAndAmount> timeAndLoads{};
		if (!table.contains(fieldName) || !table.at(fieldName).is_array())
		{
			WriteErrorMessage(tableName,
				fieldName + " not present or not an array");
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
					std::optional<double> r =
						TOML_ParseNumericValueAsDouble(t_and_r.at(1));
					if (t.has_value() && r.has_value() && r.value() >= 0)
					{
						TimeAndAmount taa{};
						taa.Time_s = t.value() * timeMult;
						taa.Amount_W = static_cast<uint32_t>(r.value() * rateMult);
						timeAndLoads.push_back(std::move(taa));
					}
					else
					{
						return {};
					}
				}
				else
				{
					WriteErrorMessage(tableName,
						"time/rate pair was not of length 2");
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

	std::optional<std::vector<double>>
	TOMLTable_ParseArrayOfDouble(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName)
	{
		std::vector<double> result;
		if (!table.contains(fieldName))
		{
			WriteErrorMessage(tableName,
				"missing field '" + fieldName + "'");
			return {};
		}
		if (!table.at(fieldName).is_array())
		{
			WriteErrorMessage(tableName, "must be an array");
			return {};
		}
		std::vector<toml::value> xs = table.at(fieldName).as_array();
		for (size_t i=0; i < xs.size(); ++i)
		{
			toml::value v = xs[i];
			if (!(v.is_integer() || v.is_floating()))
			{
				WriteErrorMessage(tableName,
					"array value at " + std::to_string(i)
					+ " must be numeric");
				return {};
			}
			std::optional<double> maybeNumber =
				TOML_ParseNumericValueAsDouble(v);
			if (!maybeNumber.has_value())
			{
				WriteErrorMessage(tableName,
					"array value at " + std::to_string(i) +
					" could not be parsed as number");
				return {};
			}
			result.push_back(maybeNumber.value());
		}
		return result;
	}

	std::optional<PairsVector>
	TOMLTable_ParseArrayOfPairsOfDouble(
		std::unordered_map<toml::key, toml::value> const& table,
		std::string const& fieldName,
		std::string const& tableName)
	{
		PairsVector result;
		if (!table.contains(fieldName))
		{
			WriteErrorMessage(tableName,
				"does not contain required field '" + fieldName + "'");
			return {};
		}
		toml::value fieldData = table.at(fieldName);
		if (!fieldData.is_array())
		{
			WriteErrorMessage(tableName,
				fieldName + " must be an array of 2-element array of numbers");
			return {};
		}
		toml::array const& pairs = fieldData.as_array();
		for (size_t i = 0; i < pairs.size(); ++i)
		{
			toml::value const& pair = pairs.at(i);
			if (!pair.is_array())
			{
				WriteErrorMessage(tableName,
					"array entry at index " + std::to_string(i)
					+ " must be an array of two numbers");
				return {};
			}
			toml::array xy = pair.as_array();
			if (xy.size() != 2)
			{
				WriteErrorMessage(tableName,
					"array entry at index " + std::to_string(i)
					+ " must be an array of two numbers");
				return {};
			}
			std::optional<double> maybeFirst =
				TOML_ParseNumericValueAsDouble(xy[0]);
			std::optional<double> maybeSecond =
				TOML_ParseNumericValueAsDouble(xy[1]);
			if (!maybeFirst.has_value() || !maybeSecond.has_value())
			{
				WriteErrorMessage(tableName,
					"array entry at index " + std::to_string(i)
					+ " must be an array of two numbers");
				return {};
			}
			result.Firsts.push_back(maybeFirst.value());
			result.Seconds.push_back(maybeSecond.value());
		}
		return result;
	}

}
