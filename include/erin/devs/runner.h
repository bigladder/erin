/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/devs.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace erin::devs
{
  enum class TransitionType {
    InitialState = 0,
    InternalTransition,
    ExternalTransition,
    ConfluentTransition
  };

  template <class S>
  struct TimeStateOutputs
  {
    TransitionType transition_type{TransitionType::InitialState};
    RealTimeType time_s{0};
    S state{};
    std::vector<PortValue> outputs{};
  };

  std::string
  port_to_tag(int port)
  {
    switch (port)
    {
      case inport_inflow_achieved:
        return "inport_inflow_achieved";
      case inport_outflow_request:
        return "inport_outflow_request";
      case outport_inflow_request:
        return "outport_inflow_request";
      case outport_outflow_achieved:
        return "outflow_outflow_achieved";
      default:
        return "unknown_port_" + std::to_string(port);
    }
  }

  std::string
  port_values_to_string(const std::vector<PortValue>& port_values)
  {
    std::ostringstream oss{};
    bool first{true};
    for (const auto& pv : port_values) {
      oss << (first ? "" : ",")
          << "PortValue{" << port_to_tag(pv.port)
          << ", " << pv.value << "}";
      first = false;
    }
    return oss.str();
  }

  std::string
  transition_type_to_tag(const TransitionType& tt)
  {
    switch (tt)
    {
      case TransitionType::InitialState:
        return "init";
      case TransitionType::InternalTransition:
        return "int";
      case TransitionType::ExternalTransition:
        return "ext";
      case TransitionType::ConfluentTransition:
        return "conf";
      default:
        return "?";
    }
  }

  template <class S>
  std::vector<TimeStateOutputs<S>>
  run_devs(
      const std::function<RealTimeType(const S&)>& ta,
      const std::function<S(const S&)>& delta_int,
      const std::function<S(const S&, RealTimeType, const std::vector<PortValue>&)>& delta_ext,
      const std::function<S(const S&, const std::vector<PortValue>&)>& delta_conf,
      const std::function<std::vector<PortValue>(const S&)>& outfn,
      const S& s0,
      const std::vector<RealTimeType>& times_s,
      const std::vector<std::vector<PortValue>>& xss,
      const RealTimeType& max_time_s)
  {
    using size_type = std::vector<RealTimeType>::size_type;
    std::vector<TimeStateOutputs<S>> log{};
    auto s{s0};
    RealTimeType t_last{0};
    RealTimeType t_next{0};
    size_type ext_idx{0};
    RealTimeType t_next_ext{infinity};
    std::vector<PortValue> ys{};
    log.emplace_back(
        TimeStateOutputs<S>{TransitionType::InitialState, t_next, s, ys});
    while ((t_next != infinity) || (t_next_ext != infinity)) {
      auto dt = ta(s);
      if (dt == infinity) {
        t_next = infinity;
      }
      else {
        t_next = t_last + dt;
      }
      if (ext_idx < times_s.size()) {
        t_next_ext = times_s[ext_idx];
      }
      else {
        t_next_ext = infinity;
      }
      if (((t_next == infinity) || (t_next > max_time_s)) &&
          ((t_next_ext == infinity) || (t_next_ext > max_time_s))) {
        break;
      }
      if ((t_next_ext == infinity) || ((t_next != infinity) && (t_next < t_next_ext))) {
        // internal transition
        ys = outfn(s);
        s = delta_int(s);
        log.emplace_back(
            TimeStateOutputs<S>{TransitionType::InternalTransition, t_next, s, ys});
      }
      else {
        const auto& xs = xss[ext_idx];
        ext_idx++;
        if (t_next == t_next_ext) {
          ys = outfn(s);
          s = delta_conf(s, xs);
          log.emplace_back(
              TimeStateOutputs<S>{TransitionType::ConfluentTransition, t_next, s, ys});
        }
        else {
          auto e{t_next_ext - t_last};
          t_next = t_last + e;
          s = delta_ext(s, e, xs);
          ys.clear();
          log.emplace_back(
              TimeStateOutputs<S>{TransitionType::ExternalTransition, t_next, s, ys});
        }
      }
      t_last = t_next;
    }
    return log;
  }
  
  template <class S>
  void
  write_details(const TimeStateOutputs<S>& out)
  {
    std::cout << "------------------------\n"
              << " transition type: "
              << transition_type_to_tag(out.transition_type) << "\n"
              << " time (seconds) : " << out.time_s << "\n"
              << " state          : " << out.state << "\n"
              << " outputs        : "
              << port_values_to_string(out.outputs) << "\n";
  }
}
