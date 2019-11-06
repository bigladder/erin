/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include <iostream>
#include "debug_utils.h"
#include "erin/erin.h"

int
main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cout << "USAGE: e2rin <input_file_path> <output_file_path> "
                 "<scenario_id>\n"
                 "  - input_file_path: path to TOML input file\n"
                 "  - output_file_path: path to TOML output file\n"
                 "  - scenario_id: the id of the scenario to run\n"
                 "SETS Exit Code 1 if issues encountered, else sets 0\n";
    return 1;
  }
  auto input_toml = std::string{argv[1]};
  auto output_csv = std::string{argv[2]};
  auto scenario_id = std::string{argv[3]};
  DB_PUTS("input_toml  : " << input_toml);
  DB_PUTS("output_csv  : " << output_csv);
  DB_PUTS("scenario_id : \"" << scenario_id << "\"");

  auto m = ERIN::Main{input_toml};
  ERIN::ScenarioResults out;
  try {
    out = m.run(scenario_id);
  }
  catch (const std::invalid_argument& ia) {
    std::cerr << "Error!\n"
                 "Invalid argument: " << ia.what() << "\n";
    return 1;
  }
  std::cout << "result of m.run(\"" << scenario_id << "\") = "
    << out.get_is_good() << "\n";
  if (!out.get_is_good())
    return 1;
  std::cout << "number of results = " << out.get_results().size() << "\n";
  std::cout << "Results:\n";
  int max_count{6};
  int count{0};
  for (const auto& item: out.get_results()) {
    count = 0;
    std::cout << "... " << item.first << " [";
    for (const auto d: item.second) {
      if (count > max_count) break;
      ++count;
      std::cout << "{" << d.time << ", " << d.value << "} ";
    }
    std::cout << "]" << std::endl;
  }
  return 0;
}
