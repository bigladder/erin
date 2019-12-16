/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "adevs.h"
#include "checkout_line/clerk.h"
#include "checkout_line/customer.h"
#include "checkout_line/generator.h"
#include "checkout_line/observer.h"
#include "debug_utils.h"
#include "erin/distribution.h"
#include "erin/erin.h"
#include "erin/fragility.h"
#include "erin/port.h"
#include "erin/type.h"
#include "erin/utils.h"
#include "erin_test_utils.h"
#include "gtest/gtest.h"
#include <functional>
#include <memory>
#include <random>
#include <sstream>
#include <unordered_map>
#include <utility>

const double tolerance{1e-6};

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

TEST(ErinBasicsTest, TestLoadItem)
{
  const auto li1 = ERIN::LoadItem(0, 1);
  const auto li2 = ERIN::LoadItem(4);
  EXPECT_EQ(li1.get_time_advance(li2), 4);
  EXPECT_EQ(li1.get_time(), 0);
  EXPECT_EQ(li1.get_value(), 1.0);
  EXPECT_EQ(li2.get_time(), 4);
  EXPECT_FALSE(li1.get_is_end());
  EXPECT_TRUE(li2.get_is_end());
}

TEST(ErinBasicsTest, FlowState)
{
  auto fs = ERIN::FlowState{0.0, 0.0};
  EXPECT_EQ(fs.getInflow(), 0.0);
  EXPECT_EQ(fs.getOutflow(), 0.0);
  EXPECT_EQ(fs.getStoreflow(), 0.0);
  EXPECT_EQ(fs.getLossflow(), 0.0);
  fs = ERIN::FlowState{100.0, 50.0};
  EXPECT_EQ(fs.getInflow(), 100.0);
  EXPECT_EQ(fs.getOutflow(), 50.0);
  EXPECT_EQ(fs.getStoreflow(), 0.0);
  EXPECT_EQ(fs.getLossflow(), 50.0);
  fs = ERIN::FlowState{100.0, 0.0, 90.0};
  EXPECT_EQ(fs.getInflow(), 100.0);
  EXPECT_EQ(fs.getOutflow(), 0.0);
  EXPECT_EQ(fs.getStoreflow(), 90.0);
  EXPECT_EQ(fs.getLossflow(), 10.0);
}

TEST(ErinBasicsTest, StandaloneSink)
{
  auto st = ::ERIN::StreamType("electrical");
  auto sink = new ::ERIN::Sink(
      "load", ::ERIN::ComponentType::Load, st,
      {::ERIN::LoadItem{0,100},
       ::ERIN::LoadItem{1,10},
       ::ERIN::LoadItem{2,0},
       ::ERIN::LoadItem{3}});
  auto meter = new ::ERIN::FlowMeter(
      "meter", ::ERIN::ComponentType::Load, st);
  adevs::Digraph<::ERIN::FlowValueType, ::ERIN::Time> network;
  network.couple(
      sink, ::ERIN::Sink::outport_inflow_request,
      meter, ::ERIN::FlowMeter::inport_outflow_request
      );
  adevs::Simulator<::ERIN::PortValue, ::ERIN::Time> sim;
  network.add(&sim);
  while (sim.next_event_time() < ::ERIN::inf) {
    sim.exec_next_event();
  }
  std::vector<::ERIN::RealTimeType> expected_times = {0, 1, 2, 3};
  auto actual_times = meter->get_event_times();
  ::erin_test_utils::compare_vectors<::ERIN::RealTimeType>(expected_times, actual_times);
  std::vector<::ERIN::FlowValueType> expected_loads = {100, 10, 0, 0};
  auto actual_loads = meter->get_achieved_flows();
  ::erin_test_utils::compare_vectors<::ERIN::FlowValueType>(expected_loads, actual_loads);
}

TEST(ErinBasicsTest, CanRunSourceSink)
{
  std::vector<::ERIN::RealTimeType> expected_time = {0, 1, 2};
  std::vector<::ERIN::FlowValueType> expected_flow = {100, 0, 0};
  auto st = ::ERIN::StreamType("electrical");
  auto sink = new ::ERIN::Sink(
      "sink", ::ERIN::ComponentType::Load, st, {
          ::ERIN::LoadItem{0,100},
          ::ERIN::LoadItem{1,0},
          ::ERIN::LoadItem{2}});
  auto meter = new ::ERIN::FlowMeter(
      "meter", ::ERIN::ComponentType::Load, st);
  adevs::Digraph<::ERIN::FlowValueType, ::ERIN::Time> network;
  network.couple(
      sink, ::ERIN::Sink::outport_inflow_request,
      meter, ::ERIN::FlowMeter::inport_outflow_request
      );
  adevs::Simulator<::ERIN::PortValue, ::ERIN::Time> sim;
  network.add(&sim);
  while (sim.next_event_time() < ::ERIN::inf) {
    sim.exec_next_event();
  }
  std::vector<::ERIN::RealTimeType> actual_time =
    meter->get_event_times();
  std::vector<::ERIN::FlowValueType> actual_flow =
    meter->get_achieved_flows();
  auto et_size = expected_time.size();
  EXPECT_EQ(et_size, actual_time.size());
  EXPECT_EQ(expected_flow.size(), actual_flow.size());
  for (decltype(et_size) i{0}; i < et_size; ++i) {
    if (i >= actual_time.size()) {
      break;
    }
    EXPECT_EQ(expected_time[i], actual_time[i])
      << "times differ at index " << i;
    if (i >= actual_flow.size()) {
      break;
    }
    EXPECT_EQ(expected_flow[i], actual_flow[i])
      << "loads differ at index " << i;
  }
}

TEST(ErinBasicsTest, CanRunPowerLimitedSink)
{
  std::vector<::ERIN::RealTimeType> expected_time = {0, 1, 2, 3, 4};
  std::vector<::ERIN::FlowValueType> expected_flow = {50, 50, 40, 0, 0};
  auto elec = ::ERIN::StreamType("electrical");
  auto meter2 = new ::ERIN::FlowMeter("meter2", ::ERIN::ComponentType::Source, elec);
  auto lim = new ::ERIN::FlowLimits("lim", ::ERIN::ComponentType::Source, elec, 0, 50);
  auto meter1 = new ::ERIN::FlowMeter("meter1", ::ERIN::ComponentType::Load, elec);
  auto sink = new ::ERIN::Sink(
      "electric-load", ::ERIN::ComponentType::Load, elec,
      {
          ::ERIN::LoadItem{0, 160},
          ::ERIN::LoadItem{1,80},
          ::ERIN::LoadItem{2,40},
          ::ERIN::LoadItem{3,0},
          ::ERIN::LoadItem{4}});
  adevs::Digraph<::ERIN::FlowValueType, ::ERIN::Time> network;
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
  adevs::Simulator<::ERIN::PortValue, ::ERIN::Time> sim;
  network.add(&sim);
  while (sim.next_event_time() < ::ERIN::inf) {
    sim.exec_next_event();
  }
  std::vector<::ERIN::RealTimeType> actual_time1 =
    meter1->get_event_times();
  std::vector<::ERIN::RealTimeType> actual_time2 =
    meter2->get_event_times();
  std::vector<::ERIN::FlowValueType> actual_flow1 =
    meter1->get_achieved_flows();
  std::vector<::ERIN::FlowValueType> actual_flow2 =
    meter2->get_achieved_flows();
  EXPECT_EQ(expected_time.size(), actual_time1.size());
  EXPECT_EQ(expected_time.size(), actual_time2.size());
  EXPECT_EQ(expected_flow.size(), actual_flow1.size());
  EXPECT_EQ(expected_flow.size(), actual_flow2.size());
  auto expected_time_size = expected_time.size();
  for (decltype(expected_time_size) i{0}; i < expected_time_size; ++i) {
    if (i >= actual_time1.size()) {
      break;
    }
    EXPECT_EQ(expected_time[i], actual_time1[i])
      << " times from meter1 differ at index " << i;
    if (i >= actual_time2.size()) {
      break;
    }
    EXPECT_EQ(expected_time[i], actual_time2[i])
      << " times from meter2 differ at index " << i;
    if (i >= actual_flow1.size()) {
      break;
    }
    EXPECT_EQ(expected_flow[i], actual_flow1[i])
      << " flows from meter1 differ at index " << i;
    if (i >= actual_flow2.size()) {
      break;
    }
    EXPECT_EQ(expected_flow[i], actual_flow2[i])
      << " flows from meter2 differ at index " << i;
  }
}

TEST(ErinBasicsTest, CanRunBasicDieselGensetExample)
{
  const double diesel_generator_efficiency{0.36};
  const std::vector<::ERIN::RealTimeType> expected_genset_output_times{
    0, 1, 2, 3, 4};
  const std::vector<::ERIN::FlowValueType> expected_genset_output{
    50, 50, 40, 0, 0};
  auto calc_output_given_input =
    [=](::ERIN::FlowValueType input_kW) -> ::ERIN::FlowValueType {
      return input_kW * diesel_generator_efficiency;
    };
  auto calc_input_given_output =
    [=](::ERIN::FlowValueType output_kW) -> ::ERIN::FlowValueType {
      return output_kW / diesel_generator_efficiency;
    };
  const std::vector<::ERIN::FlowValueType> expected_fuel_output{
    calc_input_given_output(50),
    calc_input_given_output(50),
    calc_input_given_output(40),
    calc_input_given_output(0),
    calc_input_given_output(0)
  };
  auto diesel = ::ERIN::StreamType("diesel");
  auto elec = ::ERIN::StreamType("electrical");
  auto diesel_fuel_meter = new ::ERIN::FlowMeter(
      "diesel_fuel_meter", ::ERIN::ComponentType::Source, diesel);
  auto genset_tx = new ::ERIN::Converter(
      "genset_tx",
      ::ERIN::ComponentType::Converter,
      diesel,
      elec,
      calc_output_given_input,
      calc_input_given_output
      );
  auto genset_lim = new ::ERIN::FlowLimits(
      "genset_lim", ::ERIN::ComponentType::Converter, elec, 0, 50);
  auto genset_meter = new ::ERIN::FlowMeter(
      "genset_meter", ::ERIN::ComponentType::Converter, elec);
  std::vector<::ERIN::LoadItem> load_profile{
    ::ERIN::LoadItem{0,160},
    ::ERIN::LoadItem{1,80},
    ::ERIN::LoadItem{2,40},
    ::ERIN::LoadItem{3,0},
    ::ERIN::LoadItem{4}};
  auto sink = new ::ERIN::Sink(
      "electric_load", ::ERIN::ComponentType::Load, elec, load_profile);
  adevs::Digraph<::ERIN::FlowValueType, ::ERIN::Time> network;
  network.couple(
      sink, ::ERIN::Sink::outport_inflow_request,
      genset_meter, ::ERIN::FlowMeter::inport_outflow_request);
  network.couple(
      genset_meter, ::ERIN::FlowMeter::outport_inflow_request,
      genset_lim, ::ERIN::FlowLimits::inport_outflow_request);
  network.couple(
      genset_lim, ::ERIN::FlowLimits::outport_inflow_request,
      genset_tx, ::ERIN::Converter::inport_outflow_request);
  network.couple(
      genset_tx, ::ERIN::Converter::outport_inflow_request,
      diesel_fuel_meter, ::ERIN::FlowMeter::inport_outflow_request);
  network.couple(
      diesel_fuel_meter, ::ERIN::FlowMeter::outport_outflow_achieved,
      genset_tx, ::ERIN::Converter::inport_inflow_achieved);
  network.couple(
      genset_tx, ::ERIN::Converter::outport_outflow_achieved,
      genset_lim, ::ERIN::FlowLimits::inport_inflow_achieved);
  network.couple(
      genset_lim, ::ERIN::FlowLimits::outport_outflow_achieved,
      genset_meter, ::ERIN::FlowMeter::inport_inflow_achieved);
  adevs::Simulator<::ERIN::PortValue, ::ERIN::Time> sim;
  network.add(&sim);
  ::ERIN::Time t;
  while (sim.next_event_time() < ::ERIN::inf) {
    sim.exec_next_event();
    t = sim.now();
    //std::cout << "The current time is: (" << t.real << ", " << t.logical
    //          << ")" << std::endl;
  }
  std::vector<::ERIN::FlowValueType> actual_genset_output =
    genset_meter->get_achieved_flows();
  std::vector<::ERIN::FlowValueType> requested_genset_output =
    genset_meter->get_requested_flows();
  EXPECT_EQ(actual_genset_output.size(), requested_genset_output.size());
  EXPECT_EQ(load_profile.size(), requested_genset_output.size());
  auto rgo_size = requested_genset_output.size();
  for (decltype(rgo_size) i{ 0 }; i < rgo_size; ++i) {
    if (i == (rgo_size - 1)) {
      EXPECT_EQ(requested_genset_output[i], 0.0)
        << "i = " << i << " which is the last index";
    }
    else {
      EXPECT_EQ(requested_genset_output[i], load_profile[i].get_value())
        << "i = " << i;
    }
  }
  std::vector<::ERIN::FlowValueType> actual_fuel_output =
    diesel_fuel_meter->get_achieved_flows();
  std::vector<::ERIN::RealTimeType> actual_genset_output_times =
    genset_meter->get_event_times();
  EXPECT_EQ(expected_genset_output.size(), actual_genset_output.size());
  EXPECT_EQ(
      expected_genset_output_times.size(), actual_genset_output_times.size());
  EXPECT_EQ(expected_genset_output_times.size(), actual_genset_output.size());
  EXPECT_EQ(expected_genset_output_times.size(), actual_fuel_output.size());
  auto ego_size = expected_genset_output.size();
  for (decltype(ego_size) i{0}; i < ego_size; ++i) {
    if (i >= actual_genset_output.size()) {
      break;
    }
    EXPECT_EQ(expected_genset_output.at(i), actual_genset_output.at(i));
    EXPECT_EQ(
        expected_genset_output_times.at(i), actual_genset_output_times.at(i));
    EXPECT_EQ(expected_fuel_output.at(i), actual_fuel_output.at(i));
  }
}

