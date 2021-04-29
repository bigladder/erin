/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include "erin/fragility.h"
#include <sstream>
#include <exception>

namespace erin::fragility
{

  CurveType
  tag_to_curve_type(const std::string& tag)
  {
    if (tag == "linear") {
      return CurveType::Linear;
    }
    std::ostringstream oss;
    oss << "unhandled curve type tag '" << tag << "'";
    throw std::invalid_argument(oss.str());
  }

  std::string
  curve_type_to_tag(CurveType t)
  {
    switch (t) {
      case CurveType::Linear:
        return std::string{"linear"};
    }
    std::ostringstream oss;
    oss << "unhandled curve type '" << static_cast<int>(t) << "'";
    throw std::runtime_error(oss.str());
  }

  Linear::Linear(
      double lower_bound_,
      double upper_bound_):
    Curve(),
    lower_bound{lower_bound_},
    upper_bound{upper_bound_},
    range{upper_bound_ - lower_bound_}
  {
    if (lower_bound >= upper_bound) {
      std::ostringstream oss;
      oss << "lower_bound >= upper_bound\n";
      oss << "\tupper_bound: " << upper_bound << "\n";
      oss << "\tlower_bound: " << lower_bound << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  std::unique_ptr<Curve>
  Linear::clone() const
  {
    std::unique_ptr<Curve> p =
      std::make_unique<Linear>(lower_bound, upper_bound);
    return p;
  }

  double
  Linear::apply(double x) const
  {
    if (x <= lower_bound) {
      return 0.0;
    }
    else if (x >= upper_bound) {
      return 1.0;
    }
    auto dx{x - lower_bound};
    return dx / range;
  }

  std::string
  Linear::str() const
  {
    std::ostringstream oss;
    oss << "Linear(lower_bound=" << lower_bound
        << ",upper_bound=" << upper_bound << ")";
    return oss.str();
  }

  FailureChecker::FailureChecker():
    FailureChecker(std::default_random_engine{})
  {
  }

  FailureChecker::FailureChecker(
      const std::default_random_engine& g_):
    gen{g_},
    dist{0.0, 1.0}
  {
  }

  bool
  FailureChecker::is_failed(const std::vector<double>& probs)
  {
    if (probs.empty()) {
      return false;
    }
    for (const auto p: probs) {
      if (p >= 1.0) {
        return true;
      }
      if (p <= 0.0) {
        continue;
      }
      auto roll = dist(gen);
      if (roll <= p) {
        return true;
      }
    }
    return false;
  }

  std::vector<ERIN::TimeState>
  modify_schedule_for_fragility(
      const std::vector<ERIN::TimeState>& schedule,
      bool is_failed,
      bool can_repair,
      ERIN::RealTimeType repair_time_s,
      ERIN::RealTimeType max_time_s)
  {
    if (is_failed) {
      auto new_sch = std::vector<ERIN::TimeState>{{0,0}};
      if (can_repair && (repair_time_s <= max_time_s)) {
        new_sch.emplace_back(ERIN::TimeState{repair_time_s, 1});
        bool start_appending{false};
        for (const auto& item : schedule) {
          start_appending = start_appending ||
            ((item.time > repair_time_s) && (item.time <= max_time_s) && !item.state);
          if (start_appending) {
            new_sch.emplace_back(item);
          }
        }
      }
      return new_sch;
    }
    return schedule;
  }

  std::ostream&
  operator<<(std::ostream& os, const FragilityInfo& fi)
  {
    return os << "{"
              << ":scenario-tag " << fi.scenario_tag
              << " "
              << ":start-time-s " << fi.start_time_s
              << " "
              << ":start-time-h " << (fi.start_time_s / 3600LL)
              << " "
              << ":start-time-d " << (fi.start_time_s / (3600LL * 24LL))
              << " "
              << ":start-time-y " << (fi.start_time_s / (3600LL * 8760LL))
              << " "
              << ":is-failed? " << fi.is_failed
              << "}";
  }

  std::unordered_map<std::string, std::vector<std::unordered_map<std::string, FragilityInfo>>>
  calc_fragility_schedules(
    const std::unordered_map<std::string, FragilityMode> fragility_modes,
    const std::unordered_map<std::string, std::vector<std::int64_t>>& scenario_schedules,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<double>>>& failure_probs_by_comp_id_by_scenario_id,
    const std::function<double()>& rand_fn,
    const erin::distribution::DistributionSystem& ds)
  {
    std::unordered_map<std::string,std::vector<std::unordered_map<std::string,FragilityInfo>>> out{};
    for (const auto& ss : scenario_schedules) {
      const auto& scenario_tag{ss.first};
      const auto& fpbc = failure_probs_by_comp_id_by_scenario_id.at(scenario_tag);
      std::vector<std::unordered_map<std::string, FragilityInfo>> info{};
      for (const auto& start_time_s : ss.second) {
        if (start_time_s < 0) {
          std::ostringstream oss{};
          oss << "WARNING: calc_fragility_schedules(..., ...)\n"
              << "start_time_s is negative: "
              << start_time_s
              << "\n";
          throw std::runtime_error(oss.str());
        }
        std::unordered_map<std::string, FragilityInfo> comp_frag_info{};
        for (const auto& comp_probs : fpbc) {
          const auto& comp_id = comp_probs.first;
          const auto& failure_probs = comp_probs.second;
          bool is_failed{false};
          for (const auto& p : failure_probs) {
            if (p >= 1.0) {
              is_failed = true;
              break;
            }
            else if (p <= 0.0) {
              continue;
            }
            else {
              if (rand_fn() <= p) {
                is_failed = true;
                break;
              }
            }
          }
          comp_frag_info[comp_id] = FragilityInfo{
            scenario_tag,
            start_time_s,
            is_failed};
        }
        info.emplace_back(comp_frag_info);
      }
      out[scenario_tag] = info;
    }
    return out;
  }
}
