/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/scenario.h"

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // Scenario
  Scenario::Scenario(
      std::string name_,
      std::string network_id_,
      RealTimeType duration_,
      int max_occurrences_,
      ERIN::size_type occurrence_dist_id_,
      std::unordered_map<std::string, double> intensities_,
      bool calc_reliability_):
    name{std::move(name_)},
    network_id{std::move(network_id_)},
    duration{duration_},
    max_occurrences{max_occurrences_},
    occurrence_distribution_id{occurrence_dist_id_},
    intensities{std::move(intensities_)},
    num_occurrences{0},
    calc_reliability{calc_reliability_}
  {
  }

  std::ostream&
  operator<<(std::ostream& os, const Scenario& s)
  {
    os << "Scenario("
       << "name=\"" << s.name << "\", "
       << "network_id=\"" << s.network_id << "\", "
       << "duration=" << s.duration << ", "
       << "max_occurrences=" << s.max_occurrences << ", "
       << "occurrence_distribution_id=" << s.occurrence_distribution_id << ", "
       << "intensities=..., "
       << "num_occurrences=" << s.num_occurrences << ", "
       << "calc_reliability=" << (s.calc_reliability ? "true" : "false")
       << ")";
    return os;
  }

  bool
  operator==(const Scenario& a, const Scenario& b)
  {
    return (a.name == b.name) &&
           (a.network_id == b.network_id) &&
           (a.duration == b.duration) &&
           (a.max_occurrences == b.max_occurrences) &&
           (a.occurrence_distribution_id == b.occurrence_distribution_id) &&
           (a.intensities == b.intensities) &&
           (a.calc_reliability == b.calc_reliability);
  }

  bool
  operator!=(const Scenario& a, const Scenario& b)
  {
    return !(a == b);
  }



}