/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/distribution.h"
#include "erin/utils.h"
#include "erin/version.h"
#include "gsl/span"
#include <cstdint>
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
               "  - distribution_name: one of 'fixed', 'uniform', 'normal'\n"
               "  - dist_param_1     :\n"
               "    - fixed          => the fixed value\n"
               "    - uniform        => the lower bound\n"
               "    - normal         => the mean of the distribution\n"
               "  - dist_param_2     :\n"
               "    - fixed          => the number of samples\n"
               "    - uniform        => the upper bound\n"
               "    - normal         => the standard deviation of the distribution\n"
               "  - dist_param_3     :\n"
               "    - fixed          => unused\n"
               "    - uniform        => the number of samples\n"
               "    - normal         => the number of samples\n"
               "SETS Exit Code 1 if issues encountered, else sets 0\n";
}

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
        if (argc != 4) {
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
        if (argc != 5) {
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
        if (argc != 5) {
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
