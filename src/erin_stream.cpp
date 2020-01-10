/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

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

  bool
  operator==(
      const std::unique_ptr<RandomInfo>& a,
      const std::unique_ptr<RandomInfo>& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "unique_ptr<RandomInfo> == unique_ptr<RandomInfo>\n";
    }
    auto a_type = a->get_type();
    auto b_type = b->get_type();
    if (a_type != b_type) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "a_type != b_type\n";
        std::cout << "a_type = " << static_cast<int>(a_type) << "\n";
        std::cout << "b_type = " << static_cast<int>(b_type) << "\n";
      }
      return false;
    }
    switch (a_type) {
      case RandomType::RandomProcess:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "RandomProcess compare...\n";
          }
          const auto& a_rp = dynamic_cast<const RandomProcess&>(*a);
          const auto& b_rp = dynamic_cast<const RandomProcess&>(*b);
          return a_rp == b_rp;
        }
      case RandomType::FixedProcess:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "FixedProcess compare...\n";
          }
          const auto& a_fp = dynamic_cast<const FixedProcess&>(*a);
          const auto& b_fp = dynamic_cast<const FixedProcess&>(*b);
          return a_fp == b_fp;
        }
      case RandomType::FixedSeries:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "FixedSeries compare...\n";
          }
          const auto& a_fs = dynamic_cast<const FixedSeries&>(*a);
          const auto& b_fs = dynamic_cast<const FixedSeries&>(*b);
          return a_fs == b_fs;
        }
      default:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "unhandled process...\n";
          }
          std::ostringstream oss;
          oss << "unhandled RandomType " << static_cast<int>(a_type) << "\n";
          throw std::invalid_argument(oss.str());
        }
    }
    return true;
  }

  bool
  operator!=(
      const std::unique_ptr<RandomInfo>& a,
      const std::unique_ptr<RandomInfo>& b)
  {
    return !(a == b);
  }

  ////////////////////////////////////////////////////////////
  // RandomProcess
  RandomProcess::RandomProcess() :
    RandomInfo(),
    seed{},
    generator{},
    distribution{0.0, 1.0}
  {
    namespace C = std::chrono;
    auto now = C::high_resolution_clock::now();
    auto d = now.time_since_epoch();
    constexpr unsigned int range =
      std::numeric_limits<unsigned int>::max()
      - std::numeric_limits<unsigned int>::min();
    seed = d.count() % range;
    generator.seed(seed);
  }

  RandomProcess::RandomProcess(unsigned int seed_):
    RandomInfo(),
    seed{seed_},
    generator{},
    distribution{0.0, 1.0}
  {
    generator.seed(seed);
  }

  double
  RandomProcess::call()
  {
    return distribution(generator);
  }

  bool
  operator==(const RandomProcess& a, const RandomProcess& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "RandomProcess == RandomProcess...\n";
      std::cout << "a.seed = " << a.seed << "...\n";
      std::cout << "b.seed = " << b.seed << "...\n";
    }
    return a.seed == b.seed;
  }

  bool
  operator!=(const RandomProcess& a, const RandomProcess& b)
  {
    return !(a == b);
  }

  ////////////////////////////////////////////////////////////
  // FixedProcess
  FixedProcess::FixedProcess(double fixed_value_):
    RandomInfo(),
    fixed_value{fixed_value_}
  {
    if ((fixed_value < 0.0) || (fixed_value > 1.0)) {
      throw std::invalid_argument("fixed_value must be >= 0.0 and <= 1.0");
    }
  }

  bool
  operator==(const FixedProcess& a, const FixedProcess& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FixedProcess == FixedProcess...\n";
      std::cout << "a.fixed_value = " << a.fixed_value << "...\n";
      std::cout << "b.fixed_value = " << b.fixed_value << "...\n";
    }
    return a.fixed_value == b.fixed_value;
  }

  bool
  operator!=(const FixedProcess& a, const FixedProcess& b)
  {
    return !(a == b);
  }

  ////////////////////////////////////////////////////////////
  // FixedSeries
  FixedSeries::FixedSeries(const std::vector<double>& series_):
    FixedSeries(series_, 0)
  {
  }

  FixedSeries::FixedSeries(
      const std::vector<double>& series_,
      std::vector<double>::size_type idx_):
    RandomInfo(),
    series{series_},
    idx{idx_},
    max_idx{}
  {
    max_idx = series.size() - 1;
    if (max_idx < 0) {
      std::ostringstream oss{};
      oss << "the series given to FixedSeries must be at lease of size 1 or greater: "
             "size = " << (max_idx + 1);
      throw std::invalid_argument(oss.str());
    }
    if (idx > max_idx) {
      std::ostringstream oss;
      oss << "the index cannot be greater than the length of the series: "
          << "size = " << (max_idx + 1) << "; "
          << "idx = " << idx << "; "
          << "max_idx = " << max_idx;
      throw std::invalid_argument(oss.str());
    }
  }

  double
  FixedSeries::call()
  {
    auto v = series.at(idx);
    increment_idx();
    return v;
  }

  void
  FixedSeries::increment_idx()
  {
    ++idx;
    if (idx > max_idx)
      idx = 0;
  }

  bool
  operator==(const FixedSeries& a, const FixedSeries& b)
  {
    return (a.series == b.series) && (a.idx == b.idx);
  }

  bool
  operator!=(const FixedSeries& a, const FixedSeries& b)
  {
    return !(a == b);
  }

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
      const std::string& rate_unit_,
      const std::string& quantity_unit_,
      TimeUnits time_unit_,
      RealTimeType max_time_,
      bool has_fixed_random,
      double fixed_random,
      bool has_seed,
      unsigned int seed_value):
    rate_unit{rate_unit_},
    quantity_unit{quantity_unit_},
    time_unit{time_unit_},
    max_time{max_time_},
    random_process{}
  {
    if (max_time <= 0.0) {
      std::ostringstream oss;
      oss << "max_time must be greater than 0.0";
      throw std::invalid_argument(oss.str());
    }
    if ((has_fixed_random) && (has_seed)) {
      std::ostringstream oss;
      oss << "cannot have fixed random AND specify random seed\n"
             "has_fixed_random implies a fixed process\n"
             "has_seed implies a random process\n"
             "has_fixed_random and has_seed can be:\n"
             "  (false, false) => random process seeded by clock\n"
             "  (true, false) => fixed process with specified value\n"
             "  (false, true) => random process with specified seed\n";
      throw std::invalid_argument(oss.str());
    }
    if (has_fixed_random) {
      random_process = std::make_unique<FixedProcess>(fixed_random);
    }
    else if (has_seed) {
      random_process = std::make_unique<RandomProcess>(seed_value);
    }
    else {
      random_process = std::make_unique<RandomProcess>();
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