TEST(ErinBasicsTest, CanRunUsingComponents)
{
  auto elec = ::ERIN::StreamType("electrical");
  auto loads_by_scenario = std::unordered_map<
    std::string, std::vector<::ERIN::LoadItem>>(
        {{"bluesky", {
            ::ERIN::LoadItem{0,160},
            ::ERIN::LoadItem{1,80},
            ::ERIN::LoadItem{2,40},
            ::ERIN::LoadItem{3,0},
            ::ERIN::LoadItem{4}}}});
  const std::string source_id{"electrical_pcc"};
  std::unique_ptr<::ERIN::Component> source =
    std::make_unique<::ERIN::SourceComponent>(source_id, elec);
  const std::string load_id{"electrical_load"};
  std::unique_ptr<::ERIN::Component> load =
    std::make_unique<::ERIN::LoadComponent>(
        load_id, elec, loads_by_scenario);
  ::erin::network::Connection conn{
    ::erin::network::ComponentAndPort{source_id, ::erin::port::Type::Outflow, 0},
    ::erin::network::ComponentAndPort{load_id, ::erin::port::Type::Inflow, 0}};
  std::string scenario_id{"bluesky"};
  adevs::Digraph<::ERIN::FlowValueType, ::ERIN::Time> network;
  auto a = load->add_to_network(network, scenario_id);
  auto b = source->add_to_network(network, scenario_id);
  adevs::Simulator<::ERIN::PortValue, ::ERIN::Time> sim;
  network.add(&sim);
  bool worked{false};
  int iworked{0};
  while (sim.next_event_time() < ::ERIN::inf) {
    sim.exec_next_event();
    worked = true;
    ++iworked;
  }
  EXPECT_TRUE(iworked > 0);
  EXPECT_TRUE(worked);
}

TEST(ErinBasicsTest, CanReadSimulationInfoFromToml)
{
  std::stringstream ss{};
  ss << "[simulation_info]\n"
        "rate_unit = \"kW\"\n"
        "quantity_unit = \"kJ\"\n"
        "time_unit = \"hours\"\n"
        "max_time = 3000\n";
  ::ERIN::TomlInputReader tir{ss};
  ::ERIN::SimulationInfo expected{"kW", "kJ", ::ERIN::TimeUnits::Hours, 3000};
  auto actual = tir.read_simulation_info();
  EXPECT_EQ(expected, actual);
}

TEST(ErinBasicsTest, CanReadFragilityCurvesFromToml)
{
  namespace ef = erin::fragility;
  std::stringstream ss{};
  ss << "############################################################\n"
        "# Fragility Curves\n"
        "[fragility.somewhat_vulnerable_to_flooding]\n"
        "vulnerable_to = \"inundation_depth_ft\"\n"
        "type = \"linear\"\n"
        "lower_bound = 6.0\n"
        "upper_bound = 14.0\n"
        "[fragility.highly_vulnerable_to_wind]\n"
        "vulnerable_to = \"wind_speed_mph\"\n"
        "type = \"linear\"\n"
        "lower_bound = 80.0\n"
        "upper_bound = 160.0\n";
  ::ERIN::TomlInputReader tir{ss};
  std::unordered_map<std::string, ef::FragilityCurve> expected{};
  ef::FragilityCurve c1{
    "inundation_depth_ft", std::make_unique<ef::Linear>(6.0, 14.0)};
  ef::FragilityCurve c2{
    "wind_speed_mph", std::make_unique<ef::Linear>(80.0, 160.0)};
  expected.insert(
      std::move(
        std::make_pair("somewhat_vulnerable_to_flooding", std::move(c1))));
  expected.insert(
      std::move(
        std::make_pair("highly_vulnerable_to_wind", std::move(c2))));
  auto actual = tir.read_fragility_data();
  ASSERT_EQ(expected.size(), actual.size());
  for (auto& e_pair: expected) {
    auto a_it = actual.find(e_pair.first);
    ASSERT_FALSE(a_it == actual.end());
    auto& e_fc = e_pair.second;
    auto& a_fc = a_it->second;
    EXPECT_EQ(e_fc.vulnerable_to, a_fc.vulnerable_to);
    ASSERT_EQ(e_fc.curve->get_curve_type(), a_fc.curve->get_curve_type());
    EXPECT_EQ(e_fc.curve->str(), a_fc.curve->str());
  }
}

TEST(ErinBasicsTest, CanReadStreamsFromToml)
{
  std::stringstream ss{};
  ss << "[streams.electricity]\n"
        "type = \"electricity_medium_voltage\"\n"
        "[streams.diesel]\n"
        "type = \"diesel_fuel\"\n"
        "[streams.diesel.other_rate_units]\n"
        "gallons__hour = 0.026520422449113276\n"
        "liters__hour = 0.1003904071388734\n"
        "[streams.diesel.other_quantity_units]\n"
        "gallons = 7.366784013642577e-6\n"
        "liters = 2.7886224205242612e-5\n";
  ::ERIN::TomlInputReader t{ss};
  ::ERIN::SimulationInfo si{};
  std::unordered_map<std::string, ::ERIN::StreamType> expected{
    std::make_pair("electricity", ::ERIN::StreamType(
          "electricity_medium_voltage", "kW", "kJ", 1.0, {}, {})),
    std::make_pair("diesel", ::ERIN::StreamType(
          "diesel_fuel", "kW", "kJ", 1.0,
          {std::make_pair("gallons__hour", 0.026520422449113276),
           std::make_pair("liters__hour", 0.1003904071388734)},
          {std::make_pair("gallons", 7.366784013642577e-6),
           std::make_pair("liters", 2.7886224205242612e-5)}))
  };
  auto pt = &t;
  auto actual = pt->read_streams(si);
  EXPECT_EQ(expected.size(), actual.size());
  for (auto const& e: expected) {
    const auto a = actual.find(e.first);
    ASSERT_TRUE(a != actual.end());
    EXPECT_EQ(e.second.get_type(), a->second.get_type());
    EXPECT_EQ(e.second.get_rate_units(), a->second.get_rate_units());
    EXPECT_EQ(e.second.get_quantity_units(), a->second.get_quantity_units());
    const auto eoru = e.second.get_other_rate_units();
    const auto aoru = a->second.get_other_rate_units();
    EXPECT_EQ(eoru.size(), aoru.size());
    for (auto const& eru: eoru) {
      const auto aru = aoru.find(eru.first);
      ASSERT_TRUE(aru != aoru.end());
      EXPECT_NEAR(eru.second, aru->second, tolerance);
    }
    const auto eoqu = e.second.get_other_quantity_units();
    const auto aoqu = a->second.get_other_quantity_units();
    EXPECT_EQ(eoqu.size(), aoqu.size());
    for (auto const& equ: eoqu) {
      const auto aqu = aoqu.find(equ.first);
      ASSERT_TRUE(aqu != aoqu.end());
      EXPECT_NEAR(equ.second, aqu->second, tolerance);
    }
  }
}

TEST(ErinBasicsTest, CanReadComponentsFromToml)
{
  std::stringstream ss{};
  ss << "[components.electric_utility]\n"
        "type = \"source\"\n"
        "# Point of Common Coupling for Electric Utility\n"
        "output_stream = \"electricity\"\n"
        "max_output = 10.0\n"
        "min_output = 0.0\n"
        "[components.cluster_01_electric]\n"
        "type = \"load\"\n"
        "input_stream = \"electricity\"\n"
        "loads_by_scenario.blue_sky = \"load1\"\n"
        "[components.bus]\n"
        "type = \"muxer\"\n"
        "stream = \"electricity\"\n"
        "num_inflows = 2\n"
        "num_outflows = 1\n"
        "dispatch_strategy = \"in_order\"\n";
  ::ERIN::TomlInputReader t{ss};
  std::unordered_map<std::string, ::ERIN::StreamType> streams{
    std::make_pair("electricity", ::ERIN::StreamType(
          "electricity_medium_voltage", "kW", "kJ", 1.0, {}, {}))
  };
  std::string scenario_id{"blue_sky"};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> loads_by_id{
    {std::string{"load1"}, {::ERIN::LoadItem{0,1.0},::ERIN::LoadItem{4}}}
  };
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> loads{
    {scenario_id, {::ERIN::LoadItem{0,1.0},::ERIN::LoadItem{4}}}
  };
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>> expected;
  expected.emplace(std::make_pair(
        std::string{"electric_utility"},
        std::make_unique<::ERIN::SourceComponent>(
          std::string{"electric_utility"},
          streams["electricity"],
          10,
          0)));
  expected.emplace(std::make_pair(
      std::string{"cluster_01_electric"},
      std::make_unique<::ERIN::LoadComponent>(
        std::string{"cluster_01_electric"},
        streams["electricity"],
        loads)));
  expected.emplace(std::make_pair(
      std::string{"bus"},
      std::make_unique<::ERIN::MuxerComponent>(
        std::string{"bus"},
        streams["electricity"],
        2,
        1,
        ::ERIN::MuxerDispatchStrategy::InOrder)));
  auto pt = &t;
  auto actual = pt->read_components(streams, loads_by_id);
  EXPECT_EQ(expected.size(), actual.size());
  for (auto const& e_pair: expected) {
    const auto& tag = e_pair.first;
    const auto a_it = actual.find(tag);
    ASSERT_TRUE(a_it != actual.end());
    const auto& a = a_it->second;
    const auto& e = e_pair.second;
    EXPECT_TRUE(e->equals(a.get())) << "tag = " << tag;
  }
}

TEST(ErinBasicsTest, CanReadLoadsFromToml)
{
  std::stringstream ss{};
  ss << "[loads.load1]\n"
        "time_unit = \"seconds\"\n"
        "rate_unit = \"kW\"\n"
        "time_rate_pairs = [[0.0,1.0],[4.0]]\n";
  ::ERIN::TomlInputReader t{ss};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> expected{
    {std::string{"load1"}, {::ERIN::LoadItem{0,1.0},::ERIN::LoadItem{4}}}
  };
  auto actual = t.read_loads();
  EXPECT_EQ(expected.size(), actual.size());
  for (auto const& e: expected) {
    const auto a = actual.find(e.first);
    ASSERT_TRUE(a != actual.end());
    EXPECT_EQ(e.second.size(), a->second.size());
    for (std::vector<::ERIN::LoadItem>::size_type i{0}; i < e.second.size(); ++i) {
      EXPECT_EQ(e.second[i].get_time(), a->second[i].get_time());
      if (e.second[i].get_is_end())
        EXPECT_EQ(e.second[i].get_is_end(), a->second[i].get_is_end());
      else
        EXPECT_EQ(e.second[i].get_value(), a->second[i].get_value());
    }
  }
}

