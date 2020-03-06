/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs.h"
#include "erin/devs/converter.h"
#include "debug_utils.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <typeindex>

namespace erin::devs
{
  bool
  operator==(const ConversionFun& a, const ConversionFun& b)
  {
    if constexpr (ERIN::debug_level <= ERIN::debug_level_low)
    {
      std::cout << "entering operator==(const ConversionFun& a, const ConversionFun& b)\n";
    }
    const auto& a_idx = typeid(a);
    const auto& b_idx = typeid(b);
    
    if (a_idx != b_idx)
      return false;
    if (typeid(ConstantEfficiencyFun) == a_idx)
    {
      const auto& aa = dynamic_cast<const ConstantEfficiencyFun&>(a);
      const auto& bb = dynamic_cast<const ConstantEfficiencyFun&>(b);
      return aa == bb;
    }
    std::ostringstream oss{};
    oss << "unhandled type " << a_idx.name() << "\n";
    throw std::runtime_error(oss.str());
  }

  bool
  operator!=(const ConversionFun& a, const ConversionFun& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const ConversionFun& a)
  {
    const auto& idx = typeid(a);
    
    if (typeid(ConstantEfficiencyFun) == idx) {
      const auto& aa = dynamic_cast<const ConstantEfficiencyFun&>(a);
      return os << aa;
    }
    std::ostringstream oss{};
    oss << "unhandled type " << typeid(a).name() << "\n";
    throw std::runtime_error(oss.str());
  }

  ConstantEfficiencyFun::ConstantEfficiencyFun():
    ConstantEfficiencyFun(1.0)
  {
  }

