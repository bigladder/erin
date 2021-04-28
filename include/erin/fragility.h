/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_FRAGILITY_H
#define ERIN_FRAGILITY_H
#include "erin/reliability.h"
#include "erin/distribution.h"
#include "erin/type.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace erin::fragility
{

  /**
   * Curve types available.
   */
  enum class CurveType
  {
    Linear = 0
  };

  CurveType tag_to_curve_type(const std::string& tag);
  std::string curve_type_to_tag(CurveType type);

  /**
   * A Fragility Curve that yields the chance of failure as a number
   * (0 <= n <= 1) given some intensity (a double).
   */
  class Curve
  {
    public:
      Curve() = default;
      Curve(const Curve&) = delete;
      Curve& operator=(const Curve&) = delete;
      Curve(Curve&&) = delete;
      Curve& operator=(Curve&&) = delete;
      virtual ~Curve() = default;

      [[nodiscard]] virtual std::unique_ptr<Curve> clone() const = 0;
      virtual double apply(double x) const = 0;
      [[nodiscard]] virtual CurveType get_curve_type() const = 0;
      [[nodiscard]] virtual std::string str() const = 0;
  };

  /**
   * A class to implement a linear fragility curve that starts at 0% chance of
   * failure from -infinity to the lower_bound, varies linearly from 0% to
   * 100% from lower to upper bound, and 100% chance of failure for values of
   * intensity at or above the upper bound.
   */
  class Linear : public Curve
  {
    public:
      Linear() = delete;
      Linear(double lower_bound, double upper_bound);

      [[nodiscard]] std::unique_ptr<Curve> clone() const override;
      double apply(double x) const override;
      [[nodiscard]] CurveType get_curve_type() const override {
        return CurveType::Linear;
      }
      [[nodiscard]] std::string str() const override;
      [[nodiscard]] double get_lower_bound() const { return lower_bound; }
      [[nodiscard]] double get_upper_bound() const { return upper_bound; }

    private:
      double lower_bound;
      double upper_bound;
      double range;
  };

  /**
   * A class to check for failures over multiple probabilities of failure.
   */
  class FailureChecker
  {
    public:
      FailureChecker();
      explicit FailureChecker(const std::default_random_engine& g); 

      /**
       * assess whether a component is failed based on the vector of probabilities of failure.
       * @param probs vector of probability of failure. Each failure probability must be 0<=p<=1
       * @returns true if is failed, else false
       */
      bool is_failed(const std::vector<double>& probs);

    private:
      std::default_random_engine gen;
      std::uniform_real_distribution<double> dist;
  };

  /**
   * Structure to hold the curve and the intensity it applies to (i.e., is vulnerable to).
   */
  struct FragilityCurve
  {
    std::string vulnerable_to;
    std::unique_ptr<Curve> curve;
  };

  constexpr std::int64_t no_repair_distribution{-1};

  struct FragilityMode
  {
    std::string fragility_curve_tag{};
    std::int64_t repair_dist_id{no_repair_distribution};
  };

  /**
   * modify a reliability schedule with a fragility
   * NOTE: a repair_time_s of 0 indicates no repair (NOT an instant repair)
   */
  std::vector<ERIN::TimeState>
  modify_schedule_for_fragility(
      const std::vector<ERIN::TimeState>& schedule,
      bool is_failed,
      bool can_repair,
      ERIN::RealTimeType repair_time_s,
      ERIN::RealTimeType max_time_s);

  struct FragilityInfo
  {
    std::string scenario_tag{};
    // ERIN::RealTimeType start_time_s{0};
    // bool is_failed{false};
    // ERIN::RealTimeType repair_time_s{-1}; // values less than 0 indicate cannot repair
  };

  std::unordered_map<std::string, std::vector<std::unordered_map<std::string, FragilityInfo>>>
  calc_fragility_schedules(
    const std::unordered_map<std::string, FragilityMode> fragility_modes,
    const std::unordered_map<std::string, std::vector<std::int64_t>>& scenario_schedules,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<double>>>& failure_probs_by_comp_id_by_scenario_id,
    const std::function<double()>& rand_fn,
    const erin::distribution::DistributionSystem& ds);
}

#endif // ERIN_FRAGILITY_H
