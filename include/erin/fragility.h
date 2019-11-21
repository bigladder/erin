/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_FRAGILITY_H
#define ERIN_FRAGILITY_H
#include <memory>
#include <random>

namespace erin::fragility
{
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
      virtual ~Curve() = default;

      virtual std::unique_ptr<Curve> clone() const = 0;
      virtual double apply(double x) const = 0;
  };

  /**
   * A class to implement a linear fragility curve that starts at 0% change of
   * failure from -infinity to the lower_bound, various linearly from 0% to
   * 100% from lower to upper bound, and 100% for values of intensity at or
   * above the upper bound.
   */
  class Linear : public Curve
  {
    public:
      Linear() = delete;
      Linear(double lower_bound, double upper_bound);

      std::unique_ptr<Curve> clone() const override;
      double apply(double x) const override;

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
}

#endif // ERIN_FRAGILITY_H
