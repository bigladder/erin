/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "disco/disco.h"
#include <sstream>
#include <stdexcept>

namespace DISCO
{
  FlowValueType
  clamp_toward_0(FlowValueType value, FlowValueType lower, FlowValueType upper)
  {
    if (lower > upper) {
      std::ostringstream oss;
      oss << "DISCO::clamp_toward_0 error: lower (" << lower
          << ") greater than upper (" << upper << ")"; 
      throw std::invalid_argument(oss.str());
    }
    if (value > upper) {
      if (upper > 0)
        return upper;
      else
        return 0;
    }
    else if (value < lower) {
      if (lower > 0)
        return 0;
      else
        return lower;
    }
    return value;
  }

  ///////////////////////////////////////////////////////////////////
  // Flow
  Flow::Flow(StreamType stream_type, FlowValueType flow_value):
    stream{stream_type},
    flow{flow_value}
  {
  }

  StreamType
  Flow::get_stream() const
  {
    return stream;
  }

  FlowValueType 
  Flow::get_flow() const
  {
    return flow;
  }

  ///////////////////////////////////////////////////////////////////
  // Source
  const int Source::inport_output_request = 1;

  Source::Source(StreamType stream_type):
    adevs::Atomic<PortValue>(),
    stream{stream_type},
    time{0},
    times{},
    loads{}
  {
  }

  void
  Source::delta_int()
  {
  }

