/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_FRAGILITY_H
#define ERIN_FRAGILITY_H

namespace erin::fragility
{
  class Linear
  {
    public:
      Linear(double lower_bound, double upper_bound);

      double operator()(double x) const;

    private:
      double lower_bound;
      double upper_bound;
      double range;
  };
}

#endif // ERIN_FRAGILITY_H
