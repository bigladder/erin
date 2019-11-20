/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_GENERICS_H
#define ERIN_GENERICS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include "erin/distributions.h"
#include "debug_utils.h"
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace erin_generics
{
  template <class TOut, class TResElem, class TStats>
  std::unordered_map<std::string, TOut>
  derive_statistic(
      const std::unordered_map<std::string, std::vector<TResElem>>& results,
      const std::vector<std::string>& keys,
      std::unordered_map<std::string, TStats>& statistics,
      const std::function<TStats(const std::vector<TResElem>&)>& calc_all_stats,
      const std::function<TOut(const TStats&)>& derive_stat)
  {
    std::unordered_map<std::string, TOut> out{};
    for (const auto k: keys) {
      auto stat_it = statistics.find(k);
      if (stat_it == statistics.end())
        statistics[k] = calc_all_stats(results.at(k));
      out[k] = derive_stat(statistics[k]);
    }
    return out;
  }

  template <class T>
  void
  print_unordered_map(const std::string& var, const std::unordered_map<std::string,T>& m)
  {
    std::cout << var << " = \n";
    for (const auto& e: m) {
      std::cout << "  " << e.first << ": " << e.second << "\n";
    }
  }

  template <class T>
  std::function<T(void)>
  read_toml_distribution(const std::unordered_map<std::string, toml::value>& m)
  {
    auto it = m.find(std::string{"type"});
    if (it == m.end()) {
      std::ostringstream oss;
      oss << "type of distribution not found in map!";
      throw std::runtime_error(oss.str());
    }
    auto type = toml::get<std::string>(it->second);
    if constexpr (::ERIN::debug_level >= ::ERIN::debug_level_low) {
      std::cout << "type of distribution: " << type << "\n";
    }
    return ::erin::distribution::make_fixed(10);
  }
}
#endif // ERIN_GENERICS_H
