/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#include <functional>
#include <unordered_map>
#include <sstream>
#include "gtest/gtest.h"
#include "erin/erin.h"
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
  while (sim.next_event_time() < adevs_inf<adevs::Time>()) {
    sim.exec_next_event();
  }
  std::string actual_output = o->get_results();
  EXPECT_EQ(expected_output, actual_output);
}

TEST(ErinUtilFunctions, TestClamp)
{
  // POSITIVE INTEGERS
  // at lower edge
  EXPECT_EQ(0, ERIN::clamp_toward_0(0, 0, 10));
  // at upper edge
  EXPECT_EQ(10, ERIN::clamp_toward_0(10, 0, 10));
  // in range
  EXPECT_EQ(5, ERIN::clamp_toward_0(5, 0, 10));
  // out of range above
  EXPECT_EQ(10, ERIN::clamp_toward_0(15, 0, 10));
  // out of range below
  EXPECT_EQ(0, ERIN::clamp_toward_0(2, 5, 25));
  // NEGATIVE INTEGERS
  // at lower edge
  EXPECT_EQ(-10, ERIN::clamp_toward_0(-10, -10, -5));
  // at upper edge
  EXPECT_EQ(-5, ERIN::clamp_toward_0(-5, -10, -5));
  // in range
  EXPECT_EQ(-8, ERIN::clamp_toward_0(-8, -10, -5));
  // out of range above
  EXPECT_EQ(0, ERIN::clamp_toward_0(-2, -10, -5));
  // out of range below
  EXPECT_EQ(-10, ERIN::clamp_toward_0(-15, -10, -5));
}

TEST(ErinBasicsTest, TestUnitConversion)
{
  const auto st = ::ERIN::StreamType{"electricity"};
  const auto s = ::ERIN::Stream(st, 10.0);
  EXPECT_EQ(s.get_rate(), 10.0);
  EXPECT_NEAR(s.get_quantity(3600.0), 10.0 * 3600.0, 1e-6);
  const auto st1 = ::ERIN::StreamType{"electricity", "kW", "kWh", 3600.0};
  const auto s1 = ::ERIN::Stream{st1, 1.0};
  EXPECT_EQ(s1.get_rate(), 1.0);
  EXPECT_NEAR(s1.get_quantity(3600.0), 1.0, 1e-6);
  const auto ru = std::unordered_map<std::string,::ERIN::FlowValueType>{
    {"liters/hour",0.1003904071388734},
    {"gallons/hour",0.026520422449113276}
  };
  const auto qu = std::unordered_map<std::string,::ERIN::FlowValueType>{
    {"liters",2.7886224205242612e-05},
    {"gallons",7.366784013642577e-06}
  };
  const auto st2 = ::ERIN::StreamType{
    "diesel",
    "kW",
    "kJ",
    1.0,
    ru,
    qu
  };
  const auto s2 = ::ERIN::Stream(st2, 100.0);
  EXPECT_NEAR(s2.get_rate(), 100.0, 1e-6);
  EXPECT_NEAR(s2.get_quantity(3600.0), 100.0 * 3600.0, 1e-6);
  EXPECT_NEAR(s2.get_rate_in_units("liters/hour"), 10.03904071388734, 1e-6);
  EXPECT_NEAR(s2.get_rate_in_units("gallons/hour"), 2.652042244911328, 1e-6);
  EXPECT_NEAR(s2.get_quantity_in_units(3600.0, "liters"), 10.03904071388734, 1e-6);
  EXPECT_NEAR(s2.get_quantity_in_units(3600.0, "gallons"), 2.652042244911328, 1e-6);
}

TEST(ErinBasicsTest, TestLoadItem)
{
  const auto li1 = ::ERIN::LoadItem(0, 1);
  const auto li2 = ::ERIN::LoadItem(4);
  EXPECT_NEAR(li1.get_time_advance(li2), 4.0, 1e-6);
  EXPECT_EQ(li1.get_time(), 0);
  EXPECT_EQ(li1.get_value(), 1.0);
  EXPECT_EQ(li2.get_time(), 4);
  EXPECT_FALSE(li1.get_is_end());
  EXPECT_TRUE(li2.get_is_end());
}

