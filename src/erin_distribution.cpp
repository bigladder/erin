/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/distribution.h"
#include <stdexcept>

namespace erin::distribution
{
  std::string
  cdf_type_to_tag(CdfType cdf_type)
  {
    switch (cdf_type) {
      case CdfType::Fixed:
        return std::string{"fixed"};
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
    std::ostringstream oss{};
    oss << "unhandled tag `" << tag << "` in tag_to_cdf_type";
    throw std::invalid_argument(oss.str());
  }

  CumulativeDistributionSystem::CumulativeDistributionSystem():
    cdf{},
    fixed_cdf{}
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

  RealTimeType
  CumulativeDistributionSystem::next_time_advance(size_type cdf_id)
  {
    const auto& subtype_id = cdf.subtype_id.at(cdf_id);
    const auto& cdf_type = cdf.cdf_type.at(cdf_id);
    RealTimeType dt{0};
    switch (cdf_type) {
      case CdfType::Fixed:
        {
          dt = fixed_cdf.value.at(subtype_id);
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
