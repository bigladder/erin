/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/distribution.h"
#include "erin/utils.h"
#include "erin/version.h"
#include "erin_csv.h"
#include "gsl/span"
#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>

void
print_usage(const gsl::span<const char*>& args)
{
  namespace ev = erin::version;
  namespace eu = erin::utils;
  auto exe_name = (args.size() > 0)
    ? eu::path_to_filename(args[0])
    : std::string{""};
  for (int idx{0}; idx < args.size(); ++idx) {
    std::cout << "arg[" << idx << "] = " << args[idx] << "\n";
  }
  std::cout << exe_name << " version " << ev::version_string << "\n";
  std::cout << "USAGE: " << exe_name << " <distribution_name> <dist_param_1> "
                                         "<dist_param_2> <dist_param_3>\n"
               "  - distribution_name: one of 'fixed', 'uniform', 'normal', "
                                      "'weibull', 'table'\n"
               "  - dist_param_1     :\n"
               "    - fixed          => the fixed value\n"
               "    - uniform        => the lower bound\n"
               "    - normal         => the mean of the distribution\n"
               "    - weibull        => the shape parameter of the Weibull "
                                       "distribution\n"
               "    - table          => name of a CSV file with the quantile "
                                       "(inverse cumulative distribution "
                                       "function) defined in two "
                                       "columns: variate and dtime "
                                       "(no header)\n"
               "  - dist_param_2     :\n"
               "    - fixed          => the number of samples\n"
               "    - uniform        => the upper bound\n"
               "    - normal         => the standard deviation of the "
                                       "distribution\n"
               "    - weibull        => the scale parameter of the Weibull "
                                       "distribution\n"
               "    - table          => the number of samples\n"
               "  - dist_param_3     :\n"
               "    - fixed          => unused\n"
               "    - uniform        => the number of samples\n"
               "    - normal         => the number of samples\n"
               "    - weibull        => the location parameter of the Weibull "
                                       "distribution\n"
               "    - table          => unused\n"
               "  - dist_param_4     :\n"
               "    - fixed          => unused\n"
               "    - uniform        => unused\n"
               "    - normal         => unused\n"
               "    - weibull        => the number of samples\n"
               "    - table          => unused\n"
               "SETS Exit Code 1 if issues encountered, else sets 0\n";
}

constexpr int num_args_for_fixed{4};
constexpr int num_args_for_uniform{5};
constexpr int num_args_for_normal{5};
constexpr int num_args_for_weibull{6};
constexpr int num_args_for_table{4};
const auto num_args_vec = std::vector<int>{
  num_args_for_fixed,
  num_args_for_uniform,
  num_args_for_normal,
  num_args_for_weibull,
  num_args_for_table
};
const auto min_num_args = *std::min_element(num_args_vec.begin(), num_args_vec.end());
const auto max_num_args = *std::max_element(num_args_vec.begin(), num_args_vec.end());

