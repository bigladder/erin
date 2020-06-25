/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/distribution.h"
#include <stdexcept>
#include <random>

namespace erin::distribution
{
  std::string
  cdf_type_to_tag(CdfType cdf_type)
  {
    switch (cdf_type) {
      case CdfType::Fixed:
        return std::string{"fixed"};
      case CdfType::Uniform:
        return std::string{"uniform"};
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
    std::ostringstream oss{};
    oss << "unhandled tag `" << tag << "` in tag_to_cdf_type";
    throw std::invalid_argument(oss.str());
  }

  CumulativeDistributionSystem::CumulativeDistributionSystem():
    cdf{},
    fixed_cdf{},
    uniform_cdf{},
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
      oss << "cdf_id `" << cdf_id << "` is out of range\n";
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
      default:
        {
          throw std::runtime_error("unhandled Cumulative Density Function");
        }
    }
    return dt;
  }
}
