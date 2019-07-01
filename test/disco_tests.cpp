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

TEST(DiscoBasicsTest, StandaloneSink)
{
  std::string expected_output =
    "\"time (hrs)\",\"power [IN] (kW)\"\n"
    "0,100\n"
    "1,10\n";
  std::vector<int> times = {0,1,2};
  std::vector<int> loads = {100,10,0};
  auto sink = new ::DISCO::Sink(times, loads);
  adevs::Simulator<::DISCO::PortValue> sim;
  sim.add(sink);
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
  {
    sim.exec_next_event();
  }
  std::string actual_output = sink->getResults();
  EXPECT_EQ(expected_output, actual_output);
}

TEST(DiscoBasicsTest, CanRunSourceSink)
{
  std::string expected_src_output =
    "\"time (hrs)\",\"power [OUT] (kW)\"\n"
    "0,100\n"
    "1,0\n";
  std::string expected_sink_output =
    "\"time (hrs)\",\"power [IN] (kW)\"\n"
    "0,100\n";
  auto src = new ::DISCO::Source();
  auto sink = new ::DISCO::Sink();
  adevs::Digraph<::DISCO::Flow> network;
  network.couple(sink, ::DISCO::Sink::PORT_IN_REQUEST, src, ::DISCO::Source::PORT_OUT_REQUEST);
  adevs::Simulator<::DISCO::PortValue> sim;
  network.add(&sim);
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
  {
    sim.exec_next_event();
  }
  std::string actual_src_output = src->getResults();
  std::string actual_sink_output = sink->getResults();
  EXPECT_EQ(expected_src_output, actual_src_output);
  EXPECT_EQ(expected_sink_output, actual_sink_output);
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
  // Note: adevs::SimpleDigraph and adevs::Digraph take ownership of the
  // objects used in the couple calls.
  // As such, those objects are deleted with the destructor to the store
  // object.
  // Therefore, delete is not called on c, g, or o...
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
