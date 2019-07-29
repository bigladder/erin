/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "disco/disco.h"
#include <sstream>
#include <stdexcept>

namespace DISCO
{
  int
  clamp_toward_0(int value, int lower, int upper)
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
  // FLOW
  Flow::Flow(int flow_value, StreamType stream_type):
    flow{flow_value},
    stream{stream_type}
  {
  }

  int
  Flow::get_flow()
  {
    return flow;
  }

  StreamType
  Flow::get_stream()
  {
    return stream;
  }

  ///////////////////////////////////////////////////////////////////
  // SOURCE
  const int Source::port_output_request = 1;

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
      if (x.port == port_output_request) 
      {
        time += e.real;
        Flow f = x.value;
        if (f.get_stream() != stream)
          throw MixedStreamsError();
        int load = f.get_flow();
        times.push_back(time);
        loads.push_back(load);
      }
    }
  }

  void
  Source::delta_conf(std::vector<PortValue>& xs)
  {
  }

  adevs::Time
  Source::ta()
  {
    return adevs_inf<adevs::Time>();
  }

  void
  Source::output_func(std::vector<PortValue>& y)
  {
  }

  std::string
  Source::get_results()
  {
    std::ostringstream oss;
    oss << "\"time (hrs)\",\"power [OUT] (kW)\"" << std::endl;
    for (int idx(0); idx < times.size(); ++idx)
    {
      oss << times.at(idx) << "," << loads.at(idx) << std::endl;
    }
    return oss.str();
  }

  ///////////////////////////////////////////////////////////////////
  // FLOWLIMITS
  const int FlowLimits::port_input_request = 1;
  const int FlowLimits::port_output_request = 2;
  const int FlowLimits::port_input_achieved = 3;
  const int FlowLimits::port_output_achieved = 4;

  FlowLimits::FlowLimits(StreamType stream_type, int low_lim, int up_lim) :
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
      if (x.port == port_output_request) {
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
      else if (x.port == port_input_achieved) {
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
    delta_ext(adevs::Time(0, 0), xs);
  }

  adevs::Time
  FlowLimits::ta()
  {
    if (report_input_request || report_output_achieved)
      return adevs::Time(0, 0);
    return adevs_inf<adevs::Time>();
  }

  void
  FlowLimits::output_func(std::vector<PortValue>& ys)
  {
    if (report_input_request) {
      PortValue pv = adevs::port_value<Flow>(port_input_request, Flow(input_request, stream));
      ys.push_back(pv);
    }
    if (report_output_achieved) {
      PortValue pv = adevs::port_value<Flow>(port_output_achieved, Flow(output_achieved, stream));
      ys.push_back(pv);
    }
  }

  ///////////////////////////////////////////////////////////////////
  // SINK
  const int Sink::port_input_achieved = 1;
  const int Sink::port_input_request = 2;

  Sink::Sink(StreamType stream_type):
    Sink::Sink(stream_type, {0, 1}, {100, 0})
  {
  }

  Sink::Sink(StreamType stream_type, std::vector<int> times, std::vector<int> loads):
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
    for (auto x : xs) {
      if (x.port == port_input_achieved) {
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
    delta_ext(adevs::Time(0, 0), xs);
  }

  adevs::Time
  Sink::ta()
  {
    if (idx < 0)
    {
      return adevs::Time(0, 0);
    }
    int next_idx = idx + 1;
    if (next_idx < times.size())
    {
      int time0(times.at(idx));
      int time1(times.at(next_idx));
      int dt = time1 - time0;
      return adevs::Time(dt, 0);
    }
    return adevs_inf<adevs::Time>();
  }

  void
  Sink::output_func(std::vector<PortValue>& ys)
  {
    int next_idx = idx + 1;
    if (next_idx < times.size())
    {
      PortValue pv = adevs::port_value<Flow>(port_input_request, Flow(loads.at(next_idx), stream));
      ys.push_back(pv);
    }
  }

  std::string
  Sink::get_results()
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
