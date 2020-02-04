/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/graphviz.h"
#include "erin/port.h"
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace erin::graphviz
{
  namespace ep = erin::port;

  void
  ensure_port_not_already_added(const std::set<int>& ports, int port_number)
  {
    auto it = ports.find(port_number);
    if (it != ports.end()) {
      throw std::invalid_argument{
        "network contains multi-connected ports"};
    }
  }

  void
  record_port_number(
      const en::ComponentAndPort& c,
      std::map<std::string,PortCounts>& ports)
  {
    const std::string& id = c.component_id;
    auto it = ports.find(id);
    if (it == ports.end()) {
      if (c.port_type == ep::Type::Inflow) {
        ports.emplace(
            id,
            PortCounts{std::set<int>{c.port_number}, std::set<int>{}});
      } else if (c.port_type == ep::Type::Outflow) {
        ports.emplace(
            id,
            PortCounts{std::set<int>{}, std::set<int>{c.port_number}});
      } else {
        throw std::runtime_error("unhandled port type");
      }
    } else {
      auto& pc = it->second;
      if (c.port_type == ep::Type::Inflow) {
        ensure_port_not_already_added(pc.input_ports, c.port_number);
        pc.input_ports.emplace(c.port_number);
      } else if (c.port_type == ep::Type::Outflow) {
        ensure_port_not_already_added(pc.output_ports, c.port_number);
        pc.output_ports.emplace(c.port_number);
      } else {
        throw std::runtime_error("unhandled port type");
      }
    }
  }

  std::string
  build_label(const std::string& id, const PortCounts& pc)
  {
    std::ostringstream label;
    auto num_inports = pc.input_ports.size();
    if (num_inports > 0) {
      for (auto ip: pc.input_ports) {
        label << "<I" << ip << "> I(" << ip << ")|";
      }
    }
    label << "<name> " << id;
    auto num_outports = pc.output_ports.size();
    if (num_outports > 0) {
      for (auto op: pc.output_ports) {
        label << "|<O" << op << "> O(" << op << ")";
      }
    }
    return label.str();
  }

  std::string network_to_dot(
      const std::vector<en::Connection>& network,
      const std::string& graph_name)
  {
    std::ostringstream connections;
    std::ostringstream declarations;
    std::map<std::string, PortCounts> ports;
    std::string tab{"  "};
    declarations
      << "digraph " << graph_name << " {\n"
      << tab << "node [shape=record];\n";
    for (const auto& connection: network) {
      const auto& c1 = connection.first;
      const auto& c2 = connection.second;
      record_port_number(c1, ports);
      record_port_number(c2, ports);
      connections
        << tab
        << c1.component_id << ":" << "O" << c1.port_number << " -> "
        << c2.component_id << ":" << "I" << c2.port_number << ";\n";
    }
    for (const auto& item: ports) {
      const auto& id = item.first;
      const auto& pc = item.second;
      auto label = build_label(id, pc);
      declarations
        << tab << id
        << " [shape=record,label=\"" << label << "\"];\n";
    }
    return declarations.str() + connections.str() + "}";
  }
}
