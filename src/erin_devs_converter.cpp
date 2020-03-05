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
              << "conversion_fun=" << a.conversion_fun << ", "
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
      0, Port{}, Port{}, Port{},
      std::move(p), false, false, false};
  }
}
