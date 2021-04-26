/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/stream.h"
#include "debug_utils.h"
#include <chrono>
#include <limits>
#include <sstream>
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
        default_max_time_s,
        false,
        0.0,
        false,
        0)
  {
  }

  SimulationInfo::SimulationInfo(
    TimeUnits time_unit_,
    RealTimeType max_time_):
    SimulationInfo(
        "kW",
        "kJ",
        time_unit_,
        max_time_,
        false,
        0.0,
        false,
        0)
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
        0.0,
        false,
        0)
  {
  }

  SimulationInfo::SimulationInfo(
      const std::string& rate_unit_,
      const std::string& quantity_unit_,
      TimeUnits time_unit_,
      RealTimeType max_time_,
      bool has_fixed_random_,
      double fixed_random_):
    SimulationInfo(
        rate_unit_,
        quantity_unit_,
        time_unit_,
        max_time_,
        has_fixed_random_,
        fixed_random_,
        false,
        0)
  {
  }

  SimulationInfo::SimulationInfo(
      const std::string& rate_unit,
      const std::string& quantity_unit,
      TimeUnits time_unit,
      RealTimeType max_time,
      bool has_fixed_random,
      double fixed_random,
      bool has_seed,
      unsigned int seed_value):
    SimulationInfo(
        rate_unit,
        quantity_unit,
        time_unit,
        max_time,
        make_random_info(
          has_fixed_random, fixed_random, has_seed, seed_value))
  {
  }

  SimulationInfo::SimulationInfo(
      const std::string& rate_unit_,
      const std::string& quantity_unit_,
      TimeUnits time_unit_,
      RealTimeType max_time_,
      std::unique_ptr<RandomInfo>&& random_process_):
    rate_unit{rate_unit_},
    quantity_unit{quantity_unit_},
    time_unit{time_unit_},
    max_time{max_time_},
    random_process{std::move(random_process_)}
  {
    if (max_time <= 0) {
      std::ostringstream oss;
      oss << "max_time must be greater than 0.0";
      throw std::invalid_argument(oss.str());
    }
  }
    

  SimulationInfo::SimulationInfo(const SimulationInfo& other):
    rate_unit{other.rate_unit},
    quantity_unit{other.quantity_unit},
    time_unit{other.time_unit},
    max_time{other.max_time},
    random_process{other.random_process->clone()}
  {
  }

  SimulationInfo&
  SimulationInfo::operator=(const SimulationInfo& other)
  {
    random_process = other.random_process->clone();
    return *this;
  }

  std::function<double()>
  SimulationInfo::make_random_function()
  {
    return [this]() -> double {
      return random_process->call();
    };
  }

  bool
  operator==(const SimulationInfo& a, const SimulationInfo& b)
  {
    return (a.rate_unit == b.rate_unit) &&
           (a.quantity_unit == b.quantity_unit) &&
           (a.get_max_time_in_seconds() == b.get_max_time_in_seconds()) &&
           (a.random_process == b.random_process);
  }

  bool
  operator!=(const SimulationInfo& a, const SimulationInfo& b)
  {
    return !(a == b);
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

  PortRole
  tag_to_port_role(const std::string& tag)
  {
    if (tag == "inflow") {
      return PortRole::Inflow;
    }
    else if (tag == "load-inflow") {
      return PortRole::LoadInflow;
    }
    else if (tag == "waste-inflow") {
      return PortRole::WasteInflow;
    }
    else if (tag == "storage-inflow") {
      return PortRole::StorageInflow;
    }
    else if (tag == "outflow") {
      return PortRole::Outflow;
    }
    else if (tag == "source-outflow") {
      return PortRole::SourceOutflow;
    }
    else if (tag == "storage-outflow") {
      return PortRole::StorageOutflow;
    }
    std::ostringstream oss{};
    oss << "tag_to_port_role: unhandled tag '" << tag << "'";
    throw std::invalid_argument(oss.str());
  }

  std::string
  port_role_to_tag(const PortRole& role)
  {
    switch (role) {
      case PortRole::Inflow:
        return std::string{"inflow"};
      case PortRole::LoadInflow:
        return std::string{"load-inflow"};
      case PortRole::WasteInflow:
        return std::string{"waste-inflow"};
      case PortRole::StorageInflow:
        return std::string{"storage-inflow"};
      case PortRole::Outflow:
        return std::string{"outflow"};
      case PortRole::SourceOutflow:
        return std::string{"source-outflow"};
      case PortRole::StorageOutflow:
        return std::string{"storage-outflow"};
    }
    std::ostringstream oss{};
    oss << "unhandled PortRole '" << static_cast<int>(role) << "'\n";
    throw std::invalid_argument(oss.str());
  }
}
