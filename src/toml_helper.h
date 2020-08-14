/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef TOML_HELPER_H
#define TOML_HELPER_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace toml_helper
{
  template <class T>
  std::optional<T> read_table_field_with_optional(
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
  T read_required_table_field(
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
  T read_optional_table_field(
      const toml::table& tt,
      const std::vector<std::string>& keys,
      const T& default_value,
      std::string& field_read)
  {
    auto out = read_table_field_with_optional<T>(tt, keys, field_read);
    return out.value_or(default_value);
  }
}

#endif // TOML_HELPER_H