int
run_it(const gsl::span<const char*>& args)
{
  const int argc{static_cast<int>(args.size())};
  if ((argc < min_num_args) || (argc > max_num_args)) {
    print_usage(args);
    return 1;
  }
  namespace ev = erin::version;
  namespace eu = erin::utils;
  namespace ed = erin::distribution;
  auto ds = ed::DistributionSystem{};
  ed::DistType dist_type{ed::DistType::Fixed};
  try {
    dist_type = ed::tag_to_dist_type(args[1]);
  }
  catch (const std::exception&) {
    std::ostringstream oss{};
    oss << "issue parsing dist_type \"" << args[1] << "\"\n";
    std::cerr << oss.str();
    return 1;
  }
  const std::string tag{"distribution"};
  ed::size_type id{};
  ed::size_type num_samples{};
  switch (dist_type) {
    case ed::DistType::Fixed:
      {
        if (argc != num_args_for_fixed) {
          std::cout << "Missing arguments for fixed distribution\n";
          print_usage(args);
          return 1;
        }
        std::int64_t value_in_seconds{0};
        try {
          value_in_seconds = std::stol(args[2]);
        }
        catch (const std::exception&) {
          std::cout << "Value in seconds must be convertable to an int64\n";
          print_usage(args);
          return 1;
        }
        try {
          num_samples = static_cast<ed::size_type>(std::stol(args[3]));
        }
        catch (const std::exception&) {
          std::cout << "Number of samples must be convertable to an int64\n";
          print_usage(args);
          return 1;
        }
        try {
          id = ds.add_fixed(tag, value_in_seconds);
        }
        catch (const std::exception&) {
          std::cout << "Error creating a fixed distribution\n";
          print_usage(args);
          return 1;
        }
        break;
      }
    case ed::DistType::Uniform:
      {
        if (argc != num_args_for_uniform) {
          std::cout << "Missing arguments for uniform distribution\n";
          print_usage(args);
          return 1;
        }
        std::int64_t lower_bound{0};
        std::int64_t upper_bound{1};
        try {
          lower_bound = std::stol(args[2]);
        }
        catch (const std::exception&) {
          std::cout << "Lower bound must be convertable to int64 for uniform distribution\n";
          print_usage(args);
          return 1;
        }
        try {
          upper_bound = std::stol(args[3]);
        }
        catch (const std::exception&) {
          std::cout << "Upper bound must be convertable to int64 for uniform distribution\n";
          print_usage(args);
          return 1;
        }
        try {
          num_samples = static_cast<ed::size_type>(std::stol(args[4]));
        }
        catch (const std::exception&) {
          std::cout << "Number of samples must be convertable to an int64\n";
          print_usage(args);
          return 1;
        }
        try {
          id = ds.add_uniform(tag, lower_bound, upper_bound);
        }
        catch (const std::exception&) {
          std::cout << "Error creating a uniform distribution\n";
          print_usage(args);
          return 1;
        }
        break;
      }
    case ed::DistType::Normal:
      {
        if (argc != num_args_for_normal) {
          std::cout << "Missing arguments for normal distribution\n";
          print_usage(args);
          return 1;
        }
        std::int64_t mean{0};
        std::int64_t stdev{0};
        try {
          mean = std::stol(args[2]);
        }
        catch (const std::exception&) {
          std::cout << "Mean must be convertable to an int64 for normal\n";
          print_usage(args);
          return 1;
        }
        try {
          stdev = std::stol(args[3]);
        }
        catch (const std::exception&) {
          std::cout << "Standard deviation must be convertable to an int64 for normal\n";
          print_usage(args);
          return 1;
        }
        try {
          num_samples = static_cast<ed::size_type>(std::stol(args[4]));
        }
        catch (const std::exception&) {
          std::cout << "Number of samples must be convertable to an int64\n";
          print_usage(args);
          return 1;
        }
        try {
          id = ds.add_normal(tag, mean, stdev);
        }
        catch (const std::exception&) {
          std::cout << "Error creating a normal distribution\n";
          print_usage(args);
          return 1;
        }
        break;
      }
    case ed::DistType::Weibull:
      {
        const int idx_size{2};
        const int idx_scale{3};
        const int idx_location{4};
        const int idx_num_samples{5};
        if (argc != num_args_for_weibull) {
          std::cout << "Missing arguments for Weibull distribution\n";
          print_usage(args);
          return 1;
        }
        double shape{0};
        double scale{0};
        double location{0};
        try {
          shape = std::stod(args[idx_size]);
        }
        catch (const std::exception&) {
          std::cout << "Shape must be convertable to a double for Weibull\n";
          print_usage(args);
          return 1;
        }
        try {
          scale = std::stod(args[idx_scale]);
        }
        catch (const std::exception&) {
          std::cout << "Scale must be convertable to a double for Weibull\n";
          print_usage(args);
          return 1;
        }
        try {
          location = std::stod(args[idx_location]);
        }
        catch (const std::exception&) {
          std::cout << "Location must be convertable to a double for Weibull\n";
          print_usage(args);
          return 1;
        }
        try {
          num_samples = static_cast<ed::size_type>(std::stol(args[idx_num_samples]));
        }
        catch (const std::exception&) {
          std::cout << "Number of samples must be convertable to an int64\n";
          print_usage(args);
          return 1;
        }
        try {
          id = ds.add_weibull(tag, shape, scale, location);
        }
        catch (const std::exception&) {
          std::cout << "Error creating a Weibull distribution\n";
          print_usage(args);
          return 1;
        }
        break;
      }
    case ed::DistType::QuantileTable:
      {
        if (argc != num_args_for_table) {
          std::cout << "Missing arguments for table distribution\n";
          print_usage(args);
          return 1;
        }
        std::string csv_path{args[2]};
        try {
          num_samples = static_cast<ed::size_type>(std::stol(args[4]));
        }
        catch (const std::exception&) {
          std::cout << "Number of samples must be convertable to an int64\n";
          print_usage(args);
          return 1;
        }
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
          try {
            xs.emplace_back(std::stod(cells[0]));
            dtimes_s.emplace_back(std::stod(cells[1]));
          }
          catch (const std::exception&) {
            std::ostringstream oss{};
            oss << "issue reading input file csv \"" << csv_path
                << "\"; issue on row " << row
                << "; could not conver xs (" << cells[0]
                << ") or dtimes (" << cells[1] << " to double\n";
            std::cerr << oss.str() << "\n";
            return 1;
          }
        }
        ifs.close();
        try {
          id = ds.add_quantile_table(tag, xs, dtimes_s);
        }
        catch (const std::exception&) {
          std::cout << "Error creating a tabular distribution\n";
          print_usage(args);
          return 1;
        }
        break;
      }
    default:
      {
        print_usage(args);
        return 1;
      }
  }
  using size_type = std::vector<std::int64_t>::size_type;
  std::cout << "data\n";
  for (size_type idx{0}; idx < num_samples; ++idx) {
    try {
      std::cout << ds.next_time_advance(id) << "\n";
    }
    catch (const std::exception&) {
      std::cout << "Error attempting to sample distribution on sample "
                << idx << "\n";
      print_usage(args);
      return 1;
    }
  }
  return 0;
}

int
main(const int argc, const char* argv[])
{
  int ret_val{0};
  auto args = gsl::span<const char*>(argv, argc);
  try {
    ret_val = run_it(args);
  }
  catch (const std::exception&) {
    std::cerr << "an unhandled exception occurred...\n";
    ret_val = 1;
  }
  return ret_val;
}