  void
  Source::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    for (auto x : xs)
    {
      if (x.port == inport_output_request) 
      {
        time += e.real;
        Flow f = x.value;
        if (f.get_stream() != stream)
          throw MixedStreamsError();
        FlowValueType load = f.get_flow();
        times.push_back(time);
        loads.push_back(load);
      }
    }
  }

  void
  Source::delta_conf(std::vector<PortValue>& xs)
  {
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  Source::ta()
  {
    return adevs_inf<adevs::Time>();
  }

  void
  Source::output_func(std::vector<PortValue>& ys)
  {
  }

  std::string
  Source::get_results() const
  {
    std::ostringstream oss;
    oss << "\"time (hrs)\",\"power [OUT] (kW)\"" << std::endl;
    for (int idx{0}; idx < times.size(); ++idx) {
      oss << times.at(idx) << "," << loads.at(idx) << std::endl;
    }
    return oss.str();
  }

  ///////////////////////////////////////////////////////////////////
  // FlowLimits
  const int FlowLimits::inport_input_achieved = 1;
  const int FlowLimits::inport_output_request = 2;
  const int FlowLimits::outport_input_request = 3;
  const int FlowLimits::outport_output_achieved = 4;

  FlowLimits::FlowLimits(StreamType stream_type, FlowValueType low_lim, FlowValueType up_lim) :
    adevs::Atomic<PortValue>(),
    stream{stream_type},
    lower_limit{low_lim},
    upper_limit{up_lim},
    report_input_request{false},
    report_output_achieved{false},
    input_request{0},
    output_achieved{0}
  {
    if (lower_limit > upper_limit) {
      std::ostringstream oss;
      oss << "FlowLimits error: lower_limit (" << lower_limit
          << ") > upper_limit (" << upper_limit << ")";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  FlowLimits::delta_int()
  {
    report_input_request = false;
    report_output_achieved = false;
    input_request = 0;
    output_achieved = 0;
  }

  void
  FlowLimits::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    for (auto x : xs) {
      if (x.port == inport_output_request) {
        // the output requested equals the input request,
        // provided it is within limits. We accumulate all input requests
        // accounting for the (unlikely) possibility of multiple values on one
        // port. input_request is set to 0 at class construction and at each
        // internal transition.
        report_input_request = true;
        input_request += x.value.get_flow();
        if (x.value.get_stream() != stream)
          throw MixedStreamsError();
      }
      else if (x.port == inport_input_achieved) {
        // the input achieved equals the output achieved. output_achieved is
        // initialized to 0 at construction and at each internal transition.
        report_output_achieved = true;
        output_achieved += x.value.get_flow();
        if (x.value.get_stream() != stream)
          throw MixedStreamsError();
      }
    }
    // Note: we never provide more power than requested. Therefore, if the
    // requested power, say 10 kW is less than the minimum power, say 15 kW,
    // then the request is for 0 kW, not 15 kW. Also, we have to account for
    // sign convention where we might have a negative output (which means flow
    // is going the other direction).
    if (report_input_request) {
      input_request = clamp_toward_0(input_request, lower_limit, upper_limit);
      if (!report_output_achieved) {
        // if an input request is given but no output achieved, assume that
        // output achieved is input request (limited) unless we hear otherwise.
        report_output_achieved = true;
        output_achieved = input_request;
      }
    }
  }

  void
  FlowLimits::delta_conf(std::vector<PortValue>& xs)
  {
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  FlowLimits::ta()
  {
    if (report_input_request || report_output_achieved)
      return adevs::Time{0, 0};
    return adevs_inf<adevs::Time>();
  }

  void
  FlowLimits::output_func(std::vector<PortValue>& ys)
  {
    if (report_input_request) {
      PortValue pv = adevs::port_value<Flow>(
          outport_input_request, Flow{stream, input_request});
      ys.push_back(pv);
    }
    if (report_output_achieved) {
      PortValue pv = adevs::port_value<Flow>(
          outport_output_achieved, Flow{stream, output_achieved});
      ys.push_back(pv);
    }
  }

  ////////////////////////////////////////////////////////////
  // FlowMeter
  const int FlowMeter::inport_input_achieved = 3;
  const int FlowMeter::inport_output_request = 2;
  const int FlowMeter::outport_input_request = 1;
  const int FlowMeter::outport_output_achieved = 4;

  FlowMeter::FlowMeter(StreamType stream_type) :
    adevs::Atomic<PortValue>(),
    stream{stream_type},
    event_times{},
    requested_flows{},
    achieved_flows{},
    send_requested{false},
    send_achieved{false},
    event_time{0},
    requested_flow{0.0},
    achieved_flow{0.0}
  {
  }

  void
  FlowMeter::delta_int()
  {
    send_achieved = false;
    send_requested = false;
    requested_flow = 0.0;
    achieved_flow = 0.0;
  }

  void
  FlowMeter::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    event_time += e.real;
    bool requested_flow_updated{false};
    bool achieved_flow_updated{false};
    for (const auto &x : xs) {
      if (x.port == inport_output_request) {
        requested_flow_updated = true;
        requested_flow += x.value.get_flow();
      }
      else if (x.port == inport_input_achieved) {
        achieved_flow_updated = true;
        achieved_flow += x.value.get_flow();
      }
    }
    int num_events{static_cast<int>(event_times.size())};
    if (num_events == 0 || (num_events > 0 && event_times.back() != event_time))
      event_times.push_back(event_time);
    if (requested_flow_updated) {
      send_requested = true;
      requested_flows.push_back(requested_flow);
    }
    if (achieved_flow_updated) {
      send_achieved = true;
      achieved_flows.push_back(achieved_flow);
    }
  }

  void
  FlowMeter::delta_conf(std::vector<PortValue>& xs)
  {
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  FlowMeter::ta()
  {
    if (send_requested || send_achieved)
      return adevs::Time{0, 1};
    return adevs_inf<adevs::Time>();
  }

  void
  FlowMeter::output_func(std::vector<PortValue>& ys)
  {
    if (send_requested)
      ys.push_back(
          PortValue{outport_input_request, Flow{stream, requested_flow}});
    if (send_achieved)
      ys.push_back(
          PortValue{outport_output_achieved, Flow{stream, achieved_flow}});
  }

  std::vector<RealTimeType>
  FlowMeter::get_actual_output_times() const
  {
    return std::vector<RealTimeType>(event_times);
  }

  std::vector<FlowValueType>
  FlowMeter::get_actual_output() const
  {
    return std::vector<FlowValueType>(achieved_flows);
  }

  ////////////////////////////////////////////////////////////
  // Transformer
  //  public:
  const int Transformer::inport_input_achieved = 1;
  const int Transformer::inport_output1_request = 2;
  const int Transformer::inport_output2_request = 3;
  const int Transformer::inport_output3_request = 4;
  const int Transformer::inport_output4_request = 5;
  const int Transformer::outport_input_request = 6;
  const int Transformer::outport_output1_achieved = 7;
  const int Transformer::outport_output2_achieved = 8;
  const int Transformer::outport_output3_achieved = 9;
  const int Transformer::outport_output4_achieved = 10;

  Transformer::Transformer(
      StreamType input_stream_type,
      std::vector<StreamType> output_stream_types,
      std::vector<std::function<double(double)>> calc_output_n_from_input,
      std::vector<std::function<double(double)>> calc_input_from_output_n
      ) :
    adevs::Atomic<PortValue>(),
    input_stream{input_stream_type},
    output_streams(output_stream_types),
    num_outputs{0},
    output_n_from_input(calc_output_n_from_input),
    input_from_output_n(calc_input_from_output_n)
  {
    num_outputs = output_streams.size();
    if (input_from_output_n.size() != num_outputs) {
      std::ostringstream oss;
      oss << "Transformer:: The number of output streams (" << num_outputs
          << ") must match the number of input-from-output functions ("
          << input_from_output_n.size() << ") but it doesn't\n";
      throw std::invalid_argument(oss.str());
    }
    if (output_n_from_input.size() != num_outputs) {
      std::ostringstream oss;
      oss << "Transformer:: The number of output streams (" << num_outputs
          << ") must match the number of output-from-input functions ("
          << output_n_from_input.size() << ") but it doesn't\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  Transformer::delta_int()
  {
  }

  void
  Transformer::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
  }

  void
  Transformer::delta_conf(std::vector<PortValue>& xs)
  {
  }

  adevs::Time
  Transformer::ta()
  {
    return adevs_inf<adevs::Time>();
  }

  void
  Transformer::output_func(std::vector<PortValue>& ys)
  {
  }

  ///////////////////////////////////////////////////////////////////
  // Sink
  const int Sink::inport_input_achieved = 1;
  const int Sink::outport_input_request = 2;

  Sink::Sink(StreamType stream_type):
    Sink::Sink(stream_type, {0, 1}, {100, 0})
  {
  }

  Sink::Sink(StreamType stream_type, std::vector<RealTimeType> times, std::vector<FlowValueType> loads):
    adevs::Atomic<PortValue>(),
    stream{stream_type},
    idx{-1},
    times{times},
    loads{loads},
    time{0},
    load{0},
    achieved_times{},
    achieved_loads{}
  {
    if (times.size() != loads.size())
    {
      // throw exception here
    }
    if (times.size() > 0)
    {
      time = times.at(0);
      load = loads.at(0);
    }
  }

  void
  Sink::delta_int()
  {
    ++idx;
    if (idx == 0)
    {
      return;
    }
    if (idx < times.size())
    {
      achieved_times.push_back(time);
      achieved_loads.push_back(load);
      load = loads.at(idx);
      time = times.at(idx);
    }
  }

  void
  Sink::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    int input_achieved{0};
    bool input_given{false};
    for (const auto &x : xs) {
      if (x.port == inport_input_achieved) {
        input_given = true;
        input_achieved += x.value.get_flow();
        if (x.value.get_stream() != stream)
          throw MixedStreamsError();
      }
    }
    if (input_given) {
      // update the load if we're given something on port in achieved
      load = input_achieved;
    }
  }

  void
  Sink::delta_conf(std::vector<PortValue>& xs)
  {
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  Sink::ta()
  {
    if (idx < 0)
    {
      return adevs::Time{0, 0};
    }
    int next_idx = idx + 1;
    if (next_idx < times.size())
    {
      int time0(times.at(idx));
      int time1(times.at(next_idx));
      int dt = time1 - time0;
      return adevs::Time{dt, 0};
    }
    return adevs_inf<adevs::Time>();
  }

  void
  Sink::output_func(std::vector<PortValue>& ys)
  {
    int next_idx = idx + 1;
    if (next_idx < times.size())
    {
      ys.push_back(
          adevs::port_value<Flow>{
            outport_input_request,
            Flow{stream, loads.at(next_idx)
          }});
    }
  }

  std::string
  Sink::get_results() const
  {
    std::ostringstream oss;
    oss << "\"time (hrs)\",\"power [IN] (kW)\"" << std::endl;
    for (int idx(0); idx < achieved_times.size(); ++idx)
    {
      oss << achieved_times[idx] << "," << achieved_loads[idx] << std::endl;
    }
    return oss.str();
  }
}
