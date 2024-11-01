// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_GRAPH_H
#define ERIN_GRAPH_H
#include "erin.h"
#include <string>
#include <vector>

namespace erin
{
// PUBLIC
std::string network_to_dot(std::vector<Connection> const& connections,
                           std::vector<std::string> const& component_tag_by_id,
                           std::string const& graph_name,
                           bool use_html_label = true);
} // namespace erin

#endif
