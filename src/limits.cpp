/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include <iostream>
#include <limits>

int
main()
{
  std::cout << "sizeof(int)                     : " << sizeof(int) << "\n";
  std::cout << "sizeof(long)                    : " << sizeof(long) << "\n";
  std::cout << "sizeof(long long)               : " << sizeof(long long) << "\n";
  std::cout << "sizeof(double)                  : " << sizeof(double) << "\n";
  std::cout << "std::numeric_limits<int>::max() : "
            << std::numeric_limits<int>::max() << "\n";
  std::cout << "std::numeric_limits<long>::max(): "
            << std::numeric_limits<long>::max() << "\n";
  std::cout << "std::numeric_limits<long long>::max(): "
            << std::numeric_limits<long long>::max() << "\n";
  std::cout << "std::numeric_limits<double>::max(): "
            << std::numeric_limits<double>::max() << "\n";
  return 0;
}
