/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DISTRIBUTIONS_H
#define ERIN_DISTRIBUTIONS_H
#include <chrono>
#include <exception>
#include <functional>
#include <random>
#include <sstream>

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
      FixedDistribution(const T& v): Distribution<T>(), value{v} {}

      // get the next value (which will be fixed).
      // @return the next value
      T next_value() override { return value; }

    private:
      T value;
  };

  ////////////////////////////////////////////////////////////
  // RandomIntegerDistribution
  template <class T>
  class RandomIntegerDistribution : public Distribution<T>
  {
    public:
      // construct a FixedDistribution object
      // @param lb the lower bound of type T
      // @param ub the upper bound of type T
      RandomIntegerDistribution(const T& lb, const T& ub):
        Distribution<T>(),
        lower_bound{lb},
        upper_bound{ub},
        generator{},
        dist{lb,ub},
        func{std::bind(dist, generator)}
      {
        if (lb >= ub) {
          std::ostringstream oss{};
          oss << "expected lower_bound < upper_bound but lower_bound = "
              << lb << " and upper_bound = " << ub;
          throw std::invalid_argument(oss.str());
        }
      }

      // get the next value which will be random in the range
      // (lower_bound, upper_bound).
      // @return the next value
      T next_value() override {
        return func();
      }

    private:
      T lower_bound;
      T upper_bound;
      std::default_random_engine generator;
      std::uniform_int_distribution<T> dist;
      std::function<T(void)> func;
  };
}
#endif // ERIN_DISTRIBUTIONS_H
