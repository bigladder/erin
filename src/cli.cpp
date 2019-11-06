/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include <fstream>
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

  auto m = ::ERIN::Main{input_toml};
  ::ERIN::ScenarioResults out;
  ::ERIN::RealTimeType max_time;
  try {
    out = m.run(scenario_id);
    max_time = m.max_time_for_scenario(scenario_id);
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
  std::ofstream csv{output_csv, std::ios::out | std::ios::trunc};
  if (csv.is_open())
    csv << out.to_csv(max_time);
  else {
    std::cerr << "unable to open output_csv for writing \""
              << output_csv << "\"\n";
    return 1;
  }
  csv.close();
  return 0;
}
