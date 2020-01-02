/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/stream.h"
#include "debug_utils.h"
#include <chrono>
#include <stdexcept>

namespace ERIN
{
  constexpr RealTimeType default_max_time_years{1000};
  constexpr RealTimeType default_max_time_s =
    default_max_time_years * rtt_seconds_per_year;

  ////////////////////////////////////////////////////////////
  // SimulationInfo
  SimulationInfo::SimulationInfo():
    SimulationInfo(
        "kW",
        "kJ",
        TimeUnits::Seconds,
        default_max_time_s)
  {
  }

  SimulationInfo::SimulationInfo(
    TimeUnits time_unit_,
    RealTimeType max_time_):
    SimulationInfo("kW", "kJ", time_unit_, max_time_)
  {
  }

  SimulationInfo::SimulationInfo(
      const std::string& rate_unit_,
      const std::string& quantity_unit_,
      TimeUnits time_unit_,
      RealTimeType max_time_):
    SimulationInfo(
        rate_unit_,
        quantity_unit_,
        time_unit_,
        max_time_,
        false,
        0.0)
  {
  }

  SimulationInfo::SimulationInfo(
      const std::string& rate_unit_,
      const std::string& quantity_unit_,
      TimeUnits time_unit_,
      RealTimeType max_time_,
      //bool has_seed_,
      //unsigned int seed_value_,
      bool has_fixed_random_,
      double fixed_random_):
    rate_unit{rate_unit_},
    quantity_unit{quantity_unit_},
    time_unit{time_unit_},
    max_time{max_time_},
    //has_seed{has_seed_},
    //seed{seed_value_},
    has_fixed_random_frac{has_fixed_random_},
    fixed_random_frac{fixed_random_},
    generator{},
    distribution{0.0,1.0}
  {
    if (max_time <= 0.0) {
      std::ostringstream oss;
      oss << "max_time must be greater than 0.0";
      throw std::invalid_argument(oss.str());
    }
    namespace C = std::chrono;
    auto now = C::high_resolution_clock::now();
    auto d = now.time_since_epoch();
    unsigned seed = d.count();
    generator.seed(seed);
  }

  bool
  SimulationInfo::operator==(const SimulationInfo& other) const
  {
    if (this == &other) {
      return true;
    }
    return (rate_unit == other.rate_unit) &&
           (quantity_unit == other.quantity_unit) &&
           (get_max_time_in_seconds() == other.get_max_time_in_seconds()) &&
           (has_fixed_random_frac == other.has_fixed_random_frac) &&
           (has_fixed_random_frac
              ? (fixed_random_frac == other.fixed_random_frac)
              : true);
  }

  std::function<double()>
  SimulationInfo::make_random_function()
  {
    if (has_fixed_random_frac) {
      return [this]() -> double {
        return fixed_random_frac;
      };
    }
    return [this]() -> double {
      return distribution(generator);
    };
  }

  //////////////////////////////////////////////////////////// 
  // FlowState
  FlowState::FlowState(FlowValueType in):
    FlowState(in, in, 0.0, 0.0)
  {
  }

  FlowState::FlowState(FlowValueType in, FlowValueType out):
    FlowState(in, out, 0.0, std::fabs(in - out))
  {
  }

  FlowState::FlowState(FlowValueType in, FlowValueType out, FlowValueType store):
    FlowState(in, out, store, std::fabs(in - (out + store)))
  {
  }

  FlowState::FlowState(
      FlowValueType in,
      FlowValueType out,
      FlowValueType store,
      FlowValueType loss):
    inflow{in},
    outflow{out},
    storeflow{store},
    lossflow{loss}
  {
    checkInvariants();
  }

  void
  FlowState::checkInvariants() const
  {
    auto diff{inflow - (outflow + storeflow + lossflow)};
    if (std::fabs(diff) > flow_value_tolerance) {
      if constexpr (debug_level >= debug_level_high) {
        std::cerr << "FlowState.inflow   : " << inflow << "\n";
        std::cerr << "FlowState.outflow  : " << outflow << "\n";
        std::cerr << "FlowState.storeflow: " << storeflow << "\n";
        std::cerr << "FlowState.lossflow : " << lossflow << "\n";
      }
      throw std::runtime_error("FlowState::checkInvariants");
    }
  }

  ////////////////////////////////////////////////////////////
  // StreamType
  StreamType::StreamType():
    StreamType("electricity") {}

  StreamType::StreamType(const std::string& stream_type):
    StreamType(stream_type, "kW", "kJ", 1.0)
  {
  }

  StreamType::StreamType(
      const std::string& stream_type,
      const std::string& rate_units,
      const std::string& quantity_units,
      FlowValueType seconds_per_time_unit):
    StreamType(
        stream_type,
        rate_units,
        quantity_units,
        seconds_per_time_unit,
        std::unordered_map<std::string,FlowValueType>{},
        std::unordered_map<std::string,FlowValueType>{})
  {
  }

  StreamType::StreamType(
      std::string stream_type,
      std::string  r_units,
      std::string  q_units,
      FlowValueType s_per_time_unit,
      std::unordered_map<std::string, FlowValueType>  other_r_units,
      std::unordered_map<std::string, FlowValueType>  other_q_units
      ):
    type{std::move(stream_type)},
    rate_units{std::move(r_units)},
    quantity_units{std::move(q_units)},
    seconds_per_time_unit{s_per_time_unit},
    other_rate_units{std::move(other_r_units)},
    other_quantity_units{std::move(other_q_units)}
  {
  }

  bool
  StreamType::operator==(const StreamType& other) const
  {
    if (this == &other) return true;
    return (type == other.type) &&
           (rate_units == other.rate_units) &&
           (quantity_units == other.quantity_units) &&
           (seconds_per_time_unit == other.seconds_per_time_unit);
  }

  bool
  StreamType::operator!=(const StreamType& other) const
  {
    return !operator==(other);
  }

  std::ostream&
  operator<<(std::ostream& os, const StreamType& st)
  {
    return os << "StreamType(type=\"" << st.get_type()
              << "\", rate_units=\"" << st.get_rate_units()
              << "\", quantity_units=\"" << st.get_quantity_units()
              << "\", seconds_per_time_unit=" << st.get_seconds_per_time_unit()
              << ", other_rate_units="
              << map_to_string<FlowValueType>(st.get_other_rate_units())
              << ", other_quantity_units="
              << map_to_string<FlowValueType>(st.get_other_quantity_units())
              << ")";
  }
}
