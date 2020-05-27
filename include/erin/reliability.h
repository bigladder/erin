/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_RELIABILITY_H
#define ERIN_RELIABILITY_H
#include <string>
#include <unordered_map>
#include <vector>

namespace ERIN
{
  using size_type = std::vector<std::int64_t>::size_type;
  constexpr size_type DISTRIBUTION_TYPE_FIXED{0};

  struct TimeState
  {
    std::int64_t time{0};
    bool state{true};
  };

  bool operator==(const TimeState& a, const TimeState& b);
  bool operator!=(const TimeState& a, const TimeState& b);

  std::unordered_map<size_type, std::vector<TimeState>>
  calc_reliability_schedule(
      const std::vector<size_type>& component_id_to_failure_mode_distribution_id,
      const std::vector<size_type>& component_id_to_repair_mode_distribution_id,
      const std::vector<size_type>& distribution_id_to_distribution_type,
      const std::vector<size_type>& distribution_id_to_distribution_type_id,
      const std::vector<std::int64_t>& fixed_distribution_attr_value
      );
}

#endif // ERIN_RELIABILITY_H
