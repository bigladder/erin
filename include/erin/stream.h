/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_STREAM_H
#define ERIN_STREAM_H
#include "erin/type.h"
#include <functional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // RandomType
  enum class RandomType
  {
    RandomProcess = 0,
    FixedProcess,
    FixedSeries
  };

  ////////////////////////////////////////////////////////////
  // RandomInfo
  class RandomInfo
  {
    public:
      RandomInfo() = default;
      virtual ~RandomInfo() = default;
      RandomInfo(const RandomInfo&) = delete;
      RandomInfo& operator=(const RandomInfo&) = delete;
      RandomInfo(RandomInfo&&) = delete;
      RandomInfo& operator=(RandomInfo&&) = delete;

      virtual std::unique_ptr<RandomInfo> clone() const = 0;
      virtual bool has_seed() const = 0;
      virtual unsigned int get_seed() const = 0;
      virtual RandomType get_type() const = 0;
      virtual double call() = 0;
  };

  bool operator==(
      const std::unique_ptr<RandomInfo>& a,
      const std::unique_ptr<RandomInfo>& b);
  bool operator!=(
      const std::unique_ptr<RandomInfo>& a,
      const std::unique_ptr<RandomInfo>& b);

  std::unique_ptr<RandomInfo> make_random_info(
      bool has_fixed_random,
      double fixed_random,
      bool has_seed,
      unsigned int seed_value);

  std::unique_ptr<RandomInfo> make_random_info(
      bool has_fixed_random,
      double fixed_random,
      bool has_seed,
      unsigned int seed_value,
      bool has_fixed_series,
      const std::vector<double>& series);

  ////////////////////////////////////////////////////////////
  // RandomProcess
  class RandomProcess : public RandomInfo
  {
    public:
      RandomProcess();
      RandomProcess(unsigned int seed);

      std::unique_ptr<RandomInfo> clone() const override {
        return std::make_unique<RandomProcess>(seed);
      };
      [[nodiscard]] bool has_seed() const override { return true; }
      [[nodiscard]] unsigned int get_seed() const override { return seed; }
      [[nodiscard]] RandomType get_type() const override {
        return RandomType::RandomProcess;
      }
      double call() override;

      friend bool operator==(const RandomProcess& a, const RandomProcess& b);

    private:
      unsigned int seed;
      std::mt19937 generator;
      std::uniform_real_distribution<double> distribution;
  };

  bool operator==(const RandomProcess& a, const RandomProcess& b);
  bool operator!=(const RandomProcess& a, const RandomProcess& b);

  ////////////////////////////////////////////////////////////
  // FixedProcess
  class FixedProcess : public RandomInfo
  {
    public:
      FixedProcess(double fixed_value);

      std::unique_ptr<RandomInfo> clone() const override {
        return std::make_unique<FixedProcess>(fixed_value);
      }
      [[nodiscard]] bool has_seed() const override { return false; }
      [[nodiscard]] unsigned int get_seed() const override { return 0; }
      [[nodiscard]] RandomType get_type() const override { return RandomType::FixedProcess; }
      double call() override { return fixed_value; }
      
      friend bool operator==(const FixedProcess& a, const FixedProcess& b);

    private:
      double fixed_value;
  };

  bool operator==(const FixedProcess& a, const FixedProcess& b);
  bool operator!=(const FixedProcess& a, const FixedProcess& b);

  ////////////////////////////////////////////////////////////
  // FixedSeries
  class FixedSeries : public RandomInfo
  {
    public:
      FixedSeries(const std::vector<double>& series);
      FixedSeries(
          const std::vector<double>& series,
          std::vector<double>::size_type idx);

      std::unique_ptr<RandomInfo> clone() const override {
        return std::make_unique<FixedSeries>(series, idx);
      }
      [[nodiscard]] bool has_seed() const override { return false; }
      [[nodiscard]] unsigned int get_seed() const override { return 0; }
      [[nodiscard]] RandomType get_type() const override { return RandomType::FixedSeries; }
      double call() override;
      
      friend bool operator==(const FixedSeries& a, const FixedSeries& b);

    private:
      std::vector<double> series;
      std::vector<double>::size_type idx;
      std::vector<double>::size_type max_idx;

      void increment_idx();
  };

  bool operator==(const FixedSeries& a, const FixedSeries& b);
  bool operator!=(const FixedSeries& a, const FixedSeries& b);
  
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
