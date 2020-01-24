/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEVS_H
#define ERIN_DEVS_H
#include "erin/type.h"

namespace erin::devs
{
  //constexpr int max_port_numbers{1000};
  //constexpr int inport_inflow_available{0*max_port_numbers};
  //constexpr int inport_outflow_request{1*max_port_numbers};
  //constexpr int inport_lossflow_request{2*max_port_numbers};
  //constexpr int outport_inflow_request{3*max_port_numbers};
  //constexpr int outport_outflow_achieved{4*max_port_numbers};
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
  //    Port(double requested);
      Port(double requested, double achieved);
      Port with_requested(double requested) const;
      Port with_achieved(double achieved) const;
      double get_requested() const { return requested; }
      double get_achieved() const { return achieved; }
    private:
      double requested;
      double achieved;
  };

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
          ERIN::FlowValueType lower_limit,
          ERIN::FlowValueType upper_limit);

      [[nodiscard]] ERIN::FlowValueType get_lower_limit() const { return lower_limit; }
      [[nodiscard]] ERIN::FlowValueType get_upper_limit() const { return upper_limit; }

    private:
      ERIN::FlowValueType lower_limit;
      ERIN::FlowValueType upper_limit;
  };

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
