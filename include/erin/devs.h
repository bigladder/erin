/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_H
#define ERIN_DEVS_H
#include "erin/type.h"
#include <iostream>
#include <string>
#include <vector>

namespace erin::devs
{
  using FlowValueType = ERIN::FlowValueType;
  using RealTimeType = ERIN::RealTimeType;
  using PortValue = ERIN::PortValue;
  constexpr RealTimeType infinity{-1};
  constexpr int max_port_numbers{1000};
  constexpr int inport_inflow_achieved{0*max_port_numbers};
  constexpr int inport_outflow_request{1*max_port_numbers};
  constexpr int inport_lossflow_request{2*max_port_numbers};
  constexpr int outport_inflow_request{3*max_port_numbers};
  constexpr int outport_outflow_achieved{4*max_port_numbers};
  constexpr int outport_lossflow_achieved{5*max_port_numbers};

  class Port
  {
    public:
      Port();
      Port(RealTimeType time);
      Port(RealTimeType time, FlowValueType requested);
      Port(RealTimeType time, FlowValueType requested, FlowValueType achieved);

      [[nodiscard]] RealTimeType get_time_of_last_change() const {
        return time_of_last_change;
      }
      [[nodiscard]] FlowValueType get_requested() const { return requested; }
      [[nodiscard]] FlowValueType get_achieved() const { return achieved; }
      [[nodiscard]] bool should_propagate_request_at(RealTimeType time) const;
      [[nodiscard]] bool should_propagate_achieved_at(RealTimeType time) const;

      [[nodiscard]] Port with_requested(
          FlowValueType new_requested, RealTimeType time) const;
      [[nodiscard]] Port with_requested_and_achieved(
          FlowValueType new_requested, FlowValueType new_achieved, RealTimeType time) const;
      [[nodiscard]] Port with_achieved(
          FlowValueType new_achieved, RealTimeType time) const;

      friend bool operator==(const Port& a, const Port& b);
      friend std::ostream& operator<<(std::ostream& os, const Port& p);

    private:
      RealTimeType time_of_last_change;
      FlowValueType requested;
      FlowValueType achieved;
  };

  bool operator==(const Port& a, const Port& b);
  bool operator!=(const Port& a, const Port& b);
  std::ostream& operator<<(std::ostream& os, const Port& p);
}

#endif // ERIN_DEVS_H
