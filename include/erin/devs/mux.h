/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_DEVS_MUX_H
#define ERIN_DEVS_MUX_H
#include "erin/devs.h"
#include "erin/type.h"
#include <string>
#include <vector>

namespace erin::devs
{
  ////////////////////////////////////////////////////////////
  // helper classes and functions
  enum class MuxerDispatchStrategy
  {
    InOrder = 0,
    Distribute
  };

  MuxerDispatchStrategy tag_to_muxer_dispatch_strategy(const std::string& tag);

  std::string muxer_dispatch_strategy_to_string(MuxerDispatchStrategy mds);

  void mux_check_num_flows(const std::string& tag, int n);

  bool mux_should_report(
      const std::vector<bool>& report_irs,
      const std::vector<bool>& report_oas);

  std::vector<PortUpdate> distribute_inflow_to_outflow_in_order(
      const std::vector<Port2>& outflows,
      FlowValueType amount);

  std::vector<PortUpdate> distribute_inflow_to_outflow_evenly(
      const std::vector<Port2>& outflows,
      FlowValueType amount);

  std::vector<PortUpdate> distribute_inflow_to_outflow(
      MuxerDispatchStrategy outflow_strategy,
      const std::vector<Port2>& outflows,
      FlowValueType amount);

  std::vector<PortUpdate> request_inflows_intelligently(
      const std::vector<Port2>& inflow_ports,
      FlowValueType remaining_request);

  ////////////////////////////////////////////////////////////
  // state
  const int minimum_number_of_ports{1};
  const int maximum_number_of_ports{max_port_numbers};

  struct MuxState
  {
    RealTimeType time{0};
    int num_inflows{0};
    int num_outflows{0};
    std::vector<Port2> inflow_ports{};
    std::vector<Port2> outflow_ports{};
    std::vector<bool> report_irs{};
    std::vector<bool> report_oas{};
    MuxerDispatchStrategy outflow_strategy{MuxerDispatchStrategy::Distribute};
  };

  MuxState make_mux_state(
      int num_inflows,
      int num_outflows,
      MuxerDispatchStrategy = MuxerDispatchStrategy::Distribute);

  RealTimeType mux_current_time(const MuxState& state);

  FlowValueType mux_get_inflow_request(const MuxState& state);

  FlowValueType mux_get_outflow_request(const MuxState& state);

  FlowValueType mux_get_inflow_achieved(const MuxState& state);

  FlowValueType mux_get_outflow_achieved(const MuxState& state);
  
  std::ostream& operator<<(std::ostream& os, const MuxState& s);

  ////////////////////////////////////////////////////////////
  // time advance
  RealTimeType mux_time_advance(const MuxState& state);

  ////////////////////////////////////////////////////////////
  // internal transition
  MuxState mux_internal_transition(const MuxState& state);

  ////////////////////////////////////////////////////////////
  // external transition
  MuxState mux_external_transition(
      const MuxState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // confluent transition
  MuxState mux_confluent_transition(
      const MuxState& state,
      const std::vector<PortValue>& xs);

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue> mux_output_function(const MuxState& state);

  void mux_output_function_mutable(
      const MuxState& state,
      std::vector<PortValue>& ys);
}

#endif // ERIN_DEVS_MUX_H
