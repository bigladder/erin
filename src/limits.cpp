/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */
#include <iostream>
#include <limits>

int
main()
{
  std::cout << "sizeof(int)                     : " << sizeof(int) << "\n";
  std::cout << "sizeof(long)                    : " << sizeof(long) << "\n";
  std::cout << "std::numeric_limits<int>::max() : "
            << std::numeric_limits<int>::max() << "\n";
  std::cout << "std::numeric_limits<long>::max(): "
            << std::numeric_limits<long>::max() << "\n";
  return 0;
}
