/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef TOML_HELPER_H
#define TOML_HELPER_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include "erin/type.h"
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace toml_helper
{
  std::optional<std::string>
  confirm_key_present(const toml::table& tt, const std::vector<std::string>& keys)
  {
    std::string field_read{};
    auto tt_end = tt.end();
    for (const auto& k: keys) {
      auto it = tt.find(k);
      if (it != tt_end) {
        return std::optional<std::string>(k);
      }
    }
    return std::nullopt;
  }

  bool
  confirm_key_is_present(const toml::table& tt, const std::string& key)
  {
    return (tt.find(key) != tt.end());
  }


  template <class T>
  std::optional<T>
  read_table_field_with_optional(
      const toml::table& tt,
      const std::vector<std::string>& keys,
      std::string& field_read)
  {
    auto tt_end = tt.end();
    for (const auto& k: keys) {
      auto it = tt.find(k);
      if (it != tt_end) {
        field_read = k;
        return std::optional<T>{toml::get<T>(it->second)};
      }
    }
    return std::nullopt;
  }

  template <class T>
  T
  read_required_table_field(
      const toml::table& tt,
      const std::vector<std::string>& keys,
      std::string& field_read)
  {
    auto out = read_table_field_with_optional<T>(tt, keys, field_read);
    if (out.has_value()) {
      return *out;
    }
    std::ostringstream oss{};
    oss << "Required keys not found in table. Keys searched for: \n";
    for (const auto& k : keys) {
      oss << "- key: '" << k << "'\n";
    }
    throw std::out_of_range(oss.str());
  }

  template <class T>
  T
  read_optional_table_field(
      const toml::table& tt,
      const std::vector<std::string>& keys,
      const T& default_value,
      std::string& field_read)
  {
    auto out = read_table_field_with_optional<T>(tt, keys, field_read);
    return out.value_or(default_value);
  }

  std::optional<toml::value>
  read_if_present(const toml::table& tt, const std::string& key)
  {
    const auto it = tt.find(key);
    if (it == tt.end()) {
      return std::nullopt;
    }
    return std::optional<toml::value>(it->second);
  }

  double
  read_number_from_table_as_double(
      const toml::table& tt,
      const std::string& key,
      double default_value)
  {
    const auto optval = read_if_present(tt, key);
    if (!optval.has_value()) {
      return default_value;
    }
    const auto vx = *optval;
    double val = vx.is_floating() ?
      vx.as_floating(std::nothrow) :
      static_cast<double>(vx.as_integer());
    return val;
  }

  ERIN::RealTimeType
  read_value_as_int(const toml::value& v)
  {
    return v.is_integer() ?
      static_cast<ERIN::RealTimeType>(v.as_integer(std::nothrow)) :
      static_cast<ERIN::RealTimeType>(v.as_floating());
  }

  double
  read_value_as_double(const toml::value& v)
  {
    return v.is_floating() ?
      v.as_floating(std::nothrow) : static_cast<double>(v.as_integer());
  }

  ERIN::RealTimeType
  read_number_from_table_as_int(
      const toml::table& tt,
      const std::string& key,
      ERIN::RealTimeType default_value)
  {
    namespace e = ERIN;
    const auto optval = read_if_present(tt, key);
    if (!optval.has_value()) {
      return default_value;
    }
    const auto vx = *optval;
    return vx.is_integer() ?
      static_cast<e::RealTimeType>(vx.as_integer(std::nothrow)) :
      static_cast<e::RealTimeType>(vx.as_floating());
  }
}

#endif // TOML_HELPER_H
