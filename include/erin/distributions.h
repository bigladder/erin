/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DISTRIBUTIONS_H
#define ERIN_DISTRIBUTIONS_H

namespace erin::dist
{
  ////////////////////////////////////////////////////////////
  // Distribution
  template <class T>
  class Distribution
  {
    public:
      // return the next value of type T
      // @return the next value
      virtual T next_value() = 0;
      // a virtual destructor
      //
      virtual ~Distribution() = default;
  };

  ////////////////////////////////////////////////////////////
  // FixedDistribution
  template <class T>
  class FixedDistribution : public Distribution<T>
  {
    public:
      // construct a FixedDistribution object
      // @param v a constant reference to a value of type T
      FixedDistribution(const T& v): value{v} {}

      // get the next value (which will be fixed).
      // @return the next value
      T next_value() override { return value; }

    private:
      T value;
  };
}
#endif // ERIN_DISTRIBUTIONS_H
