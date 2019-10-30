/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include <iostream>
#include "erin/erin.h"


int
main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "USAGE: aren <input_file_path> <output_file_path>\n";
    std::cout << "  - input_file_path: path to TOML input file\n";
    std::cout << "  - output_file_path: path to TOML output file\n";
    std::cout << "SETS Exit Code 1 if issues encountered, else sets 0\n";
    return 1;
  }
  auto input_toml = std::string{argv[1]};
  auto output_toml = std::string{argv[2]};

  auto m = ERIN::Main(input_toml, output_toml);
  auto out = m.run();
  std::cout << "result of m.run() = " << out.is_good << std::endl;
  std::cout << "number of results = " << out.results.size() << std::endl;
  for (const auto& item: out.results) {
    std::cout << "..." << item.first << " [";
    for (const auto d: item.second)
      std::cout << "{" << d.time << ", " << d.value << "} ";
    std::cout << "]" << std::endl;
  }
  return 0;
}
