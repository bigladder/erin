/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_NETWORK_H
#define ERIN_NETWORK_H
#include "adevs.h"
#include "erin/component.h"
#include "erin/element.h"
#include "erin/port.h"
#include "erin/type.h"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace erin::network
{
  struct ComponentAndPort
  {
    std::string component_id;
    ::erin::port::Type port_type;
    int port_number;
  };

  std::ostream& operator<<(std::ostream& os, const ComponentAndPort& cp);  

  bool operator==(const ComponentAndPort& a, const ComponentAndPort& b);

  struct Connection
  {
    ComponentAndPort first;
    ComponentAndPort second;
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
        ERIN::PortsAndElements>& ports_and_elements);

  void couple_source_to_sink(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      ERIN::FlowElement* src,
      ERIN::FlowElement* sink,
      bool two_way = true);

  ::ERIN::FlowElement* get_from_map(
      const std::unordered_map<
        ::erin::port::Type, std::vector<::ERIN::FlowElement*>>& map,
      const ::erin::port::Type& id,
      const std::string& map_name,
      const std::string& id_name,
      std::vector<::ERIN::FlowElement*>::size_type idx=0);

  void connect(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      const std::unordered_map<
        ::erin::port::Type, std::vector<ERIN::FlowElement*>>& port_map1,
      const ::erin::port::Type& port1,
      const int& port1_num,
      const std::unordered_map<
        ::erin::port::Type, std::vector<ERIN::FlowElement*>>& port_map2,
      const ::erin::port::Type& port2,
      const int& port2_num);

  std::vector<ERIN::FlowElement*> build(
      const std::string& scenario_id,
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      const std::vector<Connection>& connections,
      const std::unordered_map<
        std::string, std::unique_ptr<ERIN::Component>>& components,
      const std::unordered_map<
        std::string, std::vector<double>>& failure_probs_by_comp_id);
}

#endif // ERIN_NETWORK_H
