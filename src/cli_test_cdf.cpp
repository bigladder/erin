/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/distribution.h"
#include "erin/utils.h"
#include "erin/version.h"
#include "erin_csv.h"
#include "gsl/span"
#include <cstdint>
#include <fstream>
#include <string>
#include <iostream>

void
print_usage(const int argc, const gsl::span<const char*>& args)
{
  namespace ev = erin::version;
  namespace eu = erin::utils;
  auto exe_name = eu::path_to_filename(args[0]);
  for (int idx{0}; idx < argc; ++idx) {
    std::cout << "arg[" << idx << "] = " << args[idx] << "\n";
  }
  std::cout << exe_name << " version " << ev::version_string << "\n";
  std::cout << "USAGE: " << exe_name << " <distribution_name> <dist_param_1> <dist_param_2> <dist_param_3>\n"
               "  - distribution_name: one of 'fixed', 'uniform', 'normal', 'table'\n"
               "  - dist_param_1     :\n"
               "    - fixed          => the fixed value\n"
               "    - uniform        => the lower bound\n"
               "    - normal         => the mean of the distribution\n"
               "    - table          => name of a CSV file with the CDF defined in two columns variate and dtime (no header)\n"
               "  - dist_param_2     :\n"
               "    - fixed          => the number of samples\n"
               "    - uniform        => the upper bound\n"
               "    - normal         => the standard deviation of the distribution\n"
               "    - table          => the number of samples\n"
               "  - dist_param_3     :\n"
               "    - fixed          => unused\n"
               "    - uniform        => the number of samples\n"
               "    - normal         => the number of samples\n"
               "    - table          => unused\n"
               "SETS Exit Code 1 if issues encountered, else sets 0\n";
}

constexpr int num_args_for_fixed{4};
constexpr int num_args_for_uniform{5};
constexpr int num_args_for_normal{5};
constexpr int num_args_for_table{4};

int
main(const int argc, const char* argv[])
{
  namespace ev = erin::version;
  namespace eu = erin::utils;
  namespace ed = erin::distribution;
  auto args = gsl::span<const char*>(argv, argc);
  auto cds = ed::CumulativeDistributionSystem{};
  auto cdf_type = ed::tag_to_cdf_type(args[1]);
  const std::string tag{"cdf"};
  ed::size_type id{};
  ed::size_type num_samples{};
  switch (cdf_type) {
    case ed::CdfType::Fixed:
      {
        if (argc != num_args_for_fixed) {
          std::cout << "Missing arguments for fixed distribution\n";
          print_usage(argc, args);
          return 1;
        }
        std::int64_t value_in_seconds = std::stol(args[2]);
        num_samples = static_cast<ed::size_type>(std::stol(args[3]));
        id = cds.add_fixed_cdf(tag, value_in_seconds);
        break;
      }
    case ed::CdfType::Uniform:
      {
        if (argc != num_args_for_uniform) {
          std::cout << "Missing arguments for uniform distribution\n";
          print_usage(argc, args);
          return 1;
        }
        std::int64_t lower_bound = std::stol(args[2]);
        std::int64_t upper_bound = std::stol(args[3]);
        num_samples = static_cast<ed::size_type>(std::stol(args[4]));
        id = cds.add_uniform_cdf(tag, lower_bound, upper_bound);
        break;
      }
    case ed::CdfType::Normal:
      {
        if (argc != num_args_for_normal) {
          std::cout << "Missing arguments for uniform distribution\n";
          print_usage(argc, args);
          return 1;
        }
        std::int64_t mean = std::stol(args[2]);
        std::int64_t stdev = std::stol(args[3]);
        num_samples = static_cast<ed::size_type>(std::stol(args[4]));
        id = cds.add_normal_cdf(tag, mean, stdev);
        break;
      }
    case ed::CdfType::Table:
      {
        if (argc != num_args_for_table) {
          std::cout << "Missing arguments for table distribution\n";
          print_usage(argc, args);
          return 1;
        }
        std::string csv_path{args[2]};
        num_samples = static_cast<ed::size_type>(std::stol(args[3]));
        std::vector<double> xs{};
        std::vector<double> dtimes_s{};
        std::ifstream ifs{csv_path};
        if (!ifs.is_open()) {
          std::ostringstream oss{};
          oss << "input file stream on \"" << csv_path
              << "\" failed to open for reading\n";
          std::cerr << oss.str() << "\n";
          return 1;
        }
        for (int row{0}; ifs.good(); ++row) {
          std::string delim{""};
          auto cells = erin_csv::read_row(ifs);
          auto csize{cells.size()};
          if (csize == 0) {
            break;
          }
          if (csize != 2) {
            std::ostringstream oss{};
            oss << "issue reading input file csv \"" << csv_path
                << "\"; issue on row " << row
                << "; number of columns should be 2 but got " << csize << "\n";
            std::cerr << oss.str() << "\n";
            return 1;
          }
          xs.emplace_back(std::stod(cells[0]));
          dtimes_s.emplace_back(std::stod(cells[1]));
        }
        ifs.close();
        id = cds.add_table_cdf(tag, xs, dtimes_s);
        break;
      }
    default:
      {
        print_usage(argc, args);
        return 1;
      }
  }
  using size_type = std::vector<std::int64_t>::size_type;
  std::cout << "data\n";
  for (size_type idx{0}; idx < num_samples; ++idx) {
    std::cout << cds.next_time_advance(id) << "\n";
  }
  return 0;
}