  ConstantEfficiencyFun::ConstantEfficiencyFun(
      FlowValueType constant_efficiency_):
    ConversionFun(),
    constant_efficiency{constant_efficiency_}
  {
    if ((constant_efficiency <= 0.0) || (constant_efficiency > 1.0)) {
      std::ostringstream oss{};
      oss << "constant_efficiency must be > 0.0  and <= 1.0\n";
      oss << "constant_efficiency = " << constant_efficiency << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  std::unique_ptr<ConversionFun>
  ConstantEfficiencyFun::clone() const
  {
    std::unique_ptr<ConversionFun> p =
      std::make_unique<ConstantEfficiencyFun>(constant_efficiency);
    return p;
  }

  FlowValueType
  ConstantEfficiencyFun::outflow_given_inflow(FlowValueType inflow) const
  {
    return inflow * constant_efficiency;
  }

  FlowValueType
  ConstantEfficiencyFun::inflow_given_outflow(FlowValueType outflow) const
  {
    return outflow / constant_efficiency;
  }

  bool
  operator==(const ConstantEfficiencyFun& a, const ConstantEfficiencyFun& b)
  {
    return (a.constant_efficiency == b.constant_efficiency);
  }

  bool
  operator!=(const ConstantEfficiencyFun& a, const ConstantEfficiencyFun& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const ConstantEfficiencyFun& a)
  {
    return os << "ConstantEfficiencyFun("
              << "constant_efficiency=" << a.constant_efficiency
              << ")";
  }

  bool operator==(const ConverterState& a, const ConverterState& b)
  {
    return (a.time == b.time)
      && (a.inflow_port == b.inflow_port)
      && (a.outflow_port == b.outflow_port)
      && (a.lossflow_port == b.lossflow_port)
      && (a.wasteflow_port == b.wasteflow_port)
      && ((*a.conversion_fun) == (*b.conversion_fun))
      && (a.report_inflow_request == b.report_inflow_request)
      && (a.report_outflow_achieved == b.report_outflow_achieved)
      && (a.report_lossflow_achieved == b.report_lossflow_achieved);
  }

  std::ostream&
  operator<<(std::ostream& os, const ConverterState& a)
  {
    return os << "ConverterState("
              << "time=" << a.time << ", "
              << "inflow_port=" << a.inflow_port << ", "
              << "outflow_port=" << a.outflow_port << ", "
              << "lossflow_port=" << a.lossflow_port << ", "
              << "wasteflow_port=" << a.wasteflow_port << ", "
              << "conversion_fun=" << (*a.conversion_fun) << ", "
              << "report_inflow_request=" << a.report_inflow_request << ", "
              << "report_outflow_request=" << a.report_outflow_achieved << ", "
              << "report_lossflow_request=" << a.report_lossflow_achieved << ")";
  }

  ConverterState
  make_converter_state(FlowValueType constant_efficiency)
  {
    std::unique_ptr<ConversionFun> p =
      std::make_unique<ConstantEfficiencyFun>(constant_efficiency);
    return ConverterState{
      0,            // time
      Port{0, 0.0}, // inflow_port
      Port{0, 0.0}, // outflow_port
      Port{0, 0.0}, // lossflow_port
      Port{0, 0.0}, // wasteflow_port
      std::move(p), // conversion_fun
      false,        // report_inflow_request
      false,        // report_outflow_achieved
      false};       // report_lossflow_achieved
  }

  RealTimeType
  converter_time_advance(const ConverterState& state)
  {
    if (state.report_inflow_request || state.report_outflow_achieved || state.report_lossflow_achieved)
      return 0;
    return infinity;
  }

  ConverterState
  converter_internal_transition(const ConverterState& state)
  {
    return ConverterState{
      state.time, // internal transitions alsways take 0 time
      state.inflow_port,
      state.outflow_port,
      state.lossflow_port,
      state.wasteflow_port,
      state.conversion_fun->clone(),
      false,
      false,
      false,
    };
  }

  ConverterState
  converter_external_transition(
      const ConverterState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs)
  {
    bool got_outflow_request{false};
    bool got_inflow_achieved{false};
    bool got_lossflow_request{false};
    FlowValueType outflow_request{0.0};
    FlowValueType inflow_achieved{0.0};
    FlowValueType lossflow_request{0.0};
    for (const auto& x: xs) {
      switch (x.port) {
        case inport_outflow_request:
          {
            got_outflow_request = true;
            outflow_request += x.value;
            break;
          }
        case inport_inflow_achieved:
          {
            got_inflow_achieved = true;
            inflow_achieved += x.value;
            break;
          }
        case inport_lossflow_request:
          {
            got_lossflow_request = true;
            lossflow_request += x.value;
            break;
          }
        default:
          {
            std::ostringstream oss;
            oss << "unhandled port " << x.port
                << " in converter_external_transition(...)";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    const auto& t = state.time;
    auto new_time = t + elapsed_time;
    ConverterState new_state{
      new_time,
      state.inflow_port,
      state.outflow_port,
      state.lossflow_port,
      state.wasteflow_port,
      state.conversion_fun->clone(),
      state.report_inflow_request,
      state.report_outflow_achieved,
      state.report_lossflow_achieved,
    };
    if (got_inflow_achieved)
      new_state = converter_external_transition_on_inflow_achieved(
          new_state, new_time, inflow_achieved);
    if (got_lossflow_request)
      new_state = converter_external_transition_on_lossflow_request(
          new_state, new_time, lossflow_request);
    if (got_outflow_request)
      new_state = converter_external_transition_on_outflow_request(
          new_state, new_time, outflow_request);
    return new_state;
  }

  ConverterState
  converter_external_transition_on_outflow_request(
      const ConverterState& state,
      RealTimeType new_time,
      FlowValueType outflow_request)
  {
    const auto& ip = state.inflow_port;
    const auto& op = state.outflow_port;
    const auto& lp = state.lossflow_port;
    const auto& wp = state.wasteflow_port;
    const auto& f = state.conversion_fun;
    auto inflow_request = f->inflow_given_outflow(outflow_request);
    auto lossflow_achieved = f->lossflow_given_outflow(outflow_request);
    auto new_op{op.with_requested(outflow_request, new_time)};
    auto new_ip{ip.with_requested(inflow_request, new_time)};
    auto loss_ports = update_lossflow_ports(
        new_time,
        lp.with_achieved(lossflow_achieved, new_time),
        wp.with_requested_and_achieved(0.0, 0.0, new_time));
    return ConverterState{
      new_time,
      std::move(new_ip),
      std::move(new_op),
      std::move(loss_ports.lossflow_port),
      std::move(loss_ports.wasteflow_port),
      f->clone(),
      new_ip.should_propagate_request_at(new_time),
      state.report_outflow_achieved,
      loss_ports.lossflow_port.should_propagate_achieved_at(new_time),
    };
  }

  ConverterState
  converter_external_transition_on_inflow_achieved(
      const ConverterState& state,
      RealTimeType new_time,
      FlowValueType inflow_achieved)
  {
    const auto& ip = state.inflow_port;
    const auto& op = state.outflow_port;
    const auto& lp = state.lossflow_port;
    const auto& wp = state.wasteflow_port;
    const auto& f = state.conversion_fun;
    auto outflow_achieved = f->outflow_given_inflow(inflow_achieved);
    auto lossflow_achieved = f->lossflow_given_inflow(inflow_achieved);
    auto new_op{op.with_achieved(outflow_achieved, new_time)};
    auto new_ip{ip.with_achieved(inflow_achieved, new_time)};
    auto loss_ports = update_lossflow_ports(
        new_time,
        lp.with_achieved(lossflow_achieved, new_time),
        wp.with_requested_and_achieved(0.0, 0.0, new_time));
    return ConverterState{
      new_time,
      std::move(new_ip),
      std::move(new_op),
      std::move(loss_ports.lossflow_port),
      std::move(loss_ports.wasteflow_port),
      f->clone(),
      state.report_inflow_request,
      new_op.should_propagate_achieved_at(new_time),
      loss_ports.lossflow_port.should_propagate_achieved_at(new_time),
    };
  }

  ConverterState
  converter_external_transition_on_lossflow_request(
      const ConverterState& state,
      RealTimeType new_time,
      FlowValueType lossflow_request)
  {
    auto loss_ports = update_lossflow_ports(
        new_time,
        state.lossflow_port.with_requested(lossflow_request, new_time),
        state.wasteflow_port);
    return ConverterState{
      new_time,
      state.inflow_port,
      state.outflow_port,
      std::move(loss_ports.lossflow_port),
      std::move(loss_ports.wasteflow_port),
      state.conversion_fun->clone(),
      state.report_inflow_request,
      state.report_outflow_achieved,
      loss_ports.lossflow_port.should_propagate_achieved_at(new_time),
    };
  }

  LossflowPorts
  update_lossflow_ports(
      RealTimeType time,
      const Port& lossflow_port,
      const Port& wasteflow_port)
  {
    const auto& requested = lossflow_port.get_requested();
    const auto& achieved = lossflow_port.get_achieved() + wasteflow_port.get_achieved(); 
    if (requested > achieved)
      return LossflowPorts{
        lossflow_port.with_requested_and_achieved(requested, achieved, time),
        wasteflow_port.with_requested_and_achieved(0.0, 0.0, time)};
    auto waste = achieved - requested;
    return LossflowPorts{
      lossflow_port.with_requested_and_achieved(requested, requested, time),
      wasteflow_port.with_requested_and_achieved(waste, waste, time)};
  }

  std::vector<PortValue>
  converter_output_function(const ConverterState& state)
  {
    std::vector<PortValue> ys{};
    converter_output_function_mutable(state, ys);
    return ys;
  }

  void 
  converter_output_function_mutable(
      const ConverterState& state,
      std::vector<PortValue>& ys)
  {
    if (state.report_inflow_request)
      ys.emplace_back(
          PortValue{
            outport_inflow_request,
            state.inflow_port.get_requested()});
    if (state.report_outflow_achieved)
      ys.emplace_back(
          PortValue{
            outport_outflow_achieved,
            state.outflow_port.get_achieved()});
    if (state.report_lossflow_achieved)
      ys.emplace_back(
          PortValue{
            outport_lossflow_achieved,
            state.lossflow_port.get_achieved()});
  }
}
