/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "disco/disco.h"
#include <sstream>

namespace DISCO
{
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
    adevs::Atomic<PortValue>()
  {
  }

  std::string
  Source::getResults()
  {
    return "";
  }

  void
  Source::delta_int()
  {
  }

  void
  Source::delta_ext(adevs::Time e, std::vector<PortValue>& x)
  {
  }

  void
  Source::delta_conf(std::vector<PortValue>& x)
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
    _idx(0),
    _times(times),
    _loads(loads),
    _load(0),
    _achieved()
  {
    if (_times.size() != _loads.size())
    {
      // throw exception here
    }
    if (_idx < _times.size())
    {
      _load = _loads.at(_idx);
    }
  }

  void
  Sink::delta_int()
  {
    _achieved.push_back(_load);
    _idx++;
    if (_idx < _times.size())
    {
      _load = _loads.at(_idx);
    }
  }

  void
  Sink::delta_ext(adevs::Time e, std::vector<PortValue>& x)
  {
  }

  void
  Sink::delta_conf(std::vector<PortValue>& x)
  {
  }

  adevs::Time
  Sink::ta()
  {
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
  Sink::output_func(std::vector<PortValue>& y)
  {
    int next_idx = _idx + 1;
    if (next_idx < _times.size())
    {
      Flow f(_loads.at(next_idx));
      PortValue pv = adevs::port_value<Flow>(PORT_IN_REQUEST, f);
      y.push_back(pv);
    }
  }

  std::string
  Sink::getResults()
  {
    std::ostringstream oss;
    oss << "\"time (hrs)\",\"power [IN] (kW)\"" << std::endl;
    for (int idx(0); idx < _achieved.size(); idx++)
    {
      oss << _times[idx] << "," << _achieved[idx] << std::endl;
    }
    return oss.str();
  }
}
