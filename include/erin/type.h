/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_TYPE_H
#define ERIN_TYPE_H
#include "adevs.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ERIN
{
  using size_type = std::vector<std::int64_t>::size_type;
  using FlowValueType = double;
  using RealTimeType = std::int64_t;
  using LogicalTimeType = int;
  using Time = adevs::SuperDenseTime<RealTimeType>;
  using PortValue = adevs::port_value<FlowValueType>;

  std::ostream& operator<<(std::ostream& os, const PortValue& pv);

  constexpr FlowValueType flow_value_tolerance{1e-6};
  constexpr FlowValueType neg_flow_value_tol = -1 * flow_value_tolerance;
  const auto inf = adevs_inf<Time>();
  constexpr int precision_for_output{16};

  // Time Conversion Factors
  constexpr double seconds_per_minute{60.0};
  constexpr double minutes_per_hour{60.0};
  constexpr double seconds_per_hour{seconds_per_minute * minutes_per_hour};
  constexpr double hours_per_day{24.0};
  constexpr double seconds_per_day{seconds_per_hour * hours_per_day};
  // Note: there are actually 365.25 days per year but our time clock doesn't acknowledge
  // leap years so we use a slightly lower factor. Hopefully, this won't bite us...
  // For this simulation, one year is 365 days; period.
  constexpr double days_per_year{365.0};
  constexpr double seconds_per_year{seconds_per_day * days_per_year};

  constexpr RealTimeType rtt_seconds_per_minute{60};
  constexpr RealTimeType rtt_minutes_per_hour{60};
  constexpr RealTimeType rtt_seconds_per_hour{rtt_seconds_per_minute * rtt_minutes_per_hour};
  constexpr RealTimeType rtt_hours_per_day{24};
  constexpr RealTimeType rtt_seconds_per_day{rtt_seconds_per_hour * rtt_hours_per_day};
  // See note above about year length
  constexpr RealTimeType rtt_days_per_year{365};
  constexpr RealTimeType rtt_seconds_per_year{rtt_seconds_per_day * rtt_days_per_year};

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
  RealTimeType time_to_seconds(double max_time, TimeUnits time_unit);
  RealTimeType time_to_seconds(RealTimeType time, TimeUnits time_units);
  double convert_time_in_seconds_to(
      const RealTimeType t, const TimeUnits to_units);

  ////////////////////////////////////////////////////////////
  // RateUnits - Work per Unit Time
  enum class RateUnits
  {
    KiloWatts = 0,
  };

  RateUnits tag_to_rate_units(const std::string& tag);
  std::string rate_units_to_tag(RateUnits ru);

  //////////////////////////////////////////////////////////// 
  // WorkUnits
  enum class WorkUnits
  {
    KiloJoules = 0,
    KiloWattHours,
  };

  WorkUnits tag_to_work_units(const std::string& tag);
  std::string work_units_to_tag(WorkUnits wu);
  FlowValueType work_to_kJ(FlowValueType work, WorkUnits units);

  ////////////////////////////////////////////////////////////
  // ComponentType
  enum class ComponentType
  {
    Load = 0,
    Source,
    Converter,
    Muxer,
    PassThrough,
    Informational,
    Storage,
    UncontrolledSource,
    Mover
  };

  ComponentType tag_to_component_type(const std::string& tag);

  std::string component_type_to_tag(ComponentType ct);

  ////////////////////////////////////////////////////////////
  // Datum
  struct Datum
  {
    RealTimeType time{0};
    FlowValueType requested_value{0.0};
    FlowValueType achieved_value{0.0};
  };

  std::ostream& operator<<(std::ostream& os, const Datum& d);
  void print_datum(std::ostream& os, const Datum& d);
  bool operator==(const Datum& a, const Datum& b);
  bool operator!=(const Datum& a, const Datum& b);
  FlowValueType sum_requested_load(const std::vector<Datum>& vs);
  FlowValueType sum_achieved_load(const std::vector<Datum>& vs);

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

      friend bool operator==(const LoadItem& a, const LoadItem& b);
      friend bool operator!=(const LoadItem& a, const LoadItem& b);
      friend std::ostream& operator<<(std::ostream& os, const LoadItem& n);

    private:
      RealTimeType time;
      FlowValueType value;
      bool is_end;

      [[nodiscard]] bool is_good() const { return (time >= 0); }
  };

  bool operator==(const LoadItem& a, const LoadItem& b);
  bool operator!=(const LoadItem& a, const LoadItem& b);
  std::ostream& operator<<(std::ostream& os, const LoadItem& n);

  ////////////////////////////////////////////////////////////
  // Utility Functions
  FlowValueType clamp_toward_0(FlowValueType value, FlowValueType lower, FlowValueType upper);

  template<class T>
  std::string
  vec_to_string(const std::vector<T>& vs)
  {
    std::ostringstream oss;
    oss << '[';
    bool first{true};
    for (const auto &v : vs) {
      if (first) {
        first = false;
      }
      else {
        oss << ',';
      }
      oss << v;
    }
    oss << ']';
    return oss.str();
  }

  template<class T>
  void
  print_vec(const std::string& tag, const std::vector<T>& vs)
  {
    std::cout << tag << " = " << vec_to_string<T>(vs) << "\n";
  }

  template<class T>
  std::string
  map_to_string(const std::unordered_map<std::string, T>& m)
  {
    using size_type =
      typename std::unordered_map<std::string,T>::size_type;
    auto max_idx{m.size() - 1};
    std::ostringstream oss;
    oss << "{";
    size_type idx{0};
    for (const auto& p: m) {
      oss << "{" << p.first << ", " << p.second << "}";
      if (idx != max_idx) {
        oss << ", ";
      }
      ++idx;
    }
    oss << "}";
    return oss.str();
  }

  template<class T>
  std::string
  map_of_vec_to_string(const std::unordered_map<std::string, std::vector<T>>& m)
  {
    using size_type =
      typename std::unordered_map<std::string,std::vector<T>>::size_type;
    auto max_idx{m.size() - 1};
    std::ostringstream oss;
    oss << "{";
    size_type idx{0};
    for (const auto& p: m) {
      oss << "{" << p.first << ", " << vec_to_string<T>(p.second) << "}";
      if (idx != max_idx) {
        oss << ", ";
      }
      ++idx;
    }
    oss << "}";
    return oss.str();
  }
}

#endif // ERIN_TYPE_H
