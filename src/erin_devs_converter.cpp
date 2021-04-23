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
              << ":inflow " << a.inflow_port << " "
              << ":outflow " << a.outflow_port << " "
              << ":lossflow " << a.lossflow_port << " "
              << ":wasteflow " << a.wasteflow_port << " "
              << ":report-ir? " << a.report_inflow_request << " "
              << ":report-oa? " << a.report_outflow_achieved << " "
              << ":report-la? " << a.report_lossflow_achieved << "}";
  }

  ConverterState
  make_converter_state(FlowValueType constant_efficiency)
  {
    std::unique_ptr<ConversionFun> p =
      std::make_unique<ConstantEfficiencyFun>(constant_efficiency);
    return ConverterState{
      0,            // time
      Port3{},      // inflow_port
      Port3{},      // outflow_port
      Port3{},      // lossflow_port
      Port3{},      // wasteflow_port
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
      Port3{},      // inflow_port
      Port3{},      // outflow_port
      Port3{},      // lossflow_port
      Port3{},      // wasteflow_port
      std::move(p), // conversion_fun
      false,        // report_inflow_request
      false,        // report_outflow_achieved
      false};       // report_lossflow_achieved
  }

  RealTimeType
  converter_time_advance(const ConverterState& state)
  {
    RealTimeType dt{infinity};
    if (state.report_inflow_request || state.report_outflow_achieved || state.report_lossflow_achieved) {
      dt = 0;
    }
    return dt;
  }

  ConverterState
  converter_internal_transition(const ConverterState& state)
  {
    auto next_state = ConverterState{
      state.time, // internal transitions always take 0 time
      state.inflow_port,
      state.outflow_port,
      state.lossflow_port,
      state.wasteflow_port,
      state.conversion_fun->clone(),
      false,
      false,
      false,
    };
    return next_state;
  }

  ConverterState
  converter_external_transition(
      const ConverterState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs)
  {
    bool got_or{false};
    bool got_ia{false};
    bool got_lr{false};
    FlowValueType outflow_request{0.0};
    FlowValueType inflow_achieved{0.0};
    FlowValueType lossflow_request{0.0};
    constexpr int inport_lossflow_request{inport_outflow_request + 1};
    for (const auto& x: xs) {
      switch (x.port) {
        case inport_outflow_request:
          {
            got_or = true;
            outflow_request += x.value;
            break;
          }
        case inport_inflow_achieved:
          {
            got_ia = true;
            inflow_achieved += x.value;
            break;
          }
        case inport_lossflow_request:
          {
            got_lr = true;
            lossflow_request += x.value;
            break;
          }
        default:
          {
            std::ostringstream oss{};
            oss << "unhandled port " << x.port
                << " in converter_external_transition(...)";
            throw std::invalid_argument(oss.str());
          }
      }
    }
    auto new_time = state.time + elapsed_time;
    auto ip{state.inflow_port};
    auto op{state.outflow_port};
    auto lp{state.lossflow_port};
    auto wp{state.wasteflow_port};
    bool report_ir{false};
    bool report_oa{false};
    bool report_la{false};
    if (got_lr) {
      auto lp_update = lp.with_requested(lossflow_request);
      lp = lp_update.port;
      report_la = report_la || lp_update.send_achieved;
    }
    if (got_or) {
      auto op_update = op.with_requested(outflow_request);
      op = op_update.port;
      report_oa = report_oa || op_update.send_achieved;
      auto ip_update = ip.with_requested(
          state.conversion_fun->inflow_given_outflow(outflow_request));
      ip = ip_update.port;
      report_ir = report_ir || ip_update.send_request;
    }
    if (got_ia) {
      auto ip_update = ip.with_achieved(inflow_achieved);
      ip = ip_update.port;
      report_ir = report_ir || ip_update.send_request;
      auto op_update = op.with_achieved(
          state.conversion_fun->outflow_given_inflow(ip.get_achieved()));
      op = op_update.port;
      report_oa = report_oa || op_update.send_achieved;
    }
    auto lossflow_achieved{ip.get_achieved() - op.get_achieved()};
    auto lp_update = lp.with_achieved(
        (lossflow_achieved > lp.get_requested())
        ? lp.get_requested()
        : lossflow_achieved);
    lp = lp_update.port;
    auto wasteflow_request{
      ip.get_requested() - (op.get_requested() + lp.get_requested())};
    auto wasteflow_achieved{
      ip.get_achieved() - (op.get_achieved() + lp.get_achieved())};
    wp = Port3{wasteflow_request, wasteflow_achieved};
    report_la = report_la || lp_update.send_achieved;
    return ConverterState{
      new_time,
      ip,
      op,
      lp,
      wp,
      state.conversion_fun->clone(),
      report_ir || ip.should_send_request(state.inflow_port),
      report_oa || op.should_send_achieved(state.outflow_port),
      report_la || lp.should_send_achieved(state.lossflow_port)}; 
  }

  ConverterState
  converter_confluent_transition(
      const ConverterState& state,
      const std::vector<PortValue>& xs)
  {
    auto next_state = converter_external_transition(
        converter_internal_transition(state),
        0,
        xs);
    return next_state;
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
