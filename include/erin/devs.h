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
  //constexpr int inport_lossflow_request{2*max_port_numbers};
  constexpr int outport_inflow_request{3*max_port_numbers};
  constexpr int outport_outflow_achieved{4*max_port_numbers};
  //constexpr int outport_lossflow_achieved{5*max_port_numbers};

  //template <class TReal, class TLogical>
  //class Time
  //{
  //  public:
  //    Time();
  //    Time(TReal dt);
  //    Time(TReal dt, TLogical logical);
  //    [[nodiscard]] TReal get_real() const { return real; }
  //    [[nodiscard]] TLogical get_logical() const { return logical; }
  //  private:
  //    TReal real;
  //    TLogical logical;
  //};

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
      //bool request_changed;
      //bool achieved_changed;
  };

  bool operator==(const Port& a, const Port& b);
  bool operator!=(const Port& a, const Port& b);
  std::ostream& operator<<(std::ostream& os, const Port& p);

  //template <class S>
  //class Atomic
  //{
  //  public:
  //    virtual Time time_advance(const S& state) const = 0;
  //    virtual S internal_transition(const S& previous_state) const = 0;
  //    virtual S external_transition(const S& previous_state, const Time& elapsed, const std::vector<PortValue>& xs) const = 0;
  //    virtual S confluent_transition(const S& previous_state, const Time& elapsed, const std::vector<PortValue>& xs) const {
  //      return external_transition(internal_transition(previous_state), Time{0,0}, xs);
  //    }
  //    virtual std::vector<PortValue> output_fn(const S& state) const = 0;
  //};

  class FlowLimits
  {
    public:
      FlowLimits(
          FlowValueType lower_limit,
          FlowValueType upper_limit);

      [[nodiscard]] FlowValueType get_lower_limit() const { return lower_limit; }
      [[nodiscard]] FlowValueType get_upper_limit() const { return upper_limit; }

    private:
      FlowValueType lower_limit;
      FlowValueType upper_limit;
  };

  struct FlowLimitsState
  {
    RealTimeType time;
    Port inflow_port;
    Port outflow_port;
    FlowValueType lower_limit;
    FlowValueType upper_limit;
    bool report_inflow_request;
    bool report_outflow_achieved;
  };

  RealTimeType
  flow_limits_time_advance(const FlowLimitsState& state);

  FlowLimitsState
  flow_limits_external_transition(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs);

  FlowLimitsState
  flow_limits_external_transition_on_outflow_request(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      FlowValueType outflow_request);

  FlowLimitsState
  flow_limits_external_transition_on_inflow_achieved(
      const FlowLimitsState& state,
      RealTimeType elapsed_time,
      FlowValueType inflow_achieved);

  FlowLimitsState
  flow_limits_internal_transition(const FlowLimitsState& state);

  std::vector<PortValue> flow_limits_output_function(const FlowLimitsState& state);

  bool operator==(const FlowLimitsState& a, const FlowLimitsState& b);
  bool operator!=(const FlowLimitsState& a, const FlowLimitsState& b);
  std::ostream& operator<<(std::ostream& os, const FlowLimitsState& s);

  //struct FlowLimits
  //{
  //  std::string id;
  //  PortFlow inflow_port;
  //  PortFlow outflow_port;
  //  std::string inflow_stream;
  //  std::string outflow_stream;
  //  double lower_limit;
  //  double upper_limit;
  //};

  //class FlowLimits : public Atomic<FlowLimits>
  //{
  //  public:
  //    FlowLimits();
  //    virtual Time time_advance(const S& state) const override;
  //    virtual S internal_transition(const S& previous_state) const override;
  //    virtual S external_transition(const S& previous_state, const Time& elapsed, const std::vector<PortValue>& xs) const override;
  //    virtual S confluent_transition(const S& previous_state, const Time& elapsed, const std::vector<PortValue>& xs) const {
  //      return external_transition(internal_transition(previous_state), Time{0,0}, xs);
  //    }
  //    virtual std::vector<PortValue> output_fn(const S& state) const override;
  //};
}

#endif // ERIN_DEVS_H