TEST(ErinBasicsTest, CanReadNetworksFromToml)
{
  namespace enw = ::erin::network;
  namespace ep = ::erin::port;
  std::stringstream ss{};
  ss << "############################################################\n"
        "[networks.normal_operations]\n"
        R"(connections = [["electric_utility", "cluster_01_electric"]])" "\n";
  ::ERIN::TomlInputReader t{ss};
  std::unordered_map<std::string, std::vector<enw::Connection>> expected{
    { "normal_operations",
      { enw::Connection{
          enw::ComponentAndPort{"electric_utility", ep::Type::Outflow, 0},
          enw::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0}}}}};
  auto pt = &t;
  auto actual = pt->read_networks();
  EXPECT_EQ(expected.size(), actual.size());
  for (auto const& e: expected) {
    const auto a = actual.find(e.first);
    ASSERT_TRUE(a != actual.end());
    auto e_conn = e.second.at(0);
    auto a_conn = a->second.at(0);
    ASSERT_EQ(e_conn.first.component_id, a_conn.first.component_id);
    ASSERT_EQ(e_conn.first.port_type, a_conn.first.port_type);
    ASSERT_EQ(e_conn.first.port_number, a_conn.first.port_number);
    ASSERT_EQ(e_conn.second.component_id, a_conn.second.component_id);
    ASSERT_EQ(e_conn.second.port_type, a_conn.second.port_type);
    ASSERT_EQ(e_conn.second.port_number, a_conn.second.port_number);
  }
}

TEST(ErinBasicsTest, CanReadScenariosFromTomlForFixedDist)
{
  std::stringstream ss{};
  ss << "[scenarios.blue_sky]\n"
        "time_units = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\","
                                  " value = 1}\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n";
  ::ERIN::TomlInputReader t{ss};
  const std::string scenario_id{"blue_sky"};
  const auto expected_duration
    = static_cast<::ERIN::RealTimeType>(8760 * ::ERIN::seconds_per_hour);
  std::unordered_map<std::string, ::ERIN::Scenario> expected{{
    scenario_id,
    ::ERIN::Scenario{
      scenario_id,
      std::string{"normal_operations"},
      expected_duration,
      1,
      []() -> ::ERIN::RealTimeType { return 0; },
      {}
    }}};
  auto actual = t.read_scenarios();
  EXPECT_EQ(expected.size(), actual.size());
  for (auto const& e: expected) {
    const auto a = actual.find(e.first);
    ASSERT_TRUE(a != actual.end());
    EXPECT_EQ(e.second.get_name(), a->second.get_name());
    EXPECT_EQ(e.second.get_network_id(), a->second.get_network_id());
    EXPECT_EQ(e.second.get_duration(), a->second.get_duration());
    EXPECT_EQ(e.second.get_max_occurrences(), a->second.get_max_occurrences());
    EXPECT_EQ(
        e.second.get_number_of_occurrences(),
        a->second.get_number_of_occurrences());
  }
  ::ERIN::Time dt_expected{1, 0};
  auto scenario = actual.at(scenario_id);
  auto dt_actual = scenario.ta();
  EXPECT_EQ(dt_expected.logical, dt_actual.logical);
  EXPECT_EQ(dt_expected.real, dt_actual.real);
}

TEST(ErinBasicsTest, CanReadScenariosFromTomlForRandIntDist)
{
  const std::string scenario_id{"blue_sky"};
  const int lb{1};
  const int ub{10};
  std::stringstream ss{};
  ss << "[scenarios." << scenario_id << "]\n"
        "time_units = \"hours\"\n"
        "occurrence_distribution = {type = \"random_integer\","
        " lower_bound = " << lb << ", upper_bound = " << ub << "}\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n";
  ::ERIN::TomlInputReader t{ss};
  auto actual = t.read_scenarios();
  auto scenario = actual.at(scenario_id);
  const int max_tries{100};
  for (int i{0}; i < max_tries; ++i) {
    auto dt_actual = scenario.ta();
    auto dtr = dt_actual.real;
    ASSERT_TRUE((dtr >= lb) && (dtr <= ub))
      << "i = " << i << ", "
      << "dtr = " << dtr << "\n";
  }
}

TEST(ErinBasicsTest, CanReadScenariosIntensities)
{
  const std::string scenario_id{"class_4_hurricane"};
  std::stringstream ss{};
  ss << "[scenarios." << scenario_id << "]\n"
        "time_units = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 1}\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"emergency_operations\"\n"
        "intensity.wind_speed_mph = 156\n"
        "intensity.inundation_depth_ft = 4\n";
  ::ERIN::TomlInputReader t{ss};
  auto scenario_map = t.read_scenarios();
  auto scenario = scenario_map.at(scenario_id);
  std::unordered_map<std::string,double> expected{
    {"wind_speed_mph", 156.0},
    {"inundation_depth_ft", 4.0}};
  auto actual = scenario.get_intensities();
  EXPECT_EQ(expected.size(), actual.size());
}

TEST(ErinBasicsTest, CanRunEx01FromTomlInput)
{
  std::stringstream ss;
  ss << "[simulation_info]\n"
        "# The commonality across all streams.\n"
        "# We need to know what the common rate unit and quantity unit is.\n"
        "# The rate unit should be the quantity unit per unit of time.\n"
        "rate_unit = \"kW\"\n"
        "quantity_unit = \"kJ\"\n"
        "time_units = \"years\"\n"
        "max_time = 1000\n"
        "[streams.electricity]\n"
        "type = \"electrity_medium_voltage\"\n"
        "[streams.diesel]\n"
        "type = \"diesel_fuel\"\n"
        "[streams.diesel.other_rate_units]\n"
        "gallons__hour = 0.026520422449113276\n"
        "liters__hour = 0.1003904071388734\n"
        "[streams.diesel.other_quantity_units]\n"
        "gallons = 7.366784013642577e-6\n"
        "liters = 2.7886224205242612e-5\n"
        "############################################################\n"
        "[loads.building_electrical]\n"
        "time_unit = \"hours\"\n"
        "rate_unit = \"kW\"\n"
        "time_rate_pairs = [[0.0,1.0],[4.0]]\n"
        "############################################################\n"
        "[components.electric_utility]\n"
        "type = \"source\"\n"
        "# Point of Common Coupling for Electric Utility\n"
        "output_stream = \"electricity\"\n"
        "[components.cluster_01_electric]\n"
        "type = \"load\"\n"
        "input_stream = \"electricity\"\n"
        "loads_by_scenario.blue_sky = \"building_electrical\"\n"
        "############################################################\n"
        "[networks.normal_operations]\n"
        R"(connections=[["electric_utility", "cluster_01_electric"]])" "\n"
        "############################################################\n"
        "[scenarios.blue_sky]\n"
        "time_units = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 1}\n"
        "duration = 1\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n";
  ::ERIN::TomlInputReader r{ss};
  auto si = r.read_simulation_info();
  auto streams = r.read_streams(si);
  auto loads = r.read_loads();
  auto components = r.read_components(streams, loads);
  auto networks = r.read_networks();
  auto scenarios = r.read_scenarios();
  ::ERIN::Main m{si, streams, components, networks, scenarios};
  auto out = m.run("blue_sky");
  EXPECT_EQ(out.get_is_good(), true);
  EXPECT_EQ(out.get_results().size(), 2);
  std::unordered_set<std::string> expected_keys{"cluster_01_electric", "electric_utility"};
  // out.get_results() : Map String (Vector Datum)
  for (const auto& item: out.get_results()) {
    auto it = expected_keys.find(item.first);
    EXPECT_TRUE(it != expected_keys.end());
    ASSERT_EQ(item.second.size(), 2);
    EXPECT_EQ(item.second.at(0).time, 0);
    EXPECT_EQ(item.second.at(0).achieved_value, 1.0);
    EXPECT_EQ(item.second.at(0).requested_value, 1.0);
    EXPECT_EQ(item.second.at(1).time, 3600);
    EXPECT_NEAR(item.second.at(1).achieved_value, 0.0, tolerance);
    EXPECT_NEAR(item.second.at(1).requested_value, 0.0, tolerance);
  }
}

TEST(ErinBasicsTest, CanRunEx02FromTomlInput)
{
  std::stringstream ss;
  ss << "[simulation_info]\n"
        "rate_unit = \"kW\"\n"
        "quantity_unit = \"kJ\"\n"
        "time_unit = \"years\"\n"
        "max_time = 1000\n"
        "[streams.electricity]\n"
        "type = \"electrity\"\n"
        "############################################################\n"
        "[loads.building_electrical]\n"
        "csv_file = \"ex02.csv\"\n"
        "############################################################\n"
        "[components.electric_utility]\n"
        "type = \"source\"\n"
        "# Point of Common Coupling for Electric Utility\n"
        "output_stream = \"electricity\"\n"
        "[components.cluster_01_electric]\n"
        "type = \"load\"\n"
        "input_stream = \"electricity\"\n"
        "loads_by_scenario.blue_sky = \"building_electrical\"\n"
        "############################################################\n"
        "[networks.normal_operations]\n"
        R"(connections = [["electric_utility", "cluster_01_electric"]])" "\n"
        "############################################################\n"
        "[scenarios.blue_sky]\n"
        "time_units = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 1}\n"
        "duration = 4\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"";
  ::ERIN::TomlInputReader r{ss};
  auto si = r.read_simulation_info();
  auto streams = r.read_streams(si);
  auto loads = r.read_loads();
  auto components = r.read_components(streams, loads);
  auto networks = r.read_networks();
  auto scenarios = r.read_scenarios();
  ::ERIN::Main m{si, streams, components, networks, scenarios};
  auto out = m.run("blue_sky");
  EXPECT_EQ(out.get_is_good(), true);
  EXPECT_EQ(out.get_results().size(), 2);
  std::unordered_set<std::string> expected_keys{"cluster_01_electric", "electric_utility"};
  // out.get_results() : Map String (Vector Datum)
  for (const auto& item: out.get_results()) {
    auto it = expected_keys.find(item.first);
    EXPECT_TRUE(it != expected_keys.end());
    ASSERT_EQ(item.second.size(), 2);
    EXPECT_EQ(item.second.at(0).time, 0);
    EXPECT_EQ(item.second.at(0).achieved_value, 1.0);
    EXPECT_EQ(item.second.at(0).requested_value, 1.0);
    EXPECT_EQ(
        item.second.at(1).time,
        static_cast<::ERIN::RealTimeType>(4 * ::ERIN::seconds_per_hour));
    EXPECT_NEAR(item.second.at(1).achieved_value, 0.0, tolerance);
    EXPECT_NEAR(item.second.at(1).requested_value, 0.0, tolerance);
  }
}

TEST(ErinBasicsTest, CanRun10ForSourceSink)
{
  namespace enw = ::erin::network;
  namespace ep = ::erin::port;
  std::string scenario_id{"blue_sky"};
  std::string stream_id{"electricity"};
  std::string source_id{"electric_utility"};
  std::string load_id{"cluster_01_electric"};
  std::string net_id{"normal_operations"};
  const int N{10};
  std::vector<::ERIN::LoadItem> loads;
  for (int i{0}; i < N; ++i)
    loads.emplace_back(::ERIN::LoadItem{i, 1.0});
  loads.emplace_back(::ERIN::LoadItem{N});
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>>
    loads_by_scenario{{scenario_id, loads}};
  ::ERIN::SimulationInfo si{};
  std::unordered_map<std::string, ::ERIN::StreamType> streams{
    std::make_pair(
        stream_id,
        ::ERIN::StreamType(
          std::string{"electricity_medium_voltage"},
          si.get_rate_unit(),
          si.get_quantity_unit(),
          1,
          {}, {}))};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> loads_by_id{
    {load_id, loads}
  };
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>>
    components;
  components.insert(
      std::make_pair(
        source_id,
        std::make_unique<::ERIN::SourceComponent>(source_id, streams[stream_id])));
  components.insert(
      std::make_pair(
        load_id,
        std::make_unique<::ERIN::LoadComponent>(
          load_id,
          streams[stream_id],
          loads_by_scenario)));
  std::unordered_map<
    std::string, std::vector<enw::Connection>> networks{
      { net_id,
        { enw::Connection{
            enw::ComponentAndPort{
              source_id, ep::Type::Outflow, 0},
            enw::ComponentAndPort{
              load_id, ep::Type::Inflow, 0}}}}};
  std::unordered_map<std::string, ::ERIN::Scenario> scenarios{
    {
      scenario_id,
      ::ERIN::Scenario{
        scenario_id,
        net_id,
        1,
        -1,
        []() -> ::ERIN::RealTimeType { return 0; },
        {}
      }}};
  ::ERIN::Main m{si, streams, components, networks, scenarios};
  auto out = m.run(scenario_id);
  EXPECT_EQ(out.get_is_good(), true);
}

