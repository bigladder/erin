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
  }

  void
  Sink::delta_conf(std::vector<PortValue>& xs)
  {
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
