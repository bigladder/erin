// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <stack>
#include <algorithm>

#include "erin/network-utils.h"

namespace erin
{

void strong_connect(size_t& time_index,
                    size_t node_idx,
                    std::stack<int>& stack,
                    std::vector<int>& find_times,
                    std::vector<int>& low_links,
                    std::vector<bool>& on_stacks,
                    std::vector<std::vector<size_t>>& sccs,
                    std::vector<std::string> const& nodes,
                    std::vector<std::pair<size_t, size_t>> const& edges)
{
    find_times[node_idx] = static_cast<int>(time_index);
    low_links[node_idx] = static_cast<int>(time_index);
    time_index++;
    stack.push(static_cast<int>(node_idx));
    on_stacks[node_idx] = true;

    for (std::pair<size_t, size_t> const& e : edges)
    {
        if (e.first != node_idx)
        {
            continue;
        }
        size_t neighbor_node_idx = e.second;
        if (find_times[neighbor_node_idx] == -1)
        {
            strong_connect(time_index,
                           neighbor_node_idx,
                           stack,
                           find_times,
                           low_links,
                           on_stacks,
                           sccs,
                           nodes,
                           edges);
            low_links[node_idx] = std::min(low_links[node_idx], low_links[neighbor_node_idx]);
        }
        else if (on_stacks[neighbor_node_idx])
        {
            low_links[node_idx] = std::min(low_links[node_idx], find_times[neighbor_node_idx]);
        }
    }
    if (low_links[node_idx] == find_times[node_idx])
    {
        std::vector<size_t> scc = {};
        size_t scc_node_idx;
        do
        {
            scc_node_idx = stack.top();
            stack.pop();
            on_stacks[scc_node_idx] = false;
            scc.push_back(scc_node_idx);
        } while (scc_node_idx != node_idx);
        sccs.push_back(std::move(scc));
    }
}

std::vector<std::vector<std::string>>
find_strongly_connected_components(std::vector<std::string> const& nodes,
                                   std::vector<std::pair<size_t, size_t>> const& edges,
                                   size_t minimum_component_size)
{
    std::vector<std::vector<size_t>> sccs;
    std::stack<int> stack = {};
    size_t time_index = 0;
    std::vector<int> find_times(nodes.size(), -1);
    std::vector<int> low_links(nodes.size(), -1);
    std::vector<bool> on_stacks(nodes.size(), false);
    for (size_t node_idx = 0; node_idx < nodes.size(); ++node_idx)
    {
        if (find_times[node_idx] == -1)
        {
            strong_connect(
                time_index, node_idx, stack, find_times, low_links, on_stacks, sccs, nodes, edges);
        }
    }
    std::vector<std::vector<std::string>> result = {};
    for (std::vector<size_t> const& component : sccs)
    {
        if (component.size() < minimum_component_size)
        {
            continue;
        }
        std::vector<std::string> component_nodes = {};
        for (size_t node_idx : component)
        {
            component_nodes.push_back(nodes[node_idx]);
        }
        result.push_back(component_nodes);
    }
    return result;
}

} // namespace erin