TEST(ErinBasicsTest, ScenarioResultsToCSV)
{
  ::ERIN::RealTimeType start_time{0};
  ::ERIN::RealTimeType duration{4};
  ::ERIN::ScenarioResults out{
    true,
    start_time,
    duration,
    {{std::string{"A"},
      {
        ::ERIN::Datum{0,1.0,1.0},
        ::ERIN::Datum{1,0.5,0.5},
        ::ERIN::Datum{2,0.0,0.0}}},
     {std::string{"B"},
       {
         ::ERIN::Datum{0,10.0,10.0},
         ::ERIN::Datum{2,5.0,5.0},
         ::ERIN::Datum{4,0.0,0.0}}}},
    {{std::string{"A"}, ::ERIN::StreamType("electrical")},
     {std::string{"B"}, ::ERIN::StreamType("electrical")}},
    {{std::string{"A"}, ::ERIN::ComponentType::Load},
     {std::string{"B"}, ::ERIN::ComponentType::Source}}};
  auto actual = out.to_csv(::ERIN::TimeUnits::Seconds);
  std::string expected{
    "time (seconds),A:achieved (kW),A:requested (kW),"
      "B:achieved (kW),B:requested (kW)\n"
    "0,1,1,10,10\n1,0.5,0.5,10,10\n2,0,0,5,5\n4,0,0,0,0\n"};
  EXPECT_EQ(expected, actual);
  duration = 4;
  ::ERIN::ScenarioResults out2{
    true,
    start_time,
    duration,
    {{std::string{"A"}, {::ERIN::Datum{0,1.0,1.0}}}},
    {{std::string{"A"}, ::ERIN::StreamType("electrical")}},
    {{std::string{"A"}, ::ERIN::ComponentType::Load}}
  };
  auto actual2 = out2.to_csv(::ERIN::TimeUnits::Seconds);
  std::string expected2{
    "time (seconds),A:achieved (kW),A:requested (kW)\n0,1,1\n4,0,0\n"};
  EXPECT_EQ(expected2, actual2);
}

TEST(ErinBasicsTest, TestMaxTimeByScenario)
{
  namespace enw = ::erin::network;
  namespace ep = ::erin::port;
  std::string scenario_id{"blue_sky"};
  std::string stream_id{"electricity"};
  std::string source_id{"electric_utility"};
  std::string load_id{"cluster_01_electric"};
  std::string net_id{"normal_operations"};
  const ::ERIN::RealTimeType max_time{10};
  std::vector<::ERIN::LoadItem> loads;
  for (::ERIN::RealTimeType i{0}; i < max_time; ++i) {
    loads.emplace_back(::ERIN::LoadItem{i, 1.0});
  }
  loads.emplace_back(::ERIN::LoadItem{max_time});
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>>
    loads_by_scenario{{scenario_id, loads}};
  ::ERIN::SimulationInfo si{};
  std::unordered_map<std::string, ::ERIN::StreamType> streams{
    std::make_pair(
        stream_id,
        ::ERIN::StreamType(
          std::string{"electricity_medium_voltage"},
          si.get_rate_unit(),
          si.get_quantity_unit(),
          1,
          {}, {}))};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> loads_by_id{
    {load_id, loads}
  };
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>>
    components;
  components.insert(
      std::make_pair(
        source_id,
        std::make_unique<::ERIN::SourceComponent>(
          source_id,
          streams[stream_id])));
  components.insert(
      std::make_pair(
        load_id,
        std::make_unique<::ERIN::LoadComponent>(
          load_id,
          streams[stream_id],
          loads_by_scenario)));
  std::unordered_map<
    std::string, std::vector<enw::Connection>> networks{
      { net_id,
        { enw::Connection{
            enw::ComponentAndPort{source_id, ep::Type::Outflow, 0}},
          enw::Connection{
            enw::ComponentAndPort{load_id, ep::Type::Inflow, 0}}}}};
  std::unordered_map<std::string, ::ERIN::Scenario> scenarios{
    {
      scenario_id,
      ::ERIN::Scenario{
        scenario_id,
        net_id,
        max_time,
        -1,
        nullptr,
        {}
      }}};
  ::ERIN::Main m{si, streams, components, networks, scenarios};
  auto actual = m.max_time_for_scenario(scenario_id);
  ::ERIN::RealTimeType expected = max_time;
  EXPECT_EQ(expected, actual);
}

TEST(ErinBasicsTest, TestScenarioResultsMetrics)
{
  // ## Example 0
  ::ERIN::RealTimeType start_time{0};
  ::ERIN::RealTimeType duration{4};
  ::ERIN::ScenarioResults sr0{
    true,
    start_time,
    duration,
    {{ std::string{"A0"},
       { ::ERIN::Datum{0,1.0,1.0},
         ::ERIN::Datum{4,0.0,0.0}}}},
    {{ std::string{"A0"},
       ::ERIN::StreamType("electrical")}},
    {{ std::string{"A0"},
       ::ERIN::ComponentType::Source}}};
  // energy_availability
  std::unordered_map<std::string,double> expected0{{"A0",1.0}};
  auto actual0 = sr0.calc_energy_availability();
  ::erin_test_utils::compare_maps<double>(
      expected0, actual0, "energy_availability_with_sr0");
  // max_downtime
  std::unordered_map<std::string,::ERIN::RealTimeType> expected0_max_downtime{
    {"A0",0}};
  auto actual0_max_downtime = sr0.calc_max_downtime();
  ::erin_test_utils::compare_maps_exact<::ERIN::RealTimeType>(
      expected0_max_downtime, actual0_max_downtime, "max_downtime_with_sr0");
  // load_not_served
  std::unordered_map<std::string,::ERIN::FlowValueType> expected0_lns{
    {"A0",0.0}};
  auto actual0_lns = sr0.calc_load_not_served();
  ::erin_test_utils::compare_maps<::ERIN::FlowValueType>(
      expected0_lns, actual0_lns, "load_not_served_with_sr0");
  // energy_usage_by_stream
  std::unordered_map<std::string,::ERIN::FlowValueType> expected0_eubs{
    {"electrical", 4.0}};
  auto actual0_eubs = sr0.calc_energy_usage_by_stream(
      ::ERIN::ComponentType::Source);
  ::erin_test_utils::compare_maps<::ERIN::FlowValueType>(
      expected0_eubs, actual0_eubs, "energy_usage_by_stream_with_sr0");
  // ## Example 1
  ::ERIN::ScenarioResults sr1{
    true,
    start_time,
    duration,
    {{ std::string{"A1"},
       { ::ERIN::Datum{0,2.0,1.0},
         ::ERIN::Datum{2,0.5,0.5},
         ::ERIN::Datum{4,0.0,0.0}}}},
    {{ std::string{"A1"},
       ::ERIN::StreamType("electrical")}},
    {{ std::string{"A1"},
       ::ERIN::ComponentType::Source}}};
  // energy_availability
  std::unordered_map<std::string,double> expected1{{"A1",0.5}};
  auto actual1 = sr1.calc_energy_availability();
  ::erin_test_utils::compare_maps<double>(
      expected1, actual1, "energy_availability_with_sr1");
  // max_downtime
  std::unordered_map<std::string,::ERIN::RealTimeType> expected1_max_downtime{
    {"A1",2}};
  auto actual1_max_downtime = sr1.calc_max_downtime();
  ::erin_test_utils::compare_maps<::ERIN::RealTimeType>(
      expected1_max_downtime, actual1_max_downtime, "max_downtime_with_sr1");
  // load_not_served
  std::unordered_map<std::string,::ERIN::FlowValueType> expected1_lns{
    {"A1",2.0}};
  auto actual1_lns = sr1.calc_load_not_served();
  ::erin_test_utils::compare_maps<::ERIN::FlowValueType>(
      expected1_lns, actual1_lns, "load_not_served_with_sr1");
  // energy_usage_by_stream
  std::unordered_map<std::string,::ERIN::FlowValueType> expected1_eubs{
    {"electrical", 3.0}};
  auto actual1_eubs = sr1.calc_energy_usage_by_stream(
      ::ERIN::ComponentType::Source);
  ::erin_test_utils::compare_maps<::ERIN::FlowValueType>(
      expected1_eubs, actual1_eubs, "energy_usage_by_stream_with_sr1");
}

TEST(ErinBasicsTest, Test_calc_scenario_stats)
{
  std::vector<::ERIN::Datum> ds{
    { ::ERIN::Datum{0,1.0,1.0},
      ::ERIN::Datum{4,0.0,0.0}}
  };
  // RealTimeType uptime;
  // RealTimeType downtime;
  // FlowValueType load_not_served;
  // FlowValueType total_energy;
  ::ERIN::ScenarioStats expected{
    4,    // RealTimeType uptime
    0,    // RealTimeType downtime
    0.0,  // FlowValueType load_not_served
    4.0}; // FlowValueType total_energy
  auto actual = ::ERIN::calc_scenario_stats(ds);
  EXPECT_EQ(expected.uptime, actual.uptime);
  EXPECT_EQ(expected.downtime, actual.downtime);
  EXPECT_NEAR(expected.load_not_served, actual.load_not_served, tolerance);
  EXPECT_NEAR(expected.total_energy, actual.total_energy, tolerance);
}

TEST(ErinBasicsTest, BasicScenarioTest)
{
  // We want to create one or more scenarios and simulate them in DEVS
  // each scenario should be autonomous and not interact with each other.
  // The entire simulation has a max time limit.
  // This is where we may need to switch to long or int64_t if we go with
  // seconds for the time unit and 1000 years of simulation...
  namespace enw = ::erin::network;
  namespace ep = ::erin::port;
  std::string scenario_id{"blue_sky"};
  std::string stream_id{"electricity"};
  std::string source_id{"electric_utility"};
  std::string load_id{"cluster_01_electric"};
  std::string net_id{"normal_operations"};
  const ::ERIN::RealTimeType max_time{10};
  std::vector<::ERIN::LoadItem> loads;
  for (::ERIN::RealTimeType i{0}; i < max_time; ++i) {
    loads.emplace_back(::ERIN::LoadItem{i, 1.0});
  }
  loads.emplace_back(::ERIN::LoadItem{max_time});
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>>
    loads_by_scenario{{scenario_id, loads}};
  ::ERIN::SimulationInfo si{
    "kW", "kJ", ::ERIN::TimeUnits::Years, 1000};
  std::unordered_map<std::string, ::ERIN::StreamType> streams{
    std::make_pair(
        stream_id,
        ::ERIN::StreamType(
          std::string{"electricity_medium_voltage"},
          si.get_rate_unit(),
          si.get_quantity_unit(),
          1,
          {}, {}))};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> loads_by_id{
    {load_id, loads}
  };
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>>
    components;
  components.insert(
      std::make_pair(
        source_id,
        std::make_unique<::ERIN::SourceComponent>(
          source_id,
          streams[stream_id])));
  components.insert(
      std::make_pair(
        load_id,
        std::make_unique<::ERIN::LoadComponent>(
          load_id,
          streams[stream_id],
          loads_by_scenario)));
  std::unordered_map<
    std::string, std::vector<enw::Connection>> networks{
      { net_id,
        { enw::Connection{
            { source_id, ep::Type::Outflow, 0},
            { load_id, ep::Type::Inflow, 0}}}}};
  std::unordered_map<std::string, ::ERIN::Scenario> scenarios{
    {
      scenario_id,
      ::ERIN::Scenario{
        scenario_id,
        net_id,
        max_time,
        1,
        [](){ return 100; },
        {}
      }}};
  ::ERIN::Main m{si, streams, components, networks, scenarios};
  auto actual = m.run_all();
  EXPECT_TRUE(actual.get_is_good());
  EXPECT_TRUE(actual.get_results().size() > 0);
  for (const auto& r: actual.get_results()) {
    EXPECT_TRUE(r.second.size() > 0);
  }
}

TEST(ErinBasicsTest, DistributionTest)
{
  const int k{1};
  auto d_fixed = ::erin::distribution::make_fixed<int>(k);
  EXPECT_EQ(d_fixed(), k);
  const int lower_bound{0};
  const int upper_bound{10};
  std::default_random_engine g{};
  auto d_rand = ::erin::distribution::make_random_integer<int>(
      g, lower_bound, upper_bound);
  const int max_times{1000};
  for (int i{0}; i < max_times; ++i) {
    auto v{d_rand()};
    EXPECT_TRUE((v >= lower_bound) && (v <= upper_bound))
      << "expected v to be between (" << lower_bound << ", "
      << upper_bound << "] " << "but was " << v;
  }
}

TEST(ErinBasicsTest, FragilityCurves)
{
  const double lb{120.0};
  const double ub{180.0};
  ::erin::fragility::Linear f{lb, ub};
  EXPECT_EQ(0.0, f.apply(lb - 10.0));
  EXPECT_EQ(1.0, f.apply(ub + 10.0));
  auto probability_of_failure{f.apply((lb + ub) / 2.0)};
  EXPECT_TRUE(
      (probability_of_failure > 0.0) && (probability_of_failure < 1.0));
}

