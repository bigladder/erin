#include "erin_next/erin_next_toml.h"
#include "erin/logging.h"
#include "erin_next/erin_next_utils.h"
#include "erin_next/erin_next_validation.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cmath>

namespace erin
{
    // TODO: change to return
    // std::unordered_map<std::string, InputValue>
    // InputValue will be loaded with exactly what we need
    std::unordered_map<std::string, InputValue>
    TOMLTable_ParseWithValidation(
        std::unordered_map<toml::key, toml::value> const& table,
        ValidationInfo const& validationInfo,
        std::string const& tableName,
        std::vector<std::string>& errors,
        std::vector<std::string>& warnings
    )
    {
        std::unordered_map<std::string, InputValue> out;
        std::unordered_set<std::string> fieldsFound{};
        for (auto it = table.cbegin(); it != table.cend(); ++it)
        {
            std::string key = it->first;
            toml::value value = it->second;
            if (validationInfo.RequiredFields.contains(key)
                || validationInfo.OptionalFields.contains(key))
            {
                if (fieldsFound.contains(key))
                {
                    std::ostringstream oss;
                    oss << "Duplicate field found '" << it->first << "' (for "
                        << key << ")";
                    errors.push_back(WriteErrorToString(tableName, oss.str()));
                    return out;
                }
                fieldsFound.insert(key);
            }
            else
            {
                // check all aliases
                bool found = false;
                for (auto const& alias : validationInfo.Aliases)
                {
                    assert(key != alias.first);
                    for (auto const& aliasValue : alias.second)
                    {
                        if (aliasValue.Tag == key)
                        {
                            found = true;
                            key = alias.first;
                            if (fieldsFound.contains(key))
                            {
                                std::ostringstream oss;
                                oss << "Duplicate field found '" << it->first
                                    << "' (for " << key << ")";
                                errors.push_back(
                                    WriteErrorToString(tableName, oss.str())
                                );
                                return out;
                            }
                            fieldsFound.insert(key);
                            if (aliasValue.IsDeprecated)
                            {
                                std::ostringstream oss{};
                                oss << "WARNING! field '" << aliasValue.Tag
                                    << "' is deprecated and will be removed "
                                    << "in a future version; use '"
                                    << alias.first << "' instead (value = ";
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
                                warnings.push_back(
                                    WriteErrorToString(tableName, oss.str())
                                );
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
                    std::ostringstream oss{};
                    oss << "WARNING! unhandled field '" << key
                        << "' will be ignored";
                    warnings.push_back(
                        WriteWarningToString(tableName, oss.str())
                    );
                    continue;
                }
            }
            assert(validationInfo.TypeMap.contains(key));
            // check types
            InputType expectedType = validationInfo.TypeMap.at(key);
            InputValue v;
            v.Type = expectedType;
            switch (expectedType)
            {
                case InputType::Any:
                {
                    // NOTE: nothing to do
                }
                break;
                case InputType::AnyString:
                case InputType::EnumString:
                {
                    if (!value.is_string())
                    {
                        std::ostringstream oss;
                        oss << "Expected type string for field '" << it->first
                            << "'";
                        errors.push_back(
                            WriteErrorToString(tableName, oss.str())
                        );
                        return out;
                    }
                    if (validationInfo.TypeMap.at(key) == InputType::EnumString)
                    {
                        std::string const& valAsStr = value.as_string();
                        if (!validationInfo.EnumMap.contains(key))
                        {
                            std::ostringstream oss;
                            oss << "Could not find enumerations for field '"
                                << it->first << "'";
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        auto const& enumSet = validationInfo.EnumMap.at(key);
                        if (!enumSet.contains(valAsStr))
                        {
                            std::ostringstream oss;
                            oss << "value for field '" << it->first << "' "
                                << "is not a valid option. Valid options are:"
                                << std::endl;
                            for (auto const& item : enumSet)
                            {
                                oss << "- " << item << std::endl;
                            }
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                    }
                    v.Value = value.as_string();
                }
                break;
                case InputType::ArrayOfDouble:
                {
                    if (!value.is_array())
                    {
                        std::ostringstream oss;
                        oss << "Expected type array for field '" << it->first
                            << "'";
                        errors.push_back(
                            WriteErrorToString(tableName, oss.str())
                        );
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
                            oss << "Expected type array for field '"
                                << it->first << "' to be a number at "
                                << "index " << i;
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        auto maybeDouble = TOML_ParseNumericValueAsDouble(x);
                        if (!maybeDouble.has_value())
                        {
                            std::ostringstream oss;
                            oss << "Expected type array for field '"
                                << it->first << "' to be a number at "
                                << "index " << i;
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        aod.push_back(maybeDouble.value());
                    }
                    v.Value = std::move(aod);
                }
                break;
                case InputType::ArrayOfTuple2OfNumber:
                {
                    if (!value.is_array())
                    {
                        std::ostringstream oss;
                        oss << "Expected type array for field '" << it->first
                            << "'";
                        errors.push_back(
                            WriteErrorToString(tableName, oss.str())
                        );
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
                            oss << "Expected array item at " << i
                                << " for field '" << it->first
                                << "' to be an array of number of length 2 (a)";
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        auto const& ys = x.as_array();
                        if (ys.size() != 2)
                        {
                            std::ostringstream oss;
                            oss << "Expected array item at " << i
                                << " for field '" << it->first
                                << "' to be an array of number of length 2 (b)";
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        auto const& y0 = ys.at(0);
                        auto const& y1 = ys.at(1);
                        if (!(y0.is_integer() || y0.is_floating())
                            || !(y1.is_integer() || y1.is_floating()))
                        {
                            std::ostringstream oss;
                            oss << "Expected array item at " << i
                                << " for field '" << it->first
                                << "' to be an array of number of length 2 (c)";
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        auto maybeNum0 = TOML_ParseNumericValueAsDouble(y0);
                        auto maybeNum1 = TOML_ParseNumericValueAsDouble(y1);
                        if (!maybeNum0.has_value() || !maybeNum1.has_value())
                        {
                            std::ostringstream oss;
                            oss << "Expected array item at " << i
                                << " for field '" << it->first
                                << "' to be an array of number of length 2 (c)";
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        std::vector<double> subvec{
                            maybeNum0.value(),
                            maybeNum1.value(),
                        };
                        parentVec.push_back(std::move(subvec));
                    }
                    v.Value = std::move(parentVec);
                }
                break;
                case InputType::ArrayOfTuple3OfString:
                {
                    if (!value.is_array())
                    {
                        std::ostringstream oss;
                        oss << "Expected type array for field '" << it->first
                            << "'";
                        errors.push_back(
                            WriteErrorToString(tableName, oss.str())
                        );
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
                            oss << "Expected array item at " << i
                                << " for field '" << it->first
                                << "' to be an array of string of length >= 3";
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        auto const& ys = x.as_array();
                        if (ys.size() < 3)
                        {
                            std::ostringstream oss;
                            oss << "Expected array item at " << i
                                << " for field '" << it->first
                                << "' to be an array of string of length >= 3";
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
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
                                oss << "Expected array item at " << i
                                    << " for field '" << it->first
                                    << "' to be an array of string of length "
                                       ">= 3";
                                errors.push_back(
                                    WriteErrorToString(tableName, oss.str())
                                );
                                return out;
                            }
                            if (j < 3)
                            {
                                aos[i].push_back(y.as_string());
                            }
                        }
                    }
                    v.Value = std::move(aos);
                }
                break;
                case InputType::Integer:
                {
                    if (!value.is_integer())
                    {
                        double integerPart = 0.0;
                        double floatValue = value.as_floating();
                        double fracPart = std::modf(floatValue, &integerPart);
                        if (!value.is_floating() || fracPart != 0.0)
                        {
                            std::ostringstream oss;
                            oss << "Expected field '" << it->first
                                << "' to be convertable to an integer;"
                                << " integer part: " << integerPart
                                << "; fraction part: " << fracPart;
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                    }
                    auto maybeInt = TOML_ParseNumericValueAsInteger(value);
                    if (!maybeInt.has_value())
                    {
                        std::ostringstream oss;
                        oss << "Expected field '" << it->first
                            << "' to be convertable to an integer";
                        errors.push_back(
                            WriteErrorToString(tableName, oss.str())
                        );
                        return out;
                    }
                    v.Value = static_cast<int64_t>(maybeInt.value());
                }
                break;
                case InputType::Number:
                {
                    if (!value.is_integer() && !value.is_floating())
                    {
                        std::ostringstream oss;
                        oss << "Expected field '" << it->first
                            << "' to be convertable to a real number";
                        errors.push_back(
                            WriteErrorToString(tableName, oss.str())
                        );
                        return out;
                    }
                    auto maybeDouble = TOML_ParseNumericValueAsDouble(value);
                    if (!maybeDouble.has_value())
                    {
                        std::ostringstream oss;
                        oss << "Expected field '" << it->first
                            << "' to be convertable to a real number";
                        errors.push_back(
                            WriteErrorToString(tableName, oss.str())
                        );
                        return out;
                    }
                    v.Value = maybeDouble.value();
                }
                break;
                case InputType::ArrayOfString:
                {
                    std::vector<std::string> aos;
                    if (!value.is_array())
                    {
                        std::ostringstream oss;
                        oss << "Expected field '" << it->first
                            << "' to be an array of string";
                        errors.push_back(
                            WriteErrorToString(tableName, oss.str())
                        );
                        return out;
                    }
                    auto const& xs = value.as_array();
                    for (size_t i = 0; i < xs.size(); ++i)
                    {
                        auto const& x = xs[i];
                        if (!x.is_string())
                        {
                            std::ostringstream oss;
                            oss << "Expected field '" << it->first << "' at "
                                << i << " to be a string";
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        aos.push_back(x.as_string());
                    }
                    v.Value = std::move(aos);
                }
                break;
                case InputType::MapFromStringToString:
                {
                    std::unordered_map<std::string, std::string> map;
                    if (!value.is_table())
                    {
                        std::ostringstream oss;
                        oss << "expected field '" << it->first
                            << "' to be a map from string to string";
                        errors.push_back(
                            WriteErrorToString(tableName, oss.str())
                        );
                        return out;
                    }
                    for (auto const& item : value.as_table())
                    {
                        if (!item.second.is_string())
                        {
                            std::ostringstream oss;
                            oss << "expected field '" << it->first
                                << "' at key '" << item.first
                                << "' to be a string";
                            errors.push_back(
                                WriteErrorToString(tableName, oss.str())
                            );
                            return out;
                        }
                        map[item.first] = item.second.as_string();
                    }
                    v.Value = std::move(map);
                }
                break;
                default:
                {
                    std::ostringstream oss;
                    oss << "unhandled type conversion for '" << it->first
                        << "'";
                    WriteErrorMessage(tableName, oss.str());
                    std::exit(1);
                }
                break;
            }
            out[key] = std::move(v);
        }
        for (std::string const& fieldsToInform : validationInfo.InformIfMissing)
        {
            if (!fieldsFound.contains(fieldsToInform))
            {
                // TODO: create an inform level?
                std::string message = fieldsToInform
                    + " not found; default value of '"
                    + validationInfo.Defaults.at(fieldsToInform) + "' assumed";
                warnings.push_back(WriteWarningToString(tableName, message));
            }
        }
        // insert defaults if not defined
        for (auto const& defkv : validationInfo.Defaults)
        {
            if (out.contains(defkv.first))
            {
                continue;
            }
            InputValue iv;
            InputType itype = validationInfo.TypeMap.at(defkv.first);
            iv.Type = itype;
            switch (itype)
            {
                case InputType::AnyString:
                case InputType::EnumString:
                {
                    iv.Value = defkv.second;
                }
                break;
                case InputType::Integer:
                {
                    iv.Value = std::stoll(defkv.second);
                }
                break;
                case InputType::Number:
                {
                    iv.Value = std::stod(defkv.second);
                }
                break;
                default:
                {
                    WriteErrorMessage(
                        tableName, "Parse error: unhandled datatype for default"
                    );
                    std::exit(1);
                }
            }
            out.insert({defkv.first, std::move(iv)});
        }
        // check that all required fields are present
        for (auto const& field : validationInfo.RequiredFields)
        {
            if (!out.contains(field))
            {
                std::ostringstream oss;
                oss << "missing required field '" << field << "'";
                errors.push_back(WriteErrorToString(tableName, oss.str()));
            }
        }
        return out;
    }

    // TODO: pass in a mutable vector of error strings we can push to
    // in order to decouple printing from here.
    // TODO: see header; also pass in list of aliases for keys
    bool
    TOMLTable_IsValid(
        std::unordered_map<toml::key, toml::value> const& table,
        std::unordered_set<std::string> const& requiredFields,
        std::unordered_set<std::string> const& optionalFields,
        std::unordered_map<std::string, std::string> const& defaults,
        std::string const& tableName,
        bool verbose,
        Log const& log
    )
    {
        for (auto it = table.cbegin(); it != table.cend(); ++it)
        {
            if (!requiredFields.contains(it->first)
                && !optionalFields.contains(it->first)
                && !defaults.contains(it->first))
            {
                if (verbose)
                {
                    Log_Warning(log, tableName, fmt::format("Unrecognized key '{}'", it->first));
                }
                return false;
            }
        }
        for (auto it = requiredFields.cbegin(); it != requiredFields.cend();
             ++it)
        {
            if (!table.contains(*it))
            {
                if (verbose)
                {
                    Log_Error(log, tableName, fmt::format("Missing required key '{}'", *it));
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
        std::string const& tableName
    )
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
        std::string const& tableName
    )
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
                for (auto it = allowedResponses.cbegin();
                     it != allowedResponses.cend();
                     ++it)
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
        std::string const& tableName
    )
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
                std::cout << "[" << tableName << "] " << fieldName
                          << " value is not a number " << "'"
                          << table.at(fieldName).as_string() << "'"
                          << std::endl;
            }
        }
        return {};
    }

    std::optional<int>
    TOMLTable_ParseInteger(
        std::unordered_map<toml::key, toml::value> const& table,
        std::string const& fieldName,
        std::string const& tableName
    )
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
                std::cout << "[" << tableName << "] " << fieldName
                          << " value is not a number " << "'"
                          << table.at(fieldName).as_string() << "'"
                          << std::endl;
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
        double rateMult
    )
    {
        std::vector<TimeAndAmount> timeAndLoads{};
        if (!table.contains(fieldName) || !table.at(fieldName).is_array())
        {
            WriteErrorMessage(
                tableName, fieldName + " not present or not an array"
            );
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
                        taa.Amount_W =
                            static_cast<flow_t>(r.value() * rateMult);
                        timeAndLoads.push_back(std::move(taa));
                    }
                    else
                    {
                        return {};
                    }
                }
                else
                {
                    WriteErrorMessage(
                        tableName, "time/rate pair was not of length 2"
                    );
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
        std::string const& tableName
    )
    {
        std::vector<double> result;
        if (!table.contains(fieldName))
        {
            WriteErrorMessage(tableName, "missing field '" + fieldName + "'");
            return {};
        }
        if (!table.at(fieldName).is_array())
        {
            WriteErrorMessage(tableName, "must be an array");
            return {};
        }
        std::vector<toml::value> xs = table.at(fieldName).as_array();
        for (size_t i = 0; i < xs.size(); ++i)
        {
            toml::value v = xs[i];
            if (!(v.is_integer() || v.is_floating()))
            {
                WriteErrorMessage(
                    tableName,
                    "array value at " + std::to_string(i) + " must be numeric"
                );
                return {};
            }
            std::optional<double> maybeNumber =
                TOML_ParseNumericValueAsDouble(v);
            if (!maybeNumber.has_value())
            {
                WriteErrorMessage(
                    tableName,
                    "array value at " + std::to_string(i)
                        + " could not be parsed as number"
                );
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
        std::string const& tableName
    )
    {
        PairsVector result;
        if (!table.contains(fieldName))
        {
            WriteErrorMessage(
                tableName, "does not contain required field '" + fieldName + "'"
            );
            return {};
        }
        toml::value fieldData = table.at(fieldName);
        if (!fieldData.is_array())
        {
            WriteErrorMessage(
                tableName,
                fieldName + " must be an array of 2-element array of numbers"
            );
            return {};
        }
        toml::array const& pairs = fieldData.as_array();
        for (size_t i = 0; i < pairs.size(); ++i)
        {
            toml::value const& pair = pairs.at(i);
            if (!pair.is_array())
            {
                WriteErrorMessage(
                    tableName,
                    "array entry at index " + std::to_string(i)
                        + " must be an array of two numbers"
                );
                return {};
            }
            toml::array xy = pair.as_array();
            if (xy.size() != 2)
            {
                WriteErrorMessage(
                    tableName,
                    "array entry at index " + std::to_string(i)
                        + " must be an array of two numbers"
                );
                return {};
            }
            std::optional<double> maybeFirst =
                TOML_ParseNumericValueAsDouble(xy[0]);
            std::optional<double> maybeSecond =
                TOML_ParseNumericValueAsDouble(xy[1]);
            if (!maybeFirst.has_value() || !maybeSecond.has_value())
            {
                WriteErrorMessage(
                    tableName,
                    "array entry at index " + std::to_string(i)
                        + " must be an array of two numbers"
                );
                return {};
            }
            result.Firsts.push_back(maybeFirst.value());
            result.Seconds.push_back(maybeSecond.value());
        }
        return result;
    }

    std::unordered_set<std::string>
    TOMLTable_ParseComponentTagsInUse(toml::value const& data)
    {
        std::unordered_set<std::string> tagsInUse;
        if (!data.is_table())
        {
            return tagsInUse;
        }
        toml::table const& table = data.as_table();
        if (!table.contains("network"))
        {
            return tagsInUse;
        }
        toml::value const& network = table.at("network");
        if (!network.is_table())
        {
            return tagsInUse;
        }
        toml::table const& networkTable = network.as_table();
        if (!networkTable.contains("connections"))
        {
            return tagsInUse;
        }
        toml::value const& conns = networkTable.at("connections");
        if (!conns.is_array())
        {
            return tagsInUse;
        }
        std::vector<toml::value> const& connsArray = conns.as_array();
        for (toml::value const& item : connsArray)
        {
            if (!item.is_array())
            {
                return tagsInUse;
            }
            std::vector<toml::value> const& itemArray = item.as_array();
            // NOTE: array should be 3+ in size but we'll only access
            // the first two items.
            if (itemArray.size() < 2)
            {
                return tagsInUse;
            }
            if (!itemArray[0].is_string() || !itemArray[1].is_string())
            {
                return tagsInUse;
            }
            std::string const& first = itemArray[0].as_string();
            std::string const& second = itemArray[1].as_string();
            std::string tag1 = first.substr(0, first.find(":"));
            std::string tag2 = second.substr(0, second.find(":"));
            tagsInUse.emplace(tag1);
            tagsInUse.emplace(tag2);
        }
        return tagsInUse;
    }

} // namespace erin
