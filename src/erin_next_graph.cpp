/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_graph.h"
#include "erin_next/erin_next.h"
#include <stdint.h>
#include <string>

namespace erin
{

    struct PortCounts
    {
        std::set<size_t> input_ports;
        std::set<size_t> output_ports;
    };

    enum class PortType
    {
        Inflow,
        Outflow,
    };

    struct ComponentAndPort
    {
        std::string component_id{};
        PortType port_type{PortType::Inflow};
        size_t port_number{0};
    };

    void
    ensure_port_not_already_added(
        std::set<size_t> const& ports,
        size_t port_number
    )
    {
        auto it = ports.find(port_number);
        if (it != ports.end())
        {
            throw std::invalid_argument{"network contains multi-connected ports"
            };
        }
    }

    void
    record_port_number(
        ComponentAndPort const& c,
        std::map<std::string, PortCounts>& ports
    )
    {
        std::string const& id = c.component_id;
        if (!ports.contains(id))
        {
            if (c.port_type == PortType::Inflow)
            {
                ports.emplace(
                    id,
                    PortCounts{
                        std::set<size_t>{c.port_number}, std::set<size_t>{}
                    }
                );
            }
            else if (c.port_type == PortType::Outflow)
            {
                ports.emplace(
                    id,
                    PortCounts{
                        std::set<size_t>{}, std::set<size_t>{c.port_number}
                    }
                );
            }
            else
            {
                throw std::runtime_error("unhandled port type");
            }
        }
        else
        {
            auto& pc = ports.at(id);
            if (c.port_type == PortType::Inflow)
            {
                // ensure_port_not_already_added(pc.input_ports, c.port_number);
                pc.input_ports.emplace(c.port_number);
            }
            else if (c.port_type == PortType::Outflow)
            {
                // ensure_port_not_already_added(pc.output_ports,
                // c.port_number);
                pc.output_ports.emplace(c.port_number);
            }
            else
            {
                throw std::runtime_error("unhandled port type");
            }
        }
    }

    std::string
    build_label(std::string const& id, PortCounts const& pc)
    {
        std::ostringstream label;
        auto num_inports = pc.input_ports.size();
        if (num_inports > 0)
        {
            for (auto ip : pc.input_ports)
            {
                label << "<I" << ip << "> I(" << ip << ")|";
            }
        }
        label << "<name> " << id;
        auto num_outports = pc.output_ports.size();
        if (num_outports > 0)
        {
            for (auto op : pc.output_ports)
            {
                label << "|<O" << op << "> O(" << op << ")";
            }
        }
        return "\"" + label.str() + "\"";
    }

    std::string
    build_label_html(std::string const& id, PortCounts const& pc)
    {
        std::ostringstream label;
        label << "<\n"
              << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" "
                 "CELLPADDING=\"4\">\n"
              << "      <TR>\n";
        auto num_inports = pc.input_ports.size();
        if (num_inports > 0)
        {
            for (auto ip : pc.input_ports)
            {
                label << "        <TD PORT=\"I" << ip
                      << "\" BGCOLOR=\"lightgrey\">" << "I(" << ip << ")"
                      << "</TD>\n";
            }
        }
        label << "        <TD PORT=\"name\">" << id << "</TD>\n";
        auto num_outports = pc.output_ports.size();
        if (num_outports > 0)
        {
            for (auto op : pc.output_ports)
            {
                label << "        <TD PORT=\"O" << op
                      << "\" BGCOLOR=\"lightgrey\">" << "O(" << op << ")"
                      << "</TD>\n";
            }
        }
        label << "      </TR>\n"
              << "    </TABLE>>";
        return label.str();
    }

    std::string
    network_to_dot(
        std::vector<Connection> const& network,
        std::vector<std::string> const& componentTagById,
        std::string const& graph_name,
        bool use_html_label
    )
    {
        int waste_count = 0;
        int env_count = 0;
        std::ostringstream connections;
        std::ostringstream declarations;
        std::map<std::string, PortCounts> ports;
        std::string tab{"  "};
        declarations << "digraph " << graph_name << " {\n";
        std::string shape_type;
        if (use_html_label)
        {
            shape_type = "none";
        }
        else
        {
            shape_type = "record";
            declarations << tab << "node [shape=" << shape_type << "];\n";
        }
        for (auto const& connection : network)
        {
            std::string from_tag = componentTagById[connection.FromId];
            if (from_tag.empty()
                && connection.From == ComponentType::EnvironmentSourceType)
            {
                from_tag = "ENV" + std::to_string(env_count);
                ++env_count;
            }
            ComponentAndPort c1 = {
                .component_id = std::move(from_tag),
                .port_type = PortType::Outflow,
                .port_number = connection.FromPort,
            };
            std::string to_tag = componentTagById[connection.ToId];
            if (to_tag.empty() && connection.To == ComponentType::WasteSinkType)
            {
                to_tag = "WASTE" + std::to_string(waste_count);
                ++waste_count;
            }
            ComponentAndPort c2 = {
                .component_id = std::move(to_tag),
                .port_type = PortType::Inflow,
                .port_number = connection.ToPort,
            };
            record_port_number(c1, ports);
            record_port_number(c2, ports);
            // to add colors to the edges, add the snippet below to the end:
            //<< " [color=\"black\"];\n";
            connections << tab << "\"" << c1.component_id << "\":" << "O"
                        << c1.port_number << ":s -> \"" << c2.component_id
                        << "\":" << "I" << c2.port_number << ":n;\n";
        }
        for (const auto& item : ports)
        {
            auto const& id = item.first;
            auto const& pc = item.second;
            std::string label;
            if (use_html_label)
            {
                label = build_label_html(id, pc);
            }
            else
            {
                label = build_label(id, pc);
            }
            declarations << tab << "\"" << id << "\""
                         << " [shape=" << shape_type << ",label=" << label
                         << "];\n";
        }
        return declarations.str() + connections.str() + "}";
    }

}
