/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "debug_utils.h"
#include "erin/network.h"
#include "erin/port.h"
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <vector>
#include <stdexcept>

namespace erin::network
{
  std::ostream&
  operator<<(std::ostream& os, const ComponentAndPort& cp)
  {
    namespace ep = ::erin::port;
    os << "ComponentAndPort("
       << "component_id=\"" << cp.component_id << "\", "
       << "port_type=\"" << ep::type_to_tag(cp.port_type) << "\", "
       << "port_number=" << cp.port_number << ")";
    return os;
  }

  bool
  operator==(const ComponentAndPort& a, const ComponentAndPort& b)
  {
    return (a.component_id == b.component_id)
      && (a.port_type == b.port_type)
      && (a.port_number == b.port_number);
  }

  bool
  operator==(const Connection& c1, const Connection& c2)
  {
    return ((c1.first == c2.first) && (c1.second == c2.second))
      || ((c1.first == c2.second) && (c1.second == c2.first));
      
  }

  std::ostream&
  operator<<(std::ostream& os, const Connection& c)
  {
    os << "Connection("
       << "first=" << c.first << ", "
       << "second=" << c.second << ")";
    return os;
  }

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
      const std::unordered_map<
        ::erin::port::Type, std::vector<::ERIN::FlowElement*>>& map,
      const ::erin::port::Type& id,
      const std::string& map_name,
      const std::string& id_name,
      std::vector<::ERIN::FlowElement*>::size_type idx)
  {
    if (idx < 0) {
      throw std::invalid_argument("index must be >= 0");
    }
    auto it = map.find(id);
    if (it == map.end()) {
      std::ostringstream oss;
      oss << id_name << " \"" << ::erin::port::type_to_tag(id)
          << "\" not found in " << map_name;
      throw std::runtime_error(oss.str());
    }
    const auto& xs = (*it).second;
    auto xs_size = xs.size();
    if (idx >= xs_size) {
      std::ostringstream oss;
      oss << "index greater than size of vector for element \""
          << ::erin::port::type_to_tag(id) << "\"\n";
      oss << "vector.size() = " << xs_size << "\n";
      oss << "map_name = \"" << map_name << "\"\n";
      oss << "id_name = \"" << id_name << "\"\n";
      oss << "idx = " << idx << "\n";
      throw std::invalid_argument(oss.str());
    }
    return xs[idx];
  }

  void
  connect(
      adevs::Digraph<ERIN::FlowValueType, ERIN::Time>& network,
      const std::unordered_map<
        ::erin::port::Type, std::vector<::ERIN::FlowElement*>>& port_map1,
      const ::erin::port::Type& port1,
      const int& port1_num,
      const std::unordered_map<
        ::erin::port::Type, std::vector<::ERIN::FlowElement*>>& port_map2,
      const ::erin::port::Type& port2,
      const int& port2_num)
  {
    namespace ep = ::erin::port;
    if ((port1 == ep::Type::Outflow) && (port2 == ep::Type::Inflow)) {
      auto source = get_from_map(port_map1, port1, "port_map1", "port1", port1_num);
      auto sink = get_from_map(port_map2, port2, "port_map2", "port2", port2_num);
      couple_source_to_sink(network, source, sink);
    }
    else if ((port1 == ep::Type::Inflow) && (port2 == ep::Type::Outflow)) {
      auto source = get_from_map(port_map2, port2, "port_map2", "port2", port2_num);
      auto sink = get_from_map(port_map1, port1, "port_map1", "port1", port1_num);
      couple_source_to_sink(network, source, sink);
    }
    else {
      std::ostringstream oss;
      oss << "unhandled port combination:\n";
      oss << "port1 = " << ::erin::port::type_to_tag(port1) << "\n";
      oss << "port2 = " << ::erin::port::type_to_tag(port2) << "\n";
      oss << "available port1:";
      for (const auto pair: port_map1) {
        oss << ", " << ::erin::port::type_to_tag(pair.first);
      }
      oss << "\n";
      oss << "available port2:";
      for (const auto pair: port_map2) {
        oss << ", " << ::erin::port::type_to_tag(pair.first);
      }
      oss << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  std::vector<::ERIN::FlowElement*>
  build(
      const std::string& scenario_id,
      adevs::Digraph<ERIN::FlowValueType, ::ERIN::Time>& network,
      const std::vector<Connection>& connections,
      const std::unordered_map<
        std::string, std::unique_ptr<ERIN::Component>>& components,
      const std::unordered_map<
        std::string, std::vector<double>>& failure_probs_by_comp_id)
  {
    if constexpr (::ERIN::debug_level >= ::ERIN::debug_level_high) {
      namespace E = ::ERIN;
      std::cout << "entering build(...)\n";
      std::cout << "... failure_probs_by_comp_id = "
                << E::map_of_vec_to_string<double>(failure_probs_by_comp_id)
                << "\n";
    }
    std::unordered_set<::ERIN::FlowElement*> elements;
    std::unordered_set<std::string> comps_added;
    std::unordered_map<std::string, ::ERIN::PortsAndElements> pes;
    for (const auto connection: connections) {
      if constexpr (::ERIN::debug_level >= ::ERIN::debug_level_high) {
        std::cout << "... processing connection: " << connection << "\n";
      }
      const auto& comp1_id = connection.first.component_id;
      const auto& port1_type = connection.first.port_type;
      const auto& port1_num = connection.first.port_number;
      const auto& comp2_id = connection.second.component_id;
      const auto& port2_type = connection.second.port_type;
      const auto& port2_num = connection.second.port_number;
      if constexpr (::ERIN::debug_level >= ::ERIN::debug_level_high) {
        std::cout << "... connection: " << connection << "\n";
      }
      add_if_not_added(
          comp1_id, scenario_id, components, network, comps_added, pes,
          failure_probs_by_comp_id);
      add_if_not_added(
          comp2_id, scenario_id, components, network, comps_added, pes,
          failure_probs_by_comp_id);
      connect(
          network,
          pes.at(comp1_id).port_map,
          port1_type,
          port1_num,
          pes.at(comp2_id).port_map,
          port2_type,
          port2_num);
    }
    for (const auto& pair: pes) {
      auto& es = pair.second.elements_added;
      for (auto e: es) {
        elements.emplace(e);
      }
    }
    return std::vector<::ERIN::FlowElement*>(elements.begin(), elements.end());
  }
}
