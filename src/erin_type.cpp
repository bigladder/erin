/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/type.h"
#include <stdexcept>
#include <sstream>

namespace ERIN
{
  TimeUnits
  tag_to_time_units(const std::string& tag)
  {
    if ((tag == "s") || (tag == "second") || (tag == "seconds")) {
      return TimeUnits::Seconds;
    }
    else if ((tag == "min") || (tag == "minute") || (tag == "minutes")) {
      return TimeUnits::Minutes;
    }
    else if ((tag == "hr") || (tag == "hour") || (tag == "hours")) {
      return TimeUnits::Hours;
    }
    else if ((tag == "day") || (tag == "days")) {
      return TimeUnits::Days;
    }
    else if ((tag == "yr") || (tag == "year") || (tag == "years")) {
      return TimeUnits::Years;
    }
    std::ostringstream oss;
    oss << "unhandled tag \"" << tag << "\"";
    throw std::invalid_argument(oss.str());
  }

  std::string
  time_units_to_tag(TimeUnits tu)
  {
    switch(tu) {
      case TimeUnits::Seconds:
        return std::string{"seconds"};
      case TimeUnits::Minutes:
        return std::string{"minutes"};
      case TimeUnits::Hours:
        return std::string{"hours"};
      case TimeUnits::Days:
        return std::string{"days"};
      case TimeUnits::Years:
        return std::string{"years"};
      default:
        std::ostringstream oss;
        oss << "Unhandled TimeUnits \"" << static_cast<int>(tu) << "\"";
        throw std::invalid_argument(oss.str());
    }
  }

  RealTimeType
  time_to_seconds(RealTimeType t, TimeUnits u)
  {
    switch (u) {
      case TimeUnits::Seconds:
        return t;
      case TimeUnits::Minutes:
        return t * static_cast<RealTimeType>(seconds_per_minute);
      case TimeUnits::Hours:
        return t * static_cast<RealTimeType>(seconds_per_hour);
      case TimeUnits::Days:
        return t * static_cast<RealTimeType>(seconds_per_day);
      case TimeUnits::Years:
        return t * static_cast<RealTimeType>(seconds_per_year);
      default:
        std::ostringstream oss;
        oss << "unhandled TimeUnits \"" << static_cast<int>(u) << "\"";
        throw std::invalid_argument(oss.str());
    }
  }

  double
  convert_time_in_seconds_to(const RealTimeType t, const TimeUnits to_units)
  {
    switch (to_units) {
      case TimeUnits::Seconds:
        return t;
      case TimeUnits::Minutes:
        return (t / seconds_per_minute);
      case TimeUnits::Hours:
        return (t / seconds_per_hour);
      case TimeUnits::Days:
        return (t / seconds_per_day);
      case TimeUnits::Years:
        return (t / seconds_per_year);
      default:
        {
          std::ostringstream oss;
          oss << "unhandled time units '" << static_cast<int>(to_units) << "'";
          throw std::runtime_error(oss.str());
        }
    }
  }

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
    else if ((tag == "mux") || (tag == "bus") || (tag == "muxer")) {
      return ComponentType::Muxer;
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
      case ComponentType::Muxer:
        return std::string{"muxer"};
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
