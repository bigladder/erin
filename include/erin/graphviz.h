/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_GRAPHVIZ_H
#define ERIN_GRAPHVIZ_H
#include "erin/network.h"
#include <set>
#include <string>
#include <vector>

namespace erin::graphviz
{
  namespace en = erin::network;
  struct PortCounts
  {
    std::set<int> input_ports;
    std::set<int> output_ports;
  };

  std::string network_to_dot(
      const std::vector<en::Connection>&,
      const std::string& graph_name,
      const bool use_html_label=true);
}

#endif // ERIN_GRAPHVIZ_H
