// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_NETWORK_UTILS_H
#define ERIN_NETWORK_UTILS_H

#include <cstddef>
#include <vector>
#include <string>
#include <utility>

namespace erin
{

std::vector<std::vector<std::string>>
find_strongly_connected_components(
  std::vector<std::string> const& nodes,
  std::vector<std::pair<size_t, size_t>> const& edges,
  size_t minimum_component_size = 2
);

} // namespace erin

#endif
