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

TEST(DiscoUtilFunctions, TestClamp)
{
  // POSITIVE INTEGERS
  // at lower edge
  EXPECT_EQ(0, DISCO::clamp_toward_0(0, 0, 10));
  // at upper edge
  EXPECT_EQ(10, DISCO::clamp_toward_0(10, 0, 10));
  // in range
  EXPECT_EQ(5, DISCO::clamp_toward_0(5, 0, 10));
  // out of range above
  EXPECT_EQ(10, DISCO::clamp_toward_0(15, 0, 10));
  // out of range below
  EXPECT_EQ(0, DISCO::clamp_toward_0(2, 5, 25));
  // NEGATIVE INTEGERS
  // at lower edge
  EXPECT_EQ(-10, DISCO::clamp_toward_0(-10, -10, -5));
  // at upper edge
  EXPECT_EQ(-5, DISCO::clamp_toward_0(-5, -10, -5));
  // in range
  EXPECT_EQ(-8, DISCO::clamp_toward_0(-8, -10, -5));
  // out of range above
  EXPECT_EQ(0, DISCO::clamp_toward_0(-2, -10, -5));
  // out of range below
  EXPECT_EQ(-10, DISCO::clamp_toward_0(-15, -10, -5));
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
  std::string actual_output = sink->get_results();
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
  network.couple(sink, ::DISCO::Sink::port_input_request, src, ::DISCO::Source::port_output_request);
  adevs::Simulator<::DISCO::PortValue> sim;
  network.add(&sim);
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
  {
    sim.exec_next_event();
  }
  std::string actual_src_output = src->get_results();
  std::string actual_sink_output = sink->get_results();
  EXPECT_EQ(expected_src_output, actual_src_output);
  EXPECT_EQ(expected_sink_output, actual_sink_output);
}

TEST(DiscoBasicTest, CanRunPowerLimitedSink)
{
  std::string expected_src_output =
    "\"time (hrs)\",\"power [OUT] (kW)\"\n"
    "0,50\n"
    "1,50\n"
    "2,40\n"
    "3,0\n";
  std::string expected_sink_output =
    "\"time (hrs)\",\"power [IN] (kW)\"\n"
    "0,50\n"
    "1,50\n"
    "2,40\n";
  auto src = new ::DISCO::Source();
  auto lim = new ::DISCO::FlowLimits(0, 50);
  auto sink = new ::DISCO::Sink({0,1,2,3}, {160,80,40,0});
  adevs::Digraph<::DISCO::Flow> network;
  network.couple(
      sink, ::DISCO::Sink::port_input_request,
      lim, ::DISCO::FlowLimits::port_output_request);
  network.couple(
      lim, ::DISCO::FlowLimits::port_input_request,
      src, ::DISCO::Source::port_output_request);
  network.couple(
      lim, ::DISCO::FlowLimits::port_output_achieved,
      sink, ::DISCO::Sink::port_input_achieved);
  adevs::Simulator<::DISCO::PortValue> sim;
  network.add(&sim);
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
  {
    sim.exec_next_event();
  }
  std::string actual_src_output = src->get_results();
  std::string actual_sink_output = sink->get_results();
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
  std::string actual_output = o->get_results();
  EXPECT_EQ(expected_output, actual_output);
}

int
main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
