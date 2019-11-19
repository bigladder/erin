/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DISTRIBUTIONS_H
#define ERIN_DISTRIBUTIONS_H
#include <chrono>
#include <exception>
#include <functional>
#include <random>
#include <sstream>

namespace erin::distribution
{
  template <class T>
  std::function<T(void)>
  make_fixed(const T& value) 
  {
    return [value]() -> T { return value; };
  };

  template <class T>
  std::function<T(void)>
  make_random_integer(const std::default_random_engine& generator, const T& lb, const T& ub)
  {
    if (lb >= ub) {
      std::ostringstream oss{};
      oss << "expected lower_bound < upper_bound but lower_bound = "
        << lb << " and upper_bound = " << ub;
      throw std::invalid_argument(oss.str());
    }
    std::uniform_int_distribution<T> d{lb, ub};
    auto g = generator; // copy-assignment constructor
    return [d, g]() mutable -> T { return d(g); };
  }
}
#endif // ERIN_DISTRIBUTIONS_H
