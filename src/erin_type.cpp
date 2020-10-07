/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "debug_utils.h"
#include "erin/type.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace ERIN
{
  std::ostream&
  operator<<(std::ostream& os, const PortValue& pv)
  {
    return os << "PortValue{"
              << "port=" << pv.port << ", "
              << "value=" << pv.value << "}";
  }

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
        return t * rtt_seconds_per_minute;
      case TimeUnits::Hours:
        return t * rtt_seconds_per_hour;
      case TimeUnits::Days:
        return t * rtt_seconds_per_day;
      case TimeUnits::Years:
        return t * rtt_seconds_per_year;
      default:
        std::ostringstream oss;
        oss << "unhandled TimeUnits \"" << static_cast<int>(u) << "\"";
        throw std::invalid_argument(oss.str());
    }
  }

  RealTimeType
  time_to_seconds(double t, TimeUnits u)
  {
    switch (u) {
      case TimeUnits::Seconds:
        return static_cast<RealTimeType>(t);
      case TimeUnits::Minutes:
        return static_cast<RealTimeType>(t * seconds_per_minute);
      case TimeUnits::Hours:
        return static_cast<RealTimeType>(t * seconds_per_hour);
      case TimeUnits::Days:
        return static_cast<RealTimeType>(t * seconds_per_day);
      case TimeUnits::Years:
        return static_cast<RealTimeType>(t * seconds_per_year);
      default:
        std::ostringstream oss;
        oss << "unhandled TimeUnits \"" << static_cast<int>(u) << "\"";
        throw std::invalid_argument(oss.str());
    }
  }

  double
  convert_time_in_seconds_to(const RealTimeType t, const TimeUnits to_units)
  {
    double dt{static_cast<double>(t)};
    switch (to_units) {
      case TimeUnits::Seconds:
        return dt;
      case TimeUnits::Minutes:
        return dt / seconds_per_minute;
      case TimeUnits::Hours:
        return dt / seconds_per_hour;
      case TimeUnits::Days:
        return dt / seconds_per_day;
      case TimeUnits::Years:
        return dt / seconds_per_year;
      default:
        {
          std::ostringstream oss;
          oss << "unhandled time units '" << static_cast<int>(to_units) << "'";
          throw std::runtime_error(oss.str());
        }
    }
  }

  RateUnits
  tag_to_rate_units(const std::string& tag)
  {
    if (tag == "kW") {
      return RateUnits::KiloWatts;
    }
    std::ostringstream oss;
    oss << "unsupported rate unit for tag = '" << tag << "'";
    throw std::invalid_argument(oss.str());
  }

  std::string
  rate_units_to_tag(RateUnits ru)
  {
    switch (ru) {
      case RateUnits::KiloWatts:
        return std::string{"kW"};
    }
    std::ostringstream oss;
    oss << "unhandled rate unit '" << static_cast<int>(ru) << "'\n";
    throw std::invalid_argument(oss.str());
  }

  WorkUnits
  tag_to_work_units(const std::string& tag)
  {
    if (tag == "kJ")
      return WorkUnits::KiloJoules;
    if (tag == "kWh")
      return WorkUnits::KiloWattHours;
    std::ostringstream oss{};
    oss << "unhandled work unit for tag = '" << tag << "'\n";
    throw std::invalid_argument(oss.str());
  }

  std::string
  work_units_to_tag(WorkUnits wu)
  {
    switch (wu) {
      case WorkUnits::KiloJoules:
        return std::string{"kJ"};
      case WorkUnits::KiloWattHours:
        return std::string{"kWh"};
    }
    std::ostringstream oss{};
    oss << "unhandled work unit '" << static_cast<int>(wu) << "'\n";
    throw std::invalid_argument(oss.str());
  }

  FlowValueType
  work_to_kJ(FlowValueType work, WorkUnits units)
  {
    constexpr FlowValueType kWh_to_kJ{3600.0};
    switch (units) {
      case WorkUnits::KiloJoules:
        return work;
      case WorkUnits::KiloWattHours:
        return kWh_to_kJ * work;
    }
    std::ostringstream oss{};
    oss << "unhandled work unit '" << static_cast<int>(units) << "'\n";
    throw std::invalid_argument(oss.str());
  }

  ComponentType
  tag_to_component_type(const std::string& tag)
  {
    ComponentType ct{};
    if (tag == "load") {
      ct = ComponentType::Load;
    }
    else if (tag == "source") {
      ct = ComponentType::Source;
    }
    else if (tag == "converter") {
      ct = ComponentType::Converter;
    }
    else if ((tag == "mux") || (tag == "bus") || (tag == "muxer")) {
      ct = ComponentType::Muxer;
    }
    else if (tag == "pass_through") {
      ct = ComponentType::PassThrough;
    }
    else if ((tag == "info") || (tag == "informational") || (tag == "optional")) {
      ct = ComponentType::Informational;
    }
    else if ((tag == "store") || (tag == "storage")) {
      ct = ComponentType::Storage;
    }
    else if (tag == "uncontrolled_source") {
      ct = ComponentType::UncontrolledSource;
    }
    else if (tag == "mover") {
      ct = ComponentType::Mover;
    }
    else {
      std::ostringstream oss{};
      oss << "Unhandled tag \"" << tag << "\"";
      throw std::invalid_argument(oss.str());
    }
    return ct;
  }

  std::string
  component_type_to_tag(ComponentType ct)
  {
    std::string tag{};
    switch(ct) {
      case ComponentType::Load:
        tag = std::string{"load"};
        break;
      case ComponentType::Source:
        tag = std::string{"source"};
        break;
      case ComponentType::Converter:
        tag = std::string{"converter"};
        break;
      case ComponentType::Muxer:
        tag = std::string{"muxer"};
        break;
      case ComponentType::PassThrough:
        tag = std::string{"pass_through"};
        break;
      case ComponentType::Informational:
        tag = std::string{"informational"};
        break;
      case ComponentType::Storage:
        tag = std::string{"storage"};
        break;
      case ComponentType::UncontrolledSource:
        tag = std::string{"uncontrolled_source"};
        break;
      case ComponentType::Mover:
        tag = std::string{"mover"};
        break;
      default:
        {
          std::ostringstream oss;
          oss << "Unhandled ComponentType \"" << static_cast<int>(ct) << "\"";
          throw std::invalid_argument(oss.str());
        }
    }
    return tag;
  }

  std::ostream&
  operator<<(std::ostream& os, const Datum& d)
  {
    os << "Datum("
       << "time=" << d.time << ", "
       << "requested_value=" << d.requested_value << ", "
       << "achieved_value=" << d.achieved_value << ")";
    return os;
  }

  void
  print_datum(std::ostream& os, const Datum& d)
  {
    os << "time: " << d.time
       << ", requested_value: " << d.requested_value
       << ", achieved_value: " << d.achieved_value;
  }

  bool
  operator==(const Datum& a, const Datum& b)
  {
    const auto r_diff = std::abs(a.requested_value - b.requested_value);
    const auto a_diff = std::abs(a.achieved_value - b.achieved_value);
    const auto& tol = flow_value_tolerance;
    return (a.time == b.time) && (r_diff < tol) && (a_diff < tol);
  }

  bool
  operator!=(const Datum& a, const Datum& b)
  {
    return !(a == b);
  }

  // Hidden function: helper for sum_*_load(const std::vector<Datum>& vs)
  FlowValueType
  sum_load_helper(
      const std::vector<Datum>& vs,
      const std::function<FlowValueType(const Datum& d)>& get_flow)
  {
    if (vs.size() < 2) {
      return 0.0;
    }
    return std::inner_product(
        std::next(vs.begin()), vs.end(),
        vs.begin(), 0.0,
        std::plus<FlowValueType>(),
        [&get_flow](const Datum& a, const Datum& b) -> FlowValueType {
          const auto dt = a.time - b.time;
          const auto flow = get_flow(b);
          if (dt < 0) {
            std::ostringstream oss;
            oss << "invalid argument vs. Cannot have decreasing times";
            throw std::invalid_argument(oss.str());
          }
          return static_cast<FlowValueType>(dt) * flow;
        });
  }

  FlowValueType
  sum_requested_load(const std::vector<Datum>& vs)
  {
    return sum_load_helper(
        vs,
        [](const Datum& d) -> FlowValueType { return d.requested_value; });
  }

  FlowValueType
  sum_achieved_load(const std::vector<Datum>& vs)
  {
    return sum_load_helper(
        vs,
        [](const Datum& d) -> FlowValueType { return d.achieved_value; });
  }


  ////////////////////////////////////////////////////////////
  // LoadItem
  RealTimeType
  LoadItem::get_time_advance(const LoadItem& next) const
  {
    return (next.time - time);
  }

  bool
  operator==(const LoadItem& a, const LoadItem& b)
  {
    return (a.time == b.time) && (a.value == b.value);
  }

  bool
  operator!=(const LoadItem& a, const LoadItem& b)
  {
    return !(a == b);
  }

  std::ostream& operator<<(std::ostream& os, const LoadItem& n)
  {
    return os << "LoadItem(time=" << n.time << ", value=" << n.value << ")";
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
}