TEST(ErinBasicsTest, FlowState)
{
  ::ERIN::FlowState fs{0.0, 0.0};
  EXPECT_EQ(fs.getInflow(), 0.0);
  EXPECT_EQ(fs.getOutflow(), 0.0);
  EXPECT_EQ(fs.getStoreflow(), 0.0);
  EXPECT_EQ(fs.getLossflow(), 0.0);
  fs = ::ERIN::FlowState{100.0, 50.0};
  EXPECT_EQ(fs.getInflow(), 100.0);
  EXPECT_EQ(fs.getOutflow(), 50.0);
  EXPECT_EQ(fs.getStoreflow(), 0.0);
  EXPECT_EQ(fs.getLossflow(), 50.0);
  fs = ::ERIN::FlowState{100.0, 0.0, 90.0};
  EXPECT_EQ(fs.getInflow(), 100.0);
  EXPECT_EQ(fs.getOutflow(), 0.0);
  EXPECT_EQ(fs.getStoreflow(), 90.0);
  EXPECT_EQ(fs.getLossflow(), 10.0);
}

TEST(ErinBasicsTest, StandaloneSink)
{
  std::vector<::ERIN::RealTimeType> expected_times = {0, 1, 2};
  std::vector<::ERIN::RealTimeType> expected_loads = {100, 10, 0};
  auto st = ::ERIN::StreamType("electrical");
  auto sink = new ::ERIN::Sink(
      "load", st, {{"default", {{0,100},{1,10},{2,0},{3}}}}, "default");
  auto meter = new ::ERIN::FlowMeter("meter", st);
  adevs::Digraph<::ERIN::Stream> network;
  network.couple(
      sink, ::ERIN::Sink::outport_inflow_request,
      meter, ::ERIN::FlowMeter::inport_outflow_request
      );
  adevs::Simulator<::ERIN::PortValue> sim;
  network.add(&sim);
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
    sim.exec_next_event();
  std::vector<::ERIN::RealTimeType> actual_times =
    meter->get_actual_output_times();
  std::vector<::ERIN::FlowValueType> actual_loads = meter->get_actual_output();
  EXPECT_EQ(expected_times.size(), actual_times.size());
  EXPECT_EQ(expected_loads.size(), actual_loads.size());
  for (int i{0}; i < expected_times.size(); ++i) {
    if (i >= actual_times.size())
      break;
    EXPECT_EQ(expected_times[i], actual_times[i])
      << "times differ at index " << i;
    if (i >= actual_loads.size())
      break;
    EXPECT_EQ(expected_loads[i], actual_loads[i])
      << "loads differ at index " << i;
  }
}

TEST(ErinBasicsTest, CanRunSourceSink)
{
  std::vector<::ERIN::RealTimeType> expected_time = {0, 1};
  std::vector<::ERIN::FlowValueType> expected_flow = {100, 0};
  auto st = ::ERIN::StreamType("electrical");
  auto sink = new ::ERIN::Sink(
      "sink", st, {{"default",{{0,100},{1,0},{2}}}}, "default");
  auto meter = new ::ERIN::FlowMeter("meter", st);
  adevs::Digraph<::ERIN::Stream> network;
  network.couple(
      sink, ::ERIN::Sink::outport_inflow_request,
      meter, ::ERIN::FlowMeter::inport_outflow_request
      );
  adevs::Simulator<::ERIN::PortValue> sim;
  network.add(&sim);
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
    sim.exec_next_event();
  std::vector<::ERIN::RealTimeType> actual_time =
    meter->get_actual_output_times();
  std::vector<::ERIN::FlowValueType> actual_flow =
    meter->get_actual_output();
  EXPECT_EQ(expected_time.size(), actual_time.size());
  EXPECT_EQ(expected_flow.size(), actual_flow.size());
  for (int i{0}; i < expected_time.size(); ++i) {
    if (i >= actual_time.size())
      break;
    EXPECT_EQ(expected_time[i], actual_time[i])
      << "times differ at index " << i;
      ;
    if (i >= actual_flow.size())
      break;
    EXPECT_EQ(expected_flow[i], actual_flow[i])
      << "loads differ at index " << i;
  }
}

