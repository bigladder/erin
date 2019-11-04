/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_DEBUG_UTILS_H
#define ERIN_DEBUG_UTILS_H

namespace ERIN
{
#ifdef DEBUG_PRINT
#define DB_PUTS(x) do { \
  std::cerr << x << std::endl; \
} while (0)
#define DB_PUTS2(x,y) do { \
  std::cerr << x << y << std::endl; \
} while (0)
#define DB_PUTS3(x,y,z) do { \
  std::cerr << x << y << z << std::endl; \
} while (0)
#define DB_PUTS4(a,b,c,d) do { \
  std::cerr << a << b << c << d << std::endl; \
} while (0)
#else
#define DB_PUTS(x)
#define DB_PUTS2(x,y)
#define DB_PUTS3(x,y,z)
#define DB_PUTS4(a,b,c,d)
#endif

}

#endif // ERIN_DEBUG_UTILS_H
