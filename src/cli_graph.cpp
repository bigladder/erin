/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "debug_utils.h"
#include "erin/erin.h"
#include "erin/graphviz.h"
#include "erin/network.h"
#include "gsl/span"
#include <fstream>
#include <iostream>
#include <stdexcept>

constexpr int num_args{3};

int
doit(const std::string& input_toml, const std::string& dot_file_path, const std::string& network_id)
{
  namespace en = erin::network;
  namespace eg = erin::graphviz;

  std::cout << "input_toml    : " << input_toml << "\n";
  std::cout << "dot_file_path : " << dot_file_path << "\n";
  std::cout << "network_id    : " << network_id << "\n";

  auto m = ERIN::Main{input_toml};
  const auto& network_map = m.get_networks();
  std::vector<en::Connection> network;
  try {
    network = network_map.at(network_id);
  } catch (const std::out_of_range&) {
    std::ostringstream oss;
    oss << "network with id '" << network_id << "' not found\n";
    oss << "available options: ";
    bool first{true};
    for (const auto& item: network_map) {
      if (first) {
        oss << "[";
        first = false;
      } else {
        oss << ", ";
      }
      oss << item.first;
    }
    oss << "]\n";
    std::cerr << oss.str();
    return 1;
  }
  std::ofstream dot{dot_file_path, std::ios::out | std::ios::trunc};
  if (dot.is_open()) {
    dot << eg::network_to_dot(network, network_id);
  }
  else {
    std::cerr << "unable to open dot_file_path for writing \""
      << dot_file_path << "\"\n";
    return 1;
  }
  dot.close();
  return 0;
}

int
main(const int argc, const char* argv[])
{
  auto args = gsl::span<const char*>(argv, argc);
  if (argc != (num_args + 1)) {
    std::cout << "USAGE: " << args[0] << " <input_file_path> <output_file_path> <stats_file_path>\n"
                 "  - input_file_path : path to TOML input file\n"
                 "  - dot_file_path   : path to Graphviz DOT file to write\n"
                 "  - network_id      : id for the network to plot from input_file_path\n"
                 "SETS Exit Code 1 if issues encountered, else sets 0\n";
    return 1;
  }
  std::string input_toml{args[1]};
  std::string dot_file_path{args[2]};
  std::string network_id{args[3]};
  try {
    return doit(input_toml, dot_file_path, network_id);
  }
  catch (const std::exception& e) {
    std::cerr << "Unknown exception!\n"
                 "Message: " << e.what() << "\n";
    return 1;
  }
  return 0;
}

