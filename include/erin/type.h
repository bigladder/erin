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
