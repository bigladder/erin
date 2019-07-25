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
  Flow::Flow(int flowValue):
    _flow(flowValue)
  {
  }

  int
  Flow::getFlow()
  {
    return _flow;
  }

  ///////////////////////////////////////////////////////////////////
  // SOURCE
  const int Source::PORT_OUT_REQUEST = 1;

  Source::Source():
    adevs::Atomic<PortValue>(),
    _time(0),
    _times(),
    _loads()
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
      if (x.port == PORT_OUT_REQUEST) 
      {
        _time += e.real;
        Flow f = x.value;
        int load = f.getFlow();
        _times.push_back(_time);
        _loads.push_back(load);
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
  Source::getResults()
  {
    std::ostringstream oss;
    oss << "\"time (hrs)\",\"power [OUT] (kW)\"" << std::endl;
    for (int idx(0); idx < _times.size(); idx++)
    {
      oss << _times.at(idx) << "," << _loads.at(idx) << std::endl;
    }
    return oss.str();
  }

  ///////////////////////////////////////////////////////////////////
  // FLOWLIMITS
  const int FlowLimits::port_input_request = 1;
  const int FlowLimits::port_output_request = 2;
  const int FlowLimits::port_input_achieved = 3;
  const int FlowLimits::port_output_achieved = 4;

  FlowLimits::FlowLimits(int low_lim, int up_lim) :
    adevs::Atomic<PortValue>(),
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
        input_request += x.value.getFlow();
      }
      else if (x.port == port_input_achieved) {
        // the input achieved equals the output achieved. output_achieved is
        // initialized to 0 at construction and at each internal transition.
        report_output_achieved = true;
        output_achieved += x.value.getFlow();
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
      PortValue pv = adevs::port_value<Flow>(port_input_request, Flow(input_request));
      ys.push_back(pv);
    }
    if (report_output_achieved) {
      PortValue pv = adevs::port_value<Flow>(port_output_achieved, Flow(output_achieved));
      ys.push_back(pv);
    }
  }

  ///////////////////////////////////////////////////////////////////
  // SINK
  const int Sink::PORT_IN_ACHIEVED = 1;
  const int Sink::PORT_IN_REQUEST = 2;

  Sink::Sink():
    Sink::Sink({0, 1}, {100, 0})
  {
  }

  Sink::Sink(std::vector<int> times, std::vector<int> loads):
    adevs::Atomic<PortValue>(),
    _idx(-1),
    _times(times),
    _loads(loads),
    _time(0),
    _load(0),
    _achieved_times(),
    _achieved_loads()
  {
    if (_times.size() != _loads.size())
    {
      // throw exception here
    }
    if (_times.size() > 0)
    {
      _time = _times.at(0);
      _load = _loads.at(0);
    }
  }

  void
  Sink::delta_int()
  {
    _idx++;
    if (_idx == 0)
    {
      return;
    }
    if (_idx < _times.size())
    {
      _achieved_times.push_back(_time);
      _achieved_loads.push_back(_load);
      _load = _loads.at(_idx);
      _time = _times.at(_idx);
    }
  }

  void
  Sink::delta_ext(adevs::Time e, std::vector<PortValue>& xs)
  {
    int input_achieved{0};
    bool input_given{false};
    for (auto x : xs) {
      if (x.port == PORT_IN_ACHIEVED) {
        input_given = true;
        input_achieved += x.value.getFlow();
      }
    }
    if (input_given) {
      // update the load if we're given something on port in achieved
      _load = input_achieved;
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
    if (_idx < 0)
    {
      return adevs::Time(0, 0);
    }
    int next_idx = _idx + 1;
    if (next_idx < _times.size())
    {
      int time0(_times.at(_idx));
      int time1(_times.at(next_idx));
      int dt = time1 - time0;
      return adevs::Time(dt, 0);
    }
    return adevs_inf<adevs::Time>();
  }

  void
  Sink::output_func(std::vector<PortValue>& ys)
  {
    int next_idx = _idx + 1;
    if (next_idx < _times.size())
    {
      Flow f(_loads.at(next_idx));
      PortValue pv = adevs::port_value<Flow>(PORT_IN_REQUEST, f);
      ys.push_back(pv);
    }
  }

  std::string
  Sink::getResults()
  {
    std::ostringstream oss;
    oss << "\"time (hrs)\",\"power [IN] (kW)\"" << std::endl;
    for (int idx(0); idx < _achieved_times.size(); idx++)
    {
      oss << _achieved_times[idx] << "," << _achieved_loads[idx] << std::endl;
    }
    return oss.str();
  }
}
