/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/distribution.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <random>

namespace erin::distribution
{
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
  cdf_type_to_tag(CdfType cdf_type)
  {
    switch (cdf_type) {
      case CdfType::Fixed:
        return std::string{"fixed"};
      case CdfType::Uniform:
        return std::string{"uniform"};
      case CdfType::Normal:
        return std::string{"normal"};
      case CdfType::Table:
        return std::string{"table"};
    }
    std::ostringstream oss{};
    oss << "unhandled cdf_type `" << static_cast<int>(cdf_type) << "`";
    throw std::invalid_argument(oss.str());
  }

  CdfType
  tag_to_cdf_type(const std::string& tag)
  {
    if (tag == "fixed") {
      return CdfType::Fixed;
    }
    if (tag == "uniform") {
      return CdfType::Uniform;
    }
    if (tag == "normal") {
      return CdfType::Normal;
    }
    if (tag == "table") {
      return CdfType::Table;
    }
    std::ostringstream oss{};
    oss << "unhandled tag `" << tag << "` in tag_to_cdf_type";
    throw std::invalid_argument(oss.str());
  }

  CumulativeDistributionSystem::CumulativeDistributionSystem():
    cdf{},
    fixed_cdf{},
    uniform_cdf{},
    normal_cdf{},
    table_cdf{},
    g{},
    dist{0.0, 1.0}
  {
  }

  size_type
  CumulativeDistributionSystem::add_fixed_cdf(
      const std::string& tag,
      RealTimeType value_in_seconds)
  {
    auto id{cdf.tag.size()};
    auto subtype_id{fixed_cdf.value.size()};
    fixed_cdf.value.emplace_back(value_in_seconds);
    cdf.tag.emplace_back(tag);
    cdf.subtype_id.emplace_back(subtype_id);
    cdf.cdf_type.emplace_back(CdfType::Fixed);
    return id;
  }

  size_type
  CumulativeDistributionSystem::add_uniform_cdf(
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
    auto id{cdf.tag.size()};
    auto subtype_id{uniform_cdf.lower_bound.size()};
    uniform_cdf.lower_bound.emplace_back(lower_bound_s);
    uniform_cdf.upper_bound.emplace_back(upper_bound_s);
    cdf.tag.emplace_back(tag);
    cdf.subtype_id.emplace_back(subtype_id);
    cdf.cdf_type.emplace_back(CdfType::Uniform);
    return id;
  }

  size_type
  CumulativeDistributionSystem::add_normal_cdf(
      const std::string& tag,
      RealTimeType mean_s,
      RealTimeType stddev_s)
  {
    auto id{cdf.tag.size()};
    auto subtype_id{normal_cdf.average.size()};
    normal_cdf.average.emplace_back(mean_s);
    normal_cdf.stddev.emplace_back(stddev_s);
    cdf.tag.emplace_back(tag);
    cdf.subtype_id.emplace_back(subtype_id);
    cdf.cdf_type.emplace_back(CdfType::Normal);
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
  CumulativeDistributionSystem::add_table_cdf(
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
    auto id{cdf.tag.size()};
    auto subtype_id{table_cdf.start_idx.size()};
    size_type start_idx{
      subtype_id == 0
      ? 0
      : (table_cdf.end_idx[subtype_id - 1] + 1)};
    size_type end_idx{start_idx + count - 1};
    table_cdf.start_idx.emplace_back(start_idx);
    table_cdf.end_idx.emplace_back(end_idx);
    for (size_type i{0}; i < count; ++i) {
      table_cdf.variates.emplace_back(xs[i]);
      table_cdf.times.emplace_back(dtimes_s[i]);
    }
    cdf.tag.emplace_back(tag);
    cdf.subtype_id.emplace_back(subtype_id);
    cdf.cdf_type.emplace_back(CdfType::Table);
    return id;
  }

  size_type
  CumulativeDistributionSystem::lookup_cdf_by_tag(
      const std::string& tag) const
  {
    for (size_type i{0}; i < cdf.tag.size(); ++i) {
      if (cdf.tag[i] == tag) {
        return i;
      }
    }
    std::ostringstream oss{};
    oss << "tag `" << tag << "` not found in CDF list";
    throw std::invalid_argument(oss.str());
  }

  RealTimeType
  CumulativeDistributionSystem::next_time_advance(size_type cdf_id)
  {
    auto fraction = dist(g);
    return next_time_advance(cdf_id, fraction);
  }

  RealTimeType
  CumulativeDistributionSystem::next_time_advance(
      size_type cdf_id,
      double fraction) const
  {
    if (cdf_id >= cdf.tag.size()) {
      std::ostringstream oss{};
      oss << "cdf_id '" << cdf_id << "' is out of range\n"
          << "- id     : " << cdf_id << "\n"
          << "- max(id): " << (cdf.tag.size() - 1) << "\n";
      throw std::out_of_range(oss.str());
    }
    const auto& subtype_id = cdf.subtype_id.at(cdf_id);
    const auto& cdf_type = cdf.cdf_type.at(cdf_id);
    RealTimeType dt{0};
    switch (cdf_type) {
      case CdfType::Fixed:
        {
          dt = fixed_cdf.value.at(subtype_id);
          break;
        }
      case CdfType::Uniform:
        {
          auto lb = uniform_cdf.lower_bound.at(subtype_id);
          auto ub = uniform_cdf.upper_bound.at(subtype_id);
          auto delta = ub - lb;
          dt = static_cast<RealTimeType>(fraction * delta + lb);
          break;
        }
      case CdfType::Normal:
        {
          constexpr double sqrt2{1.4142'1356'2373'0951};
          constexpr double twice{2.0};
          auto avg = static_cast<double>(normal_cdf.average.at(subtype_id));
          auto sd  = static_cast<double>(normal_cdf.stddev.at(subtype_id));
          dt = static_cast<RealTimeType>(
              std::round(
                avg + sd * sqrt2 * erfinv(twice * fraction - 1.0)));
          break;
        }
      case CdfType::Table:
        {
          const auto& start_idx = table_cdf.start_idx[subtype_id];
          const auto& end_idx = table_cdf.end_idx[subtype_id];
          if (fraction >= 1.0) {
            dt = static_cast<RealTimeType>(
                std::round(table_cdf.times[end_idx]));
          }
          else {
            for (size_type idx{start_idx}; idx < end_idx; ++idx) {
              const auto& v0 = table_cdf.variates[idx];
              const auto& v1 = table_cdf.variates[idx + 1];
              if ((fraction >= v0) && (fraction < v1)) {
                if (fraction == v0) {
                  dt = static_cast<RealTimeType>(
                      std::round(table_cdf.times[idx]));
                  break;
                }
                else {
                  const auto df{fraction - v0};
                  const auto dv{v1 - v0};
                  const auto time0{table_cdf.times[idx]};
                  const auto time1{table_cdf.times[idx+1]};
                  const auto dtimes{time1 - time0};
                  dt = static_cast<RealTimeType>(
                      std::round(time0 + (df/dv) * dtimes));
                }
              }
            }
          }
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
  //CumulativeDistributionSystem::sample_upto_including(
  //    const RealTimeType max_time_s)
  //{
  //  std::vector<RealTimeType> samples{};
  //  RealTimeType t{0};
  //  while (true) {

  //  }
  //}
}
