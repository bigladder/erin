/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#include <functional>
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
  std::vector<::DISCO::RealTimeType> expected_times = {0, 1, 2};
  std::vector<::DISCO::RealTimeType> expected_loads = {100, 10, 0};
  std::vector<::DISCO::RealTimeType> times = {0,1,2};
  std::vector<::DISCO::FlowValueType> loads = {100,10,0};
  auto sink = new ::DISCO::Sink(::DISCO::StreamType::electric_stream_in_kW, times, loads);
  auto meter = new ::DISCO::FlowMeter(::DISCO::StreamType::electric_stream_in_kW);
  adevs::Digraph<::DISCO::Flow> network;
  network.couple(
      sink, ::DISCO::Sink::outport_input_request,
      meter, ::DISCO::FlowMeter::inport_output_request
      );
  adevs::Simulator<::DISCO::PortValue> sim;
  network.add(&sim);
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
  {
    sim.exec_next_event();
  }
  std::vector<::DISCO::RealTimeType> actual_times =
    meter->get_actual_output_times();
  std::vector<::DISCO::FlowValueType> actual_loads = meter->get_actual_output();
  EXPECT_EQ(expected_times.size(), actual_times.size());
  EXPECT_EQ(expected_loads.size(), actual_loads.size());
  for (int i{0}; i < expected_times.size(); ++i) {
    if (i >= actual_times.size())
      break;
    EXPECT_EQ(expected_times[i], actual_times[i]);
    if (i >= actual_loads.size())
      break;
    EXPECT_EQ(expected_loads[i], actual_loads[i]);
  }
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
  auto src = new ::DISCO::Source(::DISCO::StreamType::electric_stream_in_kW);
  auto sink = new ::DISCO::Sink(::DISCO::StreamType::electric_stream_in_kW);
  adevs::Digraph<::DISCO::Flow> network;
  network.couple(
      sink, ::DISCO::Sink::outport_input_request,
      src, ::DISCO::Source::inport_output_request
      );
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
  auto src = new ::DISCO::Source(::DISCO::StreamType::electric_stream_in_kW);
  auto lim = new ::DISCO::FlowLimits(
      ::DISCO::StreamType::electric_stream_in_kW, 0, 50);
  auto sink = new ::DISCO::Sink(
      ::DISCO::StreamType::electric_stream_in_kW, {0,1,2,3}, {160,80,40,0});
  adevs::Digraph<::DISCO::Flow> network;
  network.couple(
      sink, ::DISCO::Sink::outport_input_request,
      lim, ::DISCO::FlowLimits::inport_output_request);
  network.couple(
      lim, ::DISCO::FlowLimits::outport_input_request,
      src, ::DISCO::Source::inport_output_request);
  network.couple(
      lim, ::DISCO::FlowLimits::outport_output_achieved,
      sink, ::DISCO::Sink::inport_input_achieved);
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

