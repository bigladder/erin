/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/type.h"
#include <stdexcept>
#include <sstream>

namespace ERIN
{
  ComponentType
  tag_to_component_type(const std::string& tag)
  {
    if (tag == "load") {
      return ComponentType::Load;
    }
    else if (tag == "source") {
      return ComponentType::Source;
    }
    else if (tag == "converter") {
      return ComponentType::Converter;
    }
    else {
      std::ostringstream oss;
      oss << "Unhandled tag \"" << tag << "\"";
      throw std::invalid_argument(oss.str());
    }
  }

  std::string
  component_type_to_tag(ComponentType ct)
  {
    switch(ct) {
      case ComponentType::Load:
        return std::string{"load"};
      case ComponentType::Source:
        return std::string{"source"};
      case ComponentType::Converter:
        return std::string{"converter"};
      default:
        std::ostringstream oss;
        oss << "Unhandled ComponentType \"" << static_cast<int>(ct) << "\"";
        throw std::invalid_argument(oss.str());
    }
  }

  void
  print_datum(std::ostream& os, const Datum& d)
  {
    os << "time: " << d.time
       << ", requested_value: " << d.requested_value
       << ", achieved_value: " << d.achieved_value;
  }

  ////////////////////////////////////////////////////////////
  // LoadItem
  LoadItem::LoadItem(RealTimeType t):
    time{t},
    value{-1},
    is_end{true}
  {
    if (!is_good()) {
      std::ostringstream oss;
      oss << "input is invalid for LoadItem{t=" << t << "}";
      throw std::runtime_error(oss.str());
    }
  }

  LoadItem::LoadItem(RealTimeType t, FlowValueType v):
    time{t},
    value{v},
    is_end{false}
  {
    if (!is_good()) {
      std::ostringstream oss;
      oss << "input is invalid for LoadItem{t="
          << t << ", v=" << v <<"}";
      throw std::runtime_error(oss.str());
    }
  }

  RealTimeType
  LoadItem::get_time_advance(const LoadItem& next) const
  {
    return (next.get_time() - time);
  }

  ////////////////////////////////////////////////////////////
  // Utility Functions
  FlowValueType
  clamp_toward_0(FlowValueType value, FlowValueType lower, FlowValueType upper)
  {
    if (lower > upper) {
      std::ostringstream oss;
      oss << "ERIN::clamp_toward_0 error: lower (" << lower
          << ") greater than upper (" << upper << ")"; 
      throw std::invalid_argument(oss.str());
    }
    if (value > upper) {
      if (upper > 0)
        return upper;
      else
        return 0;
    }
    else if (value < lower) {
      if (lower > 0)
        return 0;
      else
        return lower;
    }
    return value;
  }

  template<class T>
  void print_vec(const std::string& tag, const std::vector<T>& vs)
  {
    char mark = '=';
    std::cout << tag;
    for (const auto &v : vs) {
      std::cout << mark << v;
      if (mark == '=')
        mark = ',';
    }
    std::cout << std::endl;
  }

  std::string
  map_to_string(const std::unordered_map<std::string, FlowValueType>& m)
  {
    auto max_idx{m.size() - 1};
    std::ostringstream oss;
    oss << "{";
    std::unordered_map<std::string, FlowValueType>::size_type idx{0};
    for (const auto& p: m) {
      oss << "{" << p.first << ", " << p.second << "}";
      if (idx != max_idx)
        oss << ", ";
      ++idx;
    }
    oss << "}";
    return oss.str();
  }
}
