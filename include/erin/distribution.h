/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

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
  // * As an alternative, consider an interface that takes a randomly
  // generated number and samples the distribution. That might be even more
  // elegant.
  // - virtual T sample(const double d); // i.e., sample the distribution at d; d must be in (0, 1]
  //   possibly, we could create a new type that would express that d is a fraction
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
    Fixed = 0,
    Uniform,
    Normal,
    Table // from times and variate: variate is from (0,1) time and variate
          // must be always increasing
    //Weibull,
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

  struct Uniform_CDF
  {
    std::vector<RealTimeType> lower_bound{};
    std::vector<RealTimeType> upper_bound{};
  };

  struct Normal_CDF
  {
    std::vector<RealTimeType> average{};
    std::vector<RealTimeType> stddev{};
  };

  struct Table_CDF
  {
    std::vector<double> variates{};
    std::vector<double> times{};
    std::vector<size_type> start_idx{};
    std::vector<size_type> end_idx{};
  };

  class CumulativeDistributionSystem
  {
    public:
      CumulativeDistributionSystem();

      size_type add_fixed_cdf(
          const std::string& tag,
          RealTimeType value_in_seconds);

      size_type add_uniform_cdf(
          const std::string& tag,
          RealTimeType lower_bound_s,
          RealTimeType upper_bound_s);

      size_type add_normal_cdf(
          const std::string& tag,
          RealTimeType mean_s,
          RealTimeType stddev_s);

      size_type add_table_cdf(
          const std::string& tag,
          const std::vector<double>& xs,
          const std::vector<double>& dtimes_s);

      [[nodiscard]] size_type lookup_cdf_by_tag(const std::string& tag) const;

      RealTimeType next_time_advance(size_type cdf_id);

      RealTimeType next_time_advance(size_type cdf_id, double fraction) const;

      //[[nodiscard]] std::vector<RealTimeType>
      //  sample_upto_including(const RealTimeType max_time_s);

    private:
      Cdf cdf;
      Fixed_CDF fixed_cdf;
      Uniform_CDF uniform_cdf;
      Normal_CDF normal_cdf;
      Table_CDF table_cdf;
      std::default_random_engine g;
      std::uniform_real_distribution<double> dist;
  };
}
#endif // ERIN_DISTRIBUTION_H