TEST(ErinBasicsTest, TestGetFragilityCurves)
{
  namespace ef = erin::fragility;
  ::ERIN::StreamType st{"electricity"};
  ::ERIN::fragility_map fragilities;
  std::vector<std::unique_ptr<ef::Curve>> vs;
  vs.emplace_back(std::make_unique<::erin::fragility::Linear>(120.0, 180.0));
  fragilities.insert(std::make_pair("wind_speed_mph", std::move(vs)));
  ::ERIN::SourceComponent c{"source", st, std::move(fragilities)};
  std::unordered_map<std::string,double> intensities{{"wind_speed_mph", 150.0}};
  auto probs = c.apply_intensities(intensities);
  EXPECT_EQ(probs.size(), 1);
  EXPECT_NEAR(probs.at(0), 0.5, 1e-6);
}

TEST(ErinBasicsTest, TestFailureChecker)
{
  ::erin::fragility::FailureChecker fc{};
  std::vector<double> probs_1 = {0.0};
  EXPECT_FALSE(fc.is_failed(probs_1));
  std::vector<double> probs_2 = {1.0};
  EXPECT_TRUE(fc.is_failed(probs_2));
  std::vector<double> probs_3 = {0.5};
  bool at_least_one_false{false};
  bool at_least_one_true{false};
  int max{100};
  for (int i{0}; i < max; ++i) {
    auto result = fc.is_failed(probs_3);
    if (result) {
      at_least_one_true = true;
    }
    if (!result) {
      at_least_one_false = true;
    }
    if (at_least_one_false && at_least_one_true) {
      break;
    }
  }
  EXPECT_TRUE(at_least_one_false && at_least_one_true);
}

TEST(ErinBasicsTest, TestFragilityWorksForNetworkSim)
{
  namespace enw = ::erin::network;
  namespace ep = ::erin::port;
  namespace ef = ::erin::fragility;
  ::ERIN::SimulationInfo si{};
  const auto elec_id = std::string{"electrical"};
  const auto elec_stream = ::ERIN::StreamType(elec_id);
  std::unordered_map<std::string, ::ERIN::StreamType> streams{
    {elec_id, elec_stream}};
  const auto pcc_id = std::string{"electric_utility"};
  const auto load_id = std::string{"cluster_01_electric"};
  const auto gen_id = std::string{"emergency_generator"};
  const auto inundation_depth_ft_lower_bound{6.0};
  const auto inundation_depth_ft_upper_bound{14.0};
  const auto wind_speed_mph_lower_bound{80.0};
  const auto wind_speed_mph_upper_bound{160.0};
  const auto intensity_wind_speed = std::string{"wind_speed_mph"};
  const auto intensity_flood = std::string{"inundation_depth_ft"};
  const auto blue_sky = std::string{"blue_sky"};
  const auto class_4_hurricane = std::string{"class_4_hurricane"};
  const auto normal = std::string{"normal_operations"};
  const auto emergency = std::string{"emergency"};
  std::unique_ptr<ef::Curve> fc_inundation =
    std::make_unique<ef::Linear>(
        inundation_depth_ft_lower_bound, inundation_depth_ft_upper_bound);
  std::unique_ptr<ef::Curve> fc_wind =
    std::make_unique<ef::Linear>(
        wind_speed_mph_lower_bound, wind_speed_mph_upper_bound);
  ::ERIN::fragility_map fs_pcc, fs_load, fs_gen;
  std::vector<std::unique_ptr<ef::Curve>> vs_pcc, vs_gen;
  vs_pcc.emplace_back(fc_wind->clone());
  vs_gen.emplace_back(fc_inundation->clone());
  fs_pcc.emplace(std::make_pair(intensity_wind_speed, std::move(vs_pcc)));
  fs_gen.emplace(std::make_pair(intensity_flood, std::move(vs_gen)));
  std::vector<::ERIN::LoadItem>
    loads{::ERIN::LoadItem{0,100.0}, ::ERIN::LoadItem{100}};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>>
    loads_by_scenario{{blue_sky, loads}, {class_4_hurricane, loads}};
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>> comps;
  comps.emplace(
      std::make_pair(
        pcc_id,
        std::make_unique<::ERIN::SourceComponent>(
          pcc_id, elec_stream, std::move(fs_pcc))));
  comps.emplace(
      std::make_pair(
        load_id,
        std::make_unique<::ERIN::LoadComponent>(
          load_id, elec_stream, loads_by_scenario, std::move(fs_load))));
  comps.emplace(
      std::make_pair(
        gen_id,
        std::make_unique<::ERIN::SourceComponent>(
          gen_id, elec_stream, std::move(fs_gen))));
  std::unordered_map<
    std::string, std::vector<enw::Connection>> networks{
      { normal,
        { enw::Connection{ { pcc_id, ep::Type::Outflow, 0},
                           { load_id, ep::Type::Inflow, 0}}}},
      { emergency,
        { enw::Connection{{ pcc_id, ep::Type::Outflow, 0},
                           { load_id, ep::Type::Inflow, 0}},
          enw::Connection{ { gen_id, ep::Type::Outflow, 0},
                           { load_id, ep::Type::Inflow, 0}}}}};
  // test 1: simulate with a fragility curve that never fails
  // - confirm the statistics show load always met
  std::unordered_map<std::string, double> intensities_low{
    { intensity_wind_speed, 0.0},
      { intensity_flood, 0.0}};
  std::unordered_map<std::string, ::ERIN::Scenario> scenarios_low{
    { blue_sky,
      ::ERIN::Scenario{
        blue_sky, normal, 10, 1, [](){return 0;}, {}}},
      { class_4_hurricane,
        ::ERIN::Scenario{
          class_4_hurricane,
          emergency, 10, -1, [](){ return 100; }, intensities_low}}};
  ::ERIN::Main m_low{si, streams, comps, networks, scenarios_low};
  auto results_low = m_low.run(class_4_hurricane);
  EXPECT_NEAR(
      results_low.calc_energy_availability().at(load_id),
      1.0,
      tolerance);

  // test 2: simulate with a fragility curve that always fails
  // - confirm the statistics show 100% load not served
  std::unordered_map<std::string, double> intensities_high{
    { intensity_wind_speed, 300.0},
    { intensity_flood, 20.0}};
  std::unordered_map<std::string, ::ERIN::Scenario> scenarios_high{
    { blue_sky,
      ::ERIN::Scenario{
        blue_sky, normal, 10, 1, [](){return 0;}, {}}},
    { class_4_hurricane,
      ::ERIN::Scenario{
        class_4_hurricane,
        emergency, 10, -1, [](){ return 100; }, intensities_high}}};
  ::ERIN::Main m_high{si, streams, comps, networks, scenarios_high};
  auto results_high = m_high.run(class_4_hurricane);
  EXPECT_NEAR(
      results_high.calc_energy_availability().at(load_id),
      0.0,
      tolerance);
}

TEST(ErinBasicsTest, TestTimeUnits)
{
  const std::string tag_for_seconds{"seconds"};
  auto expected_tu_s = ::ERIN::TimeUnits::Seconds;
  auto actual_tu_s = ::ERIN::tag_to_time_units(tag_for_seconds);
  EXPECT_EQ(expected_tu_s, actual_tu_s);
  EXPECT_EQ(::ERIN::time_units_to_tag(actual_tu_s), tag_for_seconds);
  const std::string tag_for_minutes{"minutes"};
  auto expected_tu_min = ::ERIN::TimeUnits::Minutes;
  auto actual_tu_min = ::ERIN::tag_to_time_units(tag_for_minutes);
  EXPECT_EQ(expected_tu_min, actual_tu_min);
  EXPECT_EQ(::ERIN::time_units_to_tag(actual_tu_min), tag_for_minutes);
  const std::string tag_for_hours{"hours"};
  auto expected_tu_hrs = ::ERIN::TimeUnits::Hours;
  auto actual_tu_hrs = ::ERIN::tag_to_time_units(tag_for_hours);
  EXPECT_EQ(expected_tu_hrs, actual_tu_hrs);
  EXPECT_EQ(::ERIN::time_units_to_tag(actual_tu_hrs), tag_for_hours);
  const std::string tag_for_days{"days"};
  auto expected_tu_days = ::ERIN::TimeUnits::Days;
  auto actual_tu_days = ::ERIN::tag_to_time_units(tag_for_days);
  EXPECT_EQ(expected_tu_days, actual_tu_days);
  EXPECT_EQ(::ERIN::time_units_to_tag(actual_tu_days), tag_for_days);
  const std::string tag_for_years{"years"};
  auto expected_tu_years = ::ERIN::TimeUnits::Years;
  auto actual_tu_years = ::ERIN::tag_to_time_units(tag_for_years);
  EXPECT_EQ(expected_tu_years, actual_tu_years);
  EXPECT_EQ(::ERIN::time_units_to_tag(actual_tu_years), tag_for_years);
}

TEST(ErinBasicsTest, TestTimeUnitConversion)
{
  EXPECT_EQ(
      ::ERIN::time_to_seconds(1, ::ERIN::TimeUnits::Years),
      static_cast<::ERIN::RealTimeType>(::ERIN::seconds_per_year));
  EXPECT_EQ(
      ::ERIN::time_to_seconds(1, ::ERIN::TimeUnits::Days),
      static_cast<::ERIN::RealTimeType>(::ERIN::seconds_per_day));
  EXPECT_EQ(
      ::ERIN::time_to_seconds(1, ::ERIN::TimeUnits::Hours),
      static_cast<::ERIN::RealTimeType>(::ERIN::seconds_per_hour));
  EXPECT_EQ(
      ::ERIN::time_to_seconds(1, ::ERIN::TimeUnits::Minutes),
      static_cast<::ERIN::RealTimeType>(::ERIN::seconds_per_minute));
  EXPECT_EQ(
      ::ERIN::time_to_seconds(1, ::ERIN::TimeUnits::Seconds),
      1);
}

