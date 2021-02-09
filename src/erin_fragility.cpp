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
      ERIN::RealTimeType repair_time_s)
  {
    if (is_failed) {
    }
    return schedule;
  }
}
