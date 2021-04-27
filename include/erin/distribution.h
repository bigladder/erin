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
  using FlowValueType = ERIN::FlowValueType;

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

  enum class DistType
  {
    Fixed = 0,
    Uniform,
    Normal,
    Weibull,
    QuantileTable // from times and variate: variate is from (0,1) time and variate
                  // must be always increasing
  };

  std::string dist_type_to_tag(DistType dist_type);
  DistType tag_to_dist_type(const std::string& tag);

  struct Dist {
    std::vector<std::string> tag{};
    std::vector<size_type> subtype_id{};
    std::vector<DistType> dist_type{};
  };

  struct FixedDist
  {
    std::vector<RealTimeType> value{};
  };

  struct UniformDist
  {
    std::vector<RealTimeType> lower_bound{};
    std::vector<RealTimeType> upper_bound{};
  };

  struct NormalDist
  {
    std::vector<RealTimeType> average{};
    std::vector<RealTimeType> stddev{};
  };

  struct QuantileTableDist
  {
    std::vector<double> variates{};
    std::vector<double> times{};
    std::vector<size_type> start_idx{};
    std::vector<size_type> end_idx{};
  };

  struct WeibullDist
  {
    std::vector<double> shape_params{}; // k
    std::vector<double> scale_params{}; // lambda
    std::vector<double> location_params{}; // gamma
  };

  class DistributionSystem
  {
    public:
      DistributionSystem();

      size_type add_fixed(
          const std::string& tag,
          RealTimeType value_in_seconds);

      size_type add_uniform(
          const std::string& tag,
          RealTimeType lower_bound_s,
          RealTimeType upper_bound_s);

      size_type add_normal(
          const std::string& tag,
          RealTimeType mean_s,
          RealTimeType stddev_s);

      size_type add_quantile_table(
          const std::string& tag,
          const std::vector<double>& xs,
          const std::vector<double>& dtimes_s);

      /*
      size_type add_pdf_table(
        const std::string& tag,
        const std::vector<double>& dtimes_s,
        const std::vector<double>& occurrences
        );
      */

      size_type add_weibull(
          const std::string& tag,
          const double shape_parameter,    // k
          const double scale_parameter,    // lambda
          const double location_parameter=0.0); // gamma

      [[nodiscard]] size_type lookup_dist_by_tag(const std::string& tag) const;

      RealTimeType next_time_advance(size_type dist_id);

      RealTimeType next_time_advance(size_type dist_id, double fraction) const;

      //[[nodiscard]] std::vector<RealTimeType>
      //  sample_upto_including(const RealTimeType max_time_s);

    private:
      Dist dist;
      FixedDist fixed_dist;
      UniformDist uniform_dist;
      NormalDist normal_dist;
      QuantileTableDist quantile_table_dist;
      WeibullDist weibull_dist;
      std::default_random_engine g;
      std::uniform_real_distribution<double> roll;
  };
}
#endif // ERIN_DISTRIBUTION_H
