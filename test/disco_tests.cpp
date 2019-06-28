/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#include "gtest/gtest.h"
#include "disco/disco.h"
#include "../vendor/bdevs/include/adevs.h"
#include "checkout_line/clerk.h"
#include "checkout_line/customer.h"
#include "checkout_line/generator.h"
#include "checkout_line/observer.h"

TEST(SetupTest, GoogleTestRuns)
{
  int x(1);
  int y(1);
  EXPECT_EQ(x, y);
}

TEST(DiscoBasicsTest, CanRunSourceSink)
{
  auto src = ::DISCO::Source();
  auto sink = ::DISCO::Sink();
  auto model = ::DISCO::Model();
  model.addComponent("source", src);
  model.addComponent("sink", sink);
  model.connect("source", ::DISCO::Source::OUT, "sink", ::DISCO::Sink::IN);
  auto results = model.simulate();
}

TEST(AdevsUsageTest, CanRunCheckoutLineExample)
{
  // Expected results from ADEVS manual
  // URL: https://web.ornl.gov/~nutarojj/adevs/adevs-docs/manual.pdf,
  // see Tables 3.1 and 3.2
  std::string expected_output =
    "# Col 1: Time customer enters the line\n"
    "# Col 2: Time required for customer checkout\n"
    "# Col 3: Time customer leaves the store\n"
    "# Col 4: Time spent waiting in line\n"
    "1 1 2 0\n"
    "2 4 6 0\n"
    "3 4 10 3\n"
    "5 2 12 5\n"
    "7 10 22 5\n"
    "8 20 42 14\n"
    "10 2 44 32\n"
    "11 1 45 33\n";
  adevs::SimpleDigraph<Customer> store;
  auto c = new Clerk();
  auto g = new Generator();
  auto o = new Observer();
  store.couple(g, c);
  store.couple(c, o);
  adevs::Simulator<Customer> sim;
  store.add(&sim);
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
  {
    sim.exec_next_event();
  }
  std::string actual_output = o->getResults();
  EXPECT_EQ(expected_output, actual_output);
}

int
main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
