/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#include "gtest/gtest.h"
#include "disco/disco.h"

TEST(SetupTest, GoogleTestRuns) {
  int x(1);
  int y(1);
  EXPECT_EQ(x, y);
}

TEST(DiscoBasicsTest, CanRunSourceSink) {
  auto src = ::DISCO::Source();
  auto sink = ::DISCO::Sink();
  auto model = ::DISCO::Model();
  model.addComponent("source", src);
  model.addComponent("sink", sink);
  model.connect("source", ::DISCO::Source::OUT, "sink", ::DISCO::Sink::IN);
  auto results = model.simulate();
}

int
main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
