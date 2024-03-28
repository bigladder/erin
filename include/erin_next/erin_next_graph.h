/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_GRAPH_H
#define ERIN_GRAPH_H
#include "erin_next.h"
#include <string>
#include <vector>

namespace erin
{
    std::string
    network_to_dot(
        std::vector<Connection> const&,
        std::vector<std::string> const& componentTagById,
        std::string const& graph_name,
        bool use_html_label = true
    );
}

#endif
