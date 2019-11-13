/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include <fstream>
#include <iostream>
#include "debug_utils.h"
#include "erin/erin.h"

int
main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cout << "USAGE: e2rin <input_file_path> <output_file_path> "
                 "<stats_file_path> <scenario_id>\n"
                 "  - input_file_path : path to TOML input file\n"
                 "  - output_file_path: path to CSV output file for time-series data\n"
                 "  - stats_file_path : path to CSV output file for statistics\n"
                 "  - scenario_id     : the id of the scenario to run\n"
                 "SETS Exit Code 1 if issues encountered, else sets 0\n";
    return 1;
  }
  auto input_toml = std::string{argv[1]};
  auto timeseries_csv = std::string{argv[2]};
  auto stats_csv = std::string{argv[3]};
  auto scenario_id = std::string{argv[4]};
  std::cout << "input_toml      : " << input_toml << "\n";
  std::cout << "timeseries_csv  : " << timeseries_csv << "\n";
  std::cout << "stats_csv       : " << stats_csv << "\n";
  std::cout << "scenario_id     : \"" << scenario_id << "\"" << "\n";

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
  std::cout << "max_time = " << max_time << "\n";
  if (!out.get_is_good()) {
    return 1;
  }
  std::ofstream csv{timeseries_csv, std::ios::out | std::ios::trunc};
  if (csv.is_open()) {
    csv << out.to_csv(max_time);
  }
  else {
    std::cerr << "unable to open timeseries_csv for writing \""
              << timeseries_csv << "\"\n";
    return 1;
  }
  csv.close();
  std::ofstream stats{stats_csv, std::ios::out | std::ios::trunc};
  if (stats.is_open()) {
    stats << out.to_stats_csv();
  }
  else {
    std::cerr << "unable to open stats_csv for writing \""
              << stats_csv << "\"\n";
  }
  return 0;
}
