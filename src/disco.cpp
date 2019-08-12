/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "disco/disco.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace DISCO
{
  const bool DEBUG{false};

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

  std::string
  port_value_to_string(const PortValue& pv)
  {
    std::ostringstream oss;
    oss << "PortValue(port=" << pv.port << ", flow={" << pv.value.get_flow() << "})\n";
    return oss.str();
  }

  std::string
  stream_type_to_string(StreamType st)
  {
    switch (st) {
      case (StreamType::electric_stream_in_kW):
        return "electric_stream_in_kW";
      case (StreamType::natural_gas_stream_in_kg_per_second):
        return "natural_gas_stream_in_kg_per_second";
      case (StreamType::hot_water_stream_in_kg_per_second):
        return "hot_water_stream_in_kg_per_second";
      case (StreamType::chilled_water_stream_in_kg_per_second):
        return "chilled_water_stream_in_kg_per_second";
      case (StreamType::diesel_fuel_stream_in_liters_per_minute):
        return "diesel_fuel_stream_in_liters_per_minute";
    }
  }

  ///////////////////////////////////////////////////////////////////
  // Flow
  Flow::Flow(StreamType stream_type, FlowValueType flow_value):
    stream{stream_type},
    flow{flow_value}
  {
  }

  inline
  StreamType
  Flow::get_stream() const
  {
    return stream;
  }

  inline
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
    if (DEBUG) {
      std::cout << "Source::delta_int()\n";
      std::cout << "... S.time = " << time << "\n";
      for (int i{0}; i < times.size(); ++i) {
        std::cout << "... S.times[" << i << "] = " << times[i] << "\n";
        std::cout << "... S.loads[" << i << "] = " << loads[i] << "\n";
      }
      std::cout << "---\n";
      std::cout << "... Source::delta_int(.) exit\n";
    }
  }

  void
  Source::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    if (DEBUG) {
      std::cout << "Source::delta_ext(e=dt(" << e.real << ", " << e.logical << "), xs)\n";
      for (int i{0}; i < xs.size(); ++i) {
        std::cout << "... xs[" << i << "] = " << port_value_to_string(xs[i]) << "\n";
      }
      std::cout << "... S.time = " << time << "\n";
      for (int i{0}; i < times.size(); ++i) {
        std::cout << "... S.times[" << i << "] = " << times[i] << "\n";
        std::cout << "... S.loads[" << i << "] = " << loads[i] << "\n";
      }
      std::cout << "---\n";
    }
    for (const auto &x : xs) {
      if (x.port == inport_output_request) {
        time += e.real;
        const Flow &f = x.value;
        if (f.get_stream() != stream)
          throw MixedStreamsError();
        FlowValueType load = f.get_flow();
        times.push_back(time);
        loads.push_back(load);
      }
    }
    if (DEBUG) {
      std::cout << "... S.time = " << time << "\n";
      for (int i{0}; i < times.size(); ++i) {
        std::cout << "... S.times[" << i << "] = " << times[i] << "\n";
        std::cout << "... S.loads[" << i << "] = " << loads[i] << "\n";
      }
      std::cout << "---\n";
      std::cout << "... exit Source::delta_ext(.)\n";
    }
  }

  void
  Source::delta_conf(std::vector<PortValue>& xs)
  {
    if (DEBUG)
      std::cout << "Source::delta_conf()\n";
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
    if (DEBUG)
      std::cout << "... Source::delta_conf(.) exit\n";
  }

  adevs::Time
  Source::ta()
  {
    if (DEBUG) {
      std::cout << "Source::ta()\n";
      std::cout << "... returning infinity\n";
    }
    return adevs_inf<adevs::Time>();
  }

  void
  Source::output_func(std::vector<PortValue>& ys)
  {
    if (DEBUG) {
      std::cout << "Source::output_func()\n";
      std::cout << "... Source::output_func(.) exit\n";
    }
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
    output_achieved{0},
    flow_limited{false}
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
    if (DEBUG) {
      std::cout << "FlowLimits::delta_int()\n";
      std::cout << "... stream = " << stream_type_to_string(stream) << "\n";
      std::cout << "... lower_limit = " << lower_limit << "\n";
      std::cout << "... upper_limit = " << upper_limit << "\n";
      std::cout << "... report_input_request = " << report_input_request << "\n";
      std::cout << "... report_output_achieved = " << report_output_achieved << "\n";
      std::cout << "... input_request = " << input_request << "\n";
      std::cout << "... output_achieved = " << output_achieved << "\n";
      std::cout << "... flow_limited = " << flow_limited << "\n";
      std::cout << "---\n";
    }
    report_input_request = false;
    report_output_achieved = false;
    input_request = 0;
    output_achieved = 0;
    flow_limited = false;
    if (DEBUG) {
      std::cout << "... stream = " << stream_type_to_string(stream) << "\n";
      std::cout << "... lower_limit = " << lower_limit << "\n";
      std::cout << "... upper_limit = " << upper_limit << "\n";
      std::cout << "... report_input_request = " << report_input_request << "\n";
      std::cout << "... report_output_achieved = " << report_output_achieved << "\n";
      std::cout << "... input_request = " << input_request << "\n";
      std::cout << "... output_achieved = " << output_achieved << "\n";
      std::cout << "... flow_limited = " << flow_limited << "\n";
      std::cout << "... exit FlowLimits::delta_int(.)\n";
    }
  }

  void
  FlowLimits::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    if (DEBUG) {
      std::cout << "FlowLimits::delta_ext(dt(" << e.real << ", " << e.logical << "), xs)\n";
      for (int i{0}; i < xs.size(); ++i)
        std::cout << "... xs[" << i << "] = " << port_value_to_string(xs[i]) << "\n";
      std::cout << "... stream = " << stream_type_to_string(stream) << "\n";
      std::cout << "... lower_limit = " << lower_limit << "\n";
      std::cout << "... upper_limit = " << upper_limit << "\n";
      std::cout << "... report_input_request = " << report_input_request << "\n";
      std::cout << "... report_output_achieved = " << report_output_achieved << "\n";
      std::cout << "... input_request = " << input_request << "\n";
      std::cout << "... output_achieved = " << output_achieved << "\n";
      std::cout << "... flow_limited = " << flow_limited << "\n";
      std::cout << "---\n";
    }
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
      auto temp{clamp_toward_0(input_request, lower_limit, upper_limit)};
      if (temp != input_request) {
        flow_limited = true;
        input_request = temp;
      }
      if (!report_output_achieved && flow_limited) {
        // if an input request is given but no output achieved, assume that
        // output achieved is input request (limited) unless we hear otherwise.
        report_output_achieved = true;
        output_achieved = input_request;
      }
    }
    if (DEBUG) {
      std::cout << "... stream = " << stream_type_to_string(stream) << "\n";
      std::cout << "... lower_limit = " << lower_limit << "\n";
      std::cout << "... upper_limit = " << upper_limit << "\n";
      std::cout << "... report_input_request = " << report_input_request << "\n";
      std::cout << "... report_output_achieved = " << report_output_achieved << "\n";
      std::cout << "... input_request = " << input_request << "\n";
      std::cout << "... output_achieved = " << output_achieved << "\n";
      std::cout << "... flow_limited = " << flow_limited << "\n";
      std::cout << "... exit FlowLimits::delta_ext(.)\n";
    }
  }

  void
  FlowLimits::delta_conf(std::vector<PortValue>& xs)
  {
    if (DEBUG)
      std::cout << "FlowLimits::delta_conf()\n";
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  FlowLimits::ta()
  {
    if (DEBUG)
      std::cout << "FlowLimits::ta()\n";
    if (report_input_request) {
      if (DEBUG)
        std::cout << "... returning dt(0, 0)\n";
      return adevs::Time{0, 0};
    }
    if (report_output_achieved) {
      if (DEBUG)
        std::cout << "... returning dt(0, 1)\n";
      return adevs::Time{0, 1};
    }
    if (DEBUG)
      std::cout << "... returning infinity\n";
    return adevs_inf<adevs::Time>();
  }

  void
  FlowLimits::output_func(std::vector<PortValue>& ys)
  {
    if (DEBUG)
      std::cout << "FlowLimits::output_fn()\n";
    if (report_input_request)
      ys.push_back(
          adevs::port_value<Flow>{
            outport_input_request, Flow{stream, input_request}});
    if (report_output_achieved || flow_limited)
      ys.push_back(
          adevs::port_value<Flow>{
            outport_output_achieved, Flow{stream, output_achieved}});
  }

  ////////////////////////////////////////////////////////////
  // FlowMeter
  const int FlowMeter::inport_input_achieved = 1;
  const int FlowMeter::inport_output_request = 2;
  const int FlowMeter::outport_input_request = 3;
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
    if (DEBUG) {
      std::cout << "FlowMeter::delta_int()\n";
      std::cout << "... S.event_time = " << event_time << "\n";
      std::cout << "... S.requested_flow = " << requested_flow << "\n";
      std::cout << "... S.achieved_flow = " << requested_flow << "\n";
      for (int i{0}; i < event_times.size(); ++i) {
        std::cout << "... S.event_times[" << i << "] = " << event_times[i] << "\n";
      }
      for (int i{0}; i < requested_flows.size(); ++i) {
        std::cout << "... S.requested_flows[" << i << "] = " << requested_flows[i] << "\n";
      }
      for (int i{0}; i < achieved_flows.size(); ++i) {
        std::cout << "... S.achieved_flows[" << i << "] = " << achieved_flows[i] << "\n";
      }
      std::cout << "---\n";
    }
    int num_events{static_cast<int>(event_times.size())};
    if (num_events == 0 || (num_events > 0 && event_times.back() != event_time)) {
      event_times.push_back(event_time);
      ++num_events;
    }
    if (send_requested) {
      requested_flows.push_back(requested_flow);
      if (achieved_flows.size() == num_events && achieved_flows.size() > 0) {
        auto &v = achieved_flows.back();
        v = requested_flow;
      }
      else
        achieved_flows.push_back(requested_flow);
    }
    if (send_achieved) {
      if (achieved_flows.size() == num_events && achieved_flows.size() > 0) {
        auto &v = achieved_flows.back();
        v = achieved_flow;
      }
      else
        achieved_flows.push_back(achieved_flow);
    }
    send_achieved = false;
    send_requested = false;
    requested_flow = 0.0;
    achieved_flow = 0.0;
    if (DEBUG) {
      std::cout << "... S.event_time = " << event_time << "\n";
      std::cout << "... S.requested_flow = " << requested_flow << "\n";
      std::cout << "... S.achieved_flow = " << requested_flow << "\n";
      for (int i{0}; i < event_times.size(); ++i) {
        std::cout << "... S.event_times[" << i << "] = " << event_times[i] << "\n";
      }
      for (int i{0}; i < requested_flows.size(); ++i) {
        std::cout << "... S.requested_flows[" << i << "] = " << requested_flows[i] << "\n";
      }
      for (int i{0}; i < achieved_flows.size(); ++i) {
        std::cout << "... S.achieved_flows[" << i << "] = " << achieved_flows[i] << "\n";
      }
      std::cout << "... exit FlowMeter::delta_int(.)\n";
    }
  }

  void
  FlowMeter::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    if (DEBUG) {
      std::cout << "FlowMeter::delta_ext(dt(" << e.real << ", " << e.logical << "), xs)\n";
      for (int i{0}; i < xs.size(); ++i)
        std::cout << "... xs[" << i << "] = " << port_value_to_string(xs[i]) << "\n";
      std::cout << "... S.event_time = " << event_time << "\n";
      std::cout << "... S.requested_flow = " << requested_flow << "\n";
      std::cout << "... S.achieved_flow = " << requested_flow << "\n";
      for (int i{0}; i < event_times.size(); ++i) {
        std::cout << "... S.event_times[" << i << "] = " << event_times[i] << "\n";
      }
      for (int i{0}; i < requested_flows.size(); ++i) {
        std::cout << "... S.requested_flows[" << i << "] = " << requested_flows[i] << "\n";
      }
      for (int i{0}; i < achieved_flows.size(); ++i) {
        std::cout << "... S.achieved_flows[" << i << "] = " << achieved_flows[i] << "\n";
      }
      std::cout << "---\n";
    }
    event_time += e.real;
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_output_request:
          send_requested = true;
          requested_flow += x.value.get_flow();
          break;
        case inport_input_achieved:
          send_achieved = true;
          achieved_flow += x.value.get_flow();
          break;
        default:
          break;
      }
    }
    if (DEBUG) {
      std::cout << "... S.event_time = " << event_time << "\n";
      std::cout << "... S.requested_flow = " << requested_flow << "\n";
      std::cout << "... S.achieved_flow = " << requested_flow << "\n";
      for (int i{0}; i < event_times.size(); ++i) {
        std::cout << "... S.event_times[" << i << "] = " << event_times[i] << "\n";
      }
      for (int i{0}; i < requested_flows.size(); ++i) {
        std::cout << "... S.requested_flows[" << i << "] = " << requested_flows[i] << "\n";
      }
      for (int i{0}; i < achieved_flows.size(); ++i) {
        std::cout << "... S.achieved_flows[" << i << "] = " << achieved_flows[i] << "\n";
      }
      std::cout << "... exit FlowMeter::delta_ext(.)\n";
    }
  }

  void
  FlowMeter::delta_conf(std::vector<PortValue>& xs)
  {
    if (DEBUG)
      std::cout << "FlowMeter::delta_conf()\n";
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  FlowMeter::ta()
  {
    if (DEBUG)
      std::cout << "FlowMeter::ta()\n";
    if (send_requested) {
      if (DEBUG)
        std::cout << "... send_requested branch; returning dt(0,0)\n";
      return adevs::Time{0, 0};
    }
    if (send_achieved) {
      if (DEBUG)
        std::cout << "... send_requested branch; returning dt(0,0)\n";
      return adevs::Time{0, 0};
    }
    if (DEBUG)
      std::cout << "... returning infinity\n";
    return adevs_inf<adevs::Time>();
  }

  void
  FlowMeter::output_func(std::vector<PortValue>& ys)
  {
    if (DEBUG)
      std::cout << "FlowMeter::output_func()\n";
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
  const int Transformer::inport_output_request = 2;
  const int Transformer::outport_input_request = 3;
  const int Transformer::outport_output_achieved = 4;

  Transformer::Transformer(
      StreamType input_stream_type,
      StreamType output_stream_type,
      std::function<FlowValueType(FlowValueType)> calc_output_from_input,
      std::function<FlowValueType(FlowValueType)> calc_input_from_output
      ) :
    adevs::Atomic<PortValue>(),
    input_stream{input_stream_type},
    output_stream{output_stream_type},
    output_from_input{calc_output_from_input},
    input_from_output{calc_input_from_output},
    output{0.0},
    input{0.0},
    send_input_request{false},
    send_output_achieved{false}
  {
  }

  void
  Transformer::delta_int()
  {
    if (DEBUG)
      std::cout << "Transformer::delta_int()\n";
    input = 0.0;
    output = 0.0;
    send_input_request = false;
    send_output_achieved = false;
  }

  void
  Transformer::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    if (DEBUG) {
      std::cout << "Transformer::delta_ext(" << e.real << ")\n";
      for (const auto &x : xs)
        std::cout << "... x = " << port_value_to_string(x) << "\n";
    }
    for (const auto &x : xs) {
      switch (x.port) {
        case inport_input_achieved:
          send_output_achieved = true;
          if (x.value.get_stream() != input_stream)
            throw MixedStreamsError();
          input += x.value.get_flow();
          break;
        case inport_output_request:
        {
          send_input_request = true;
          if (x.value.get_stream() != output_stream)
            throw MixedStreamsError();
          output += x.value.get_flow();
          break;
        }
        default:
          break;
      }
    }
    // update inputs / outputs
    if (send_output_achieved) {
      output = 0.0;
      output = output_from_input(input);
    }
    if (send_input_request) {
      input = 0.0;
      input = input_from_output(output);
    }
  }

  void
  Transformer::delta_conf(std::vector<PortValue>& xs)
  {
    if (DEBUG)
      std::cout << "Transformer::delta_conf()\n";
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  Transformer::ta()
  {
    if (DEBUG)
      std::cout << "Transformer::ta()\n";
    if (send_input_request)
      return adevs::Time{0, 0};
    if (send_output_achieved)
      return adevs::Time{0, 0};
    return adevs_inf<adevs::Time>();
  }

  void
  Transformer::output_func(std::vector<PortValue>& ys)
  {
    if (DEBUG)
      std::cout << "Transformer::output_func()\n";
    if (send_input_request)
      ys.push_back(
          adevs::port_value<Flow>{
            outport_input_request, Flow{input_stream, input}});
    if (send_output_achieved)
      ys.push_back(
          adevs::port_value<Flow>{
            outport_output_achieved, Flow{output_stream, output}});
  }

  ///////////////////////////////////////////////////////////////////
  // Sink
  const int Sink::inport_input_achieved = 1;
  const int Sink::outport_input_request = 2;

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
    if (times.size() != loads.size()) {
      std::ostringstream oss;
      oss << "DISCO::Sink::Sink: times.size() (" << times.size()
          << ") != loads.size() (" << loads.size() << ")" << std::endl;
      throw std::invalid_argument(oss.str());
    }
    if (times.size() > 0) {
      time = times.at(0);
      load = loads.at(0);
    }
  }

  void
  Sink::delta_int()
  {
    if (DEBUG)
      std::cout << "Sink::delta_int()\n";
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
    if (DEBUG) {
      std::cout << "Sink::delta_ext(" << e.real << ")\n";
      for (const auto &x : xs)
        std::cout << "... x = " << port_value_to_string(x) << "\n";
    }
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
    if (DEBUG)
      std::cout << "Sink::delta_conf()\n";
    delta_int();
    delta_ext(adevs::Time{0, 0}, xs);
  }

  adevs::Time
  Sink::ta()
  {
    if (DEBUG)
      std::cout << "Sink::ta()\n";
    if (idx < 0)
      return adevs::Time{0, 0};
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
    if (DEBUG)
      std::cout << "Sink::output_func()\n";
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
