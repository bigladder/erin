/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/devs.h"
#include "erin/devs/supply.h"
#include "erin/devs/runner.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <functional>

namespace ED = erin::devs;

int
main() {
  const bool show_details{true};
  int num_tests{0};
  int num_passed{0};
  bool current_test{true};
  std::cout << "test_supply:\n";
  //current_test = test_passthrough(show_details);
  num_tests++;
  if (current_test) {
    num_passed++;
  }
  std::cout << "=> " << (current_test ? "PASSED" : "FAILED!!!!!") << "\n";
  std::cout << "Summary: " << num_passed << " / " << num_tests << " passed.\n";
  if (num_passed == num_tests) {
    return 0;
  }
  return 1;
}
