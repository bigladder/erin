/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/distribution.h"
#include <iostream>

int
main() {
  namespace ED = erin::distribution;
  auto cds = ED::CumulativeDistributionSystem();
  auto id = cds.add_normal_cdf("normal", 0.0, 1'000.0);
  int y_{0};
  int dy_{1};
  double x{0.0};
  double y{0.0};
  constexpr int max_y_{10'000};
  constexpr double max_y{static_cast<double>(max_y_)};
  std::cout << "x,y\n";
  while (y_ <= max_y_) {
    y = static_cast<double>(y_) / max_y;
    x = cds.next_time_advance(id, y);
    y_ += dy_;
    std::cout << x << "," << y << "\n";
  }
  return 0;
}