TEST(ErinBasicsTest, TestMuxerComponent)
{
  namespace enw = ::erin::network; 
  namespace ep = ::erin::port;
  const std::string s1_id{"s1"};
  const ::ERIN::FlowValueType s1_max{12.0};
  const ::ERIN::FlowValueType s2_max{4.0};
  const std::string s2_id{"s2"};
  const std::string muxer_id{"bus"};
  const std::string l1_id{"l1"};
  const std::string l2_id{"l2"};
  const int num_inflows{2};
  const int num_outflows{2};
  const ::ERIN::MuxerDispatchStrategy strategy =
    ::ERIN::MuxerDispatchStrategy::InOrder;
  const auto stream = ::ERIN::StreamType{"electrical"};
  const std::string scenario_id{"blue_sky"};
  const ::ERIN::RealTimeType t_max{12};
  std::unique_ptr<::ERIN::Component> m =
    std::make_unique<::ERIN::MuxerComponent>(
        muxer_id, stream, num_inflows, num_outflows, strategy);
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>>
    l1_loads_by_scenario{
      { scenario_id,
        { ::ERIN::LoadItem{0,10},
          ::ERIN::LoadItem{t_max}}}};
  std::unique_ptr<::ERIN::Component> l1 =
    std::make_unique<::ERIN::LoadComponent>(
        l1_id,
        stream,
        l1_loads_by_scenario);
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>>
    l2_loads_by_scenario{
      { scenario_id,
        { ::ERIN::LoadItem{0,0},
          ::ERIN::LoadItem{5,5},
          ::ERIN::LoadItem{8,10},
          ::ERIN::LoadItem{10,5},
          ::ERIN::LoadItem{t_max}}}};
  std::unique_ptr<::ERIN::Component> l2 =
    std::make_unique<::ERIN::LoadComponent>(
        l2_id,
        stream,
        l2_loads_by_scenario);
  std::unique_ptr<::ERIN::Component> s1 =
    std::make_unique<::ERIN::SourceComponent>(
        s1_id,
        stream,
        s1_max);
  std::unique_ptr<::ERIN::Component> s2 =
    std::make_unique<::ERIN::SourceComponent>(
        s2_id,
        stream,
        s2_max);
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>>
    components;
  components.insert(std::make_pair(muxer_id, std::move(m)));
  components.insert(std::make_pair(l1_id, std::move(l1)));
  components.insert(std::make_pair(l2_id, std::move(l2)));
  components.insert(std::make_pair(s1_id, std::move(s1)));
  components.insert(std::make_pair(s2_id, std::move(s2)));
  adevs::Digraph<::ERIN::FlowValueType, ::ERIN::Time> network;
  const std::vector<enw::Connection> connections{
    {{l1_id, ep::Type::Inflow, 0}, {muxer_id, ep::Type::Outflow, 0}},
    {{l2_id, ep::Type::Inflow, 0}, {muxer_id, ep::Type::Outflow, 1}},
    {{muxer_id, ep::Type::Inflow, 0}, {s1_id, ep::Type::Outflow, 0}},
    {{muxer_id, ep::Type::Inflow, 1}, {s2_id, ep::Type::Outflow, 0}}};
  auto elements = enw::build(
      scenario_id, network, connections, components, {});
  adevs::Simulator<::ERIN::PortValue, ::ERIN::Time> sim;
  network.add(&sim);
  const auto duration{t_max};
  const int max_no_advance{static_cast<int>(elements.size()) * 10};
  auto is_good = ::ERIN::run_devs(sim, duration, max_no_advance);
  EXPECT_TRUE(is_good);
  ::ERIN::RealTimeType scenario_start_time_s{0};
  auto sr = ::ERIN::process_single_scenario_results(
      is_good, elements, duration, scenario_start_time_s);
  EXPECT_TRUE(sr.get_is_good());
  auto results = sr.get_results();
  const std::vector<std::string> expected_keys{
    "s1", "s2", "l1", "l2",
    "bus-inflow(0)", "bus-inflow(1)", "bus-outflow(0)", "bus-outflow(1)"};
  const auto expected_num_keys{expected_keys.size()};
  EXPECT_EQ(expected_num_keys, results.size());
  for (const auto& k: expected_keys) {
    auto it = results.find(k);
    ASSERT_FALSE(it == results.end())
      << "key \"" << k << "\" not found in results";
  }
  if (false) {
    std::cout << "RESULTS DUMP:\n";
    for (const auto& r : results) {
      const auto& k = r.first;
      const auto& vs = r.second;
      auto n = vs.size();
      for (std::vector<::ERIN::Datum>::size_type i{0}; i < n; ++i) {
        const auto& v = vs[i];
        std::cout << k << "[" << i << "]{t=" << v.time
                  << ",r=" << v.requested_value
                  << ",a=" << v.achieved_value << "}\n";
      }
    }
  }
  // bus-inflow(0)
  const std::vector<::ERIN::Datum> expected_bus_inflow0{
    ::ERIN::Datum{0,10.0,10.0},
    ::ERIN::Datum{5,15.0,12.0},
    ::ERIN::Datum{8,20.0,12.0},
    ::ERIN::Datum{10,15.0,12.0},
    ::ERIN::Datum{t_max,0.0,0.0}};
  const auto n_bus_inflow0 = expected_bus_inflow0.size();
  const auto& actual_bus_inflow0 = results.at("bus-inflow(0)");
  EXPECT_EQ(n_bus_inflow0, actual_bus_inflow0.size());
  for (std::vector<::ERIN::Datum>::size_type i{0}; i < n_bus_inflow0; ++i) {
    const auto& e = expected_bus_inflow0[i];
    const auto& a = actual_bus_inflow0[i];
    EXPECT_EQ(e.time, a.time);
    EXPECT_NEAR(e.requested_value, a.requested_value, tolerance)
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.achieved_value, a.achieved_value, tolerance)
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
  }
  // bus-inflow(1)
  const std::vector<::ERIN::Datum> expected_bus_inflow1{
    ::ERIN::Datum{0,0.0,0.0},
    ::ERIN::Datum{5,3.0,3.0},
    ::ERIN::Datum{8,8.0,4.0},
    ::ERIN::Datum{10,3.0,3.0},
    ::ERIN::Datum{t_max,0.0,0.0}};
  const auto n_bus_inflow1 = expected_bus_inflow1.size();
  const auto& actual_bus_inflow1 = results.at("bus-inflow(1)");
  EXPECT_EQ(n_bus_inflow1, actual_bus_inflow1.size());
  for (std::vector<::ERIN::Datum>::size_type i{0}; i < n_bus_inflow1; ++i) {
    const auto& e = expected_bus_inflow1[i];
    const auto& a = actual_bus_inflow1[i];
    EXPECT_EQ(e.time, a.time);
    EXPECT_NEAR(e.requested_value, a.requested_value, tolerance)
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.achieved_value, a.achieved_value, tolerance)
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
  }
  // bus-outflow(0)
  const std::vector<::ERIN::Datum> expected_bus_outflow0{
    ::ERIN::Datum{0,10.0,10.0},
    ::ERIN::Datum{8,10.0,8.0},
    ::ERIN::Datum{10,10.0,10.0},
    ::ERIN::Datum{t_max,0.0,0.0}};
  const auto n_bus_outflow0 = expected_bus_outflow0.size();
  const auto& actual_bus_outflow0 = results.at("bus-outflow(0)");
  EXPECT_EQ(n_bus_outflow0, actual_bus_outflow0.size());
  for (std::vector<::ERIN::Datum>::size_type i{0}; i < n_bus_outflow0; ++i) {
    const auto& e = expected_bus_outflow0[i];
    const auto& a = actual_bus_outflow0[i];
    EXPECT_EQ(e.time, a.time);
    EXPECT_NEAR(e.requested_value, a.requested_value, tolerance)
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.achieved_value, a.achieved_value, tolerance)
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
  }
  // bus-outflow(1)
  const std::vector<::ERIN::Datum> expected_bus_outflow1{
    ::ERIN::Datum{0,0.0,0.0},
    ::ERIN::Datum{5,5.0,5.0},
    ::ERIN::Datum{8,10.0,8.0},
    ::ERIN::Datum{10,5.0,5.0},
    ::ERIN::Datum{t_max,0.0,0.0}};
  const auto n_bus_outflow1 = expected_bus_outflow1.size();
  const auto& actual_bus_outflow1 = results.at("bus-outflow(1)");
  EXPECT_EQ(n_bus_outflow1, actual_bus_outflow1.size());
  for (std::vector<::ERIN::Datum>::size_type i{0}; i < n_bus_outflow1; ++i) {
    const auto& e = expected_bus_outflow1[i];
    const auto& a = actual_bus_outflow1[i];
    EXPECT_EQ(e.time, a.time);
    EXPECT_NEAR(e.requested_value, a.requested_value, tolerance)
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.achieved_value, a.achieved_value, tolerance)
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
  }
}

TEST(ErinBasicsTest, TestAddMultipleFragilitiesToAComponent)
{
  namespace ef = erin::fragility;
  std::string id{"source"};
  auto stream = ::ERIN::StreamType{"electricity"};
  std::unordered_map<std::string, std::vector<std::unique_ptr<ef::Curve>>> frags;
  std::vector<std::unique_ptr<ef::Curve>> v1, v2;
  v1.emplace_back(std::make_unique<ef::Linear>(80, 160.0));
  v1.emplace_back(std::make_unique<ef::Linear>(40.0, 220.0));
  v2.emplace_back(std::make_unique<ef::Linear>(4.0, 12.0));
  frags.emplace(std::make_pair("wind_speed_mph", std::move(v1)));
  frags.emplace(std::make_pair("flood_depth_ft", std::move(v2)));
  auto comp = ::ERIN::SourceComponent(id, stream, std::move(frags));
}

