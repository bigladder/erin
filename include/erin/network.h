/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_NETWORK_H
#define ERIN_NETWORK_H
#include "adevs.h"
#include "erin/component.h"
#include "erin/element.h"
#include "erin/port.h"
#include "erin/type.h"
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace erin::network
{
  struct ComponentAndPort
  {
    std::string component_id{};
    erin::port::Type port_type{erin::port::Type::Inflow};
    int port_number{0};
  };

  std::ostream& operator<<(std::ostream& os, const ComponentAndPort& cp);  

  bool operator==(const ComponentAndPort& a, const ComponentAndPort& b);

  struct Connection
  {
    ComponentAndPort first{};
    ComponentAndPort second{};
    // REFAC std::string stream{};
  };

  std::ostream& operator<<(std::ostream& os, const Connection& c);  

  bool operator==(const Connection& c1, const Connection& c2);

  void add_if_not_added(
      const std::string& comp_id,
      const std::string& scenario_id,
      const std::unordered_map<
        std::string,
        std::unique_ptr<ERIN::Component>>& components,
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      std::unordered_set<std::string>& comps_added,
      std::unordered_map<
        std::string,
        ERIN::PortsAndElements>& ports_and_elements,
      const std::unordered_map<std::string, std::vector<double>>&
        failure_probs_by_comp_id,
      const std::function<double()>& rand_fn);

  void couple_source_to_sink(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      ERIN::FlowElement* src,
      ERIN::FlowElement* sink,
      bool two_way = true);

  void couple_source_loss_to_sink(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      ERIN::FlowElement* src,
      ERIN::FlowElement* sink,
      bool two_way = true);

  void couple_source_loss_to_sink_with_ports(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      ERIN::FlowElement* src,
      int source_port,
      ERIN::FlowElement* sink,
      int sink_port,
      bool two_way = true);

  ERIN::ElementPort get_from_map(
      const std::unordered_map<
         erin::port::Type, std::vector<ERIN::ElementPort>>& map,
      const erin::port::Type& id,
      const std::string& map_name,
      const std::string& id_name,
      std::vector<ERIN::ElementPort>::size_type idx=0);

  void check_stream_consistency(
      const std::string& source,
      const std::string& sink);
    // REFAC const std::string& stream)

  void connect_source_to_sink_with_ports(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      ERIN::FlowElement* source,
      int source_port,
      ERIN::FlowElement* sink,
      int sink_port,
      bool both_way);
  // REFAC const std::string& stream);

  void connect(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      const std::unordered_map<
         erin::port::Type, std::vector<ERIN::ElementPort>>& port_map1,
      const ::erin::port::Type& port1,
      const int& port1_num,
      const std::unordered_map<
         erin::port::Type, std::vector<ERIN::ElementPort>>& port_map2,
      const ::erin::port::Type& port2,
      const int& port2_num,
      bool two_way = false);

  std::vector<ERIN::FlowElement*> build(
      const std::string& scenario_id,
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      const std::vector<Connection>& connections,
      const std::unordered_map<
        std::string, std::unique_ptr<ERIN::Component>>& components,
      const std::unordered_map<
        std::string, std::vector<double>>& failure_probs_by_comp_id,
      const std::function<double()>& rand_fn,
      bool two_way = false);
}

#endif // ERIN_NETWORK_H
