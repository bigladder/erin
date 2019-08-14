/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#ifndef DISCO_DISCO_H
#define DISCO_DISCO_H
#include <string>
#include <vector>
#include <exception>
#include <functional>
#include "../../vendor/bdevs/include/adevs.h"

namespace DISCO
{
  ////////////////////////////////////////////////////////////
  // Type Definitions
  typedef double FlowValueType;
  typedef int RealTimeType;
  typedef int LogicalTimeType;

  ////////////////////////////////////////////////////////////
  // Utility Functions
  FlowValueType clamp_toward_0(FlowValueType value, FlowValueType lower, FlowValueType upper);

  ////////////////////////////////////////////////////////////
  // MixedStreamsError
  struct MixedStreamsError : public std::exception {};

  ////////////////////////////////////////////////////////////
  // FlowInvariantError
  struct FlowInvariantError : public std::exception {};

  ////////////////////////////////////////////////////////////
  // BadPortError
  struct BadPortError : public std::exception {};

  ////////////////////////////////////////////////////////////
  // SimultaneousIORequestError
  struct SimultaneousIORequestError : public std::exception {};

  ////////////////////////////////////////////////////////////
  // AchievedMoreThanRequestedError
  struct AchievedMoreThanRequestedError : public std::exception {};

  ////////////////////////////////////////////////////////////
  // StreamType
  class StreamType
  {
    public:
      StreamType(std::string type, std::string effort_units, std::string flow_units, std::string power_units);
      bool operator==(const StreamType& other) const;
      bool operator!=(const StreamType& other) const;
      std::string get_type() const { return type; }
      std::string get_effort_units() const { return effort_units; }
      std::string get_flow_units() const { return flow_units; }
      std::string get_power_units() const { return power_units; }

    private:
      std::string type;
      std::string effort_units;
      std::string flow_units;
      std::string power_units;
  };

  std::ostream& operator<<(std::ostream& os, const StreamType& st);

  ////////////////////////////////////////////////////////////
  // Stream
  class Stream
  {
    public:
      Stream(StreamType stream_type, FlowValueType power_value);
      Stream(StreamType stream_type, FlowValueType effort_value, FlowValueType flow_value);
      StreamType get_type() const { return type; }
      FlowValueType get_effort() const { return effort; }
      FlowValueType get_flow() const { return flow; }
      FlowValueType get_power() const { return power; }

    private:
      StreamType type;
      FlowValueType effort;
      FlowValueType flow;
      FlowValueType power;
  };

  std::ostream& operator<<(std::ostream& os, const Stream& s);

  ////////////////////////////////////////////////////////////
  // Type Definitions
  typedef adevs::port_value<Stream> PortValue;

  std::ostream& operator<<(std::ostream& os, const PortValue& pv);


  ////////////////////////////////////////////////////////////
  // FlowElement - Abstract Class
  class FlowElement : public adevs::Atomic<PortValue>
  {
    public:
      static const int inport_inflow_achieved;
      static const int inport_outflow_request;
      static const int outport_inflow_request;
      static const int outport_outflow_achieved;
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;
      virtual void update_state_for_outflow_request(FlowValueType outflow) = 0;
      virtual void update_state_for_inflow_achieved(FlowValueType inflow) = 0;

    protected:
      FlowElement(StreamType flow_type);
      FlowElement(StreamType inflow_type, StreamType outflow_type);

      adevs::Time time;
      StreamType inflow_type;
      StreamType outflow_type;
      FlowValueType inflow;
      FlowValueType outflow;
      FlowValueType storeflow;
      FlowValueType lossflow;
      bool report_inflow_request;
      bool report_outflow_achieved;

    private:
      static constexpr FlowValueType tol{1e-6};
      void check_flow_invariants() const;
  };

  ////////////////////////////////////////////////////////////
  // FlowLimits
  class FlowLimits : public adevs::Atomic<PortValue>
  {
    public:
      static const int inport_input_achieved;
      static const int inport_output_request;
      static const int outport_input_request;
      static const int outport_output_achieved;
      FlowLimits(StreamType stream_type, FlowValueType lower_limit, FlowValueType upper_limit);
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

    private:
      StreamType stream;
      FlowValueType lower_limit;
      FlowValueType upper_limit;
      bool report_input_request;
      bool report_output_achieved;
      FlowValueType input_request;
      FlowValueType output_achieved;
      bool flow_limited;
  };

  ////////////////////////////////////////////////////////////
  // FlowMeter
  class FlowMeter : public adevs::Atomic<PortValue>
  {
    public:
      static const int inport_input_achieved;
      static const int inport_output_request;
      static const int outport_input_request;
      static const int outport_output_achieved;
      FlowMeter(StreamType stream_type);
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;
      std::vector<RealTimeType> get_actual_output_times() const;
      std::vector<FlowValueType> get_actual_output() const;

    private:
      StreamType stream;
      std::vector<RealTimeType> event_times;
      std::vector<FlowValueType> requested_flows;
      std::vector<FlowValueType> achieved_flows;
      bool send_requested;
      bool send_achieved;
      RealTimeType event_time;
      FlowValueType requested_flow;
      FlowValueType achieved_flow;
  };

  ////////////////////////////////////////////////////////////
  // Transformer
  class Transformer : public adevs::Atomic<PortValue>
  {
    public:
      static const int inport_input_achieved;
      static const int inport_output_request;
      static const int outport_input_request;
      static const int outport_output_achieved;
      Transformer(
          StreamType input_stream_type,
          StreamType output_stream_type,
          std::function<FlowValueType(FlowValueType)> calc_output_from_input,
          std::function<FlowValueType(FlowValueType)> calc_input_from_output
          );
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

    private:
      StreamType input_stream;
      StreamType output_stream;
      std::function<FlowValueType(FlowValueType)> output_from_input;
      std::function<FlowValueType(FlowValueType)> input_from_output;
      FlowValueType output;
      FlowValueType input;
      bool send_input_request;
      bool send_output_achieved;
  };

  ////////////////////////////////////////////////////////////
  // Sink
  class Sink : public adevs::Atomic<PortValue>
  {
    public:
      static const int outport_inflow_request;
      Sink(StreamType stream_type, std::vector<RealTimeType> times, std::vector<FlowValueType> loads);
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

    private:
      StreamType stream;
      int idx;
      std::vector<RealTimeType>::size_type num_items;
      std::vector<RealTimeType> times;
      std::vector<FlowValueType> loads;
  };
}

#endif // DISCO_DISCO_H