TEST(DiscoBasicTest, CanRunBasicDieselGensetExample)
{
  // Source https://en.wikipedia.org/wiki/Diesel_fuel
  //const double diesel_fuel_energy_density_MJ_per_liter{35.86};
  const double diesel_fuel_energy_density_MJ_per_liter{1.0};
  // Source EUDP-Task-028-Task C Equipment 250719.xlsx > "3. Energy systems" > G133
  //const double diesel_generator_efficiency{0.36};
  const double diesel_generator_efficiency{0.5};
  //const double seconds_per_minute{60.0};
  const double seconds_per_minute{1.0};
  //const double kW_per_MW{1000.0};
  const double kW_per_MW{1.0};
  const std::vector<::DISCO::RealTimeType> expected_genset_output_times{0,1,2,3};
  const std::vector<::DISCO::FlowValueType> expected_genset_output{50,50,40,0};
  auto calc_output_given_input = [=](::DISCO::FlowValueType input_liter_per_minute) -> ::DISCO::FlowValueType {
    return input_liter_per_minute
      * diesel_fuel_energy_density_MJ_per_liter
      / seconds_per_minute
      * kW_per_MW;
  };
  auto calc_input_given_output = [=](::DISCO::FlowValueType output_kW) -> ::DISCO::FlowValueType {
    return output_kW
      / kW_per_MW
      * seconds_per_minute
      / diesel_fuel_energy_density_MJ_per_liter;
  };
  const std::vector<::DISCO::FlowValueType> expected_fuel_output{
    calc_input_given_output(50),
    calc_input_given_output(50),
    calc_input_given_output(40),
    calc_input_given_output(0),
  };
  auto diesel_fuel_src = new ::DISCO::Source(
      ::DISCO::StreamType::diesel_fuel_stream_in_liters_per_minute
      );
  auto diesel_fuel_meter = new ::DISCO::FlowMeter(
      ::DISCO::StreamType::diesel_fuel_stream_in_liters_per_minute
      );
  auto genset_tx = new ::DISCO::Transformer(
      ::DISCO::StreamType::diesel_fuel_stream_in_liters_per_minute,
      ::DISCO::StreamType::electric_stream_in_kW,
      calc_output_given_input,
      calc_input_given_output
      );
  auto genset_lim = new ::DISCO::FlowLimits(
      ::DISCO::StreamType::electric_stream_in_kW, 0, 50);
  auto genset_meter = new ::DISCO::FlowMeter(
      ::DISCO::StreamType::electric_stream_in_kW
      );
  auto sink = new ::DISCO::Sink(
      ::DISCO::StreamType::electric_stream_in_kW, {0,1,2,3}, {160,80,40,0});
  adevs::Digraph<::DISCO::Flow> network;
  network.couple(
      sink, ::DISCO::Sink::outport_input_request,
      genset_meter, ::DISCO::FlowMeter::inport_output_request);
  network.couple(
      genset_meter, ::DISCO::FlowMeter::outport_input_request,
      genset_lim, ::DISCO::FlowLimits::inport_output_request);
  network.couple(
      genset_lim, ::DISCO::FlowLimits::outport_input_request,
      genset_tx, ::DISCO::Transformer::inport_output_request);
  network.couple(
      genset_tx, ::DISCO::Transformer::outport_input_request,
      diesel_fuel_meter, ::DISCO::FlowMeter::inport_output_request);
  network.couple(
      diesel_fuel_meter, ::DISCO::FlowMeter::outport_input_request,
      diesel_fuel_src, ::DISCO::Source::inport_output_request);
  network.couple(
      diesel_fuel_meter, ::DISCO::FlowMeter::outport_output_achieved,
      genset_tx, ::DISCO::Transformer::inport_input_achieved);
  network.couple(
      genset_tx, ::DISCO::Transformer::outport_output_achieved,
      genset_lim, ::DISCO::FlowLimits::inport_input_achieved);
  network.couple(
      genset_lim, ::DISCO::FlowLimits::outport_output_achieved,
      genset_meter, ::DISCO::FlowMeter::inport_input_achieved);
  network.couple(
      genset_meter, ::DISCO::FlowMeter::outport_output_achieved,
      sink, ::DISCO::Sink::inport_input_achieved);
  adevs::Simulator<::DISCO::PortValue> sim;
  network.add(&sim);
  adevs::Time t;
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
  {
    sim.exec_next_event();
    t = sim.now();
    std::cout << "The current time is: (" << t.real << ", " << t.logical << ")" << std::endl;
  }
  std::vector<::DISCO::FlowValueType> actual_genset_output =
    genset_meter->get_actual_output();
  std::vector<::DISCO::FlowValueType> actual_fuel_output =
    diesel_fuel_meter->get_actual_output();
  std::vector<::DISCO::RealTimeType> actual_genset_output_times =
    genset_meter->get_actual_output_times();
  EXPECT_EQ(expected_genset_output.size(), actual_genset_output.size());
  EXPECT_EQ(expected_genset_output_times.size(), actual_genset_output_times.size());
  EXPECT_EQ(expected_genset_output_times.size(), actual_genset_output.size());
  EXPECT_EQ(expected_genset_output_times.size(), actual_fuel_output.size());
  for (int i{0}; i < expected_genset_output.size(); ++i) {
    if (i >= actual_genset_output.size())
      break;
    EXPECT_EQ(expected_genset_output.at(i), actual_genset_output.at(i));
    EXPECT_EQ(expected_genset_output_times.at(i), actual_genset_output_times.at(i));
    EXPECT_EQ(expected_fuel_output.at(i), actual_fuel_output.at(i));
  }
}

int
main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
