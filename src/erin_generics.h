/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_GENERICS_H
#define ERIN_GENERICS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "../vendor/toml11/toml.hpp"
#pragma clang diagnostic pop
#include "erin/distribution.h"
#include "erin/type.h"
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
    for (const auto& k: keys) {
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
    namespace E = ::ERIN;
    const std::string fixed_type{"fixed"}; 
    const std::string random_int_type{"random_integer"};
    auto it = m.find(std::string{"type"});
    if (it == m.end()) {
      std::ostringstream oss;
      oss << "type of distribution not found in map!";
      throw std::runtime_error(oss.str());
    }
    const auto& type = toml::get<std::string>(it->second);
    if constexpr (E::debug_level >= E::debug_level_high) {
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
      std::string time_units_tag{"seconds"};
      auto it_time_units = m.find("time_unit");
      if (it_time_units != m.end()) {
        time_units_tag = toml::get<std::string>(it_time_units->second);
      }
      auto time_units = E::tag_to_time_units(time_units_tag);
      return ::erin::distribution::make_fixed<T>(
          time_to_seconds(v, time_units));
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

  template <class KeyType, class ValType>
  bool
  unordered_map_equality(
      const std::unordered_map<KeyType, ValType>& a,
      const std::unordered_map<KeyType, ValType>& b)
  {
    auto a_size = a.size();
    auto b_size = b.size();
    if (a_size != b_size) {
      return false;
    }
    for (const auto& item: a) {
      auto b_it = b.find(item.first);
      if (b_it == b.end()) {
        return false;
      }
      if (item.second != b_it->second) {
        return false;
      }
    }
    return true;
  }

  template <class KeyType, class ValType>
  bool
  unordered_map_to_vector_equality(
      const std::unordered_map<KeyType, std::vector<ValType>>& a,
      const std::unordered_map<KeyType, std::vector<ValType>>& b)
  {
    auto a_size = a.size();
    auto b_size = b.size();
    if (a_size != b_size) {
      return false;
    }
    for (const auto& item: a) {
      auto b_it = b.find(item.first);
      if (b_it == b.end()) {
        return false;
      }
      auto a_item_size = item.second.size();
      auto b_item_size = b_it->second.size();
      if (a_item_size != b_item_size) {
        return false;
      }
      for (decltype(a_item_size) i{0}; i < a_item_size; ++i) {
        if (item.second[i] != b_it->second[i]) {
          return false;
        }
      }
    }
    return true;
  }

  template <class KeyType, class ValType>
  ValType
  find_or(
      const std::unordered_map<KeyType,ValType>& map,
      const KeyType& key,
      const ValType& default_return)
  {
    auto it = map.find(key);
    if (it == map.end()) {
      return default_return;
    }
    return it->second;
  }

  template <class KeyType, class Val1Type, class Val2Type>
  Val2Type
  find_and_transform_or(
      const std::unordered_map<KeyType,Val1Type>& map,
      const KeyType& key,
      const Val2Type& default_return,
      const std::function<Val2Type(const Val1Type&)>& f)
  {
    auto it = map.find(key);
    if (it == map.end()) {
      return default_return;
    }
    return f(it->second);
  }
}
#endif // ERIN_GENERICS_H
