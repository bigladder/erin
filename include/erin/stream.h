/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_STREAM_H
#define ERIN_STREAM_H
#include "erin/type.h"
#include <string>
#include <unordered_map>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // SimulationInfo
  class SimulationInfo
  {
    public:
      SimulationInfo();
      SimulationInfo(
          const std::string& rate_unit,
          const std::string& quantity_unit,
          TimeUnits time_unit,
          RealTimeType max_time);
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
      bool operator==(const SimulationInfo& other) const;
      bool operator!=(const SimulationInfo& other) const {
        return !(operator==(other));
      }

    private:
      std::string rate_unit;
      std::string quantity_unit;
      TimeUnits time_unit;
      RealTimeType max_time;
  };

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
