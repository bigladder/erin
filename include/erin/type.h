/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_TYPE_H
#define ERIN_TYPE_H
#include "../../vendor/bdevs/include/adevs.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ERIN
{
  using FlowValueType = double;
  using RealTimeType = int;
  using LogicalTimeType = int;
  using PortValue = adevs::port_value<FlowValueType>;

  const FlowValueType flow_value_tolerance{1e-6};
  const auto inf = adevs_inf<adevs::Time>();

  // Time Conversion Factors
  constexpr double seconds_per_minute{60.0};
  constexpr double minutes_per_hour{60.0};
  constexpr double seconds_per_hour{seconds_per_minute * minutes_per_hour};
  constexpr double hours_per_day{24.0};
  constexpr double seconds_per_day{seconds_per_hour * hours_per_day};
  constexpr double days_per_year{365.25};
  constexpr double seconds_per_year{seconds_per_day * days_per_year};

  ////////////////////////////////////////////////////////////
  // TimeUnits
  enum class TimeUnits
  {
    Seconds = 0,
    Minutes,
    Hours,
    Days,
    Years
  };

  TimeUnits tag_to_time_units(const std::string& tag);
  std::string time_units_to_tag(TimeUnits tu);
  RealTimeType time_to_seconds(RealTimeType max_time, TimeUnits time_unit);

  ////////////////////////////////////////////////////////////
  // ComponentType
  enum class ComponentType
  {
    Load = 0,
    Source,
    Converter
  };

  ComponentType tag_to_component_type(const std::string& tag);

  std::string component_type_to_tag(ComponentType ct);

  ////////////////////////////////////////////////////////////
  // Datum
  struct Datum
  {
    RealTimeType time;
    FlowValueType requested_value;
    FlowValueType achieved_value;
  };

  void
  print_datum(std::ostream& os, const Datum& d);

  //////////////////////////////////////////////////////////// 
  // LoadItem
  class LoadItem
  {
    public:
      explicit LoadItem(RealTimeType t);
      LoadItem(RealTimeType t, FlowValueType v);
      [[nodiscard]] RealTimeType get_time() const { return time; }
      [[nodiscard]] FlowValueType get_value() const { return value; }
      [[nodiscard]] bool get_is_end() const { return is_end; }
      [[nodiscard]] RealTimeType get_time_advance(const LoadItem& next) const;

    private:
      RealTimeType time;
      FlowValueType value;
      bool is_end;

      [[nodiscard]] bool is_good() const { return (time >= 0); }
  };

  ////////////////////////////////////////////////////////////
  // Utility Functions
  FlowValueType clamp_toward_0(FlowValueType value, FlowValueType lower, FlowValueType upper);
  template<class T>
  void print_vec(const std::string& tag, const std::vector<T>& vs);
  std::string map_to_string(
      const std::unordered_map<std::string,FlowValueType>& m);
}

#endif // ERIN_TYPE_H
