/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

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
    if constexpr (ERIN::debug_level >= ERIN::debug_level_high)
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
    if (typeid(FunctionBasedEfficiencyFun) == a_idx)
    {
      const auto& aa = dynamic_cast<const FunctionBasedEfficiencyFun&>(a);
      const auto& bb = dynamic_cast<const FunctionBasedEfficiencyFun&>(b);
      return aa == bb;
    }
    std::ostringstream oss{};
    oss << "I'm sorry, this is an unhandled type " << a_idx.name() << "\n";
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
    if (typeid(FunctionBasedEfficiencyFun) == idx) {
      const auto& aa = dynamic_cast<const FunctionBasedEfficiencyFun&>(a);
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
    return std::round(
      inflow * constant_efficiency * constant_efficiency_fun_precision_factor)
      / constant_efficiency_fun_precision_factor;
  }

  FlowValueType
  ConstantEfficiencyFun::inflow_given_outflow(FlowValueType outflow) const
  {
    return std::round(
      (outflow / constant_efficiency) * constant_efficiency_fun_precision_factor)
      / constant_efficiency_fun_precision_factor;
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

  FunctionBasedEfficiencyFun::FunctionBasedEfficiencyFun():
    FunctionBasedEfficiencyFun(
        [](FlowValueType a) -> FlowValueType { return a; },
        [](FlowValueType a) -> FlowValueType { return a; })
  {
  }

  FunctionBasedEfficiencyFun::FunctionBasedEfficiencyFun(
      std::function<FlowValueType(FlowValueType)> calc_output_from_input_,
      std::function<FlowValueType(FlowValueType)> calc_input_from_output_):
    ConversionFun(),
    calc_output_from_input{std::move(calc_output_from_input_)},
    calc_input_from_output{std::move(calc_input_from_output_)}
  {
  }

  std::unique_ptr<ConversionFun>
  FunctionBasedEfficiencyFun::clone() const
  {
    std::unique_ptr<ConversionFun> p = std::make_unique<FunctionBasedEfficiencyFun>(
        calc_output_from_input, calc_input_from_output);
    return p;
  }

  FlowValueType
  FunctionBasedEfficiencyFun::outflow_given_inflow(FlowValueType inflow) const
  {
    return calc_output_from_input(inflow);
  }

  FlowValueType
  FunctionBasedEfficiencyFun::inflow_given_outflow(FlowValueType outflow) const
  {
    return calc_input_from_output(outflow);
  }

  bool
  operator==(const FunctionBasedEfficiencyFun& a, const FunctionBasedEfficiencyFun& b)
  {
    return (&a.calc_output_from_input == &b.calc_output_from_input)
      && (&a.calc_input_from_output == &b.calc_input_from_output);
  }

  bool
  operator!=(const FunctionBasedEfficiencyFun& a, const FunctionBasedEfficiencyFun& b)
  {
    return !(a == b);
  }

  std::ostream&
  operator<<(std::ostream& os, const FunctionBasedEfficiencyFun& a)
  {
    return os << "FunctionBasedEfficiencyFun("
              << "calc_output_from_input=" << (&a.calc_output_from_input) << ", "
              << "calc_input_from_output=" << (&a.calc_input_from_output) << ")";
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
    return os << "{"
              << ":t " << a.time << ", "
              << ":inflow " << a.inflow_port << ", "
              << ":outflow " << a.outflow_port << ", "
              << ":lossflow " << a.lossflow_port << ", "
              << ":wasteflow " << a.wasteflow_port << ", "
              << ":report-ir? " << a.report_inflow_request << ", "
              << "report-oa? " << a.report_outflow_achieved << ", "
              << "report-la? " << a.report_lossflow_achieved << "}";
  }

  ConverterState
  make_converter_state(FlowValueType constant_efficiency)
  {
    std::unique_ptr<ConversionFun> p =
      std::make_unique<ConstantEfficiencyFun>(constant_efficiency);
    return ConverterState{
      0,            // time
      Port2{},      // inflow_port
      Port2{},      // outflow_port
      Port2{},      // lossflow_port
      Port2{},      // wasteflow_port
      std::move(p), // conversion_fun
      false,        // report_inflow_request
      false,        // report_outflow_achieved
      false};       // report_lossflow_achieved
  }

  ConverterState
  make_converter_state(
      const std::function<FlowValueType(FlowValueType)>& calc_output_from_input,
      const std::function<FlowValueType(FlowValueType)>& calc_input_from_output)
  {
    std::unique_ptr<ConversionFun> p =
      std::make_unique<FunctionBasedEfficiencyFun>(
          calc_output_from_input, calc_input_from_output);
    return ConverterState{
      0,            // time
      Port2{},      // inflow_port
      Port2{},      // outflow_port
      Port2{},      // lossflow_port
      Port2{},      // wasteflow_port
      std::move(p), // conversion_fun
      false,        // report_inflow_request
      false,        // report_outflow_achieved
      false};       // report_lossflow_achieved
  }

  RealTimeType
  converter_time_advance(const ConverterState& state)
  {
    if (state.report_inflow_request || state.report_outflow_achieved || state.report_lossflow_achieved) {
      return 0;
    }
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
    bool got_inflow_achieved_flag{false};
    bool got_lossflow_request{false};
    FlowValueType outflow_request{0.0};
    FlowValueType inflow_achieved{0.0};
    FlowValueType lossflow_request{0.0};
    constexpr int inport_lossflow_request{inport_outflow_request + 1};
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
            got_inflow_achieved_flag = true;
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
    auto new_time = state.time + elapsed_time;
    bool report_ir{false};
    auto ip{state.inflow_port};
    auto op{state.outflow_port};
    auto lp{state.lossflow_port};
    auto ip_for_comp{state.inflow_port};
    auto op_for_comp{state.outflow_port};
    auto lp_for_comp{state.lossflow_port};
    auto wp{state.wasteflow_port};
    if (got_outflow_request) {
      op_for_comp = state.outflow_port.with_requested(outflow_request).port;
      op = op.with_requested(outflow_request).port;
      auto inflow_request{
        state.conversion_fun->inflow_given_outflow(outflow_request)};
      ip = ip.with_requested(inflow_request).port;
    }
    if (got_lossflow_request) {
      lp_for_comp = state.lossflow_port.with_requested(lossflow_request).port;
      lp = lp.with_requested(lossflow_request).port;
    }
    if (got_inflow_achieved_flag) {
      report_ir = inflow_achieved > ip.get_requested();
      ip = ip.with_achieved(
          report_ir
          ? ip.get_requested()
          : inflow_achieved).port;
      ip_for_comp = state.inflow_port.with_achieved(
          (inflow_achieved > state.inflow_port.get_requested())
          ? state.inflow_port.get_requested()
          : inflow_achieved).port;
      auto outflow_achieved{
        state.conversion_fun->outflow_given_inflow(
            ip.get_achieved())};
      if (outflow_achieved > op.get_requested()) {
        op = op.with_achieved(op.get_requested()).port;
        ip = ip.with_requested(
            state.conversion_fun->inflow_given_outflow(
              op.get_requested())).port;
      }
      else {
        op = op.with_achieved(outflow_achieved).port;
      }
    }
    auto lossflow_achieved{ip.get_achieved() - op.get_achieved()};
    lp = lp.with_achieved(
        (lossflow_achieved > lp.get_requested())
        ? lp.get_requested()
        : lossflow_achieved).port;
    wp = Port2{lossflow_achieved - lp.get_achieved()};
    return ConverterState{
      new_time,
      ip,
      op,
      lp,
      wp,
      state.conversion_fun->clone(),
      report_ir || ip.should_send_request(ip_for_comp),
      op.should_send_achieved(op_for_comp),
      lp.should_send_achieved(lp_for_comp)};
  }

  ConverterState
  converter_confluent_transition(
      const ConverterState& state,
      const std::vector<PortValue>& xs)
  {
    return converter_external_transition(
        converter_internal_transition(state), 0, xs);
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
    if (state.report_inflow_request) {
      ys.emplace_back(
          PortValue{
            outport_inflow_request,
            state.inflow_port.get_requested()});
    }
    if (state.report_outflow_achieved) {
      ys.emplace_back(
          PortValue{
            outport_outflow_achieved,
            state.outflow_port.get_achieved()});
    }
    if (state.report_lossflow_achieved) {
      ys.emplace_back(
          PortValue{
            outport_outflow_achieved + 1,
            state.lossflow_port.get_achieved()});
    }
  }
}
