/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/random.h"
#include "debug_utils.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace ERIN
{
  bool
  operator==(
      const std::unique_ptr<RandomInfo>& a,
      const std::unique_ptr<RandomInfo>& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "unique_ptr<RandomInfo> == unique_ptr<RandomInfo>\n";
    }
    auto a_type = a->get_type();
    auto b_type = b->get_type();
    if (a_type != b_type) {
      if constexpr (debug_level >= debug_level_high) {
        std::cout << "a_type != b_type\n";
        std::cout << "a_type = " << static_cast<int>(a_type) << "\n";
        std::cout << "b_type = " << static_cast<int>(b_type) << "\n";
      }
      return false;
    }
    switch (a_type) {
      case RandomType::RandomProcess:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "RandomProcess compare...\n";
          }
          const auto& a_rp = dynamic_cast<const RandomProcess&>(*a);
          const auto& b_rp = dynamic_cast<const RandomProcess&>(*b);
          return a_rp == b_rp;
        }
      case RandomType::FixedProcess:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "FixedProcess compare...\n";
          }
          const auto& a_fp = dynamic_cast<const FixedProcess&>(*a);
          const auto& b_fp = dynamic_cast<const FixedProcess&>(*b);
          return a_fp == b_fp;
        }
      case RandomType::FixedSeries:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "FixedSeries compare...\n";
          }
          const auto& a_fs = dynamic_cast<const FixedSeries&>(*a);
          const auto& b_fs = dynamic_cast<const FixedSeries&>(*b);
          return a_fs == b_fs;
        }
      default:
        {
          if constexpr (debug_level >= debug_level_high) {
            std::cout << "unhandled process...\n";
          }
          std::ostringstream oss;
          oss << "unhandled RandomType " << static_cast<int>(a_type) << "\n";
          throw std::invalid_argument(oss.str());
        }
    }
  }

  bool
  operator!=(
      const std::unique_ptr<RandomInfo>& a,
      const std::unique_ptr<RandomInfo>& b)
  {
    return !(a == b);
  }

  std::unique_ptr<RandomInfo>
  make_random_info(
      bool has_fixed_random,
      double fixed_random,
      bool has_seed,
      unsigned int seed_value)
  {
    std::unique_ptr<RandomInfo> ri{};
    if ((has_fixed_random) && (has_seed)) {
      std::ostringstream oss;
      oss << "cannot have fixed random AND specify random seed\n"
             "has_fixed_random implies a fixed process\n"
             "has_seed implies a random process\n"
             "has_fixed_random and has_seed can be:\n"
             "  (false, false) => random process seeded by clock\n"
             "  (true, false) => fixed process with specified value\n"
             "  (false, true) => random process with specified seed\n";
      throw std::invalid_argument(oss.str());
    }
    if (has_fixed_random) {
      ri = std::make_unique<FixedProcess>(fixed_random);
    }
    else if (has_seed) {
      ri = std::make_unique<RandomProcess>(seed_value);
    }
    else {
      ri = std::make_unique<RandomProcess>();
    }
    return ri;
  }

  std::unique_ptr<RandomInfo>
  make_random_info(
      bool has_fixed_random,
      double fixed_random,
      bool has_seed,
      unsigned int seed_value,
      bool has_fixed_series,
      const std::vector<double>& series)
  {
    if (!has_fixed_series) {
      return make_random_info(has_fixed_random, fixed_random, has_seed, seed_value);
    }
    if (has_fixed_series && (has_fixed_random || has_seed)) {
      std::ostringstream oss;
      oss << "cannot have fixed_series AND (fixed_random OR has_seed) "
             "at the same time\n"
             "has_fixed_random implies a fixed process using a single number\n"
             "has_fixed_series implies a fixed process using a single "
             "series of numbers\n"
             "has_seed implies a random process\n"
             "has_fixed_random, has_seed, and has_fixed_series can be:\n"
             "  (false, false, false) => random process seeded by clock\n"
             "  (true, false, false) => fixed process with specified value\n"
             "  (false, true, false) => random process with specified seed\n"
             "  (false, false, true) => series of (repating) random values\n";
      throw std::invalid_argument(oss.str());
    }
    std::unique_ptr<RandomInfo> ri = std::make_unique<FixedSeries>(series);
    return ri;
  }

  ////////////////////////////////////////////////////////////
  // RandomProcess
  RandomProcess::RandomProcess() :
    RandomInfo(),
    seed{},
    generator{},
    distribution{0.0, 1.0}
  {
    namespace C = std::chrono;
    auto now = C::high_resolution_clock::now();
    auto d = now.time_since_epoch();
    constexpr unsigned int range =
      std::numeric_limits<unsigned int>::max()
      - std::numeric_limits<unsigned int>::min();
    seed = d.count() % range;
    generator.seed(seed);
  }

  RandomProcess::RandomProcess(unsigned int seed_):
    RandomInfo(),
    seed{seed_},
    generator{},
    distribution{0.0, 1.0}
  {
    generator.seed(seed);
  }

  double
  RandomProcess::call()
  {
    return distribution(generator);
  }

  bool
  operator==(const RandomProcess& a, const RandomProcess& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "RandomProcess == RandomProcess...\n";
      std::cout << "a.seed = " << a.seed << "...\n";
      std::cout << "b.seed = " << b.seed << "...\n";
    }
    return a.seed == b.seed;
  }

  bool
  operator!=(const RandomProcess& a, const RandomProcess& b)
  {
    return !(a == b);
  }

  ////////////////////////////////////////////////////////////
  // FixedProcess
  FixedProcess::FixedProcess(double fixed_value_):
    RandomInfo(),
    fixed_value{fixed_value_}
  {
    if ((fixed_value < 0.0) || (fixed_value > 1.0)) {
      throw std::invalid_argument("fixed_value must be >= 0.0 and <= 1.0");
    }
  }

  bool
  operator==(const FixedProcess& a, const FixedProcess& b)
  {
    if constexpr (debug_level >= debug_level_high) {
      std::cout << "FixedProcess == FixedProcess...\n";
      std::cout << "a.fixed_value = " << a.fixed_value << "...\n";
      std::cout << "b.fixed_value = " << b.fixed_value << "...\n";
    }
    return a.fixed_value == b.fixed_value;
  }

  bool
  operator!=(const FixedProcess& a, const FixedProcess& b)
  {
    return !(a == b);
  }

  ////////////////////////////////////////////////////////////
  // FixedSeries
  FixedSeries::FixedSeries(const std::vector<double>& series_):
    FixedSeries(series_, 0)
  {
  }

  FixedSeries::FixedSeries(
      const std::vector<double>& series_,
      std::vector<double>::size_type idx_):
    RandomInfo(),
    series{series_},
    idx{idx_},
    max_idx{}
  {
    max_idx = series.size() - 1;
    if (max_idx < 0) {
      std::ostringstream oss{};
      oss << "the series given to FixedSeries must be at lease of size 1 or greater: "
             "size = " << (max_idx + 1);
      throw std::invalid_argument(oss.str());
    }
    if (idx > max_idx) {
      std::ostringstream oss;
      oss << "the index cannot be greater than the length of the series: "
          << "size = " << (max_idx + 1) << "; "
          << "idx = " << idx << "; "
          << "max_idx = " << max_idx;
      throw std::invalid_argument(oss.str());
    }
  }

  double
  FixedSeries::call()
  {
    auto v = series.at(idx);
    increment_idx();
    return v;
  }

  void
  FixedSeries::increment_idx()
  {
    ++idx;
    if (idx > max_idx)
      idx = 0;
  }

  bool
  operator==(const FixedSeries& a, const FixedSeries& b)
  {
    return (a.series == b.series) && (a.idx == b.idx);
  }

  bool
  operator!=(const FixedSeries& a, const FixedSeries& b)
  {
    return !(a == b);
  }

}
