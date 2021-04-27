/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_DEVS_CONVERTER_H
#define ERIN_DEVS_CONVERTER_H
#include "erin/devs.h"
#include "erin/type.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions
  class ConversionFun
  {
    public:
      ConversionFun() = default;
      virtual ~ConversionFun() = default;
      ConversionFun(const ConversionFun&) = delete;
      ConversionFun& operator=(const ConversionFun&) = delete;
      ConversionFun(ConversionFun&&) = delete;
      ConversionFun& operator=(ConversionFun&&) = delete;

      virtual std::unique_ptr<ConversionFun> clone() const = 0;

      virtual FlowValueType outflow_given_inflow(FlowValueType inflow) const = 0;
      virtual FlowValueType inflow_given_outflow(FlowValueType outflow) const = 0;
      FlowValueType lossflow_given_inflow(FlowValueType inflow) const
      {
        return (inflow - outflow_given_inflow(inflow));
      }
      FlowValueType lossflow_given_outflow(FlowValueType outflow) const
      {
        return (inflow_given_outflow(outflow) - outflow);
      }
  };

  bool operator==(const ConversionFun& a, const ConversionFun& b);
  bool operator!=(const ConversionFun& a, const ConversionFun& b);
  std::ostream& operator<<(std::ostream& os, const ConversionFun& a);
  
  constexpr FlowValueType constant_efficiency_fun_precision_exp{6};
  const FlowValueType constant_efficiency_fun_precision_factor{
    std::pow(10.0, constant_efficiency_fun_precision_exp)};

  class ConstantEfficiencyFun : public ConversionFun
  {
    public:
      ConstantEfficiencyFun();
      explicit ConstantEfficiencyFun(FlowValueType constant_efficiency);
      ~ConstantEfficiencyFun() = default;
      ConstantEfficiencyFun(const ConstantEfficiencyFun&) = delete;
      ConstantEfficiencyFun& operator=(const ConstantEfficiencyFun&) = delete;
      ConstantEfficiencyFun(ConstantEfficiencyFun&&) = delete;
      ConstantEfficiencyFun& operator=(ConstantEfficiencyFun&&) = delete;

      std::unique_ptr<ConversionFun> clone() const override;
      FlowValueType outflow_given_inflow(FlowValueType inflow) const override;
      FlowValueType inflow_given_outflow(FlowValueType outflow) const override;

      [[nodiscard]] FlowValueType get_constant_efficiency() const {
        return constant_efficiency;
      }

      friend bool operator==(const ConstantEfficiencyFun& a, const ConstantEfficiencyFun& b);

      friend std::ostream& operator<<(std::ostream& os, const ConstantEfficiencyFun& a);

    private:
      FlowValueType constant_efficiency;
  };

  bool operator==(const ConstantEfficiencyFun& a, const ConstantEfficiencyFun& b);
  bool operator!=(const ConstantEfficiencyFun& a, const ConstantEfficiencyFun& b);
  std::ostream& operator<<(std::ostream& os, const ConstantEfficiencyFun& a);

  class FunctionBasedEfficiencyFun : public ConversionFun
  {
    public:
      FunctionBasedEfficiencyFun();
      FunctionBasedEfficiencyFun(
          std::function<FlowValueType(FlowValueType)> calc_output_from_input,
          std::function<FlowValueType(FlowValueType)> calc_input_from_output);
      ~FunctionBasedEfficiencyFun() = default;
      FunctionBasedEfficiencyFun(const FunctionBasedEfficiencyFun&) = delete;
      FunctionBasedEfficiencyFun& operator=(const FunctionBasedEfficiencyFun&) = delete;
      FunctionBasedEfficiencyFun(FunctionBasedEfficiencyFun&&) = delete;
      FunctionBasedEfficiencyFun& operator=(FunctionBasedEfficiencyFun&&) = delete;

      std::unique_ptr<ConversionFun> clone() const override;
      FlowValueType outflow_given_inflow(FlowValueType inflow) const override;
      FlowValueType inflow_given_outflow(FlowValueType outflow) const override;

      friend bool operator==(const FunctionBasedEfficiencyFun& a, const FunctionBasedEfficiencyFun& b);

      friend std::ostream& operator<<(std::ostream& os, const FunctionBasedEfficiencyFun& a);

    private:
      std::function<FlowValueType(FlowValueType)> calc_output_from_input;
      std::function<FlowValueType(FlowValueType)> calc_input_from_output;
  };

  bool operator==(const FunctionBasedEfficiencyFun& a, const FunctionBasedEfficiencyFun& b);
  bool operator!=(const FunctionBasedEfficiencyFun& a, const FunctionBasedEfficiencyFun& b);
  std::ostream& operator<<(std::ostream& os, const FunctionBasedEfficiencyFun& a);

  ////////////////////////////////////////////////////////////
  // state
  struct ConverterState
  {
    RealTimeType time{0};
    Port3 inflow_port{};
    Port3 outflow_port{};
    Port3 lossflow_port{};
    Port3 wasteflow_port{};
    std::unique_ptr<ConversionFun> conversion_fun{};
    bool report_inflow_request{false};
    bool report_outflow_achieved{false};
    bool report_lossflow_achieved{false};
  };

  bool operator==(const ConverterState& a, const ConverterState& b);
  bool operator!=(const ConverterState& a, const ConverterState& b);
  std::ostream& operator<<(std::ostream& os, const ConverterState& a);

  ConverterState
  make_converter_state(FlowValueType constant_efficiency);

  ConverterState
  make_converter_state(
      const std::function<FlowValueType(FlowValueType)>& calc_output_from_input,
      const std::function<FlowValueType(FlowValueType)>& calc_input_from_output);

  struct LossflowPorts
  {
    bool report_lossflow_achieved{false};
    Port3 lossflow_port{};
    Port3 wasteflow_port{};
  };

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType
  converter_time_advance(const ConverterState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  ConverterState
  converter_internal_transition(
    const ConverterState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  ConverterState
  converter_external_transition(
      const ConverterState& state,
      RealTimeType elapsed_time,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // confluent transition
  ConverterState
  converter_confluent_transition(
      const ConverterState& state,
      const std::vector<PortValue>& xs); 

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  converter_output_function(const ConverterState& state);

  void 
  converter_output_function_mutable(
      const ConverterState& state,
      std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_CONVERTER_H
