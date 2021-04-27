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
        ++ext_idx;
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

  struct EnergyAudit
  {
    double in{0.0};
    double out{0.0};
    double waste{0.0};
    double store{0.0};
  };

  double
  energy_audit_error(const EnergyAudit& ea)
  {
    return ea.in - (ea.out + ea.waste + ea.store);
  }

  template <class S>
  struct TimeStateOutputsV2
  {
    TransitionType transition_type{TransitionType::InitialState};
    RealTimeType time_s{0};
    S state{};
    std::vector<PortValue> outputs{};
    std::vector<PortValue> inputs{};
    EnergyAudit energy{};
  };

  template <class S>
  void
  write_details_v2(const TimeStateOutputsV2<S>& out)
  {
    std::cout << "------------------------\n"
              << " transition type: "
              << transition_type_to_tag(out.transition_type) << "\n"
              << " time (seconds) : " << out.time_s << "\n"
              << " state          : " << out.state << "\n"
              << " outputs        : "
              << port_values_to_string(out.outputs) << "\n"
              << " inputs         : "
              << port_values_to_string(out.inputs) << "\n"
              << "Energy Audit    : \n"
              << " in             : " << out.energy.in << "\n"
              << " out            : " << out.energy.out << "\n"
              << " waste          : " << out.energy.waste << "\n"
              << " store          : " << out.energy.store << "\n"
              << " error          : " << energy_audit_error(out.energy) << "\n";
  }

  template <class S>
  std::vector<TimeStateOutputsV2<S>>
  run_devs_v2(
      const std::function<RealTimeType(const S&)>& ta,
      const std::function<S(const S&)>& delta_int,
      const std::function<S(const S&, RealTimeType, const std::vector<PortValue>&)>& delta_ext,
      const std::function<S(const S&, const std::vector<PortValue>&)>& delta_conf,
      const std::function<std::vector<PortValue>(const S&)>& outfn,
      const S& s0,
      const std::vector<RealTimeType>& times_s,
      const std::vector<std::vector<PortValue>>& xss,
      const RealTimeType& max_time_s,
      const std::function<EnergyAudit(const S&, const EnergyAudit&, const RealTimeType& dt)>& energy_fn)
  {
    constexpr bool debug{false};
    using size_type = std::vector<RealTimeType>::size_type;
    std::vector<TimeStateOutputsV2<S>> log{};
    auto s{s0};
    RealTimeType t_last{0};
    RealTimeType t_next{0};
    size_type ext_idx{0};
    RealTimeType t_next_ext{infinity};
    std::vector<PortValue> ys{};
    EnergyAudit energy{};
    log.emplace_back(
        TimeStateOutputsV2<S>{TransitionType::InitialState, t_next, s, ys, {}, energy});
    int counter{0};
    const int max_count{1000};
    while ((t_next != infinity) || (t_next_ext != infinity)) {
      ++counter;
      if (counter > max_count) {
        break;
      }
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
      if (debug) {
        std::cout 
                  << "counter = " << counter << "\n"
                  << "dt = " << dt << "\n"
                  << "t_next = " << ((t_next == infinity) ? "infinity" : std::to_string(t_next)) << "\n"
                  << "t_last = " << t_last << "\n"
                  << "t_next_ext = " << t_next_ext << "\n"
                  ;
      }
      if (((t_next == infinity) || (t_next > max_time_s)) &&
          ((t_next_ext == infinity) || (t_next_ext > max_time_s))) {
        break;
      }
      if ((t_next_ext == infinity) || ((t_next != infinity) && (t_next < t_next_ext))) {
        // internal transition
        if (debug) {
          std::cout << "- internal transition:\n";
        }
        ys = outfn(s);
        energy = energy_fn(s, energy, t_next - t_last);
        s = delta_int(s);
        log.emplace_back(
            TimeStateOutputsV2<S>{TransitionType::InternalTransition, t_next, s, ys, {}, energy});
      }
      else {
        const auto& xs = xss[ext_idx];
        ++ext_idx;
        if (t_next == t_next_ext) {
          if (debug) {
            std::cout << "- confluent transition:\n";
          }
          ys = outfn(s);
          energy = energy_fn(s, energy, t_next - t_last);
          s = delta_conf(s, xs);
          log.emplace_back(
              TimeStateOutputsV2<S>{TransitionType::ConfluentTransition, t_next, s, ys, xs, energy});
        }
        else {
          if (debug) {
            std::cout << "- external transition:\n";
          }
          auto e{t_next_ext - t_last};
          t_next = t_last + e;
          s = delta_ext(s, e, xs);
          ys.clear();
          log.emplace_back(
              TimeStateOutputsV2<S>{TransitionType::ExternalTransition, t_next, s, ys, xs, energy});
        }
      }
      t_last = t_next;
    }
    return log;
  }
}
