/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DISTRIBUTION_H
#define ERIN_DISTRIBUTION_H
#include "erin/type.h"
#include <chrono>
#include <exception>
#include <functional>
#include <random>
#include <sstream>
#include <string>

namespace erin::distribution
{
  using size_type = std::vector<std::int64_t>::size_type;
  using RealTimeType = ERIN::RealTimeType;

  // TODO: consider going back to classes with simple data that can take a
  // generator at a later point. Ideally, we'd like the distribution objects to
  // all reference a single default_random_engine that is held by Main.
  // Interface:
  // - virtual void set_random_generator(const std::default_random_engine& g);
  // - virtual T next_value();
  // - virtual std::string get_type();
  template <class T>
  std::function<T(void)>
  make_fixed(const T& value) 
  {
    return [value]() -> T { return value; };
  };

  template <class T>
  std::function<T(void)>
  make_random_integer(
      const std::default_random_engine& generator, const T& lb, const T& ub)
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

  enum class CdfType
  {
    Fixed = 0//,
    //Normal,
    //Weibull
  };

  std::string cdf_type_to_tag(CdfType cdf_type);
  CdfType tag_to_cdf_type(const std::string& tag);

  struct Cdf {
    std::vector<std::string> tag{};
    std::vector<size_type> subtype_id{};
    std::vector<CdfType> cdf_type{};
  };

  struct Fixed_CDF
  {
    std::vector<RealTimeType> value{};
  };

  class CumulativeDistributionSystem
  {
    public:
      CumulativeDistributionSystem();

      size_type add_fixed_cdf(const std::string& tag, RealTimeType value_in_seconds);
      [[nodiscard]] size_type lookup_cdf_by_tag(const std::string& tag) const;
      RealTimeType next_time_advance(size_type cdf_id);

    private:
      Cdf cdf;
      Fixed_CDF fixed_cdf;
  };
}
#endif // ERIN_DISTRIBUTION_H
