/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_FRAGILITY_H
#define ERIN_FRAGILITY_H
#include <memory>

namespace erin::fragility
{
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

  class Linear : public Curve
  {
    public:
      Linear() = delete;
      Linear(double lower_bound, double upper_bound);

      virtual std::unique_ptr<Curve> clone() const override;
      double apply(double x) const override;

    private:
      double lower_bound;
      double upper_bound;
      double range;
  };
}

#endif // ERIN_FRAGILITY_H
