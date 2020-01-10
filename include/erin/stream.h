/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_STREAM_H
#define ERIN_STREAM_H
#include "erin/type.h"
#include "erin/random.h"
#include <functional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // SimulationInfo
  class SimulationInfo
  {
    public:
      SimulationInfo();
      SimulationInfo(TimeUnits time_units, RealTimeType max_time);
      SimulationInfo(
          const std::string& rate_unit,
          const std::string& quantity_unit,
          TimeUnits time_unit,
          RealTimeType max_time);
      SimulationInfo(
          const std::string& rate_unit,
          const std::string& quantity_unit,
          TimeUnits time_unit,
          RealTimeType max_time,
          bool has_fixed_random_frac,
          double fixed_random_frac);
      SimulationInfo(
          const std::string& rate_unit,
          const std::string& quantity_unit,
          TimeUnits time_unit,
          RealTimeType max_time,
          bool has_fixed_random_frac,
          double fixed_random_frac,
          bool has_seed,
          unsigned int seed_value);
      SimulationInfo(
          const std::string& rate_unit,
          const std::string& quantity_unit,
          TimeUnits time_unit,
          RealTimeType max_time,
          std::unique_ptr<RandomInfo>&& random_process);

      // Rule of Five
      // see https://stackoverflow.com/a/43263477
      ~SimulationInfo() = default;
      SimulationInfo(const SimulationInfo& other);
      SimulationInfo& operator=(const SimulationInfo&);
      SimulationInfo(SimulationInfo&& other) = default;
      SimulationInfo& operator=(SimulationInfo&&) = default;

      [[nodiscard]] const std::string& get_rate_unit() const {
        return rate_unit;
      }
      [[nodiscard]] const std::string& get_quantity_unit() const {
        return quantity_unit;
      }
      [[nodiscard]] TimeUnits get_time_units() const {
        return time_unit;
      }
      [[nodiscard]] RealTimeType get_max_time() const {
        return max_time;
      }
      [[nodiscard]] RealTimeType get_max_time_in_seconds() const {
        return time_to_seconds(max_time, time_unit);
      }
      [[nodiscard]] bool has_random_seed() const {
        return random_process->has_seed();
      }
      [[nodiscard]] unsigned int get_random_seed() const {
        return random_process->get_seed();
      }
      [[nodiscard]] RandomType get_random_type() const {
        return random_process->get_type();
      }
      [[nodiscard]] std::function<double()> make_random_function();

      friend bool operator==(const SimulationInfo& a, const SimulationInfo& b);

    private:
      std::string rate_unit;
      std::string quantity_unit;
      TimeUnits time_unit;
      RealTimeType max_time;
      std::unique_ptr<RandomInfo> random_process;
  };

  bool operator==(const SimulationInfo& a, const SimulationInfo& b);
  bool operator!=(const SimulationInfo& a, const SimulationInfo& b);

  ////////////////////////////////////////////////////////////
  // FlowState
  class FlowState
  {
    public:
      explicit FlowState(FlowValueType in);
      FlowState(FlowValueType in, FlowValueType out);
      FlowState(FlowValueType in, FlowValueType out, FlowValueType store);
      FlowState(
          FlowValueType in,
          FlowValueType out,
          FlowValueType store,
          FlowValueType loss
          );

      [[nodiscard]] FlowValueType getInflow() const { return inflow; };
      [[nodiscard]] FlowValueType getOutflow() const { return outflow; };
      [[nodiscard]] FlowValueType getStoreflow() const { return storeflow; };
      [[nodiscard]] FlowValueType getLossflow() const { return lossflow; };

    private:
      FlowValueType inflow;
      FlowValueType outflow;
      FlowValueType storeflow;
      FlowValueType lossflow;
      void checkInvariants() const;
  };

  ////////////////////////////////////////////////////////////
  // StreamType
  class StreamType
  {
    public:
      StreamType();
      explicit StreamType(const std::string& type);
      StreamType(
          const std::string& type,
          const std::string& rate_units,
          const std::string& quantity_units,
          FlowValueType seconds_per_time_unit
          );
      StreamType(
          std::string  type,
          std::string  rate_units,
          std::string  quantity_units,
          FlowValueType seconds_per_time_unit,
          std::unordered_map<std::string, FlowValueType> other_rate_units,
          std::unordered_map<std::string, FlowValueType> other_quantity_units
          );
      bool operator==(const StreamType& other) const;
      bool operator!=(const StreamType& other) const;
      [[nodiscard]] const std::string& get_type() const { return type; }
      [[nodiscard]] const std::string& get_rate_units() const {
        return rate_units;
      }
      [[nodiscard]] const std::string& get_quantity_units() const {
        return quantity_units;
      }
      [[nodiscard]] FlowValueType get_seconds_per_time_unit() const {
        return seconds_per_time_unit;
      }
      [[nodiscard]] const std::unordered_map<std::string,FlowValueType>&
        get_other_rate_units() const {
          return other_rate_units;
        }
      [[nodiscard]] const std::unordered_map<std::string,FlowValueType>&
        get_other_quantity_units() const {
          return other_quantity_units;
        }

      friend std::ostream& operator<<(std::ostream& os, const StreamType& s);

    private:
      std::string type;
      std::string rate_units;
      std::string quantity_units;
      FlowValueType seconds_per_time_unit;
      std::unordered_map<std::string,FlowValueType> other_rate_units;
      std::unordered_map<std::string,FlowValueType> other_quantity_units;
  };

  std::ostream& operator<<(std::ostream& os, const StreamType& s);
}

#endif // ERIN_STREAM_H
