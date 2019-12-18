/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_GENERICS_H
#define ERIN_GENERICS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include "erin/distribution.h"
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
      if (stat_it == statistics.end()) {
        statistics[k] = calc_all_stats(results.at(k));
      }
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
    const std::string fixed_type{"fixed"}; 
    const std::string random_int_type{"random_integer"};
    auto it = m.find(std::string{"type"});
    if (it == m.end()) {
      std::ostringstream oss;
      oss << "type of distribution not found in map!";
      throw std::runtime_error(oss.str());
    }
    auto type = toml::get<std::string>(it->second);
    if constexpr (::ERIN::debug_level >= ::ERIN::debug_level_high) {
      std::cout << "type of distribution: " << type << "\n";
    }
    if (type == fixed_type) {
      auto it_v = m.find(std::string{"value"});
      if (it_v == m.end()) {
        std::ostringstream oss;
        oss << "value missing from fixed distribution specification";
        throw std::runtime_error(oss.str());
      }
      auto v = toml::get<T>(it_v->second);
      return ::erin::distribution::make_fixed<T>(v);
    }
    else if (type == random_int_type) {
      std::default_random_engine g;
      auto it_lb = m.find("lower_bound");
      auto it_ub = m.find("upper_bound");
      if ((it_lb == m.end()) || (it_ub == m.end())) {
        std::ostringstream oss;
        oss << "lower_bound and upper_bound missing from random_integer "
            << "distribution specification";
        throw std::runtime_error(oss.str());
      }
      auto lb = toml::get<T>(it_lb->second);
      auto ub = toml::get<T>(it_ub->second);
      return ::erin::distribution::make_random_integer(g, lb, ub);
    }
    std::ostringstream oss;
    oss << "unhandled distribution type\n";
    oss << "type = \"" << type << "\"\n";
    throw std::runtime_error(oss.str());
  }
}
#endif // ERIN_GENERICS_H
