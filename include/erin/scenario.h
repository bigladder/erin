/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_SCENARIO_H
#define ERIN_SCENARIO_H
#include "erin/type.h"
#include <string>
#include <iostream>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // Scenario
  class Scenario
  {
    public:
      Scenario(
          std::string name,
          std::string network_id,
          RealTimeType duration_in_seconds,
          int max_occurrences,
          size_type occurrence_distribution_id,
          std::unordered_map<std::string, double> intensities,
          bool calc_reliability);

      [[nodiscard]] const std::string& get_name() const { return name; }
      [[nodiscard]] const std::string& get_network_id() const {
        return network_id;
      }
      [[nodiscard]] RealTimeType get_duration() const { return duration; }
      [[nodiscard]] int get_max_occurrences() const { return max_occurrences; }
      [[nodiscard]] size_type get_occurrence_distribution_id() const {
        return occurrence_distribution_id;
      }
      [[nodiscard]] int get_number_of_occurrences() const {
        return num_occurrences;
      }
      [[nodiscard]] const std::unordered_map<std::string,double>&
        get_intensities() const { return intensities; }
      [[nodiscard]] bool get_calc_reliability() const {
        return calc_reliability;
      }

      friend std::ostream& operator<<(std::ostream& os, const Scenario& s);
      friend bool operator==(const Scenario& a, const Scenario& b);
      friend bool operator!=(const Scenario& a, const Scenario& b);

    private:
      std::string name;
      std::string network_id;
      RealTimeType duration;
      int max_occurrences;
      size_type occurrence_distribution_id;
      std::unordered_map<std::string, double> intensities;
      int num_occurrences;
      bool calc_reliability;
  };

  std::ostream& operator<<(std::ostream& os, const Scenario& s);
  bool operator==(const Scenario& a, const Scenario& b);
  bool operator!=(const Scenario& a, const Scenario& b);
}

#endif // ERIN_SCENARIO_H