TEST(ErinBasicTest, CanRunPowerLimitedSink)
{
  std::vector<::ERIN::RealTimeType> expected_time = {0, 1, 2, 3};
  std::vector<::ERIN::FlowValueType> expected_flow = {50, 50, 40, 0};
  auto elec = ::ERIN::StreamType("electrical");
  auto meter2 = new ::ERIN::FlowMeter("meter2", elec);
  auto lim = new ::ERIN::FlowLimits("lim", elec, 0, 50);
  auto meter1 = new ::ERIN::FlowMeter("meter1", elec);
  auto sink = new ::ERIN::Sink(
      "electric-load", elec,
      {{"default", {{0, 160},{1,80},{2,40},{3,0},{4}}}},
      "default");
  adevs::Digraph<::ERIN::Stream> network;
  network.couple(
      sink, ::ERIN::Sink::outport_inflow_request,
      meter1, ::ERIN::FlowMeter::inport_outflow_request);
  network.couple(
      meter1, ::ERIN::FlowMeter::outport_inflow_request,
      lim, ::ERIN::FlowLimits::inport_outflow_request);
  network.couple(
      lim, ::ERIN::FlowLimits::outport_inflow_request,
      meter2, ::ERIN::FlowMeter::inport_outflow_request);
  network.couple(
      meter2, ::ERIN::FlowMeter::outport_outflow_achieved,
      lim, ::ERIN::FlowLimits::inport_inflow_achieved);
  network.couple(
      lim, ::ERIN::FlowLimits::outport_outflow_achieved,
      meter1, ::ERIN::FlowMeter::inport_inflow_achieved);
  adevs::Simulator<::ERIN::PortValue> sim;
  network.add(&sim);
  while (sim.next_event_time() < adevs_inf<adevs::Time>())
    sim.exec_next_event();
  std::vector<::ERIN::RealTimeType> actual_time1 =
    meter1->get_actual_output_times();
  std::vector<::ERIN::RealTimeType> actual_time2 =
    meter2->get_actual_output_times();
  std::vector<::ERIN::FlowValueType> actual_flow1 =
    meter1->get_actual_output();
  std::vector<::ERIN::FlowValueType> actual_flow2 =
    meter2->get_actual_output();
  EXPECT_EQ(expected_time.size(), actual_time1.size());
  EXPECT_EQ(expected_time.size(), actual_time2.size());
  EXPECT_EQ(expected_flow.size(), actual_flow1.size());
  EXPECT_EQ(expected_flow.size(), actual_flow2.size());
  for (int i{0}; i < expected_time.size(); ++i) {
    if (i >= actual_time1.size())
      break;
    EXPECT_EQ(expected_time[i], actual_time1[i])
      << " times from meter1 differ at index " << i;
    if (i >= actual_time2.size())
      break;
    EXPECT_EQ(expected_time[i], actual_time2[i])
      << " times from meter2 differ at index " << i;
    if (i >= actual_flow1.size())
      break;
    EXPECT_EQ(expected_flow[i], actual_flow1[i])
      << " flows from meter1 differ at index " << i;
    if (i >= actual_flow2.size())
      break;
    EXPECT_EQ(expected_flow[i], actual_flow2[i])
      << " flows from meter2 differ at index " << i;
  }
}

