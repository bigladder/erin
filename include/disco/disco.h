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

  ////////////////////////////////////////////////////////////
  // Utility Functions
  FlowValueType clamp_toward_0(FlowValueType value, FlowValueType lower, FlowValueType upper);

  ////////////////////////////////////////////////////////////
  // MixedStreamsError
  struct MixedStreamsError : public std::exception {};

  ////////////////////////////////////////////////////////////
  // StreamType
  enum class StreamType
  {
    electric_stream_in_kW = 0,
    natural_gas_stream_in_kg_per_second,
    hot_water_stream_in_kg_per_second,
    chilled_water_stream_in_kg_per_second,
    diesel_fuel_stream_in_liters_per_minute
  };

  ////////////////////////////////////////////////////////////
  // Flow
  class Flow
  {
    public:
      Flow(StreamType stream_type, FlowValueType flow_value);
      StreamType get_stream() const;
      FlowValueType get_flow() const;

    private:
      StreamType stream;
      FlowValueType flow;
  };

  ////////////////////////////////////////////////////////////
  // Type Definitions
  typedef adevs::port_value<Flow> PortValue;

  ////////////////////////////////////////////////////////////
  // Source
  class Source : public adevs::Atomic<PortValue>
  {
    public:
      static const int port_output_request;
      Source(StreamType stream_type);
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;
      std::string get_results() const;

    private:
      StreamType stream;
      int time;
      std::vector<int> times;
      std::vector<int> loads;
  };

  ////////////////////////////////////////////////////////////
  // FlowLimits
  class FlowLimits : public adevs::Atomic<PortValue>
  {
    public:
      static const int port_input_request;
      static const int port_output_request;
      static const int port_input_achieved;
      static const int port_output_achieved;
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
  };

  ////////////////////////////////////////////////////////////
  // FlowMeter
  class FlowMeter : public adevs::Atomic<PortValue>
  {
    public:
      static const int port_input_request;
      static const int port_output_request;
      static const int port_input_achieved;
      static const int port_output_achieved;
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
      static const int port_input_request;
      static const int port_output1_request;
      static const int port_output2_request;
      static const int port_output3_request;
      static const int port_output4_request;
      static const int port_input_achieved;
      static const int port_output1_achieved;
      static const int port_output2_achieved;
      static const int port_output3_achieved;
      static const int port_output4_achieved;
      Transformer(
          StreamType input_stream_type,
          std::vector<StreamType> output_stream_types,
          std::vector<std::function<FlowValueType(FlowValueType)>> calc_output_n_from_input,
          std::vector<std::function<FlowValueType(FlowValueType)>> calc_input_from_output_n
          );
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

    private:
      StreamType input_stream;
      std::vector<StreamType> output_streams;
      int num_outputs;
      std::vector<std::function<FlowValueType(FlowValueType)>> output_n_from_input;
      std::vector<std::function<FlowValueType(FlowValueType)>> input_from_output_n;
  };

  ////////////////////////////////////////////////////////////
  // Sink
  class Sink : public adevs::Atomic<PortValue>
  {
    public:
      static const int port_input_achieved;
      static const int port_input_request;
      Sink(StreamType stream_type);
      Sink(StreamType stream_type, std::vector<RealTimeType> times, std::vector<FlowValueType> loads);
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;
      std::string get_results() const;

    private:
      StreamType stream;
      int idx;
      std::vector<RealTimeType> times;
      std::vector<FlowValueType> loads;
      RealTimeType time;
      FlowValueType load;
      std::vector<RealTimeType> achieved_times;
      std::vector<FlowValueType> achieved_loads;
  };
}

#endif // DISCO_DISCO_H
