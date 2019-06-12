/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#include "gtest/gtest.h"
#include "disco/disco.h"

TEST(DISCO, test_gtest) {
  int x(1);
  int y(1);
  EXPECT_EQ(x, y);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