TEST(ErinBasicsTest, CanRunEx03FromTomlInput)
{
  namespace enw = ::erin::network;
  namespace ef = ::erin::fragility;
  namespace ep = ::erin::port;
  std::stringstream ss;
  ss << "[simulation_info]\n"
        "rate_unit = \"kW\"\n"
        "quantity_unit = \"kJ\"\n"
        "time_unit = \"years\"\n"
        "max_time = 1000\n"
        "[streams.electricity]\n"
        "type = \"electricity_medium_voltage\"\n"
        "[loads.building_electrical]\n"
        "csv_file = \"ex02.csv\"\n"
        "[components.electric_utility]\n"
        "type = \"source\"\n"
        "output_stream = \"electricity\"\n"
        "fragilities = [\"highly_vulnerable_to_wind\"]\n"
        "[components.cluster_01_electric]\n"
        "type = \"load\"\n"
        "input_stream = \"electricity\"\n"
        "loads_by_scenario.blue_sky = \"building_electrical\"\n"
        "loads_by_scenario.class_4_hurricane = \"building_electrical\"\n"
        "fragilities = [\"somewhat_vulnerable_to_flooding\"]\n"
        "[components.emergency_generator]\n"
        "type = \"source\"\n"
        "output_stream = \"electricity\"\n"
        "fragilities = [\"somewhat_vulnerable_to_flooding\"]\n"
        "[components.bus]\n"
        "type = \"muxer\"\n"
        "stream = \"electricity\"\n"
        "num_inflows = 2\n"
        "num_outflows = 1\n"
        "dispatch_strategy = \"in_order\"\n"
        "fragilities = [\"highly_vulnerable_to_wind\", \"somewhat_vulnerable_to_flooding\"]\n"
        "[fragility.somewhat_vulnerable_to_flooding]\n"
        "vulnerable_to = \"inundation_depth_ft\"\n"
        "type = \"linear\"\n"
        "lower_bound = 6.0\n"
        "upper_bound = 14.0\n"
        "[fragility.highly_vulnerable_to_wind]\n"
        "vulnerable_to = \"wind_speed_mph\"\n"
        "type = \"linear\"\n"
        "lower_bound = 80.0\n"
        "upper_bound = 160.0\n"
        "[networks.normal_operations]\n"
        "connections = [[\"electric_utility\", \"cluster_01_electric\"]]\n"
        "[networks.emergency_operations]\n"
        "connections = [\n"
        "  [\"electric_utility\", \"outflow\", \"0\", \"bus\", \"inflow\", \"0\"],\n"
        "  [\"emergency_generator\", \"outflow\", \"0\", \"bus\", \"inflow\", \"1\"],\n"
        "  [\"bus\", \"cluster_01_electric\"]]\n"
        "[scenarios.blue_sky]\n"
        "time_units = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 0}\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n"
        "[scenarios.class_4_hurricane]\n"
        "time_units = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 87600}\n"
        "duration = 336\n"
        "max_occurrences = -1\n"
        "network = \"emergency_operations\"\n"
        "intensity.wind_speed_mph = 156\n"
        "intensity.inundation_depth_ft = 8\n";
  const std::vector<int>::size_type num_comps{4};
  const std::vector<int>::size_type num_networks{2};
  ::ERIN::TomlInputReader r{ss};
  auto si = r.read_simulation_info();
  auto streams = r.read_streams(si);
  auto loads = r.read_loads();
  auto fragilities = r.read_fragility_data();
  auto components = r.read_components(streams, loads, fragilities);
  EXPECT_EQ(num_comps, components.size());
  // Test that components have fragilities
  for (const auto& c_pair : components) {
    const auto& c_id = c_pair.first; 
    const auto& c = c_pair.second;
    EXPECT_TRUE(c->is_fragile())
      << "component '" << c_id << "' should be fragile but is not";
  }
  auto networks = r.read_networks();
  ASSERT_EQ(num_networks, networks.size());
  const auto& normal_nw = networks["normal_operations"];
  const std::vector<enw::Connection> expected_normal_nw{
    enw::Connection{
      enw::ComponentAndPort{"electric_utility", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0}},
  };
  ASSERT_EQ(expected_normal_nw.size(), normal_nw.size());
  ASSERT_EQ(expected_normal_nw, normal_nw);
  const std::vector<enw::Connection> expected_eo{
    enw::Connection{
      enw::ComponentAndPort{"electric_utility", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"bus", ep::Type::Inflow, 0}},
    enw::Connection{
      enw::ComponentAndPort{"emergency_generator", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"bus", ep::Type::Inflow, 1}},
    enw::Connection{
      enw::ComponentAndPort{"bus", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0}}};
  const auto& actual_eo = networks["emergency_operations"];
  ASSERT_EQ(expected_eo.size(), actual_eo.size());
  ASSERT_EQ(expected_eo, actual_eo);
  auto scenarios = r.read_scenarios();
  constexpr ::ERIN::RealTimeType blue_sky_duration
    = 8760 * ::ERIN::seconds_per_hour;
  constexpr int blue_sky_max_occurrence = 1;
  constexpr ::ERIN::RealTimeType hurricane_duration
    = 336 * ::ERIN::seconds_per_hour;
  constexpr int hurricane_max_occurrence = -1;
  const std::unordered_map<std::string, ::ERIN::Scenario> expected_scenarios{
    { "blue_sky",
      ::ERIN::Scenario{
        "blue_sky",
        "normal_operations",
        blue_sky_duration, 
        blue_sky_max_occurrence,
        []() -> ::ERIN::RealTimeType { return 0; },
        {}}},
    { "class_4_hurricane",
      ::ERIN::Scenario{
        "class_4_hurricane",
        "emergency_operations",
        hurricane_duration,
        hurricane_max_occurrence,
        []() -> ::ERIN::RealTimeType { return 87600; },
        {{"wind_speed_mph", 156.0}, {"inundation_depth_ft", 8.0}}}}};
  ASSERT_EQ(expected_scenarios.size(), scenarios.size());
  for (const auto& scenario_pair : expected_scenarios) {
    auto scenario_it = scenarios.find(scenario_pair.first);
    ASSERT_TRUE(scenario_it != scenarios.end());
    const auto& es = scenario_pair.second;
    const auto& as = scenario_it->second;
    EXPECT_EQ(es.get_name(), as.get_name());
    EXPECT_EQ(es.get_network_id(), as.get_network_id());
    EXPECT_EQ(es.get_duration(), as.get_duration());
    EXPECT_EQ(es.get_max_occurrences(), as.get_max_occurrences());
    EXPECT_EQ(es.get_intensities(), as.get_intensities());
  }
  EXPECT_EQ(expected_scenarios, scenarios);
  ::ERIN::Main m{si, streams, components, networks, scenarios};
  auto out = m.run("blue_sky");
  EXPECT_EQ(out.get_is_good(), true);
  EXPECT_EQ(out.get_results().size(), 2);
  std::unordered_set<std::string> expected_keys{"cluster_01_electric", "electric_utility"};
  // out.get_results() : Map String (Vector Datum)
  for (const auto& item: out.get_results()) {
    auto it = expected_keys.find(item.first);
    ASSERT_TRUE(it != expected_keys.end());
    const auto& results = item.second;
    int i{0};
    for (const auto& x : results) {
      std::cout << "id: " << item.first << "\n";
      std::cout << "x[" << i << "].time            = " << x.time << "\n"
                << "x[" << i << "].achieved_value  = " << x.achieved_value
                << "\n"
                << "x[" << i << "].requested_value = " << x.requested_value
                << "\n";
      ++i;
    }
    ASSERT_EQ(item.second.size(), 3);
    EXPECT_EQ(item.second.at(0).time, 0);
    EXPECT_EQ(item.second.at(0).achieved_value, 1.0);
    EXPECT_EQ(item.second.at(0).requested_value, 1.0);
    EXPECT_EQ(item.second.at(1).time, 4 * 3'600);
    EXPECT_NEAR(item.second.at(1).achieved_value, 0.0, tolerance);
    EXPECT_NEAR(item.second.at(1).requested_value, 0.0, tolerance);
    EXPECT_EQ(item.second.at(2).time, 31'536'000);
    EXPECT_NEAR(item.second.at(2).achieved_value, 0.0, tolerance);
    EXPECT_NEAR(item.second.at(2).requested_value, 0.0, tolerance);
  }
}

TEST(ErinBasicsTest, CanRunEx03Class4HurricaneFromTomlInput)
{
  namespace enw = ::erin::network;
  namespace ef = ::erin::fragility;
  namespace ep = ::erin::port;
  std::stringstream ss;
  ss << "[simulation_info]\n"
        "rate_unit = \"kW\"\n"
        "quantity_unit = \"kJ\"\n"
        "time_unit = \"years\"\n"
        "max_time = 1000\n"
        "[streams.electricity]\n"
        "type = \"electricity_medium_voltage\"\n"
        "[loads.building_electrical]\n"
        "csv_file = \"ex02.csv\"\n"
        "[components.electric_utility]\n"
        "type = \"source\"\n"
        "output_stream = \"electricity\"\n"
        "fragilities = [\"highly_vulnerable_to_wind\"]\n"
        "[components.cluster_01_electric]\n"
        "type = \"load\"\n"
        "input_stream = \"electricity\"\n"
        "loads_by_scenario.blue_sky = \"building_electrical\"\n"
        "loads_by_scenario.class_4_hurricane = \"building_electrical\"\n"
        "fragilities = [\"somewhat_vulnerable_to_flooding\"]\n"
        "[components.emergency_generator]\n"
        "type = \"source\"\n"
        "output_stream = \"electricity\"\n"
        "fragilities = [\"somewhat_vulnerable_to_flooding\"]\n"
        "[components.bus]\n"
        "type = \"muxer\"\n"
        "stream = \"electricity\"\n"
        "num_inflows = 2\n"
        "num_outflows = 1\n"
        "dispatch_strategy = \"in_order\"\n"
        "fragilities = [\"highly_vulnerable_to_wind\", \"somewhat_vulnerable_to_flooding\"]\n"
        "[fragility.somewhat_vulnerable_to_flooding]\n"
        "vulnerable_to = \"inundation_depth_ft\"\n"
        "type = \"linear\"\n"
        "lower_bound = 6.0\n"
        "upper_bound = 14.0\n"
        "[fragility.highly_vulnerable_to_wind]\n"
        "vulnerable_to = \"wind_speed_mph\"\n"
        "type = \"linear\"\n"
        "lower_bound = 80.0\n"
        "upper_bound = 160.0\n"
        "[networks.normal_operations]\n"
        "connections = [[\"electric_utility\", \"cluster_01_electric\"]]\n"
        "[networks.emergency_operations]\n"
        "connections = [\n"
        "  [\"electric_utility\", \"outflow\", \"0\", \"bus\", \"inflow\", \"0\"],\n"
        "  [\"emergency_generator\", \"outflow\", \"0\", \"bus\", \"inflow\", \"1\"],\n"
        "  [\"bus\", \"cluster_01_electric\"]]\n"
        "[scenarios.blue_sky]\n"
        "time_units = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 0}\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n"
        "[scenarios.class_4_hurricane]\n"
        "time_units = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 87600}\n"
        "duration = 336\n"
        "max_occurrences = -1\n"
        "network = \"emergency_operations\"\n"
        "intensity.wind_speed_mph = 200.0\n"
        "intensity.inundation_depth_ft = 20.0\n";
  const std::vector<int>::size_type num_comps{4};
  const std::vector<int>::size_type num_networks{2};
  ::ERIN::TomlInputReader r{ss};
  auto si = r.read_simulation_info();
  auto streams = r.read_streams(si);
  auto loads = r.read_loads();
  auto fragilities = r.read_fragility_data();
  auto components = r.read_components(streams, loads, fragilities);
  EXPECT_EQ(num_comps, components.size());
  // Test that components have fragilities
  for (const auto& c_pair : components) {
    const auto& c_id = c_pair.first; 
    const auto& c = c_pair.second;
    EXPECT_TRUE(c->is_fragile())
      << "component '" << c_id << "' should be fragile but is not";
  }
  auto networks = r.read_networks();
  ASSERT_EQ(num_networks, networks.size());
  const auto& normal_nw = networks["normal_operations"];
  const std::vector<enw::Connection> expected_normal_nw{
    enw::Connection{
      enw::ComponentAndPort{"electric_utility", ep::Type::Outflow, 0},
        enw::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0}},
  };
  ASSERT_EQ(expected_normal_nw.size(), normal_nw.size());
  ASSERT_EQ(expected_normal_nw, normal_nw);
  const std::vector<enw::Connection> expected_eo{
    enw::Connection{
      enw::ComponentAndPort{"electric_utility", ep::Type::Outflow, 0},
        enw::ComponentAndPort{"bus", ep::Type::Inflow, 0}},
      enw::Connection{
        enw::ComponentAndPort{"emergency_generator", ep::Type::Outflow, 0},
        enw::ComponentAndPort{"bus", ep::Type::Inflow, 1}},
      enw::Connection{
        enw::ComponentAndPort{"bus", ep::Type::Outflow, 0},
        enw::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0}}};
  const auto& actual_eo = networks["emergency_operations"];
  ASSERT_EQ(expected_eo.size(), actual_eo.size());
  ASSERT_EQ(expected_eo, actual_eo);
  auto scenarios = r.read_scenarios();
  constexpr ::ERIN::RealTimeType blue_sky_duration
    = 8760 * ::ERIN::seconds_per_hour;
  constexpr int blue_sky_max_occurrence = 1;
  constexpr ::ERIN::RealTimeType hurricane_duration
    = 336 * ::ERIN::seconds_per_hour;
  constexpr int hurricane_max_occurrence = -1;
  const std::unordered_map<std::string, ::ERIN::Scenario> expected_scenarios{
    { "blue_sky",
      ::ERIN::Scenario{
        "blue_sky",
        "normal_operations",
        blue_sky_duration, 
        blue_sky_max_occurrence,
        []() -> ::ERIN::RealTimeType { return 0; },
        {}}},
    { "class_4_hurricane",
      ::ERIN::Scenario{
        "class_4_hurricane",
        "emergency_operations",
        hurricane_duration,
        hurricane_max_occurrence,
        []() -> ::ERIN::RealTimeType { return 87600; },
        {{"wind_speed_mph", 200.0}, {"inundation_depth_ft", 20.0}}}}};
  ASSERT_EQ(expected_scenarios.size(), scenarios.size());
  for (const auto& scenario_pair : expected_scenarios) {
    auto scenario_it = scenarios.find(scenario_pair.first);
    ASSERT_TRUE(scenario_it != scenarios.end());
    const auto& es = scenario_pair.second;
    const auto& as = scenario_it->second;
    EXPECT_EQ(es.get_name(), as.get_name());
    EXPECT_EQ(es.get_network_id(), as.get_network_id());
    EXPECT_EQ(es.get_duration(), as.get_duration());
    EXPECT_EQ(es.get_max_occurrences(), as.get_max_occurrences());
    EXPECT_EQ(es.get_intensities(), as.get_intensities());
  }
  EXPECT_EQ(expected_scenarios, scenarios);
  ::ERIN::Main m{si, streams, components, networks, scenarios};
  auto out = m.run("class_4_hurricane");
  EXPECT_EQ(out.get_is_good(), true);
  std::unordered_set<std::string> expected_keys{
    "cluster_01_electric", "electric_utility",
    "bus-inflow(0)", "bus-inflow(1)", "bus-outflow(0)",
    "emergency_generator"};
  std::unordered_map<std::string,std::vector<::ERIN::Datum>> expected_results;
  expected_results.emplace(
      std::make_pair(
        std::string{"cluster_01_electric"},
        std::vector<::ERIN::Datum>{
          ::ERIN::Datum{0          , 1.0, 0.0},
          ::ERIN::Datum{4   * 3'600, 0.0, 0.0},
          ::ERIN::Datum{336 * 3'600, 0.0, 0.0}}));
  expected_results.emplace(
      std::make_pair(
        std::string{"electric_utility"},
        std::vector<::ERIN::Datum>{
          ::ERIN::Datum{0          , 0.0, 0.0},
          ::ERIN::Datum{336 * 3'600, 0.0, 0.0}}));
  expected_results.emplace(
      std::make_pair(
        std::string{"emergency_generator"},
        std::vector<::ERIN::Datum>{
          ::ERIN::Datum{0          , 0.0, 0.0},
          ::ERIN::Datum{336 * 3'600, 0.0, 0.0}}));
  expected_results.emplace(
      std::make_pair(
        std::string{"bus-inflow(0)"},
        std::vector<::ERIN::Datum>{
          ::ERIN::Datum{0          , 0.0, 0.0},
          ::ERIN::Datum{336 * 3'600, 0.0, 0.0}}));
  expected_results.emplace(
      std::make_pair(
        std::string{"bus-inflow(1)"},
        std::vector<::ERIN::Datum>{
          ::ERIN::Datum{0          , 0.0, 0.0},
          ::ERIN::Datum{336 * 3'600, 0.0, 0.0}}));
  expected_results.emplace(
      std::make_pair(
        std::string{"bus-outflow(0)"},
        std::vector<::ERIN::Datum>{
          ::ERIN::Datum{0          , 0.0, 0.0},
          ::ERIN::Datum{4   * 3'600, 0.0, 0.0},
          ::ERIN::Datum{336 * 3'600, 0.0, 0.0}}));
  EXPECT_EQ(out.get_results().size(), expected_results.size());
  // out.get_results() : Map String (Vector Datum)
  for (const auto& item: out.get_results()) {
    const auto& tag = item.first;
    auto it = expected_results.find(tag);
    ASSERT_TRUE(it != expected_results.end());
    const auto& a_results = item.second;
    const auto& e_results = it->second;
    const auto a_size = a_results.size();
    const auto e_size = e_results.size();
    ASSERT_EQ(a_size, e_size)
      << "tag = " << tag << "\n"
      << "a_results = " << ::ERIN::vec_to_string<::ERIN::Datum>(a_results)
      << "\n"
      << "e_results = " << ::ERIN::vec_to_string<::ERIN::Datum>(e_results)
      << "\n";
    for (std::vector<::ERIN::Datum>::size_type i{0}; i < a_size; ++i) {
      const auto& a = a_results.at(i);
      const auto& e = e_results.at(i);
      EXPECT_EQ(a, e)
        << "tag = " << tag << "\n"
        << "i = " << i << "\n"
        << "a = " << a << "\n"
        << "e = " << e << "\n";
    }
  }
}

TEST(ErinBasicsTest, AllResultsToCsv0)
{
  namespace E = ::ERIN;
  const bool is_good{true};
  const E::RealTimeType hours_to_seconds{3600};
  std::unordered_map<std::string,std::vector<E::ScenarioResults>> results{};
  E::AllResults ar{is_good, results};
  const std::string expected_csv{
    "scenario id,scenario start time (P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]),"
    "elapsed (hours)\n"};
  auto actual_csv = ar.to_csv();
  EXPECT_EQ(expected_csv, actual_csv);
  const std::string expected_stats_csv{
    "scenario id,number of occurrences,total time in scenario (hours),"
    "component id,type,stream,energy availability,max downtime (hours),"
    "load not served (kJ)\n"};
  auto actual_stats_csv = ar.to_stats_csv();
  EXPECT_EQ(expected_stats_csv, actual_stats_csv);
}

TEST(ErinBasicsTest, AllResultsToCsv)
{
  namespace E = ::ERIN;
  const E::RealTimeType hours_to_seconds{3600};
  const bool is_good{true};
  const std::string id_cluster_01_electric{"cluster_01_electric"};
  const std::string id_electric_utility{"electric_utility"};
  const std::string id_electricity{"electricity"};
  const std::string id_blue_sky{"blue_sky"};
  E::StreamType elec{id_electricity};
  std::unordered_map<std::string,std::vector<E::Datum>> data{
    { id_cluster_01_electric,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{4 * hours_to_seconds, 0.0, 0.0}}},
    { id_electric_utility,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{4 * hours_to_seconds, 0.0, 0.0}}}};
  std::unordered_map<std::string,E::StreamType> stream_types{
    { id_cluster_01_electric, elec},
    { id_electric_utility, elec}};
  std::unordered_map<std::string, E::ComponentType> comp_types{
    { id_cluster_01_electric, E::ComponentType::Load},
    { id_electric_utility, E::ComponentType::Source}};
  E::RealTimeType scenario_start{0 * hours_to_seconds};
  E::RealTimeType duration{4 * hours_to_seconds};
  E::ScenarioResults sr{
    is_good, scenario_start, duration, data, stream_types, comp_types};
  EXPECT_EQ(scenario_start, sr.get_start_time_in_seconds());
  EXPECT_EQ(duration, sr.get_duration_in_seconds());
  std::unordered_map<std::string,std::vector<E::ScenarioResults>> results{
    { id_blue_sky, { sr }}};
  E::AllResults ar{is_good, results};
  const std::string expected_csv{
    "scenario id,scenario start time (P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]),"
    "elapsed (hours),cluster_01_electric:achieved (kW),"
    "cluster_01_electric:requested (kW),electric_utility:achieved (kW),"
    "electric_utility:requested (kW)\n"
    "blue_sky,P0000-00-00T00:00:00,0,1,1,1,1\n"
    "blue_sky,P0000-00-00T00:00:00,4,0,0,0,0\n"};
  auto actual_csv = ar.to_csv();
  EXPECT_EQ(expected_csv, actual_csv);
  /*
  const std::string expected_stats_csv{
    "scenario id,number of occurrences,total time in scenario (hours),"
    "component id,type,stream,energy availability,max downtime (hours),"
    "load not served (kJ),electricity_medium_voltage energy used (kJ)\n"
    "blue_sky,1,4,"};
  auto actual_stats_csv = ar.to_stats_csv();
  EXPECT_EQ(expected_stats_csv, actual_stats_csv);
  */
}

TEST(ErinBasicsTest, AllResultsToCsv2)
{
  namespace E = ::ERIN;
  const E::RealTimeType hours_to_seconds{3600};
  const bool is_good{true};
  const std::string id_cluster_01_electric{"cluster_01_electric"};
  const std::string id_electric_utility{"electric_utility"};
  const std::string id_electricity{"electricity"};
  const std::string id_blue_sky{"blue_sky"};
  E::StreamType elec{id_electricity};
  std::unordered_map<std::string,std::vector<E::Datum>> data{
    { id_cluster_01_electric,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{4 * hours_to_seconds, 0.0, 0.0}}},
    { id_electric_utility,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{4 * hours_to_seconds, 0.0, 0.0}}}};
  std::unordered_map<std::string,E::StreamType> stream_types{
    { id_cluster_01_electric, elec},
    { id_electric_utility, elec}};
  std::unordered_map<std::string, E::ComponentType> comp_types{
    { id_cluster_01_electric, E::ComponentType::Load},
    { id_electric_utility, E::ComponentType::Source}};
  E::RealTimeType scenario_start{10 * hours_to_seconds};
  E::RealTimeType duration{4 * hours_to_seconds};
  E::ScenarioResults sr{
    is_good, scenario_start, duration, data, stream_types, comp_types};
  std::unordered_map<std::string,std::vector<E::ScenarioResults>> results{
    { id_blue_sky, { sr }}};
  E::AllResults ar{is_good, results};
  const std::string expected_csv{
    "scenario id,scenario start time (P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]),"
    "elapsed (hours),cluster_01_electric:achieved (kW),"
    "cluster_01_electric:requested (kW),electric_utility:achieved (kW),"
    "electric_utility:requested (kW)\n"
    "blue_sky,P0000-00-00T10:00:00,0,1,1,1,1\n"
    "blue_sky,P0000-00-00T10:00:00,4,0,0,0,0\n"};
  auto actual_csv = ar.to_csv();
  EXPECT_EQ(expected_csv, actual_csv);
}

TEST(ErinBasicsTest, AllResultsToCsv3)
{
  namespace E = ::ERIN;
  const E::RealTimeType hours_to_seconds{3600};
  const bool is_good{true};
  const std::string id_cluster_01_electric{"cluster_01_electric"};
  const std::string id_electric_utility{"electric_utility"};
  const std::string id_electricity{"electricity"};
  const std::string id_blue_sky{"blue_sky"};
  E::StreamType elec{id_electricity};
  std::unordered_map<std::string,std::vector<E::Datum>> data{
    { id_cluster_01_electric,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{8 * hours_to_seconds, 0.0, 0.0}}},
    { id_electric_utility,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{8 * hours_to_seconds, 0.0, 0.0}}}};
  std::unordered_map<std::string,E::StreamType> stream_types{
    { id_cluster_01_electric, elec},
    { id_electric_utility, elec}};
  std::unordered_map<std::string, E::ComponentType> comp_types{
    { id_cluster_01_electric, E::ComponentType::Load},
    { id_electric_utility, E::ComponentType::Source}};
  E::RealTimeType scenario_start{10 * hours_to_seconds};
  E::RealTimeType duration{8 * hours_to_seconds};
  E::ScenarioResults sr{
    is_good, scenario_start, duration, data, stream_types, comp_types};
  EXPECT_EQ(duration, sr.get_duration_in_seconds());
  std::unordered_map<std::string,std::vector<E::ScenarioResults>> results{
    { id_blue_sky, { sr }}};
  E::AllResults ar{is_good, results};
  const std::string expected_csv{
    "scenario id,scenario start time (P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]),"
    "elapsed (hours),cluster_01_electric:achieved (kW),"
    "cluster_01_electric:requested (kW),electric_utility:achieved (kW),"
    "electric_utility:requested (kW)\n"
    "blue_sky,P0000-00-00T10:00:00,0,1,1,1,1\n"
    "blue_sky,P0000-00-00T10:00:00,8,0,0,0,0\n"};
  auto actual_csv = ar.to_csv();
  EXPECT_EQ(expected_csv, actual_csv);
}

TEST(ErinBasicsTest, TimeToIso8601Period)
{
  namespace eu = erin::utils;
  std::string expected{"P0000-00-00T00:00:00"};
  auto achieved = eu::time_to_iso_8601_period(0);
  EXPECT_EQ(expected, achieved);
  expected = "";
  achieved = eu::time_to_iso_8601_period(-10);
  EXPECT_EQ(expected, achieved);
  expected = "P0000-00-00T00:00:01";
  achieved = eu::time_to_iso_8601_period(1);
  EXPECT_EQ(expected, achieved);
  expected = "P0000-00-00T00:01:00";
  achieved = eu::time_to_iso_8601_period(60);
  EXPECT_EQ(expected, achieved);
  expected = "P0000-00-00T00:01:30";
  achieved = eu::time_to_iso_8601_period(90);
  EXPECT_EQ(expected, achieved);
  expected = "P0000-00-00T01:00:00";
  achieved = eu::time_to_iso_8601_period(3600);
  EXPECT_EQ(expected, achieved);
  expected = "P0000-00-00T01:30:30";
  achieved = eu::time_to_iso_8601_period(3600 + (30 * 60) + 30);
  EXPECT_EQ(expected, achieved);
  expected = "P0000-00-01T00:00:00";
  achieved = eu::time_to_iso_8601_period(3600 * 24);
  EXPECT_EQ(expected, achieved);
  expected = "P0000-00-30T00:30:30";
  achieved = eu::time_to_iso_8601_period((30 * 3600 * 24) + (30 * 60) + 30);
  EXPECT_EQ(expected, achieved);
  expected = "P0000-01-00T00:30:30";
  achieved = eu::time_to_iso_8601_period((31 * 3600 * 24) + (30 * 60) + 30);
  EXPECT_EQ(expected, achieved);
  expected = "P0001-00-00T00:00:00";
  achieved = eu::time_to_iso_8601_period(365 * 3600 * 24);
  EXPECT_EQ(expected, achieved);
  expected = "P0010-06-04T05:42:15";
  achieved = eu::time_to_iso_8601_period(
      (10 * 365 * 3600 * 24) //  10 years
      + (185 * 3600 * 24)    // 185 days
      + (5 * 3600)           //   5 hours
      + (42 * 60)            //  42 minutes
      + 15);                 //  15 seconds
  EXPECT_EQ(expected, achieved);
}

TEST(ErinBasicsTest, DayOfYearToDayOfMonth)
{
  namespace eu = erin::utils;
  eu::Months_days_elapsed expected{0, 0};
  auto achieved = eu::day_of_year_to_months_days_elapsed(0);
  EXPECT_EQ(expected, achieved);
  expected = eu::Months_days_elapsed{0, 1};
  achieved = eu::day_of_year_to_months_days_elapsed(1);
  EXPECT_EQ(expected, achieved);
  expected = eu::Months_days_elapsed{0,1};
  achieved = eu::day_of_year_to_months_days_elapsed(-364);
  EXPECT_EQ(expected, achieved);
  expected = eu::Months_days_elapsed{0,0};
  achieved = eu::day_of_year_to_months_days_elapsed(365);
  EXPECT_EQ(expected, achieved);
  expected = eu::Months_days_elapsed{0,0};
  achieved = eu::day_of_year_to_months_days_elapsed(-365);
  EXPECT_EQ(expected, achieved);
  expected = eu::Months_days_elapsed{0,0};
  achieved = eu::day_of_year_to_months_days_elapsed(365*2);
  EXPECT_EQ(expected, achieved);
  expected = eu::Months_days_elapsed{1,0};
  achieved = eu::day_of_year_to_months_days_elapsed(31);
  EXPECT_EQ(expected, achieved);
  expected = eu::Months_days_elapsed{1,1};
  achieved = eu::day_of_year_to_months_days_elapsed(32);
  EXPECT_EQ(expected, achieved);
  expected = eu::Months_days_elapsed{6,2};
  achieved = eu::day_of_year_to_months_days_elapsed(183);
  EXPECT_EQ(expected, achieved);
}

TEST(ErinBasicsTest, TestIsSuperset)
{
  std::vector<std::string> a(0);
  std::vector<std::string> b(0);
  EXPECT_TRUE(::erin::utils::is_superset(a, b));
  a = {"A", "B", "C"};
  b = {"B", "C"};
  EXPECT_TRUE(::erin::utils::is_superset(a, a));
  EXPECT_TRUE(::erin::utils::is_superset(b, b));
  EXPECT_TRUE(::erin::utils::is_superset(a, b));
  EXPECT_FALSE(::erin::utils::is_superset(b, a));
}

int
main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
