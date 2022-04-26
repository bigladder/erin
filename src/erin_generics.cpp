/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin_generics.h"

namespace erin_generics
{
  std::function<ERIN::RealTimeType(void)>
  read_toml_distribution(const std::unordered_map<std::string, toml::value>& m)
  {
    namespace E = ERIN;
    namespace th = toml_helper;
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
      auto v =
        static_cast<E::RealTimeType>(th::read_value_as_int(it_v->second));
      std::string time_units_tag{"seconds"};
      auto it_time_units = m.find("time_unit");
      if (it_time_units != m.end()) {
        time_units_tag = toml::get<std::string>(it_time_units->second);
      }
      auto time_units = E::tag_to_time_units(time_units_tag);
      return erin::distribution::make_fixed<E::RealTimeType>(
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
      auto lb =
        static_cast<E::RealTimeType>(th::read_value_as_int(it_lb->second));
      auto ub =
        static_cast<E::RealTimeType>(th::read_value_as_int(it_ub->second));
      return erin::distribution::make_random_integer(g, lb, ub);
    }
    std::ostringstream oss;
    oss << "unhandled distribution type\n";
    oss << "type = \"" << type << "\"\n";
    throw std::runtime_error(oss.str());
  }

}