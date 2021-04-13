/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

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
  constexpr int outport_inflow_request{2*max_port_numbers};
  constexpr int outport_outflow_achieved{3*max_port_numbers};

  class Port
  {
    public:
      Port();
      explicit Port(RealTimeType time);
      Port(RealTimeType time, FlowValueType requested);
      Port(RealTimeType time, FlowValueType requested, FlowValueType achieved);
      Port(
          RealTimeType time,
          FlowValueType requested,
          FlowValueType achieved,
          bool propagate_request,
          bool propagate_achieved);

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
      bool propagate_request;
      bool propagate_achieved;
  };

  bool operator==(const Port& a, const Port& b);
  bool operator!=(const Port& a, const Port& b);
  std::ostream& operator<<(std::ostream& os, const Port& p);
  
  struct PortUpdate;

  class Port2
  {
    public:
      Port2();
      explicit Port2(FlowValueType requested);
      Port2(FlowValueType requested, FlowValueType achieved);

      [[nodiscard]] FlowValueType get_requested() const { return requested; }
      [[nodiscard]] FlowValueType get_achieved() const { return achieved; }
      [[nodiscard]] PortUpdate with_requested(FlowValueType r) const;
      [[nodiscard]] PortUpdate with_achieved(FlowValueType a) const;
      [[nodiscard]] bool should_send_request(const Port2& previous) const;
      [[nodiscard]] bool should_send_achieved(const Port2& previous) const;

      friend bool operator==(const Port2& a, const Port2& b);
      friend std::ostream& operator<<(std::ostream& os, const Port2& p);

    private:
      FlowValueType requested;
      FlowValueType achieved;
      
      bool achieved_is_limited() const {
        return achieved < requested;
      };
      bool should_send_achieved_internal(
          FlowValueType r, FlowValueType a,
          FlowValueType prev_r, FlowValueType prev_a) const;
      bool should_send_request_internal(
          FlowValueType r, FlowValueType prev_r) const;
  };

  bool operator==(const Port2& a, const Port2& b);
  bool operator!=(const Port2& a, const Port2& b);
  std::ostream& operator<<(std::ostream& os, const Port2& p);
  
  struct PortUpdate
  {
    bool send_update{false};
    Port2 port{};
  };
  
  bool operator==(const PortUpdate& a, const PortUpdate& b);
  bool operator!=(const PortUpdate& a, const PortUpdate& b);
  std::ostream& operator<<(std::ostream& os, const PortUpdate& p);

  struct PortUpdate3;

  class Port3
  {
    public:
      Port3();
      explicit Port3(FlowValueType requested);
      Port3(FlowValueType requested, FlowValueType achieved);

      [[nodiscard]] FlowValueType get_requested() const { return requested; }
      [[nodiscard]] FlowValueType get_achieved() const { return achieved; }
      [[nodiscard]] PortUpdate3 with_requested(FlowValueType r) const;
      [[nodiscard]] PortUpdate3 with_achieved(FlowValueType a) const;
      [[nodiscard]] PortUpdate3 with_requested_and_available(
          FlowValueType r,
          FlowValueType available) const;

      friend bool operator==(const Port3& a, const Port3& b);
      friend std::ostream& operator<<(std::ostream& os, const Port3& p);

    private:
      FlowValueType requested;
      FlowValueType achieved;
  };

  bool operator==(const Port3& a, const Port3& b);
  bool operator!=(const Port3& a, const Port3& b);
  std::ostream& operator<<(std::ostream& os, const Port3& p);

  struct PortUpdate3
  {
    Port3 port{};
    bool propagate{false};
    bool back_propagate{false};
  };

  bool operator==(const PortUpdate3& a, const PortUpdate3& b);
  bool operator!=(const PortUpdate3& a, const PortUpdate3& b);
  std::ostream& operator<<(std::ostream& os, const PortUpdate3& p);

  // Helper Functions
  // TODO: rename got_inflow_achieved as it shadows many local variables in external_transitions
  bool got_inflow_achieved(const std::vector<PortValue>& xs);
  FlowValueType total_inflow_achieved(const std::vector<PortValue>& xs);
}

#endif // ERIN_DEVS_H
