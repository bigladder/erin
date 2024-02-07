/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#include <iostream>
#include <stdint.h>
#include <limits>

int
main()
{
  std::cout << "sizeof(int)                     : " << sizeof(int) << "\n";
  std::cout << "sizeof(long)                    : " << sizeof(long) << "\n";
  std::cout << "sizeof(long long)               : " << sizeof(long long) << "\n";
  std::cout << "sizeof(double)                  : " << sizeof(double) << "\n";
  std::cout << "sizeof(uint32_t)                : " << sizeof(uint32_t) << "\n";
  std::cout << "sizeof(uint64_t)                : " << sizeof(uint64_t) << "\n";
  std::cout << "std::numeric_limits<int>::max() : "
      << std::numeric_limits<int>::max() << "\n";
  std::cout << "std::numeric_limits<long>::max(): "
      << std::numeric_limits<long>::max() << "\n";
  std::cout << "std::numeric_limits<long long>::max(): "
      << std::numeric_limits<long long>::max() << "\n";
  std::cout << "std::numeric_limits<double>::max(): "
      << std::numeric_limits<double>::max() << "\n";
  std::cout << "std::numeric_limits<uint32_t>::max(): "
      << std::numeric_limits<uint32_t>::max() << std::endl;
  std::cout << "std::numeric_limits<uint64_t>::max(): "
      << std::numeric_limits<uint64_t>::max() << std::endl;
  return 0;
}
