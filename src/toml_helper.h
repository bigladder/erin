/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef TOML_HELPER_H
#define TOML_HELPER_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace toml_helper
{
  template <class T>
  T read_required_table_field(
      const toml::table& tt,
      const std::vector<std::string>& keys,
      std::string& field_read)
  {
    auto tt_end = tt.end();
    for (const auto& k: keys) {
      auto it = tt.find(k);
      if (it != tt_end) {
        field_read = k;
        return toml::get<T>(it->second);
      }
    }
    throw std::out_of_range("required keys not found in table");
  }

  template <class T>
  T read_optional_table_field(
      const toml::table& tt,
      const std::vector<std::string>& keys,
      const T& default_value,
      std::string& field_read)
  {
    try {
      return read_required_table_field<T>(tt, keys, field_read);
    }
    catch (const std::out_of_range&) {
      return default_value;
    }
  }
}

#endif // TOML_HELPER_H
