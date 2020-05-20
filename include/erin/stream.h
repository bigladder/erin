/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
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
          FlowValueType loss);

      [[nodiscard]] FlowValueType get_inflow() const { return inflow; };
      [[nodiscard]] FlowValueType get_outflow() const { return outflow; };
      [[nodiscard]] FlowValueType get_storeflow() const { return storeflow; };
      [[nodiscard]] FlowValueType get_lossflow() const { return lossflow; };

    private:
      FlowValueType inflow;
      FlowValueType outflow;
      FlowValueType storeflow;
      FlowValueType lossflow;
      void checkInvariants() const;
  };

  ////////////////////////////////////////////////////////////
  // PortRole
  enum class PortRole
  {
    Inflow = 0,
    LoadInflow,
    WasteInflow,
    Outflow,
    SourceOutflow
  };

  PortRole tag_to_port_role(const std::string& tag);

  std::string port_role_to_tag(const PortRole& role);
}

#endif // ERIN_STREAM_H
