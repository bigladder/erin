/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include "debug_utils.h"
#include "erin/erin.h"
#include "gsl/span"
#include <fstream>
#include <iostream>

constexpr int num_args{3};

int
main(const int argc, const char* argv[])
{
  const auto default_time_units = ::ERIN::TimeUnits::Hours;
  auto args = gsl::span<const char*>(argv, argc);
  if (argc != (num_args + 1)) {
    std::cout << "USAGE: " << args[0] << " <input_file_path> <output_file_path> <stats_file_path>\n"
                 "  - input_file_path : path to TOML input file\n"
                 "  - output_file_path: path to CSV output file for time-series data\n"
                 "  - stats_file_path : path to CSV output file for statistics\n"
                 "SETS Exit Code 1 if issues encountered, else sets 0\n";
    return 1;
  }
  std::string input_toml{args[1]};
  std::string timeseries_csv{args[2]};
  std::string stats_csv{args[3]};
  std::cout << "input_toml      : " << input_toml << "\n";
  std::cout << "timeseries_csv  : " << timeseries_csv << "\n";
  std::cout << "stats_csv       : " << stats_csv << "\n";

  auto m = ::ERIN::Main{input_toml};
  auto out = m.run_all();
  std::cout << "result of m.run_all() = "
    << (out.get_is_good() ? "good" : "failed") << "\n";
  if (!out.get_is_good()) {
    return 1;
  }
  std::ofstream csv{timeseries_csv, std::ios::out | std::ios::trunc};
  if (csv.is_open()) {
    csv << out.to_csv();
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

