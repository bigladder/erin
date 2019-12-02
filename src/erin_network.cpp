/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/network.h"
#include "erin/port.h"
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <vector>
#include <stdexcept>

namespace erin::network
{
  void
  add_if_not_added(
      const std::string& comp_id,
      const std::string& scenario_id,
      const std::unordered_map<
        std::string,
        std::unique_ptr<ERIN::Component>>& components,
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      std::unordered_set<std::string>& comps_added,
      std::unordered_map<
        std::string,
        ::ERIN::PortsAndElements>& ports_and_elements,
      const std::unordered_map<std::string, std::vector<double>>&
        failure_probs_by_comp_id)
  {
    auto comp_it = comps_added.find(comp_id);
    if (comp_it == comps_added.end()) {
      auto it = components.find(comp_id);
      if (it == components.end()) {
        std::ostringstream oss;
        oss << "component_id \"" << comp_id
            << "\" not found in components map";
        throw std::runtime_error(oss.str());
      }
      const auto& c = it->second;
      bool is_failed{false};
      auto probs_it = failure_probs_by_comp_id.find(comp_id);
      if (probs_it != failure_probs_by_comp_id.end()) {
        for (const auto p: probs_it->second) {
          if (p >= 1.0) {
            is_failed = true;
            break;
          }
        }
      }
      auto pe = c->add_to_network(network, scenario_id, is_failed);
      ports_and_elements[comp_id] = pe;
      comps_added.emplace(comp_id);
    }
  }

  void
  couple_source_to_sink(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      ::ERIN::FlowElement* src,
      ::ERIN::FlowElement* sink,
      bool two_way)
  {
    network.couple(
        sink, ::ERIN::FlowElement::outport_inflow_request,
        src, ::ERIN::FlowElement::inport_outflow_request);
    if (two_way) {
      network.couple(
          src, ::ERIN::FlowElement::outport_outflow_achieved,
          sink, ::ERIN::FlowElement::inport_inflow_achieved);
    }
  }

  ::ERIN::FlowElement*
  get_from_map(
      const std::unordered_map<std::string, ::ERIN::FlowElement*>& map,
      const std::string& id,
      const std::string& map_name,
      const std::string& id_name)
  {
    auto it = map.find(id);
    if (it == map.end()) {
      std::ostringstream oss;
      oss << id_name << " \"" << id << "\" not found in " << map_name;
      throw std::runtime_error(oss.str());
    }
    return (*it).second;
  }

  void
  connect(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      const std::unordered_map<std::string, ::ERIN::FlowElement*>& port_map1,
      const std::string& port1,
      const std::unordered_map<std::string, ::ERIN::FlowElement*>& port_map2,
      const std::string& port2)
  {
    namespace ep = ::erin::port;
    if ((port1 == ep::outflow) && (port2 == ep::inflow)) {
      auto source = get_from_map(port_map1, port1, "port_map1", "port1");
      auto sink = get_from_map(port_map2, port2, "port_map2", "port2");
      couple_source_to_sink(network, source, sink);
    }
    else if ((port1 == ep::inflow) && (port2 == ep::outflow)) {
      auto source = get_from_map(port_map2, port2, "port_map2", "port2");
      auto sink = get_from_map(port_map1, port1, "port_map1", "port1");
      couple_source_to_sink(network, source, sink);
    }
    else {
      std::ostringstream oss;
      oss << "unhandled port combination:\n";
      oss << "port1 = " << port1 << "\n";
      oss << "port2 = " << port2 << "\n";
      oss << "available port1:";
      for (const auto pair: port_map1) {
        oss << ", " << pair.first;
      }
      oss << "\n";
      oss << "available port2:";
      for (const auto pair: port_map2) {
        oss << ", " << pair.first;
      }
      oss << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  std::unordered_set<::ERIN::FlowElement*>
  build(
      const std::string& scenario_id,
      adevs::Digraph<ERIN::FlowValueType, ::ERIN::Time>& network,
      const std::vector<Connection>& connections,
      const std::unordered_map<
        std::string, std::unique_ptr<ERIN::Component>>& components,
      const std::unordered_map<
        std::string, std::vector<double>>& failure_probs_by_comp_id)
  {
    std::unordered_set<::ERIN::FlowElement*> elements;
    std::unordered_set<std::string> comps_added;
    std::unordered_map<std::string, ::ERIN::PortsAndElements> pes;
    for (const auto connection: connections) {
      auto comp1_id = connection.first.component_id;
      auto port1_id = connection.first.port_id;
      auto comp2_id = connection.second.component_id;
      auto port2_id = connection.second.port_id;
      add_if_not_added(
          comp1_id, scenario_id, components, network, comps_added, pes,
          failure_probs_by_comp_id);
      add_if_not_added(
          comp2_id, scenario_id, components, network, comps_added, pes,
          failure_probs_by_comp_id);
      connect(
          network,
          pes.at(comp1_id).port_map,
          port1_id,
          pes.at(comp2_id).port_map,
          port2_id);
    }
    for (const auto& pair: pes) {
      auto& es = pair.second.elements_added;
      for (auto e: es) {
        elements.emplace(e);
      }
    }
    return elements;
  }
}