TEST(ErinBasicTest, CanRunBasicDieselGensetExample)
{
  const double diesel_generator_efficiency{0.36};
  const std::vector<::ERIN::RealTimeType> expected_genset_output_times{0, 1, 2, 3};
  const std::vector<::ERIN::FlowValueType> expected_genset_output{50, 50, 40, 0};
  auto calc_output_given_input = [=](::ERIN::FlowValueType input_kW) -> ::ERIN::FlowValueType {
    return input_kW * diesel_generator_efficiency;
  };
  auto calc_input_given_output = [=](::ERIN::FlowValueType output_kW) -> ::ERIN::FlowValueType {
    return output_kW / diesel_generator_efficiency;
  };
  const std::vector<::ERIN::FlowValueType> expected_fuel_output{
    calc_input_given_output(50),
    calc_input_given_output(50),
    calc_input_given_output(40),
    calc_input_given_output(0),
  };
  auto diesel = ::ERIN::StreamType("diesel");
  auto elec = ::ERIN::StreamType("electrical");
  auto diesel_fuel_meter = new ::ERIN::FlowMeter("diesel_fuel_meter", diesel);
  auto genset_tx = new ::ERIN::Transformer(
      "genset_tx",
      diesel,
      elec,
      calc_output_given_input,
      calc_input_given_output
      );
  auto genset_lim = new ::ERIN::FlowLimits("genset_lim", elec, 0, 50);
  auto genset_meter = new ::ERIN::FlowMeter("genset_meter", elec);
  auto sink = new ::ERIN::Sink(
      "electric_load", elec,
      {{"bluesky", {{0,160},{1,80},{2,40},{3,0},{4}}}}, "bluesky");
  adevs::Digraph<::ERIN::Stream> network;
  network.couple(
      sink, ::ERIN::Sink::outport_inflow_request,
      genset_meter, ::ERIN::FlowMeter::inport_outflow_request);
  network.couple(
      genset_meter, ::ERIN::FlowMeter::outport_inflow_request,
      genset_lim, ::ERIN::FlowLimits::inport_outflow_request);
  network.couple(
      genset_lim, ::ERIN::FlowLimits::outport_inflow_request,
      genset_tx, ::ERIN::Transformer::inport_outflow_request);
  network.couple(
      genset_tx, ::ERIN::Transformer::outport_inflow_request,
      diesel_fuel_meter, ::ERIN::FlowMeter::inport_outflow_request);
  network.couple(
      diesel_fuel_meter, ::ERIN::FlowMeter::outport_outflow_achieved,
      genset_tx, ::ERIN::Transformer::inport_inflow_achieved);
  network.couple(
      genset_tx, ::ERIN::Transformer::outport_outflow_achieved,
      genset_lim, ::ERIN::FlowLimits::inport_inflow_achieved);
  network.couple(
      genset_lim, ::ERIN::FlowLimits::outport_outflow_achieved,
      genset_meter, ::ERIN::FlowMeter::inport_inflow_achieved);
  adevs::Simulator<::ERIN::PortValue> sim;
  network.add(&sim);
  adevs::Time t;
  while (sim.next_event_time() < adevs_inf<adevs::Time>()) {
    sim.exec_next_event();
    t = sim.now();
    std::cout << "The current time is: (" << t.real << ", " << t.logical << ")" << std::endl;
  }
  std::vector<::ERIN::FlowValueType> actual_genset_output =
    genset_meter->get_actual_output();
  std::vector<::ERIN::FlowValueType> actual_fuel_output =
    diesel_fuel_meter->get_actual_output();
  std::vector<::ERIN::RealTimeType> actual_genset_output_times =
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

TEST(ErinBasicTest, CanRunUsingComponents)
{
  auto elec = ::ERIN::StreamType("electrical");
  auto loads_by_scenario = std::unordered_map<std::string,std::vector<::ERIN::LoadItem>>(
      {{"bluesky", {{0,160},{1,80},{2,40},{3,0},{4}}}});
  std::shared_ptr<::ERIN::Component> source =
    std::make_shared<::ERIN::SourceComponent>("electrical_pcc", elec);
  std::shared_ptr<::ERIN::Component> load =
    std::make_shared<::ERIN::LoadComponent>(
        "electrical_load",
        elec,
        loads_by_scenario,
        "bluesky");
  load->add_input(source);
  adevs::Digraph<::ERIN::Stream> network;
  load->add_to_network(network);
  source->add_to_network(network);
  adevs::Simulator<::ERIN::PortValue> sim;
  network.add(&sim);
  bool worked{false};
  int iworked{0};
  while (sim.next_event_time() < adevs_inf<adevs::Time>()) {
    sim.exec_next_event();
    worked = true;
    ++iworked;
  }
  EXPECT_TRUE(iworked > 0);
  EXPECT_TRUE(worked);
}

TEST(ErinBasicTest, CanReadStreamInfoFromToml)
{
  std::stringstream ss{};
  ss << "[stream_info]\n"
     << "# The commonality across all streams.\n"
     << "# We need to know what the common rate unit and quantity unit is.\n"
     << "# The rate unit should be the quantity unit per unit of time.\n"
     << "rate_unit = \"kW\"\n"
     << "quantity_unit = \"kJ\"\n"
     << "# seconds_per_time_unit : Number\n"
     << "# defines how many seconds per unit of time used to specify the quantity unit\n"
     << "# here since we are using kW and kJ, the answer is 1.0 second.\n"
     << "# However, if we were doing kW and kWh, we would use 3600 s (i.e., 1 hour) here.\n"
     << "seconds_per_time_unit = 1.0\n";
  ::ERIN::TomlInputReader t{ss};
  ::ERIN::StreamInfo expected{"kW", "kJ", 1.0};
  auto pt = &t;
  auto actual = pt->read_stream_info();
  EXPECT_EQ(expected, actual);
}

int
main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
