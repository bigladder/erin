/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/distribution.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <random>

namespace erin::distribution
{
  // k = shape parameter, k > 0
  // a = scale parameter, a > 0, also called 'lambda'
  // b = location parameter, also called 'gamma'
  // 0 <= p < 1
  // reference: https://www.real-statistics.com/other-key-distributions/
  //    weibull-distribution/three-parameter-weibull-distribution/
  double
  weibull_quantile(const double& p, const double& k, const double& a, const double& b)
  {
    constexpr double highest_q{0.9999};
    double ans{0.0};
    if (p <= 0.0) {
      ans = b;
    }
    else {
      auto q{p};
      if (p >= 1.0) {
        q = highest_q;
      }
      ans = b + a * std::pow(-1.0 * std::log(1.0 - q), 1.0 / k);
    }
    return (ans < 0.0) ? 0.0 : ans;
  }

  double
  erfinv(double x)
  {
    /*
     * From "A handy approximation for the error function and its inverse"
     * by Sergei Winitzki February 6, 2008
     * a = 8887/63473
     * erfinv(x) ~= [
     *    ((-2) / (pi * a))
     *    - (ln(1 - x**2) / 2)
     *    + sqrt(
     *        ( (2 / (pi * a)) + (ln(1 - x**2) / 2) )**2
     *        - ((1/a) * ln(1 - x**2)))
     *    ] ** (1/2)
     *  OR
     *  A = C * (2.0 / pi)
     *  B = ln(1 - x**2)
     *  C = 1/a
     *  D = B / 2
     *  erfinv(x) ~= ((-A) + (-D) + sqrt((A + D)**2 - (C*B)))**0.5
     *
     *  domain is (-1, 1) but outliter values are allowed (they just get
     *  cropped)
     */
    constexpr double extent{3.0};
    constexpr double max_domain{1.0};
    if (x <= -max_domain) {
      return -extent;
    }
    if (x >= max_domain) {
      return extent;
    }
    constexpr double pi{3.1415'9265'3589'7932'3846'26433};
    constexpr double a{8'887.0/63'473.0};
    constexpr double C{1.0 / a};
    constexpr double two{2.0}; 
    constexpr double C_times_2{C * two};
    constexpr double A{C_times_2 / pi};
    double B = std::log(1.0 - (x * x));
    double D{B / two};
    double sum_A_D{A + D};
    double sum_A_D2{sum_A_D * sum_A_D};
    auto answer = std::sqrt((-A) + (-D) + std::sqrt(sum_A_D2 - (C*B)));
    if (x < 0.0) {
      answer = (-1.0) * answer;
    }
    if (answer > extent) {
      answer = extent;
    }
    if (answer < -extent) {
      answer = -extent;
    }
    return answer;
  }

  std::string
  dist_type_to_tag(DistType dist_type)
  {
    switch (dist_type) {
      case DistType::Fixed:
        return std::string{"fixed"};
      case DistType::Uniform:
        return std::string{"uniform"};
      case DistType::Normal:
        return std::string{"normal"};
      case DistType::Weibull:
        return std::string{"weibull"};
      case DistType::QuantileTable:
        return std::string{"table"};
    }
    std::ostringstream oss{};
    oss << "unhandled dist_type `" << static_cast<int>(dist_type) << "`";
    throw std::invalid_argument(oss.str());
  }

  DistType
  tag_to_dist_type(const std::string& tag)
  {
    if (tag == "fixed") {
      return DistType::Fixed;
    }
    if (tag == "uniform") {
      return DistType::Uniform;
    }
    if (tag == "normal") {
      return DistType::Normal;
    }
    if (tag == "weibull") {
      return DistType::Weibull;
    }
    if (tag == "quantile_table") {
      return DistType::QuantileTable;
    }
    std::ostringstream oss{};
    oss << "unhandled tag `" << tag << "` in tag_to_dist_type";
    throw std::invalid_argument(oss.str());
  }

  DistributionSystem::DistributionSystem():
    dist{},
    fixed_dist{},
    uniform_dist{},
    normal_dist{},
    quantile_table_dist{},
    weibull_dist{},
    g{},
    roll{0.0, 1.0}
  {
  }

  size_type
  DistributionSystem::add_fixed(
      const std::string& tag,
      RealTimeType value_in_seconds)
  {
    auto id{dist.tag.size()};
    auto subtype_id{fixed_dist.value.size()};
    fixed_dist.value.emplace_back(value_in_seconds);
    dist.tag.emplace_back(tag);
    dist.subtype_id.emplace_back(subtype_id);
    dist.dist_type.emplace_back(DistType::Fixed);
    return id;
  }

  size_type
  DistributionSystem::add_uniform(
      const std::string& tag,
      RealTimeType lower_bound_s,
      RealTimeType upper_bound_s)
  {
    if (lower_bound_s > upper_bound_s) {
      std::ostringstream oss{};
      oss << "lower_bound_s is greater than upper_bound_s\n"
          << "lower_bound_s: " << lower_bound_s << "\n"
          << "upper_bound_s: " << upper_bound_s << "\n";
      throw std::invalid_argument(oss.str());
    }
    auto id{dist.tag.size()};
    auto subtype_id{uniform_dist.lower_bound.size()};
    uniform_dist.lower_bound.emplace_back(lower_bound_s);
    uniform_dist.upper_bound.emplace_back(upper_bound_s);
    dist.tag.emplace_back(tag);
    dist.subtype_id.emplace_back(subtype_id);
    dist.dist_type.emplace_back(DistType::Uniform);
    return id;
  }

  size_type
  DistributionSystem::add_normal(
      const std::string& tag,
      RealTimeType mean_s,
      RealTimeType stddev_s)
  {
    auto id{dist.tag.size()};
    auto subtype_id{normal_dist.average.size()};
    normal_dist.average.emplace_back(mean_s);
    normal_dist.stddev.emplace_back(stddev_s);
    dist.tag.emplace_back(tag);
    dist.subtype_id.emplace_back(subtype_id);
    dist.dist_type.emplace_back(DistType::Normal);
    return id;
  }

  void
  ensure_sizes_equal(
      const std::string& tag, const size_type& a, const size_type& b)
  {
    if (a != b) {
      std::ostringstream oss{};
      oss << "tag `" << tag << "` not a valid tabular distribution.\n"
          << "xs.size() (" << a << ") must equal ("
          << "dtimes_s.size() (" << b << ")\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  ensure_size_greater_than_or_equal_to(
      const std::string& tag,
      const size_type& a,
      const size_type& n)
  {
    if (a < n) {
      std::ostringstream oss{};
      oss << "tag `" << tag << "` not a valid tabular distribution.\n"
          << "xs.size() (" << a << ") must be greater than 1\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  ensure_always_increasing(
      const std::string& tag,
      const std::vector<double>& xs)
  {
    bool first{true};
    double last{0.0};
    for (size_type idx{0}; idx < xs.size(); ++idx) {
      const auto& x{xs[idx]};
      if (first) {
        first = false;
        last = x;
      }
      else {
        if (x <= last) {
          std::ostringstream oss{};
          oss << "tag `" << tag << "` not a valid tabular distribution.\n"
              << "values must be always increasing\n";
          throw std::invalid_argument(oss.str());
        }
      }
    }
  }

  void
  ensure_for_all(
      const std::string& tag,
      const std::vector<double>& xs,
      const std::function<bool(double)>& f)
  {
    for (const auto& x : xs) {
      if (!f(x)) {
        std::ostringstream oss{};
        oss << "tag `" << tag << "` not valid.\n"
            << "issue for x == " << x << "\n";
        throw std::invalid_argument(oss.str());
      }
    }
  }

  void
  ensure_equals(
      const std::string& tag,
      const double x,
      const double val)
  {
    if (x != val) {
      std::ostringstream oss{};
      oss << tag << ": expected x to be equal to " << val << "\n"
          << ", but got x == " << x << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  ensure_greater_than_or_equal_to(const double x, const double val)
  {
    if (x < val) {
      std::ostringstream oss{};
      oss << "expected x to be greater than or equal to " << val << "\n"
          << ", but got x == " << x << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  ensure_greater_than(const double x, const double val)
  {
    if (x <= val) {
      std::ostringstream oss{};
      oss << "expected x to be greater than " << val << "\n"
          << ", but got x == " << x << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  void
  ensure_greater_than_zero(const double x)
  {
    ensure_greater_than(x, 0.0);
  }

  size_type
  DistributionSystem::add_quantile_table(
      const std::string& tag,
      const std::vector<double>& xs,
      const std::vector<double>& dtimes_s)
  {
    const size_type count{xs.size()};
    const size_type last_idx{(count == 0) ? 0 : (count - 1)};
    ensure_sizes_equal(tag, count, dtimes_s.size());
    ensure_size_greater_than_or_equal_to(tag, count, 2);
    ensure_always_increasing(tag, xs);
    ensure_always_increasing(tag, dtimes_s);
    ensure_equals(tag + "[0]", xs[0], 0.0);
    ensure_equals(tag + "[" + std::to_string(last_idx) + "]",
        xs[last_idx], 1.0);
    auto id{dist.tag.size()};
    auto subtype_id{quantile_table_dist.start_idx.size()};
    size_type start_idx{
      subtype_id == 0
      ? 0
      : (quantile_table_dist.end_idx[subtype_id - 1] + 1)};
    size_type end_idx{start_idx + count - 1};
    quantile_table_dist.start_idx.emplace_back(start_idx);
    quantile_table_dist.end_idx.emplace_back(end_idx);
    for (size_type i{0}; i < count; ++i) {
      quantile_table_dist.variates.emplace_back(xs[i]);
      quantile_table_dist.times.emplace_back(dtimes_s[i]);
    }
    dist.tag.emplace_back(tag);
    dist.subtype_id.emplace_back(subtype_id);
    dist.dist_type.emplace_back(DistType::QuantileTable);
    return id;
  }

  /*
  size_type DistributionSystem::add_pdf_table(
    const std::string& tag,
    const std::vector<double>& dtimes_s,
    const std::vector<double>& occurrences)
  {
    const size_type count{dtimes_s.size()};
    const size_type last_idx{(count == 0) ? 0 : (count - 1)};
    ensure_sizes_equal(tag, count, occurrences.size());
    ensure_size_greater_than_or_equal_to(tag, count, 2);
    ensure_always_increasing(tag, dtimes_s);
    ensure_for_all(tag, occurrences, [](double x)->bool {
      return x >= 0.0;
    });
    ensure_equals(tag + "[0]", occurrences[0], 0.0);
    const double total{std::accumulate(
      occurrences.begin(), occurrences.end(), 0.0)};
    ensure_greater_than_zero(total);
    std::vector<double> dist_xs{};
    std::vector<double> dist_ys{};
    double running_sum{0.0};
    for (std::vector<double>::size_type idx{0}; idx < count; ++idx) {
      const double& x = occurrences[idx];
      running_sum += x;
      dist_xs.emplace_back(dtimes_s[idx]);
      dist_ys.emplace_back(running_sum / total);
    }
    return add_quantile_table(tag, dist_ys, dist_xs);
  }
  */

  size_type
  DistributionSystem::add_weibull(
    const std::string& tag,
    const double shape_parameter,   // k
    const double scale_parameter,   // lambda
    const double location_parameter // gamma
  )
  {
    ensure_greater_than_zero(shape_parameter);
    ensure_greater_than_zero(scale_parameter);
    ensure_greater_than_or_equal_to(location_parameter, 0.0);
    auto id{dist.tag.size()};
    auto subtype_id{weibull_dist.shape_params.size()};
    weibull_dist.shape_params.emplace_back(shape_parameter);
    weibull_dist.scale_params.emplace_back(scale_parameter);
    weibull_dist.location_params.emplace_back(location_parameter);
    dist.tag.emplace_back(tag);
    dist.subtype_id.emplace_back(subtype_id);
    dist.dist_type.emplace_back(DistType::Weibull);
    return id;
  }

  size_type
  DistributionSystem::lookup_dist_by_tag(
      const std::string& tag) const
  {
    for (size_type i{0}; i < dist.tag.size(); ++i) {
      if (dist.tag[i] == tag) {
        return i;
      }
    }
    std::ostringstream oss{};
    oss << "tag `" << tag << "` not found in distribution list";
    throw std::invalid_argument(oss.str());
  }

  RealTimeType
  DistributionSystem::next_time_advance(size_type dist_id)
  {
    auto fraction = roll(g);
    return next_time_advance(dist_id, fraction);
  }

  RealTimeType
  DistributionSystem::next_time_advance(
      size_type dist_id,
      double fraction) const
  {
    if (dist_id >= dist.tag.size()) {
      std::ostringstream oss{};
      oss << "dist_id '" << dist_id << "' is out of range\n"
          << "- id     : " << dist_id << "\n"
          << "- max(id): " << (dist.tag.size() - 1) << "\n";
      throw std::out_of_range(oss.str());
    }
    const auto& subtype_id = dist.subtype_id.at(dist_id);
    const auto& dist_type = dist.dist_type.at(dist_id);
    RealTimeType dt{0};
    switch (dist_type) {
      case DistType::Fixed:
        {
          dt = fixed_dist.value.at(subtype_id);
          break;
        }
      case DistType::Uniform:
        {
          auto lb = uniform_dist.lower_bound.at(subtype_id);
          auto ub = uniform_dist.upper_bound.at(subtype_id);
          auto delta = ub - lb;
          dt = static_cast<RealTimeType>(
              fraction * static_cast<FlowValueType>(delta)
              + static_cast<FlowValueType>(lb));
          break;
        }
      case DistType::Normal:
        {
          constexpr double sqrt2{1.4142'1356'2373'0951};
          constexpr double twice{2.0};
          auto avg = static_cast<double>(normal_dist.average.at(subtype_id));
          auto sd  = static_cast<double>(normal_dist.stddev.at(subtype_id));
          dt = static_cast<RealTimeType>(
              std::round(
                avg + sd * sqrt2 * erfinv(twice * fraction - 1.0)));
          break;
        }
      case DistType::QuantileTable:
        {
          const auto& start_idx = quantile_table_dist.start_idx[subtype_id];
          const auto& end_idx = quantile_table_dist.end_idx[subtype_id];
          if (fraction >= 1.0) {
            dt = static_cast<RealTimeType>(
                std::round(quantile_table_dist.times[end_idx]));
          }
          else {
            for (size_type idx{start_idx}; idx < end_idx; ++idx) {
              const auto& v0 = quantile_table_dist.variates[idx];
              const auto& v1 = quantile_table_dist.variates[idx + 1];
              if ((fraction >= v0) && (fraction < v1)) {
                if (fraction == v0) {
                  dt = static_cast<RealTimeType>(
                      std::round(quantile_table_dist.times[idx]));
                  break;
                }
                else {
                  const auto df{fraction - v0};
                  const auto dv{v1 - v0};
                  const auto time0{quantile_table_dist.times[idx]};
                  const auto time1{quantile_table_dist.times[idx+1]};
                  const auto dtimes{time1 - time0};
                  dt = static_cast<RealTimeType>(
                      std::round(time0 + (df/dv) * dtimes));
                }
              }
            }
          }
          break;
        }
      case DistType::Weibull:
        {
          const auto& k = weibull_dist.shape_params[subtype_id];
          const auto& a = weibull_dist.scale_params[subtype_id];
          const auto& b = weibull_dist.location_params[subtype_id];
          dt = static_cast<RealTimeType>(
              std::round(weibull_quantile(fraction, k, a, b)));
          break;
        }
      default:
        {
          throw std::runtime_error("unhandled Cumulative Density Function");
        }
    }
    if (dt < 0) {
      dt = 0;
    }
    return dt;
  }

  //std::vector<RealTimeType>
  //DistributionSystem::sample_upto_including(
  //    const RealTimeType max_time_s)
  //{
  //  std::vector<RealTimeType> samples{};
  //  RealTimeType t{0};
  //  while (true) {

  //  }
  //}
}
