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
      RealTimeType time,
      const std::vector<Port>& inflow_ports,
      const std::vector<Port>& outflow_ports);

  std::vector<Port> distribute_inflow_to_outflow_in_order(
      const std::vector<Port>& outflows,
      FlowValueType amount,
      RealTimeType time);

  std::vector<Port> distribute_inflow_to_outflow_evenly(
      const std::vector<Port>& outflows,
      FlowValueType amount,
      RealTimeType time);

  std::vector<Port> distribute_inflow_to_outflow(
      MuxerDispatchStrategy outflow_strategy,
      const std::vector<Port>& outflows,
      FlowValueType amount,
      RealTimeType time);

  std::vector<Port> request_difference_from_next_highest_inflow_port(
      const std::vector<Port>& inflow_ports,
      int idx_of_request,
      FlowValueType request,
      RealTimeType time);

  std::vector<Port> rerequest_inflows_in_order(
      const std::vector<Port>& inflow_ports,
      FlowValueType total_outflow_request,
      RealTimeType time);

  ////////////////////////////////////////////////////////////
  // state
  const int minimum_number_of_ports{1};
  const int maximum_number_of_ports{max_port_numbers};

  struct MuxState
  {
    RealTimeType time{0};
    int num_inflows{0};
    int num_outflows{0};
    std::vector<Port> inflow_ports{};
    std::vector<Port> outflow_ports{};
    bool do_report{false};
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
