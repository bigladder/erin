// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <cmath>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <fmt/format.h>

#include "erin/logging.h"
#include "erin/toml.h"
#include "erin/utils.h"
#include "erin/validation.h"

namespace erin
{
// TODO: change to return
// std::unordered_map<std::string, InputValue>
// InputValue will be loaded with exactly what we need
std::unordered_map<std::string, InputValue>
TOMLTable_parse_with_validation(std::unordered_map<toml::key, toml::value> const& table,
                                ValidationInfo const& validationInfo,
                                std::string const& tableName,
                                std::vector<std::string>& errors,
                                std::vector<std::string>& warnings)
{
    std::unordered_map<std::string, InputValue> out;
    std::unordered_set<std::string> fieldsFound {};
    for (auto it = table.cbegin(); it != table.cend(); ++it)
    {
        std::string key = it->first;
        toml::value value = it->second;
        if (validationInfo.required_fields.contains(key) ||
            validationInfo.optional_fields.contains(key))
        {
            if (fieldsFound.contains(key))
            {
                std::ostringstream oss;
                oss << "Duplicate field found '" << it->first << "' (for " << key << ")";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            fieldsFound.insert(key);
        }
        else
        {
            // check all aliases
            bool found = false;
            for (auto const& alias : validationInfo.aliases)
            {
                assert(key != alias.first);
                for (auto const& aliasValue : alias.second)
                {
                    if (aliasValue.tag == key)
                    {
                        found = true;
                        key = alias.first;
                        if (fieldsFound.contains(key))
                        {
                            std::ostringstream oss;
                            oss << "Duplicate field found '" << it->first << "' (for " << key
                                << ")";
                            errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                            return out;
                        }
                        fieldsFound.insert(key);
                        if (aliasValue.is_deprecated)
                        {
                            std::ostringstream oss {};
                            oss << "field '" << aliasValue.tag
                                << "' is deprecated and will be removed "
                                << "in a future version; use '" << alias.first
                                << "' instead (value = ";
                            if (value.is_string())
                            {
                                oss << value.as_string();
                            }
                            if (value.is_integer())
                            {
                                oss << value.as_integer();
                            }
                            if (value.is_floating())
                            {
                                oss << value.as_floating();
                            }
                            oss << ")";
                            warnings.push_back(fmt::format("{}: {}", tableName, oss.str()));
                        }
                        break;
                    }
                }
                if (found)
                {
                    break;
                }
            }
            if (!found)
            {
                std::ostringstream oss {};
                oss << "unhandled field '" << key << "' will be ignored";
                warnings.push_back(fmt::format("{}: {}", tableName, oss.str()));
                continue;
            }
        }
        assert(validationInfo.type_map.contains(key));
        // check types
        InputType expectedType = validationInfo.type_map.at(key);
        InputValue v;
        v.input_type = expectedType;
        switch (expectedType)
        {
        case InputType::any:
        {
            // NOTE: nothing to do
        }
        break;
        case InputType::string:
        case InputType::enum_string:
        {
            if (!value.is_string())
            {
                std::ostringstream oss;
                oss << "Expected type string for field '" << it->first << "'";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            if (validationInfo.type_map.at(key) == InputType::enum_string)
            {
                std::string const& valAsStr = value.as_string();
                if (!validationInfo.enum_map.contains(key))
                {
                    std::ostringstream oss;
                    oss << "Could not find enumerations for field '" << it->first << "'";
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                auto const& enumSet = validationInfo.enum_map.at(key);
                if (!enumSet.contains(valAsStr))
                {
                    std::ostringstream oss;
                    oss << "value for field '" << it->first << "' "
                        << "is not a valid option. Valid options are:" << std::endl;
                    for (auto const& item : enumSet)
                    {
                        oss << "- " << item << std::endl;
                    }
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
            }
            v.value = value.as_string();
        }
        break;
        case InputType::array_of_double:
        {
            if (!value.is_array())
            {
                std::ostringstream oss;
                oss << "Expected type array for field '" << it->first << "'";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            auto const& xs = value.as_array();
            std::vector<double> aod;
            aod.reserve(xs.size());
            for (size_t i = 0; i < xs.size(); ++i)
            {
                auto const& x = xs.at(i);
                if (!x.is_integer() && !x.is_floating())
                {
                    std::ostringstream oss;
                    oss << "Expected type array for field '" << it->first << "' to be a number at "
                        << "index " << i;
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                auto maybeDouble = TOML_parse_numeric_value_as_double(x);
                if (!maybeDouble.has_value())
                {
                    std::ostringstream oss;
                    oss << "Expected type array for field '" << it->first << "' to be a number at "
                        << "index " << i;
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                aod.push_back(maybeDouble.value());
            }
            v.value = std::move(aod);
        }
        break;
        case InputType::array_of_tuple2_of_number:
        {
            if (!value.is_array())
            {
                std::ostringstream oss;
                oss << "Expected type array for field '" << it->first << "'";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            std::vector<std::vector<double>> parentVec;
            auto const& xs = value.as_array();
            for (size_t i = 0; i < xs.size(); ++i)
            {
                auto const& x = xs[i];
                if (!x.is_array())
                {
                    std::ostringstream oss;
                    oss << "Expected array item at " << i << " for field '" << it->first
                        << "' to be an array of number of length 2 (a)";
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                auto const& ys = x.as_array();
                if (ys.size() != 2)
                {
                    std::ostringstream oss;
                    oss << "Expected array item at " << i << " for field '" << it->first
                        << "' to be an array of number of length 2 (b)";
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                auto const& y0 = ys.at(0);
                auto const& y1 = ys.at(1);
                if (!(y0.is_integer() || y0.is_floating()) ||
                    !(y1.is_integer() || y1.is_floating()))
                {
                    std::ostringstream oss;
                    oss << "Expected array item at " << i << " for field '" << it->first
                        << "' to be an array of number of length 2 (c)";
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                auto maybeNum0 = TOML_parse_numeric_value_as_double(y0);
                auto maybeNum1 = TOML_parse_numeric_value_as_double(y1);
                if (!maybeNum0.has_value() || !maybeNum1.has_value())
                {
                    std::ostringstream oss;
                    oss << "Expected array item at " << i << " for field '" << it->first
                        << "' to be an array of number of length 2 (c)";
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                std::vector<double> subvec {
                    maybeNum0.value(),
                    maybeNum1.value(),
                };
                parentVec.push_back(std::move(subvec));
            }
            v.value = std::move(parentVec);
        }
        break;
        case InputType::array_of_tuple3_of_string:
        {
            if (!value.is_array())
            {
                std::ostringstream oss;
                oss << "Expected type array for field '" << it->first << "'";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            std::vector<std::vector<std::string>> aos;
            auto const& xs = value.as_array();
            for (size_t i = 0; i < xs.size(); ++i)
            {
                auto const& x = xs.at(i);
                if (!x.is_array())
                {
                    std::ostringstream oss;
                    oss << "Expected array item at " << i << " for field '" << it->first
                        << "' to be an array of string of length >= 3";
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                auto const& ys = x.as_array();
                if (ys.size() < 3)
                {
                    std::ostringstream oss;
                    oss << "Expected array item at " << i << " for field '" << it->first
                        << "' to be an array of string of length >= 3";
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                aos[i] = std::vector<std::string>();
                aos[i].reserve(3);
                for (size_t j = 0; j < ys.size(); ++j)
                {
                    auto const& y = ys[j];
                    if (!y.is_string())
                    {
                        std::ostringstream oss;
                        oss << "Expected array item at " << i << " for field '" << it->first
                            << "' to be an array of string of length "
                               ">= 3";
                        errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                        return out;
                    }
                    if (j < 3)
                    {
                        aos[i].push_back(y.as_string());
                    }
                }
            }
            v.value = std::move(aos);
        }
        break;
        case InputType::boolean:
        {
            std::optional<bool> maybeBool = TOML_parse_value_as_bool(value);
            if (maybeBool.has_value())
            {
                v.value = maybeBool.value();
            }
            else
            {
                std::ostringstream oss;
                oss << "Expected an item of type bool "
                    << " for field '" << it->first << "' to be a bool, 'true', 'false', "
                    << "1, -1, or 0";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
        }
        break;
        case InputType::integer:
        {
            if (!value.is_integer())
            {
                double integerPart = 0.0;
                double floatValue = value.as_floating();
                double fracPart = std::modf(floatValue, &integerPart);
                if (!value.is_floating() || fracPart != 0.0)
                {
                    std::ostringstream oss;
                    oss << "Expected field '" << it->first << "' to be convertable to an integer;"
                        << " integer part: " << integerPart << "; fraction part: " << fracPart;
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
            }
            auto maybeInt = TOML_parse_numeric_value_as_integer(value);
            if (!maybeInt.has_value())
            {
                std::ostringstream oss;
                oss << "Expected field '" << it->first << "' to be convertable to an integer";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            v.value = static_cast<int64_t>(maybeInt.value());
        }
        break;
        case InputType::number:
        {
            if (!value.is_integer() && !value.is_floating())
            {
                std::ostringstream oss;
                oss << "Expected field '" << it->first << "' to be convertable to a real number";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            auto maybeDouble = TOML_parse_numeric_value_as_double(value);
            if (!maybeDouble.has_value())
            {
                std::ostringstream oss;
                oss << "Expected field '" << it->first << "' to be convertable to a real number";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            v.value = maybeDouble.value();
        }
        break;
        case InputType::array_of_string:
        {
            std::vector<std::string> aos;
            if (!value.is_array())
            {
                std::ostringstream oss;
                oss << "Expected field '" << it->first << "' to be an array of string";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            auto const& xs = value.as_array();
            for (size_t i = 0; i < xs.size(); ++i)
            {
                auto const& x = xs[i];
                if (!x.is_string())
                {
                    std::ostringstream oss;
                    oss << "Expected field '" << it->first << "' at " << i << " to be a string";
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                aos.push_back(x.as_string());
            }
            v.value = std::move(aos);
        }
        break;
        case InputType::map_from_string_to_string:
        {
            std::unordered_map<std::string, std::string> map;
            if (!value.is_table())
            {
                std::ostringstream oss;
                oss << "expected field '" << it->first << "' to be a map from string to string";
                errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                return out;
            }
            for (auto const& item : value.as_table())
            {
                if (!item.second.is_string())
                {
                    std::ostringstream oss;
                    oss << "expected field '" << it->first << "' at key '" << item.first
                        << "' to be a string";
                    errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
                    return out;
                }
                map[item.first] = item.second.as_string();
            }
            v.value = std::move(map);
        }
        break;
        default:
        {
            std::ostringstream oss;
            oss << "unhandled type conversion for '" << it->first << "'";
            write_error_message(tableName, oss.str());
            std::exit(1);
        }
        break;
        }
        out[key] = std::move(v);
    }
    for (std::string const& fieldsToInform : validationInfo.inform_if_missing)
    {
        if (!fieldsFound.contains(fieldsToInform))
        {
            // TODO: create an inform level?
            std::string message = fieldsToInform + " not found; default value of '" +
                                  validationInfo.default_values.at(fieldsToInform) + "' assumed";
            warnings.push_back(fmt::format("{}: {}", tableName, message));
        }
    }
    // insert defaults if not defined
    for (auto const& defkv : validationInfo.default_values)
    {
        if (out.contains(defkv.first))
        {
            continue;
        }
        InputValue iv;
        InputType itype = validationInfo.type_map.at(defkv.first);
        iv.input_type = itype;
        switch (itype)
        {
        case InputType::string:
        case InputType::enum_string:
        {
            iv.value = defkv.second;
        }
        break;
        case InputType::integer:
        {
            iv.value = std::stoll(defkv.second);
        }
        break;
        case InputType::number:
        {
            iv.value = std::stod(defkv.second);
        }
        break;
        case InputType::boolean:
        {
            if (defkv.second == "true")
            {
                iv.value = true;
            }
            else if (defkv.second == "false")
            {
                iv.value = false;
            }
            else
            {
                write_error_message(tableName,
                                    "Parse error: unhandled datatype for boolean "
                                    "default '" +
                                        defkv.second + "'");
                std::exit(1);
            }
        }
        break;
        default:
        {
            write_error_message(tableName, "Parse error: unhandled datatype for default");
            std::exit(1);
        }
        }
        out.insert({defkv.first, std::move(iv)});
    }
    // check that all required fields are present
    for (auto const& field : validationInfo.required_fields)
    {
        if (!out.contains(field))
        {
            std::ostringstream oss;
            oss << "missing required field '" << field << "'";
            errors.push_back(fmt::format("{}: {}", tableName, oss.str()));
        }
    }
    return out;
}

std::optional<std::string>
TOMLTable_parse_string(std::unordered_map<toml::key, toml::value> const& table,
                       std::string const& fieldName,
                       std::string const& tableName)
{
    if (table.contains(fieldName))
    {
        std::string rawField = table.at(fieldName).as_string();
        return rawField;
    }
    std::cout << "[" << tableName << "] does not contain expected field '" << fieldName << "'"
              << std::endl;
    return {};
}

std::optional<std::string>
TOMLTable_parse_string_with_set_responses(std::unordered_map<toml::key, toml::value> const& table,
                                          std::unordered_set<std::string> const& allowedResponses,
                                          std::string const& fieldName,
                                          std::string const& tableName)
{
    auto field = TOMLTable_parse_string(table, fieldName, tableName);
    if (field.has_value())
    {
        if (allowedResponses.contains(field.value()))
        {
            return field;
        }
        else
        {
            std::cout << "[" << tableName << "] Invalid value for field '" << fieldName << "' = '"
                      << field.value() << "'" << std::endl;
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

std::optional<bool> TOML_parse_value_as_bool(toml::value const& v)
{
    if (v.is_boolean())
    {
        return static_cast<bool>(v.as_boolean());
    }
    if (v.is_integer())
    {
        toml::value::integer_type val = v.as_integer();
        if (val == 0)
        {
            return false;
        }
        if (val == -1 || val == 1)
        {
            return true;
        }
    }
    if (v.is_string())
    {
        std::string val = v.as_string();
        if (val == "true" || val == "True")
        {
            return true;
        }
        if (val == "false" || val == "False")
        {
            return false;
        }
    }
    return {};
}

std::optional<double> TOML_parse_numeric_value_as_double(toml::value const& v)
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

std::optional<int> TOML_parse_numeric_value_as_integer(toml::value const& v)
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
TOMLTable_parse_double(std::unordered_map<toml::key, toml::value> const& table,
                       std::string const& fieldName,
                       std::string const& tableName)
{
    if (table.contains(fieldName))
    {
        auto v = TOML_parse_numeric_value_as_double(table.at(fieldName));
        if (v.has_value())
        {
            return v;
        }
        else
        {
            std::cout << "[" << tableName << "] " << fieldName << " value is not a number "
                      << "'" << table.at(fieldName).as_string() << "'" << std::endl;
        }
    }
    return {};
}

std::optional<int> TOMLTable_parse_integer(std::unordered_map<toml::key, toml::value> const& table,
                                           std::string const& fieldName,
                                           std::string const& tableName)
{
    if (table.contains(fieldName))
    {
        auto v = TOML_parse_numeric_value_as_integer(table.at(fieldName));
        if (v.has_value())
        {
            return v;
        }
        else
        {
            std::cout << "[" << tableName << "] " << fieldName << " value is not a number "
                      << "'" << table.at(fieldName).as_string() << "'" << std::endl;
        }
    }
    return {};
}

std::optional<std::vector<TimeAndAmount>>
TOMLTable_parse_vector_of_time_rate_pairs(std::unordered_map<toml::key, toml::value> const& table,
                                          std::string const& fieldName,
                                          std::string const& tableName,
                                          double timeMult,
                                          double rateMult)
{
    std::vector<TimeAndAmount> timeAndLoads {};
    if (!table.contains(fieldName) || !table.at(fieldName).is_array())
    {
        write_error_message(tableName, fieldName + " not present or not an array");
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
                std::optional<double> t = TOML_parse_numeric_value_as_double(t_and_r.at(0));
                std::optional<double> r = TOML_parse_numeric_value_as_double(t_and_r.at(1));
                if (t.has_value() && r.has_value() && r.value() >= 0)
                {
                    TimeAndAmount taa {};
                    taa.Time_s = t.value() * timeMult;
                    taa.Amount_W = static_cast<flow_t>(r.value() * rateMult);
                    timeAndLoads.push_back(std::move(taa));
                }
                else
                {
                    return {};
                }
            }
            else
            {
                write_error_message(tableName, "time/rate pair was not of length 2");
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
TOMLTable_parse_array_of_double(std::unordered_map<toml::key, toml::value> const& table,
                                std::string const& fieldName,
                                std::string const& tableName)
{
    std::vector<double> result;
    if (!table.contains(fieldName))
    {
        write_error_message(tableName, "missing field '" + fieldName + "'");
        return {};
    }
    if (!table.at(fieldName).is_array())
    {
        write_error_message(tableName, "must be an array");
        return {};
    }
    std::vector<toml::value> xs = table.at(fieldName).as_array();
    for (size_t i = 0; i < xs.size(); ++i)
    {
        toml::value v = xs[i];
        if (!(v.is_integer() || v.is_floating()))
        {
            write_error_message(tableName,
                                "array value at " + std::to_string(i) + " must be numeric");
            return {};
        }
        std::optional<double> maybeNumber = TOML_parse_numeric_value_as_double(v);
        if (!maybeNumber.has_value())
        {
            write_error_message(tableName,
                                "array value at " + std::to_string(i) +
                                    " could not be parsed as number");
            return {};
        }
        result.push_back(maybeNumber.value());
    }
    return result;
}

std::optional<PairsVector>
TOMLTable_parse_array_of_pairs_of_double(std::unordered_map<toml::key, toml::value> const& table,
                                         std::string const& fieldName,
                                         std::string const& tableName)
{
    PairsVector result;
    if (!table.contains(fieldName))
    {
        write_error_message(tableName, "does not contain required field '" + fieldName + "'");
        return {};
    }
    toml::value fieldData = table.at(fieldName);
    if (!fieldData.is_array())
    {
        write_error_message(tableName,
                            fieldName + " must be an array of 2-element array of numbers");
        return {};
    }
    toml::array const& pairs = fieldData.as_array();
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        toml::value const& pair = pairs.at(i);
        if (!pair.is_array())
        {
            write_error_message(tableName,
                                "array entry at index " + std::to_string(i) +
                                    " must be an array of two numbers");
            return {};
        }
        toml::array xy = pair.as_array();
        if (xy.size() != 2)
        {
            write_error_message(tableName,
                                "array entry at index " + std::to_string(i) +
                                    " must be an array of two numbers");
            return {};
        }
        std::optional<double> maybeFirst = TOML_parse_numeric_value_as_double(xy[0]);
        std::optional<double> maybeSecond = TOML_parse_numeric_value_as_double(xy[1]);
        if (!maybeFirst.has_value() || !maybeSecond.has_value())
        {
            write_error_message(tableName,
                                "array entry at index " + std::to_string(i) +
                                    " must be an array of two numbers");
            return {};
        }
        result.firsts.push_back(maybeFirst.value());
        result.seconds.push_back(maybeSecond.value());
    }
    return result;
}

std::unordered_set<std::string> TOMLTable_parse_component_tags_in_use(toml::value const& data)
{
    std::unordered_set<std::string> tags_in_use;
    if (!data.is_table())
    {
        return tags_in_use;
    }
    toml::table const& table = data.as_table();
    if (!table.contains("network"))
    {
        return tags_in_use;
    }
    toml::value const& network = table.at("network");
    if (!network.is_table())
    {
        return tags_in_use;
    }
    toml::table const& network_table = network.as_table();
    if (!network_table.contains("connections"))
    {
        return tags_in_use;
    }
    toml::value const& conns = network_table.at("connections");
    if (!conns.is_array())
    {
        return tags_in_use;
    }
    std::vector<toml::value> const& conns_array = conns.as_array();
    for (toml::value const& item : conns_array)
    {
        if (!item.is_array())
        {
            return tags_in_use;
        }
        std::vector<toml::value> const& item_array = item.as_array();
        // NOTE: array should be 3+ in size but we'll only access
        // the first two items.
        if (item_array.size() < 2)
        {
            return tags_in_use;
        }
        if (!item_array[0].is_string() || !item_array[1].is_string())
        {
            return tags_in_use;
        }
        std::string const& first = item_array[0].as_string();
        std::string const& second = item_array[1].as_string();
        std::string tag1 = first.substr(0, first.find(":"));
        std::string tag2 = second.substr(0, second.find(":"));
        tags_in_use.emplace(tag1);
        tags_in_use.emplace(tag2);
    }
    return tags_in_use;
}

} // namespace erin
