/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "adevs.h"
#include "checkout_line/clerk.h"
#include "checkout_line/customer.h"
#include "checkout_line/generator.h"
#include "checkout_line/observer.h"
#include "debug_utils.h"
#include "erin/devs.h"
#include "erin/devs/flow_limits.h"
#include "erin/devs/converter.h"
#include "erin/devs/load.h"
#include "erin/devs/mux.h"
#include "erin/devs/storage.h"
#include "erin/distribution.h"
#include "erin/erin.h"
#include "erin/fragility.h"
#include "erin/graphviz.h"
#include "erin/port.h"
#include "erin/random.h"
#include "erin/stream.h"
#include "erin/type.h"
#include "erin/utils.h"
#include "erin/version.h"
#include "erin_test_utils.h"
#include "gtest/gtest.h"
#include <functional>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <unordered_map>
#include <utility>

const double tolerance{1e-6};

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
  EXPECT_EQ(fs.get_inflow(), 0.0);
  EXPECT_EQ(fs.get_outflow(), 0.0);
  EXPECT_EQ(fs.get_storeflow(), 0.0);
  EXPECT_EQ(fs.get_lossflow(), 0.0);
  fs = ERIN::FlowState{100.0, 50.0};
  EXPECT_EQ(fs.get_inflow(), 100.0);
  EXPECT_EQ(fs.get_outflow(), 50.0);
  EXPECT_EQ(fs.get_storeflow(), 0.0);
  EXPECT_EQ(fs.get_lossflow(), 50.0);
  fs = ERIN::FlowState{100.0, 0.0, 90.0};
  EXPECT_EQ(fs.get_inflow(), 100.0);
  EXPECT_EQ(fs.get_outflow(), 0.0);
  EXPECT_EQ(fs.get_storeflow(), 90.0);
  EXPECT_EQ(fs.get_lossflow(), 10.0);
}

TEST(ErinBasicsTest, StandaloneSink)
{
  auto st = ::ERIN::StreamType("electrical");
  ERIN::RealTimeType t_max{3};
  auto sink = new ::ERIN::Sink(
      "load", ::ERIN::ComponentType::Load, st,
      {::ERIN::LoadItem{0,100},
       ::ERIN::LoadItem{1,10},
       ::ERIN::LoadItem{2,0},
       ::ERIN::LoadItem{t_max}});
  auto meter = new ::ERIN::FlowMeter(
      "meter", ::ERIN::ComponentType::Load, st);
  std::shared_ptr<ERIN::FlowWriter> fw =
    std::make_shared<ERIN::DefaultFlowWriter>();
  meter->set_flow_writer(fw);
  adevs::Digraph<::ERIN::FlowValueType, ::ERIN::Time> network;
  network.couple(
      sink, ::ERIN::Sink::outport_inflow_request,
      meter, ::ERIN::FlowMeter::inport_outflow_request);
  adevs::Simulator<::ERIN::PortValue, ::ERIN::Time> sim;
  network.add(&sim);
  while (sim.next_event_time() < ::ERIN::inf) {
    sim.exec_next_event();
  }
  std::vector<::ERIN::RealTimeType> expected_times = {0, 1, 2, 3};
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  auto actual_times = ERIN::get_times_from_results_for_component(results, "meter");
  ::erin_test_utils::compare_vectors<::ERIN::RealTimeType>(expected_times, actual_times);
  std::vector<::ERIN::FlowValueType> expected_loads = {100, 10, 0, 0};
  auto actual_loads = ERIN::get_actual_flows_from_results_for_component(results, "meter");
  ::erin_test_utils::compare_vectors<::ERIN::FlowValueType>(expected_loads, actual_loads);
}

TEST(ErinBasicsTest, CanRunSourceSink)
{
  std::vector<::ERIN::RealTimeType> expected_time = {0, 1, 2};
  std::vector<::ERIN::FlowValueType> expected_flow = {100, 0, 0};
  ERIN::RealTimeType t_max{2};
  auto st = ::ERIN::StreamType("electrical");
  auto sink = new ::ERIN::Sink(
      "sink", ::ERIN::ComponentType::Load, st, {
          ::ERIN::LoadItem{0,100},
          ::ERIN::LoadItem{1,0},
          ::ERIN::LoadItem{t_max}});
  auto meter = new ::ERIN::FlowMeter(
      "meter", ::ERIN::ComponentType::Load, st);
  std::shared_ptr<ERIN::FlowWriter> fw =
    std::make_shared<ERIN::DefaultFlowWriter>();
  meter->set_flow_writer(fw);
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
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  auto actual_time = ERIN::get_times_from_results_for_component(results, "meter");
  auto actual_flow = ERIN::get_actual_flows_from_results_for_component(results, "meter");
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
  ERIN::RealTimeType t_max{4};
  std::vector<::ERIN::RealTimeType> expected_time = {0, 1, 2, 3, t_max};
  std::vector<::ERIN::FlowValueType> expected_flow = {50, 50, 40, 0, 0};
  auto elec = ::ERIN::StreamType("electrical");
  auto meter2 = new ::ERIN::FlowMeter(
      "meter2", ::ERIN::ComponentType::Source, elec);
  auto lim = new ::ERIN::FlowLimits(
      "lim", ::ERIN::ComponentType::Source, elec, 0, 50);
  auto meter1 = new ::ERIN::FlowMeter(
      "meter1", ::ERIN::ComponentType::Load, elec);
  auto sink = new ::ERIN::Sink(
      "electric-load", ::ERIN::ComponentType::Load, elec,
      { ::ERIN::LoadItem{0, 160},
        ::ERIN::LoadItem{1,80},
        ::ERIN::LoadItem{2,40},
        ::ERIN::LoadItem{3,0},
        ::ERIN::LoadItem{t_max}});
  std::shared_ptr<ERIN::FlowWriter> fw = std::make_shared<ERIN::DefaultFlowWriter>();
  meter1->set_flow_writer(fw);
  meter2->set_flow_writer(fw);
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
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  auto actual_time1 = ERIN::get_times_from_results_for_component(results, "meter1");
  auto actual_time2 = ERIN::get_times_from_results_for_component(results, "meter2");
  auto actual_flow1 = ERIN::get_actual_flows_from_results_for_component(results, "meter1");
  auto actual_flow2 = ERIN::get_actual_flows_from_results_for_component(results, "meter2");
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
  ERIN::RealTimeType t_max{4};
  const double diesel_generator_efficiency{0.36};
  const std::vector<::ERIN::RealTimeType> expected_genset_output_times{
    0, 1, 2, 3, t_max};
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
    ::ERIN::LoadItem{t_max}};
  auto sink = new ::ERIN::Sink(
      "electric_load", ::ERIN::ComponentType::Load, elec, load_profile);
  std::shared_ptr<ERIN::FlowWriter> fw = std::make_shared<ERIN::DefaultFlowWriter>();
  diesel_fuel_meter->set_flow_writer(fw);
  genset_meter->set_flow_writer(fw);
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
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  auto actual_genset_output = ERIN::get_actual_flows_from_results_for_component(results, "genset_meter");
  auto requested_genset_output = ERIN::get_requested_flows_from_results_for_component(results, "genset_meter");
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
  auto actual_fuel_output = ERIN::get_actual_flows_from_results_for_component(results, "diesel_fuel_meter");
  auto actual_genset_output_times = ERIN::get_times_from_results_for_component(results, "genset_meter");
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
  namespace EP = ::erin::port;
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
    ::erin::network::ComponentAndPort{source_id, EP::Type::Outflow, 0},
    ::erin::network::ComponentAndPort{load_id, EP::Type::Inflow, 0}};
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
  namespace E = ::ERIN;
  std::stringstream ss{};
  ss << "[simulation_info]\n"
        "rate_unit = \"kW\"\n"
        "quantity_unit = \"kJ\"\n"
        "time_unit = \"hours\"\n"
        "max_time = 3000\n"
        "random_seed = 0\n";
  E::TomlInputReader tir{ss};
  E::SimulationInfo expected{
    "kW", "kJ", E::TimeUnits::Hours, 3000, false, 0.0, true, 0};
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
    EXPECT_EQ(e, a) << "tag = " << tag;
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
          enw::ComponentAndPort{
            "electric_utility", ep::Type::Outflow, 0},
          enw::ComponentAndPort{
            "cluster_01_electric", ep::Type::Inflow, 0}}}}};
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
        "time_unit = \"hours\"\n"
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
    EXPECT_EQ(
        e.second.get_max_occurrences(),
        a->second.get_max_occurrences());
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
        "time_unit = \"hours\"\n"
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
        "time_unit = \"hours\"\n"
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
        "time_unit = \"years\"\n"
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
        "time_unit = \"hours\"\n"
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
  std::unordered_set<std::string> expected_keys{
    "cluster_01_electric", "electric_utility"};
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
        "time_unit = \"hours\"\n"
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
  std::unordered_set<std::string> expected_keys{
    "cluster_01_electric", "electric_utility"};
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
        std::make_unique<::ERIN::SourceComponent>(
          source_id, streams[stream_id])));
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

TEST(ErinBasicsTest, ScenarioResultsMethods)
{
  namespace E = ::ERIN;
  const E::RealTimeType start_time{0};
  const E::RealTimeType duration{4};
  const std::string elec_id{"electrical"};
  const std::string A_id{"A"};
  const std::string B_id{"B"};
  const E::ScenarioResults sr{
    true,
    start_time,
    duration,
    { { A_id,
        { E::Datum{0,2.0,1.0},
          E::Datum{1,1.0,0.5},
          E::Datum{2,0.0,0.0}}},
      { B_id,
        { E::Datum{0,10.0,10.0},
          E::Datum{2,5.0,5.0},
          E::Datum{4,0.0,0.0}}}},
    { { A_id, elec_id}, { B_id, elec_id}},
    { { A_id, E::ComponentType::Load},
      { B_id, E::ComponentType::Source}}};
  using T_stream_name = std::string;
  using T_total_requested_load_kJ = E::FlowValueType;
  using T_total_energy_availability = E::FlowValueType;
  // total requested loads by stream
  std::unordered_map<T_stream_name, T_total_requested_load_kJ> trlbs_expected{
    { elec_id, 3.0}};
  auto trlbs_actual = sr.total_requested_loads_by_stream();
  ASSERT_EQ(trlbs_expected.size(), trlbs_actual.size());
  for (const auto& expected_pair : trlbs_expected) {
    const auto& key = expected_pair.first;
    const auto& value = expected_pair.second;
    auto it = trlbs_actual.find(key);
    ASSERT_TRUE(it != trlbs_actual.end());
    EXPECT_NEAR(it->second, value, tolerance);
  }
  // total achieved loads by stream
  std::unordered_map<T_stream_name, T_total_requested_load_kJ> talbs_expected{
    { elec_id, 1.5}};
  auto talbs_actual = sr.total_achieved_loads_by_stream();
  ASSERT_EQ(talbs_expected.size(), talbs_actual.size());
  for (const auto& expected_pair : talbs_expected) {
    const auto& key = expected_pair.first;
    const auto& value = expected_pair.second;
    auto it = talbs_actual.find(key);
    ASSERT_TRUE(it != talbs_actual.end());
    EXPECT_NEAR(it->second, value, tolerance);
  }
  // total energy availability by stream
  std::unordered_map<T_stream_name, T_total_energy_availability> tea_expected{
    { elec_id, 0.5}};
  auto tea_actual = sr.total_energy_availability_by_stream();
  ASSERT_EQ(tea_expected.size(), tea_actual.size());
  for (const auto& expected_pair : tea_expected) {
    const auto& key = expected_pair.first;
    const auto& value = expected_pair.second;
    auto it = tea_actual.find(key);
    ASSERT_TRUE(it != tea_actual.end());
    EXPECT_NEAR(it->second, value, tolerance);
  }
}

TEST(ErinBasicsTest, TestSumRequestedLoad)
{
  namespace E = ::ERIN;
  std::vector<E::Datum> vs{
    E::Datum{0,1.0,1.0}, E::Datum{1,0.5,0.5}, E::Datum{2,0.0,0.0}};
  E::FlowValueType expected = 1.5;
  auto actual = E::sum_requested_load(vs);
  EXPECT_NEAR(expected, actual, tolerance);
  vs = std::vector<E::Datum>{
    E::Datum{10,100.0,100.0},E::Datum{20,10.0,10.0},E::Datum{22,0.0,0.0}};
  expected = ((10.0 - 0.0) * 0.0) + ((20.0 - 10.0) * 100.0) + ((22.0 - 20.0) * 10.0);
  actual = E::sum_requested_load(vs);
  EXPECT_NEAR(expected, actual, tolerance);
  vs = std::vector<E::Datum>(0);
  expected = 0.0;
  actual = E::sum_requested_load(vs);
  EXPECT_NEAR(expected, actual, tolerance);
  try {
    vs = std::vector<E::Datum>{ E::Datum{10,1.0,1.0}, E::Datum{5,0.0,0.0}};
    actual = E::sum_requested_load(vs);
    ASSERT_TRUE(false) << "expected exception but didn't throw";
  }
  catch (const std::invalid_argument&) {
    ASSERT_TRUE(true);
  }
  catch (...) {
    ASSERT_TRUE(false) << "unexpected exception thrown";
  }
}

TEST(ErinBasicsTest, TestSumAchievedLoads)
{
  namespace E = ::ERIN;
  std::vector<E::Datum> vs{
    E::Datum{0,1.0,1.0}, E::Datum{1,0.5,0.5}, E::Datum{2,0.0,0.0}};
  E::FlowValueType expected = 1.5;
  auto actual = E::sum_achieved_load(vs);
  EXPECT_NEAR(expected, actual, tolerance);
  vs = std::vector<E::Datum>{
    E::Datum{10,200.0,100.0},E::Datum{20,20.0,10.0},E::Datum{22,0.0,0.0}};
  expected = ((10.0 - 0.0) * 0.0) + ((20.0 - 10.0) * 100.0) + ((22.0 - 20.0) * 10.0);
  actual = E::sum_achieved_load(vs);
  EXPECT_NEAR(expected, actual, tolerance);
  vs = std::vector<E::Datum>(0);
  expected = 0.0;
  actual = E::sum_achieved_load(vs);
  EXPECT_NEAR(expected, actual, tolerance);
  try {
    vs = std::vector<E::Datum>{ E::Datum{10,1.0,1.0}, E::Datum{5,0.0,0.0}};
    actual = E::sum_achieved_load(vs);
    ASSERT_TRUE(false) << "expected exception but didn't throw";
  }
  catch (const std::invalid_argument&) {
    ASSERT_TRUE(true);
  }
  catch (...) {
    ASSERT_TRUE(false) << "unexpected exception thrown";
  }
}

TEST(ErinBasicsTest, ScenarioResultsToCSV)
{
  ::ERIN::RealTimeType start_time{0};
  ::ERIN::RealTimeType duration{4};
  std::string elec_stream_id{"electrical"};
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
    {{std::string{"A"}, elec_stream_id},
     {std::string{"B"}, elec_stream_id}},
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
    {{std::string{"A"}, elec_stream_id}},
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
       std::string{"electrical"}}},
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
       std::string{"electrical"}}},
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
    0,    // RealTimeType max_downtime
    0.0,  // FlowValueType load_not_served
    4.0}; // FlowValueType total_energy
  auto actual = ::ERIN::calc_scenario_stats(ds);
  EXPECT_EQ(expected.uptime, actual.uptime);
  EXPECT_EQ(expected.downtime, actual.downtime);
  EXPECT_EQ(expected.max_downtime, actual.max_downtime);
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
  std::unordered_map<std::string,double> intensities{
    {"wind_speed_mph", 150.0}};
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
  ::ERIN::RealTimeType t{1};
  EXPECT_EQ(
      ::ERIN::time_to_seconds(t, ::ERIN::TimeUnits::Years),
      ::ERIN::rtt_seconds_per_year);
  EXPECT_EQ(
      ::ERIN::time_to_seconds(t, ::ERIN::TimeUnits::Days),
      ::ERIN::rtt_seconds_per_day);
  EXPECT_EQ(
      ::ERIN::time_to_seconds(t, ::ERIN::TimeUnits::Hours),
      ::ERIN::rtt_seconds_per_hour);
  EXPECT_EQ(
      ::ERIN::time_to_seconds(t, ::ERIN::TimeUnits::Minutes),
      ::ERIN::rtt_seconds_per_minute);
  EXPECT_EQ(
      ::ERIN::time_to_seconds(t, ::ERIN::TimeUnits::Seconds),
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
      scenario_id, network, connections, components,
      {}, []()->double{ return 0.0; });
  std::shared_ptr<ERIN::FlowWriter> fw =
    std::make_shared<ERIN::DefaultFlowWriter>();
  for (auto e: elements) {
    e->set_flow_writer(fw);
  }
  adevs::Simulator<::ERIN::PortValue, ::ERIN::Time> sim;
  network.add(&sim);
  const auto duration{t_max};
  const int max_no_advance{static_cast<int>(elements.size()) * 10};
  auto is_good = ::ERIN::run_devs(sim, duration, max_no_advance, "test");
  EXPECT_TRUE(is_good);
  fw->finalize_at_time(t_max);
  auto fw_results = fw->get_results();
  ::ERIN::RealTimeType scenario_start_time_s{0};
  auto sr = ::ERIN::process_single_scenario_results(
      is_good, elements, duration, scenario_start_time_s,
      fw_results);
  EXPECT_TRUE(sr.get_is_good());
  auto results = sr.get_results();
  EXPECT_EQ(results, fw_results);
  fw->clear();
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
  using size_type = std::vector<::ERIN::Datum>::size_type;
  size_type min_bus_inflow_0_size{std::min(n_bus_inflow0, actual_bus_inflow0.size())};
  for (size_type i{0}; i < min_bus_inflow_0_size; ++i) {
    const auto& e = expected_bus_inflow0[i];
    const auto& a = actual_bus_inflow0[i];
    EXPECT_EQ(e.time, a.time)
      << "bus-inflow(0):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.requested_value, a.requested_value, tolerance)
      << "bus-inflow(0):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.achieved_value, a.achieved_value, tolerance)
      << "bus-inflow(0):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    if (true && (n_bus_inflow0 != actual_bus_inflow0.size())) {
      std::cout << "bus-inflow(0):"
                << "expected[" << std::setw(2) << i << "]{t=" << std::setw(4) << e.time
                << ",r=" << std::setw(6) << e.requested_value
                << ",a=" << std::setw(6) << e.achieved_value << "} "
                << "| actual[" << std::setw(2) << i << "]{t=" << std::setw(4) << a.time
                << ",r=" << std::setw(6) << a.requested_value
                << ",a=" << std::setw(6) << a.achieved_value << "}\n";
    }
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
  size_type min_bus_inflow_1_size{std::min(n_bus_inflow1, actual_bus_inflow1.size())};
  for (size_type i{0}; i < min_bus_inflow_1_size; ++i) {
    const auto& e = expected_bus_inflow1[i];
    const auto& a = actual_bus_inflow1[i];
    EXPECT_EQ(e.time, a.time)
      << "bus-inflow(1):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.requested_value, a.requested_value, tolerance)
      << "bus-inflow(1):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.achieved_value, a.achieved_value, tolerance)
      << "bus-inflow(1):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    if (true && (n_bus_inflow1 != actual_bus_inflow1.size())) {
      std::cout << "bus-inflow(1):"
                << "expected[" << std::setw(2) << i << "]{t=" << std::setw(4) << e.time
                << ",r=" << std::setw(6) << e.requested_value
                << ",a=" << std::setw(6) << e.achieved_value << "} "
                << "| actual[" << std::setw(2) << i << "]{t=" << std::setw(4) << a.time
                << ",r=" << std::setw(6) << a.requested_value
                << ",a=" << std::setw(6) << a.achieved_value << "}\n";
    }
  }
  // bus-outflow(0)
  const std::vector<::ERIN::Datum> expected_bus_outflow0{
    ::ERIN::Datum{0,10.0,10.0},
    ::ERIN::Datum{5,10.0,10.0},
    ::ERIN::Datum{8,10.0,8.0},
    ::ERIN::Datum{10,10.0,10.0},
    ::ERIN::Datum{t_max,0.0,0.0}};
  const auto n_bus_outflow0 = expected_bus_outflow0.size();
  const auto& actual_bus_outflow0 = results.at("bus-outflow(0)");
  EXPECT_EQ(n_bus_outflow0, actual_bus_outflow0.size());
  size_type min_bus_outflow_0{
    std::min(n_bus_outflow0, actual_bus_outflow0.size())};
  for (size_type i{0}; i < min_bus_outflow_0; ++i) {
    const auto& e = expected_bus_outflow0[i];
    const auto& a = actual_bus_outflow0[i];
    EXPECT_EQ(e.time, a.time)
      << "bus-outflow(0):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.requested_value, a.requested_value, tolerance)
      << "bus-outflow(0):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.achieved_value, a.achieved_value, tolerance)
      << "bus-outflow(0):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    if (true && (n_bus_outflow0 != actual_bus_outflow0.size())) {
      std::cout << "bus-outflow(0):"
                << "expected[" << std::setw(2) << i << "]{t=" << std::setw(4) << e.time
                << ",r=" << std::setw(6) << e.requested_value
                << ",a=" << std::setw(6) << e.achieved_value << "} "
                << "| actual[" << std::setw(2) << i << "]{t=" << std::setw(4) << a.time
                << ",r=" << std::setw(6) << a.requested_value
                << ",a=" << std::setw(6) << a.achieved_value << "}\n";
    }
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
  size_type min_bus_outflow_1{std::min(n_bus_outflow1, actual_bus_outflow1.size())};
  for (size_type i{0}; i < min_bus_outflow_1; ++i) {
    const auto& e = expected_bus_outflow1[i];
    const auto& a = actual_bus_outflow1[i];
    EXPECT_EQ(e.time, a.time)
      << "bus-outflow(1):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.requested_value, a.requested_value, tolerance)
      << "bus-outflow(1):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    EXPECT_NEAR(e.achieved_value, a.achieved_value, tolerance)
      << "bus-outflow(1):"
      << "expected[" << i << "]{t=" << e.time
      << ",r=" << e.requested_value
      << ",a=" << e.achieved_value << "} "
      << "!= actual[" << i << "]{t=" << a.time
      << ",r=" << a.requested_value
      << ",a=" << a.achieved_value << "}";
    if (true && (n_bus_outflow1 != actual_bus_outflow1.size())) {
      std::cout << "bus-outflow(1):"
                << "expected[" << std::setw(2) << i << "]{t=" << std::setw(4) << e.time
                << ",r=" << std::setw(6) << e.requested_value
                << ",a=" << std::setw(6) << e.achieved_value << "} "
                << "| actual[" << std::setw(2) << i << "]{t=" << std::setw(4) << a.time
                << ",r=" << std::setw(6) << a.requested_value
                << ",a=" << std::setw(6) << a.achieved_value << "}\n";
    }
  }
}

TEST(ErinBasicsTest, TestAddMultipleFragilitiesToAComponent)
{
  namespace ef = erin::fragility;
  std::string id{"source"};
  auto stream = ::ERIN::StreamType{"electricity"};
  std::unordered_map<
    std::string,
    std::vector<std::unique_ptr<ef::Curve>>> frags;
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
        "fragilities = [\"highly_vulnerable_to_wind\", "
                       "\"somewhat_vulnerable_to_flooding\"]\n"
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
        "  [\"electric_utility\", \"outflow\", \"0\", "
           "\"bus\", \"inflow\", \"0\"],\n"
        "  [\"emergency_generator\", \"outflow\", \"0\", "
           "\"bus\", \"inflow\", \"1\"],\n"
        "  [\"bus\", \"cluster_01_electric\"]]\n"
        "[scenarios.blue_sky]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 0}\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n"
        "[scenarios.class_4_hurricane]\n"
        "time_unit = \"hours\"\n"
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
    = 8760 * ::ERIN::rtt_seconds_per_hour;
  constexpr int blue_sky_max_occurrence = 1;
  constexpr ::ERIN::RealTimeType hurricane_duration
    = 336 * ::ERIN::rtt_seconds_per_hour;
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
  std::unordered_set<std::string> expected_keys{
    "cluster_01_electric", "electric_utility"};
  // out.get_results() : Map String (Vector Datum)
  for (const auto& item: out.get_results()) {
    auto it = expected_keys.find(item.first);
    ASSERT_TRUE(it != expected_keys.end());
    const auto& results = item.second;
    int i{0};
    for (const auto& x : results) {
      if constexpr (::ERIN::debug_level >= ::ERIN::debug_level_high) {
        std::cout << "id: " << item.first << "\n";
        std::cout << "x[" << i << "].time            = " << x.time << "\n"
          << "x[" << i << "].achieved_value  = " << x.achieved_value
          << "\n"
          << "x[" << i << "].requested_value = " << x.requested_value
          << "\n";
      }
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
        "fragilities = [\"highly_vulnerable_to_wind\", "
                       "\"somewhat_vulnerable_to_flooding\"]\n"
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
        "  [\"electric_utility\", \"outflow\", \"0\", "
           "\"bus\", \"inflow\", \"0\"],\n"
        "  [\"emergency_generator\", \"outflow\", \"0\", "
           "\"bus\", \"inflow\", \"1\"],\n"
        "  [\"bus\", \"cluster_01_electric\"]]\n"
        "[scenarios.blue_sky]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 0}\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n"
        "[scenarios.class_4_hurricane]\n"
        "time_unit = \"hours\"\n"
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
    = 8760 * ::ERIN::rtt_seconds_per_hour;
  constexpr int blue_sky_max_occurrence = 1;
  constexpr ::ERIN::RealTimeType hurricane_duration
    = 336 * ::ERIN::rtt_seconds_per_hour;
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
          ::ERIN::Datum{4   * 3'600, 0.0, 0.0},
          ::ERIN::Datum{336 * 3'600, 0.0, 0.0}}));
  expected_results.emplace(
      std::make_pair(
        std::string{"emergency_generator"},
        std::vector<::ERIN::Datum>{
          ::ERIN::Datum{0          , 0.0, 0.0},
          ::ERIN::Datum{4   * 3'600, 0.0, 0.0},
          ::ERIN::Datum{336 * 3'600, 0.0, 0.0}}));
  expected_results.emplace(
      std::make_pair(
        std::string{"bus-inflow(0)"},
        std::vector<::ERIN::Datum>{
          ::ERIN::Datum{0          , 0.0, 0.0},
          ::ERIN::Datum{4   * 3'600, 0.0, 0.0},
          ::ERIN::Datum{336 * 3'600, 0.0, 0.0}}));
  expected_results.emplace(
      std::make_pair(
        std::string{"bus-inflow(1)"},
        std::vector<::ERIN::Datum>{
          ::ERIN::Datum{0          , 0.0, 0.0},
          ::ERIN::Datum{4   * 3'600, 0.0, 0.0},
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
  std::unordered_map<std::string,std::vector<E::Datum>> data{
    { id_cluster_01_electric,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{4 * hours_to_seconds, 0.0, 0.0}}},
    { id_electric_utility,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{4 * hours_to_seconds, 0.0, 0.0}}}};
  std::unordered_map<std::string,std::string> stream_types{
    { id_cluster_01_electric, id_electricity},
    { id_electric_utility, id_electricity}};
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
  const std::string expected_stats_csv{
    "scenario id,number of occurrences,total time in scenario (hours),"
    "component id,type,stream,energy availability,max downtime (hours),"
    "load not served (kJ),electricity energy used (kJ)\n"
    "blue_sky,1,4,cluster_01_electric,load,electricity,"
    "1,0,0,14400\n"
    "blue_sky,1,4,electric_utility,source,electricity,"
    "1,0,0,14400\n"
    "blue_sky,1,4,TOTAL (source),,,,,,14400\n"
    "blue_sky,1,4,TOTAL (load),,,,,,14400\n"};
  auto actual_stats_csv = ar.to_stats_csv();
  EXPECT_EQ(expected_stats_csv, actual_stats_csv);
}

TEST(ErinBasicsTest, ScenarioStatsAddAndAddEq)
{
  ::ERIN::ScenarioStats a{1,2,2,1.0,1.0};
  ::ERIN::ScenarioStats b{10,20,10,10.0,10.0};
  ::ERIN::ScenarioStats expected{11,22,10,11.0,11.0};
  auto c = a + b;
  EXPECT_EQ(c.uptime, expected.uptime);
  EXPECT_EQ(c.downtime, expected.downtime);
  EXPECT_EQ(c.max_downtime, expected.max_downtime);
  EXPECT_EQ(c.load_not_served, expected.load_not_served);
  EXPECT_EQ(c.total_energy, expected.total_energy);
  a += b;
  EXPECT_EQ(a.uptime, expected.uptime);
  EXPECT_EQ(a.downtime, expected.downtime);
  EXPECT_EQ(a.max_downtime, expected.max_downtime);
  EXPECT_EQ(a.load_not_served, expected.load_not_served);
  EXPECT_EQ(a.total_energy, expected.total_energy);
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
  std::unordered_map<std::string,std::vector<E::Datum>> data{
    { id_cluster_01_electric,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{4 * hours_to_seconds, 0.0, 0.0}}},
    { id_electric_utility,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{4 * hours_to_seconds, 0.0, 0.0}}}};
  std::unordered_map<std::string,std::string> stream_types{
    { id_cluster_01_electric, id_electricity},
    { id_electric_utility, id_electricity}};
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
  std::unordered_map<std::string,std::vector<E::Datum>> data{
    { id_cluster_01_electric,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{8 * hours_to_seconds, 0.0, 0.0}}},
    { id_electric_utility,
      std::vector<E::Datum>{
        E::Datum{0 * hours_to_seconds, 1.0, 1.0},
        E::Datum{8 * hours_to_seconds, 0.0, 0.0}}}};
  std::unordered_map<std::string,std::string> stream_types{
    { id_cluster_01_electric, id_electricity},
    { id_electric_utility, id_electricity}};
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

TEST(ErinBasicsTest, TestRepeatableRandom)
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 100\n"
    "fixed_random = 0.5\n"
    "#random_seed = 1\n"
    "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[loads.default]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[4.0]]\n"
    "[components.electric_utility]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "max_outflow = 100.0\n"
    "fragilities = [\"highly_vulnerable_to_wind\"]\n"
    "[components.cluster_01_electric]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "loads_by_scenario.class_4_hurricane = \"default\"\n"
    "[components.emergency_generator]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "max_outflow = 50.0\n"
    "fragilities = [\"somewhat_vulnerable_to_flooding\"]\n"
    "[components.bus]\n"
    "type = \"muxer\"\n"
    "stream = \"electricity\"\n"
    "num_inflows = 2\n"
    "num_outflows = 1\n"
    "dispatch_strategy = \"in_order\"\n"
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
    "  [\"electric_utility\", \"outflow\", \"0\", "
       "\"bus\", \"inflow\", \"0\"],\n"
    "  [\"emergency_generator\", \"outflow\", \"0\", "
       "\"bus\", \"inflow\", \"1\"],\n"
    "  [\"bus\", \"cluster_01_electric\"]]\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = {type = \"fixed\", value = 0}\n"
    "duration = 4\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "[scenarios.class_4_hurricane]\n"
    "time_unit = \"hours\"\n"
    "occurrence_distribution = {type = \"fixed\", value = 10}\n"
    "duration = 4\n"
    "max_occurrences = -1\n"
    "network = \"emergency_operations\"\n"
    "intensity.wind_speed_mph = 156\n"
    "intensity.inundation_depth_ft = 8\n";
  namespace E = ::ERIN;
  auto m = E::make_main_from_string(input);
  auto results = m.run_all();
  std::vector<std::string> expected_scenario_ids{
    "blue_sky", "class_4_hurricane"};
  std::unordered_map<
    std::string,
    std::vector<E::ScenarioResults>::size_type> expected_num_results{
      {"blue_sky", 1},
      {"class_4_hurricane", 10}};
  EXPECT_EQ(
      expected_scenario_ids.size(),
      results.number_of_scenarios());
  EXPECT_EQ(expected_scenario_ids, results.get_scenario_ids());
  EXPECT_EQ(expected_num_results, results.get_num_results());
  std::unordered_map<
    std::string,
    std::unordered_map<std::string, std::vector<double>>> expected_teas;
  expected_teas.emplace(
      std::make_pair(
        std::string{"blue_sky"},
        std::unordered_map<
          std::string,
          std::vector<double>>{{std::string{"electricity"}, {1.0}}}));
  const std::vector<E::FlowValueType> ten_halves(10, 0.5);
  expected_teas.emplace(
      std::make_pair(
        std::string{"class_4_hurricane"},
        std::unordered_map<std::string, std::vector<double>>{
          {std::string{"electricity"}, ten_halves}}));
  auto actual_teas = results.get_total_energy_availabilities();
  ASSERT_EQ(expected_teas.size(), actual_teas.size());
  for (const auto& pair : expected_teas) {
    const auto& scenario_id = pair.first;
    const auto& expected_stream_to_teas_map = pair.second;
    auto it = actual_teas.find(scenario_id);
    ASSERT_TRUE(it != actual_teas.end());
    const auto& actual_stream_to_teas_map = it->second;
    ASSERT_EQ(
        expected_stream_to_teas_map.size(),
        actual_stream_to_teas_map.size());
    for (const auto& sub_pair : expected_stream_to_teas_map) {
      const auto& stream_name = sub_pair.first;
      const auto& expected_teas = sub_pair.second;
      auto sub_it = actual_stream_to_teas_map.find(stream_name);
      ASSERT_TRUE(sub_it != actual_stream_to_teas_map.end());
      const auto& actual_teas = sub_it->second;
      auto num_teas = expected_teas.size();
      ASSERT_EQ(num_teas, actual_teas.size());
      for (decltype(num_teas) i{0}; i < num_teas; ++i) {
        EXPECT_NEAR(expected_teas[i], actual_teas[i], tolerance)
          << scenario_id << ":" << stream_name << "[" << i << "]"
          << " expected_teas=" << expected_teas[i]
          << " actual_teas  =" << actual_teas[i] << "\n";
      }
    }
  }
}

TEST(ErinBasicsTest, TestRepeatableRandom2)
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 100\n"
    "fixed_random = 0.1\n"
    "#random_seed = 1\n"
    "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[loads.default]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[4.0]]\n"
    "[components.electric_utility]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "max_outflow = 100.0\n"
    "fragilities = [\"highly_vulnerable_to_wind\"]\n"
    "[components.cluster_01_electric]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "loads_by_scenario.class_4_hurricane = \"default\"\n"
    "[components.emergency_generator]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "max_outflow = 50.0\n"
    "fragilities = [\"somewhat_vulnerable_to_flooding\"]\n"
    "[components.bus]\n"
    "type = \"muxer\"\n"
    "stream = \"electricity\"\n"
    "num_inflows = 2\n"
    "num_outflows = 1\n"
    "dispatch_strategy = \"in_order\"\n"
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
    "  [\"electric_utility\", \"outflow\", \"0\", "
       "\"bus\", \"inflow\", \"0\"],\n"
    "  [\"emergency_generator\", \"outflow\", \"0\", "
       "\"bus\", \"inflow\", \"1\"],\n"
    "  [\"bus\", \"cluster_01_electric\"]]\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = {type = \"fixed\", value = 0}\n"
    "duration = 4\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "[scenarios.class_4_hurricane]\n"
    "time_unit = \"hours\"\n"
    "occurrence_distribution = {type = \"fixed\", value = 10}\n"
    "duration = 4\n"
    "max_occurrences = -1\n"
    "network = \"emergency_operations\"\n"
    "intensity.wind_speed_mph = 156\n"
    "intensity.inundation_depth_ft = 8\n";
  namespace E = ::ERIN;
  auto m = E::make_main_from_string(input);
  auto results = m.run_all();
  std::vector<std::string> expected_scenario_ids{
    "blue_sky", "class_4_hurricane"};
  std::unordered_map<
    std::string,
    std::vector<E::ScenarioResults>::size_type> expected_num_results{
      {"blue_sky", 1},
      {"class_4_hurricane", 10}};
  EXPECT_EQ(
      expected_scenario_ids.size(),
      results.number_of_scenarios());
  EXPECT_EQ(expected_scenario_ids, results.get_scenario_ids());
  EXPECT_EQ(expected_num_results, results.get_num_results());
  std::unordered_map<
    std::string,
    std::unordered_map<std::string, std::vector<double>>> expected_teas;
  expected_teas.emplace(
      std::make_pair(
        std::string{"blue_sky"},
        std::unordered_map<
          std::string,
          std::vector<double>>{{std::string{"electricity"}, {1.0}}}));
  const std::vector<E::FlowValueType> ten_zeros(10, 0.0);
  expected_teas.emplace(
      std::make_pair(
        std::string{"class_4_hurricane"},
        std::unordered_map<std::string, std::vector<double>>{
          {std::string{"electricity"}, ten_zeros}}));
  auto actual_teas = results.get_total_energy_availabilities();
  ASSERT_EQ(expected_teas.size(), actual_teas.size());
  for (const auto& pair : expected_teas) {
    const auto& scenario_id = pair.first;
    const auto& expected_stream_to_teas_map = pair.second;
    auto it = actual_teas.find(scenario_id);
    ASSERT_TRUE(it != actual_teas.end());
    const auto& actual_stream_to_teas_map = it->second;
    ASSERT_EQ(
        expected_stream_to_teas_map.size(),
        actual_stream_to_teas_map.size());
    for (const auto& sub_pair : expected_stream_to_teas_map) {
      const auto& stream_name = sub_pair.first;
      const auto& expected_teas = sub_pair.second;
      auto sub_it = actual_stream_to_teas_map.find(stream_name);
      ASSERT_TRUE(sub_it != actual_stream_to_teas_map.end());
      const auto& actual_teas = sub_it->second;
      auto num_teas = expected_teas.size();
      ASSERT_EQ(num_teas, actual_teas.size());
      for (decltype(num_teas) i{0}; i < num_teas; ++i) {
        EXPECT_NEAR(expected_teas[i], actual_teas[i], tolerance)
          << scenario_id << ":" << stream_name << "[" << i << "]"
          << " expected_teas=" << expected_teas[i]
          << " actual_teas  =" << actual_teas[i] << "\n";
      }
    }
  }
}

TEST(ErinBasicsTest, TestThatRandomProcessWorks)
{
  namespace E = ::ERIN;
  E::SimulationInfo si{"kW", "kJ", E::TimeUnits::Hours, 4};
  auto f = si.make_random_function();
  double previous{0.0};
  double current{0.0};
  bool passed{false};
  const int max_tries{100};
  for (int i{0}; i < max_tries; ++i) {
    current = f();
    if ((i != 0) && (previous != current)) {
      passed = true;
      break;
    }
    previous = current;
  }
  ASSERT_TRUE(passed);
}

TEST(ErinBasicsTest, TestThatRandomProcessDoesNotCreateTheSameSeriesTwice)
{
  namespace E = ::ERIN;
  E::SimulationInfo si1{"kW", "kJ", E::TimeUnits::Hours, 4, false, 0.0};
  E::SimulationInfo si2{"kW", "kJ", E::TimeUnits::Hours, 4, false, 0.0};
  ASSERT_NE(si1.get_random_seed(), si2.get_random_seed());
  auto f1 = si1.make_random_function();
  auto f2 = si2.make_random_function();
  const int num_queries{100};
  std::vector<double> series1(num_queries);
  std::vector<double> series2(num_queries);
  for (int i{0}; i < num_queries; ++i) {
    series1[i] = f1();
    series2[i] = f2();
  }
  EXPECT_NE(series1, series2);
}

TEST(ErinBasicsTest, TestThatRandomProcessCreatesTheSameSeriesTwiceIfSeeded)
{
  namespace E = ::ERIN;
  unsigned int seed{1};
  E::SimulationInfo si1{
    "kW", "kJ", E::TimeUnits::Hours, 4, false, 0.0, true, seed};
  E::SimulationInfo si2{
    "kW", "kJ", E::TimeUnits::Hours, 4, false, 0.0, true, seed};
  EXPECT_TRUE(si1.has_random_seed());
  EXPECT_TRUE(si2.has_random_seed());
  EXPECT_EQ(seed, si1.get_random_seed());
  EXPECT_EQ(seed, si2.get_random_seed());
  auto f1 = si1.make_random_function();
  auto f2 = si2.make_random_function();
  const int num_queries{100};
  std::vector<double> series1(num_queries, 2.0);
  std::vector<double> series2(num_queries, 2.0);
  for (int i{0}; i < num_queries; ++i) {
    series1[i] = f1();
    series2[i] = f2();
  }
  EXPECT_EQ(series1, series2);
}

TEST(ErinBasicsTest, TestRepeatableRandom3)
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 100\n"
    "random_seed = 1\n"
    "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[loads.default]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[4.0]]\n"
    "[components.electric_utility]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "max_outflow = 100.0\n"
    "fragilities = [\"highly_vulnerable_to_wind\"]\n"
    "[components.cluster_01_electric]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "loads_by_scenario.class_4_hurricane = \"default\"\n"
    "[components.emergency_generator]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "max_outflow = 50.0\n"
    "fragilities = [\"somewhat_vulnerable_to_flooding\"]\n"
    "[components.bus]\n"
    "type = \"muxer\"\n"
    "stream = \"electricity\"\n"
    "num_inflows = 2\n"
    "num_outflows = 1\n"
    "dispatch_strategy = \"in_order\"\n"
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
    "  [\"electric_utility\", \"outflow\", \"0\", "
       "\"bus\", \"inflow\", \"0\"],\n"
    "  [\"emergency_generator\", \"outflow\", \"0\", "
       "\"bus\", \"inflow\", \"1\"],\n"
    "  [\"bus\", \"cluster_01_electric\"]]\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = {type = \"fixed\", value = 0}\n"
    "duration = 4\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "[scenarios.class_4_hurricane]\n"
    "time_unit = \"hours\"\n"
    "occurrence_distribution = {type = \"fixed\", value = 10}\n"
    "duration = 4\n"
    "max_occurrences = -1\n"
    "network = \"emergency_operations\"\n"
    "intensity.wind_speed_mph = 156\n"
    "intensity.inundation_depth_ft = 8\n";
  namespace E = ::ERIN;
  auto m1 = E::make_main_from_string(input);
  auto results1 = m1.run_all();
  auto m2 = E::make_main_from_string(input);
  auto results2 = m2.run_all();
  EXPECT_EQ(results1, results2);
}

TEST(ErinBasicsTest, ScenarioResultsEquality)
{
  namespace E = ::ERIN;
  bool is_good{true};
  E::RealTimeType start_time_s{0};
  E::RealTimeType max_time_s{60};
  E::ScenarioResults sr1{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  E::ScenarioResults sr2{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  ASSERT_EQ(sr1, sr2);
  E::ScenarioResults sr3{
    is_good,
    start_time_s + 2,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  ASSERT_NE(sr1, sr3);
  ASSERT_NE(sr2, sr3);
  E::ScenarioResults sr4{
    !is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  ASSERT_NE(sr1, sr4);
  ASSERT_NE(sr2, sr4);
  auto mt2{max_time_s - 1};
  E::ScenarioResults sr5{
    is_good,
    start_time_s,
    mt2,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{mt2 - 1,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{mt2 - 1,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  ASSERT_NE(sr1, sr5);
  ASSERT_NE(sr2, sr5);
  E::ScenarioResults sr6{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"C", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"C", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"C", E::ComponentType::Source}}};
  ASSERT_NE(sr1, sr6);
  ASSERT_NE(sr2, sr6);
  E::ScenarioResults sr7{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  ASSERT_NE(sr1, sr7);
  ASSERT_NE(sr2, sr7);
  E::ScenarioResults sr8{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.5,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  ASSERT_NE(sr1, sr8);
  ASSERT_NE(sr2, sr8);
  E::ScenarioResults sr9{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"C", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  ASSERT_NE(sr1, sr9);
  ASSERT_NE(sr2, sr9);
  E::ScenarioResults sr10{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"gasoline"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  ASSERT_NE(sr1, sr10);
  ASSERT_NE(sr2, sr10);
  E::ScenarioResults sr11{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"C", E::ComponentType::Source}}};
  ASSERT_NE(sr1, sr11);
  ASSERT_NE(sr2, sr11);
  E::ScenarioResults sr12{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Load}}};
  ASSERT_NE(sr1, sr12);
  ASSERT_NE(sr2, sr12);
}

TEST(ErinBasicsTest, AllResultsEquality)
{
  namespace E = ::ERIN;
  bool is_good{true};
  E::RealTimeType start_time_s{0};
  E::RealTimeType max_time_s{60};
  E::ScenarioResults sr1{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  E::ScenarioResults sr2{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"B", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"B", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}}};
  E::ScenarioResults sr3{
    is_good,
    start_time_s,
    max_time_s,
    std::unordered_map<std::string,std::vector<E::Datum>>{
      {"A", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}},
      {"C", {E::Datum{start_time_s,1.0,1.0}, E::Datum{max_time_s,0.0,0.0}}}},
    std::unordered_map<std::string,std::string>{
      {"A", std::string{"electricity"}},
      {"C", std::string{"electricity"}}},
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"C", E::ComponentType::Source}}};
  std::unordered_map<std::string, std::vector<E::ScenarioResults>> sr_map1{
    {"A", {sr1}},
    {"B", {sr1, sr2}}};
  std::unordered_map<std::string, std::vector<E::ScenarioResults>> sr_map2{
    {"A", {sr1}},
    {"B", {sr1, sr2}}};
  std::unordered_map<std::string, std::vector<E::ScenarioResults>> sr_map3{
    {"A", {sr1}}};
  std::unordered_map<std::string, std::vector<E::ScenarioResults>> sr_map4{
    {"A", {sr1, sr2}},
    {"C", {sr1}}};
  std::unordered_map<std::string, std::vector<E::ScenarioResults>> sr_map5{
    {"A", {sr1, sr2}},
    {"B", {sr1, sr2}}};
  std::unordered_map<std::string, std::vector<E::ScenarioResults>> sr_map6{
    {"A", {sr1}},
    {"B", {sr1, sr3}}};
  const E::AllResults ar1{is_good, sr_map1};
  const E::AllResults ar2{is_good, sr_map2};
  ASSERT_EQ(ar1, ar2);
  const E::AllResults ar3{!is_good, sr_map1};
  ASSERT_NE(ar1, ar3);
  const E::AllResults ar4{is_good, sr_map3};
  ASSERT_NE(ar1, ar4);
  const E::AllResults ar5{is_good, sr_map4};
  ASSERT_NE(ar1, ar5);
  const E::AllResults ar6{is_good, sr_map5};
  ASSERT_NE(ar1, ar6);
  const E::AllResults ar7{is_good, sr_map6};
  ASSERT_NE(ar1, ar7);
}

TEST(ErinBasicsTest, TestWeCanReadDistributionWithOptionalTimeUnits)
{
  namespace E = ::ERIN;
  std::stringstream ss{};
  ss << "[scenarios.a]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", time_unit = \"years\", value = 10}\n"
        "duration = 10\n"
        "max_occurrences = 1\n"
        "network = \"nw_A\"\n"
        "[scenarios.b]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = {type = \"fixed\", value = 10}\n"
        "duration = 10\n"
        "max_occurrences = 1\n"
        "network = \"nw_B\"\n";
  E::TomlInputReader t{ss};
  auto scenario_map = t.read_scenarios();
  auto a_it = scenario_map.find("a");
  ASSERT_TRUE(a_it != scenario_map.end());
  auto b_it = scenario_map.find("b");
  ASSERT_TRUE(b_it != scenario_map.end());
  auto& scenario_a = a_it->second;
  auto dt_a = scenario_a.ta().real;
  EXPECT_EQ(dt_a, E::rtt_seconds_per_year * 10);
  auto& scenario_b = b_it->second;
  auto dt_b = scenario_b.ta().real;
  EXPECT_EQ(dt_b, 10);
}

ERIN::Main
load_example_results(
    const std::vector<double>& fixed_rolls,
    double intensity = 15.0)
{
  namespace E = ::ERIN;
  std::string random_line{};
  auto num_rolls{fixed_rolls.size()};
  if (num_rolls == 1) {
    std::ostringstream oss;
    oss << "fixed_random = " << fixed_rolls[0] << "\n";
    random_line = oss.str();
  }
  else if (num_rolls > 1) {
    std::ostringstream oss;
    oss << "fixed_random_series = ";
    std::string delim{"["};
    for (const auto& x: fixed_rolls) {
      oss << delim << x;
      delim = ",";
    }
    oss << "]\n";
    random_line = oss.str();
  }
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"years\"\n"
    "max_time = 40\n" + random_line + "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[loads.load01]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1.0],[10.0]]\n"
    "[components.A]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "fragilities = [\"frag01\"]\n"
    "[components.B]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.scenario01 = \"load01\"\n"
    "[fragility.frag01]\n"
    "vulnerable_to = \"intensity01\"\n"
    "type = \"linear\"\n"
    "lower_bound = 10.0\n"
    "upper_bound = 20.0\n"
    "[networks.nw01]\n"
    "connections = [[\"A\", \"B\"]]\n"
    "[scenarios.scenario01]\n"
    "time_unit = \"hours\"\n"
    "occurrence_distribution = {type = \"fixed\", time_unit = \"years\", value = 10}\n"
    "duration = 10\n"
    "max_occurrences = -1\n"
    "network = \"nw01\"\n"
    "intensity.intensity01 = " + std::to_string(intensity) + "\n";
  return E::make_main_from_string(input);
}

TEST(ErinBasicsTest, TestThatMaxDowntimeIsMaxContiguousDowntime)
{
  namespace E = ::ERIN;
  const std::string scenario_id{"scenario01"};
  const E::RealTimeType scenario_duration_hrs{10};
  const E::RealTimeType scenario_duration_s{
    scenario_duration_hrs * E::rtt_seconds_per_hour};
  auto m = load_example_results({0.5}, 30.0);
  const auto& si = m.get_sim_info();
  EXPECT_EQ(si.get_random_type(), E::RandomType::FixedProcess);
  auto results = m.run_all();
  ASSERT_TRUE(results.get_is_good());
  auto actual_number_of_scenarios = results.number_of_scenarios();
  decltype(actual_number_of_scenarios) expected_number_of_scenarios{1};
  EXPECT_EQ(expected_number_of_scenarios, actual_number_of_scenarios);
  auto stats = results.get_stats();
  ASSERT_EQ(expected_number_of_scenarios, stats.size());
  auto all_ss_it = stats.find(scenario_id);
  ASSERT_TRUE(all_ss_it != stats.end());
  const auto& all_ss = all_ss_it->second;
  const std::vector<E::ScenarioResults>::size_type expected_num_occurrences{4};
  EXPECT_EQ(all_ss.num_occurrences, expected_num_occurrences);
  const std::unordered_map<std::string, E::RealTimeType>::size_type expected_num_comps{2};
  EXPECT_EQ(expected_num_comps, results.get_comp_ids().size());
  EXPECT_EQ(expected_num_comps, all_ss.max_downtime_by_comp_id_s.size());
  EXPECT_EQ(all_ss.max_downtime_by_comp_id_s.at("A"), scenario_duration_s);
  EXPECT_EQ(all_ss.max_downtime_by_comp_id_s.at("B"), scenario_duration_s);
  EXPECT_EQ(all_ss.energy_availability_by_comp_id.at("A"), 0.0);
  EXPECT_EQ(all_ss.energy_availability_by_comp_id.at("B"), 0.0);
  auto bad_results = results.with_is_good_as(false);
  auto bad_stats = bad_results.get_stats();
  EXPECT_EQ(bad_stats.size(), 0);
}

TEST(ErinBasicsTest, TestThatEnergyAvailabilityIsCorrect)
{
  namespace E = ::ERIN;
  const std::string scenario_id{"scenario01"};
  const E::RealTimeType scenario_duration_hrs{10};
  const E::RealTimeType scenario_duration_s{
    scenario_duration_hrs * E::rtt_seconds_per_hour};
  // should translate to a [not-failed, not-failed, failed, failed] result for component A.
  auto m = load_example_results({0.75, 0.75, 0.25, 0.25});
  const auto& si = m.get_sim_info();
  ASSERT_EQ(si.get_random_type(), E::RandomType::FixedSeries);
  auto results = m.run_all();
  ASSERT_TRUE(results.get_is_good());
  auto actual_number_of_scenarios = results.number_of_scenarios();
  decltype(actual_number_of_scenarios) expected_number_of_scenarios{1};
  EXPECT_EQ(expected_number_of_scenarios, actual_number_of_scenarios);
  auto stats = results.get_stats();
  ASSERT_EQ(expected_number_of_scenarios, stats.size());
  auto all_ss_it = stats.find(scenario_id);
  ASSERT_TRUE(all_ss_it != stats.end());
  const auto& all_ss = all_ss_it->second;
  const std::vector<E::ScenarioResults>::size_type expected_num_occurrences{4};
  EXPECT_EQ(all_ss.num_occurrences, expected_num_occurrences);
  const std::unordered_map<std::string, E::RealTimeType>::size_type expected_num_comps{2};
  EXPECT_EQ(expected_num_comps, results.get_comp_ids().size());
  EXPECT_EQ(expected_num_comps, all_ss.max_downtime_by_comp_id_s.size());
  EXPECT_EQ(all_ss.max_downtime_by_comp_id_s.at("A"), scenario_duration_s);
  EXPECT_EQ(all_ss.max_downtime_by_comp_id_s.at("B"), scenario_duration_s);
  EXPECT_EQ(all_ss.energy_availability_by_comp_id.at("A"), 0.5);
  EXPECT_EQ(all_ss.energy_availability_by_comp_id.at("B"), 0.5);
}

TEST(ErinBasicsTest, TestRandomProcesses)
{
  double expected{0.5};
  auto fp = ERIN::FixedProcess{expected};
  EXPECT_EQ(fp.call(), expected);
  std::vector<double> series{0.1, 0.2, 0.3};
  auto fs = ERIN::FixedSeries{series};
  auto fs_alt = ERIN::FixedSeries{series};
  EXPECT_EQ(fs, fs_alt);
  fs_alt.call();
  EXPECT_NE(fs, fs_alt);
  EXPECT_EQ(fs.call(), series[0]);
  EXPECT_EQ(fs.call(), series[1]);
  EXPECT_EQ(fs.call(), series[2]);
  EXPECT_EQ(fs.call(), series[0]);
  std::unique_ptr<ERIN::RandomInfo> a = std::make_unique<ERIN::FixedSeries>(series);
  std::unique_ptr<ERIN::RandomInfo> b = std::make_unique<ERIN::FixedSeries>(series);
  EXPECT_EQ(a, b);
  b->call();
  EXPECT_NE(a, b);
}

TEST(ErinBasicsTest, Test_that_we_can_specify_different_random_processes)
{
  std::string stub = 
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"years\"\n"
    "max_time = 40\n";
  unsigned int seed{17};
  std::vector<std::string> inputs{
    "fixed_random = 0.5", 
    "fixed_random_series = [0.25,0.5,0.75]",
    "random_seed = " + std::to_string(seed),
    ""};
  std::vector<ERIN::RandomType> expected_types{
    ERIN::RandomType::FixedProcess,
    ERIN::RandomType::FixedSeries,
    ERIN::RandomType::RandomProcess,
    ERIN::RandomType::RandomProcess};
  std::vector<bool> expect_known_seed{false, false, true, false};
  std::vector<unsigned int> expected_seeds{0, 0, seed, 0};
  using size_type = std::vector<std::string>::size_type;
  for (size_type i{0}; i < inputs.size(); ++i) {
    std::stringstream ss{};
    ss << stub << inputs.at(i) << "\n";
    ERIN::TomlInputReader tir{ss};
    auto si = tir.read_simulation_info();
    const auto& expected_type = expected_types.at(i);
    EXPECT_EQ(si.get_random_type(), expected_type)
      << "i = " << i << "\n"
      << "inputs[i] = " << inputs.at(i);
    if (expect_known_seed.at(i)) {
      EXPECT_TRUE(si.has_random_seed());
      EXPECT_EQ(si.get_random_seed(), expected_seeds.at(i));
    }
  }
}

ERIN::Main
load_converter_example()
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[streams.diesel]\n"
    "type = \"diesel\"\n"
    "[loads.load01]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1.0],[10.0]]\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"diesel\"\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.scenario01 = \"load01\"\n"
    "[components.C]\n"
    "type = \"converter\"\n"
    "input_stream = \"diesel\"\n"
    "output_stream = \"electricity\"\n"
    "constant_efficiency = 0.5\n"
    "[networks.nw01]\n"
    "connections = [[\"S\", \"C\"], [\"C\", \"L\"]]\n"
    "[scenarios.scenario01]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = {type = \"fixed\", value = 0}\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"nw01\"\n";
  return ERIN::make_main_from_string(input);
}

TEST(ErinBasicsTest, Test_that_we_can_simulate_with_a_converter)
{
  auto m = load_converter_example();
  const auto& comps = m.get_components();
  using size_type = std::unordered_map<
    std::string, std::unique_ptr<ERIN::Component>>::size_type;
  const size_type expected_num_components{3};
  EXPECT_EQ(expected_num_components, comps.size());
  auto results = m.run("scenario01");
  EXPECT_TRUE(results.get_is_good());
  auto stats_by_comp_id = results.get_statistics();
  // num_components + 2 because for converter we have an input, output, and
  // lossport meter
  EXPECT_EQ(stats_by_comp_id.size(), expected_num_components + 2);
  auto load_stats = stats_by_comp_id.at("L");
  ERIN::RealTimeType scenario_duration_s{10};
  ERIN::FlowValueType load_kW{1.0};
  ERIN::FlowValueType expected_load_energy_kJ{load_kW * scenario_duration_s};
  EXPECT_EQ(load_stats.total_energy, expected_load_energy_kJ);
  ERIN::FlowValueType const_eff{0.5};
  ERIN::FlowValueType expected_source_energy_kJ{
    expected_load_energy_kJ / const_eff};
  auto source_stats = stats_by_comp_id.at("S");
  EXPECT_EQ(source_stats.total_energy, expected_source_energy_kJ);
  const auto& conv = comps.at("C");
  std::unique_ptr<ERIN::Component> expected_conv =
    std::make_unique<ERIN::ConverterComponent>(
        std::string{"C"},
        ERIN::StreamType{std::string{"diesel"}},
        ERIN::StreamType{std::string{"electricity"}},
        ERIN::StreamType{std::string{"waste_heat"}},
        const_eff);
  EXPECT_EQ(expected_conv, conv);
  std::ostringstream oss;
  oss << conv;
  std::string expected_str{
    "ConverterComponent(id=C, component_type=converter, "
                       "input_stream=StreamType(type=\"diesel\", "
                                               "rate_units=\"kW\", "
                                               "quantity_units=\"kJ\", "
                                               "seconds_per_time_unit=1, "
                                               "other_rate_units={}, "
                                               "other_quantity_units={}), "
                       "output_stream=StreamType(type=\"electricity\", "
                                                "rate_units=\"kW\", "
                                                "quantity_units=\"kJ\", "
                                                "seconds_per_time_unit=1, "
                                                "other_rate_units={}, "
                                                "other_quantity_units={}), "
                       "fragilities=..., has_fragilities=false, "
                       "const_eff=0.5)"};
  EXPECT_EQ(oss.str(), expected_str);
}

ERIN::Main
load_combined_heat_and_power_example()
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[streams.natural_gas]\n"
    "type = \"natural_gas\"\n"
    "[streams.district_hot_water]\n"
    "type = \"district_hot_water\"\n"
    "[streams.waste_heat]\n"
    "type = \"waste_heat\"\n"
    "[loads.electric_load]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0]]\n"
    "[loads.heating_load]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1.0],[10.0]]\n"
    "[loads.waste_heat_load]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1000.0],[10.0]]\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "outflow = \"natural_gas\"\n"
    "[components.LE]\n"
    "type = \"load\"\n"
    "inflow = \"electricity\"\n"
    "loads_by_scenario.scenario01 = \"electric_load\"\n"
    "[components.LT]\n"
    "type = \"load\"\n"
    "inflow = \"district_hot_water\"\n"
    "loads_by_scenario.scenario01 = \"heating_load\"\n"
    "[components.LW]\n"
    "type = \"load\"\n"
    "description = \"waste heat load\"\n"
    "inflow = \"waste_heat\"\n"
    "loads_by_scenario.scenario01 = \"waste_heat_load\"\n"
    "[components.M]\n"
    "type = \"muxer\"\n"
    "stream = \"waste_heat\"\n"
    "num_inflows = 1\n"
    "num_outflows = 2\n"
    "dispatch_strategy = \"in_order\"\n"
    "outflow_dispatch_strategy = \"in_order\"\n"
    "[components.C0]\n"
    "type = \"converter\"\n"
    "inflow = \"natural_gas\"\n"
    "outflow = \"electricity\"\n"
    "lossflow = \"waste_heat\"\n"
    "constant_efficiency = 0.5\n"
    "[components.C1]\n"
    "type = \"converter\"\n"
    "inflow = \"waste_heat\"\n"
    "outflow = \"district_hot_water\"\n"
    "constant_efficiency = 0.5\n"
    "dispatch_strategy = \"dump_load\"\n"
    "[networks.nw01]\n"
    "connections = [[\"S\", \"C0\"], "
                   "[\"C0\", \"LE\"], "
                   "[\"C0\", \"outflow\", \"1\", \"M\", \"inflow\", \"0\"], "
                   "[\"M\", \"outflow\", \"0\", \"C1\", \"inflow\", \"0\"], "
                   "[\"M\", \"outflow\", \"1\", \"LW\", \"inflow\", \"0\"], "
                   "[\"C1\", \"LT\"]]\n"
    "[scenarios.scenario01]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = {type = \"fixed\", value = 0}\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"nw01\"\n";
  return ERIN::make_main_from_string(input);
}

TEST(ErinBasicsTest, Test_that_we_can_simulate_with_a_CHP_converter)
{
  using size_type =
    std::unordered_map<std::string, std::unique_ptr<ERIN::Component>>::size_type;
  auto m = load_combined_heat_and_power_example();
  const auto& comps = m.get_components();
  const size_type expected_num_components{7};
  EXPECT_EQ(expected_num_components, comps.size());
  auto results = m.run("scenario01");
  EXPECT_TRUE(results.get_is_good());
  auto stats_by_comp_id = results.get_statistics();
  EXPECT_EQ(stats_by_comp_id.size(), expected_num_components + 6);
  auto electrical_load_stats = stats_by_comp_id.at("LE");
  ERIN::RealTimeType scenario_duration_s{10};
  ERIN::FlowValueType electrical_load_kW{10.0};
  ERIN::FlowValueType expected_electrical_load_energy_kJ{
    electrical_load_kW * scenario_duration_s};
  EXPECT_EQ(
      electrical_load_stats.total_energy,
      expected_electrical_load_energy_kJ);
  ERIN::FlowValueType const_eff{0.5};
  ERIN::FlowValueType expected_source_energy_kJ{
    expected_electrical_load_energy_kJ / const_eff};
  auto source_stats = stats_by_comp_id.at("S");
  EXPECT_EQ(source_stats.total_energy, expected_source_energy_kJ);
  auto thermal_load_stats = stats_by_comp_id.at("LT");
  ERIN::FlowValueType thermal_load_kW{1.0};
  ERIN::FlowValueType expected_thermal_load_energy_kJ{
    thermal_load_kW * scenario_duration_s};
  EXPECT_EQ(
      thermal_load_stats.total_energy,
      expected_thermal_load_energy_kJ);
  ERIN::FlowValueType expected_waste_energy_kJ{
    (expected_source_energy_kJ * const_eff)
    - (expected_thermal_load_energy_kJ / const_eff)};
  auto waste_energy_stats = stats_by_comp_id.at("LW");
  EXPECT_EQ(
      waste_energy_stats.total_energy,
      expected_waste_energy_kJ);
}

TEST(ErinDevs, Test_smart_port_object)
{
  namespace D = erin::devs;
  namespace E = ERIN;
  D::Port p{};
  E::RealTimeType t_init{-1};
  E::RealTimeType t0{0};
  E::RealTimeType t1{10};
  E::RealTimeType t2{20};
  E::FlowValueType v0{0.0};
  E::FlowValueType v1{100.0};
  E::FlowValueType v2{10.0};
  EXPECT_EQ(p.get_time_of_last_change(), t_init);
  EXPECT_EQ(p.get_requested(), v0);
  EXPECT_EQ(p.get_achieved(), v0);
  EXPECT_FALSE(p.should_propagate_request_at(t0));
  EXPECT_FALSE(p.should_propagate_achieved_at(t0));
  EXPECT_FALSE(p.should_propagate_request_at(t1));
  EXPECT_FALSE(p.should_propagate_achieved_at(t1));
  EXPECT_FALSE(p.should_propagate_request_at(t2));
  EXPECT_FALSE(p.should_propagate_achieved_at(t2));
  auto p1 = p.with_requested(v1, t1);
  EXPECT_EQ(p1.get_time_of_last_change(), t1);
  try {
    auto p_junk = p1.with_requested(v2, t0);
    ASSERT_FALSE(true) << "didn't catch a reverse time exception...";
  } catch (const std::invalid_argument&) {
    // passed
  }
  EXPECT_EQ(p1.get_requested(), v1);
  EXPECT_EQ(p1.get_achieved(), v1);
  EXPECT_FALSE(p1.should_propagate_request_at(t0));
  EXPECT_FALSE(p1.should_propagate_achieved_at(t0));
  EXPECT_TRUE(p1.should_propagate_request_at(t1));
  EXPECT_FALSE(p1.should_propagate_achieved_at(t1));
  EXPECT_FALSE(p1.should_propagate_request_at(t2));
  EXPECT_FALSE(p1.should_propagate_achieved_at(t2));
  auto p1a = p1.with_achieved(v2, t1);
  EXPECT_EQ(p1a.get_time_of_last_change(), t1);
  EXPECT_EQ(p1a.get_requested(), p1.get_requested());
  EXPECT_EQ(p1a.get_achieved(), v2);
  auto p2 = p1.with_requested(v1, t2);
  EXPECT_EQ(p2.get_time_of_last_change(), t1);
  EXPECT_EQ(p2.get_requested(), v1);
  EXPECT_EQ(p2.get_achieved(), v1);
  EXPECT_FALSE(p2.should_propagate_request_at(t0));
  EXPECT_FALSE(p2.should_propagate_achieved_at(t0));
  std::cout << "p=" << p << "\n";
  std::cout << "p1=" << p1 << "\n";
  std::cout << "p2=" << p2 << "\n";
  EXPECT_TRUE(p2.should_propagate_request_at(t1));
  EXPECT_FALSE(p2.should_propagate_achieved_at(t1));
  EXPECT_FALSE(p2.should_propagate_request_at(t2));
  EXPECT_FALSE(p2.should_propagate_achieved_at(t2));
  //auto p2 = p1.with_achieved(10.0);
  //EXPECT_EQ(p2.get_requested(), 20.0);
  //EXPECT_EQ(p2.get_achieved(), 10.0);
}

TEST(ErinComponents, Test_passthrough_component)
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[loads.load0]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0]]\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "[components.P]\n"
    "type = \"pass_through\"\n"
    "stream = \"electricity\"\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.scenario0 = \"load0\"\n"
    "[networks.nw0]\n"
    "connections = [[\"S\", \"P\"], [\"P\", \"L\"]]\n"
    "[scenarios.scenario0]\n"
    "time_unit = \"seconds\"\n"
    "duration = 10\n"
    "occurrence_distribution = {type = \"fixed\", value = 0}\n"
    "max_occurrences = 1\n"
    "network = \"nw0\"\n";
  namespace E = ::ERIN;
  auto pt = E::ComponentType::PassThrough;
  EXPECT_EQ("pass_through", E::component_type_to_tag(pt));
  EXPECT_EQ(E::tag_to_component_type("pass_through"), pt);
  auto ptc = E::PassThroughComponent{"my_comp", E::StreamType("electrical")};
  auto ptc2 = E::PassThroughComponent{"my_comp", E::StreamType("electrical")};
  std::ostringstream oss;
  oss << ptc;
  std::string stream_str{
    "StreamType(type=\"electrical\", "
    "rate_units=\"kW\", "
    "quantity_units=\"kJ\", "
    "seconds_per_time_unit=1, "
    "other_rate_units={}, "
    "other_quantity_units={})"};
  EXPECT_EQ(
      std::string{"PassThroughComponent("} +
      std::string{"id=my_comp, "} +
      std::string{"component_type=pass_through, "} +
      std::string{"input_stream="} + stream_str + std::string{", "} +
      std::string{"output_stream="} + stream_str + std::string{", "} +
      std::string{"fragilities=..., has_fragilities=false)"},
      oss.str());
  EXPECT_EQ(ptc, ptc2);
  auto m = E::make_main_from_string(input);
  auto results = m.run("scenario0");
  EXPECT_TRUE(results.get_is_good());
  auto results_map = results.get_results();
  EXPECT_EQ(3, results_map.size());
  auto comp_ids = results.get_component_ids();
  std::vector<std::string> expected_comp_ids{"L","P","S"};
  ASSERT_EQ(expected_comp_ids.size(), comp_ids.size());
  EXPECT_EQ(expected_comp_ids, comp_ids);
  std::string elec{"electricity"};
  std::unordered_map<std::string,std::string> expected_streams{
    {"L", elec},
    {"P", elec},
    {"S", elec}};
  auto streams = results.get_stream_ids();
  EXPECT_EQ(expected_streams, streams);
  auto comps = results.get_component_types();
  std::unordered_map<std::string, E::ComponentType> expected_comps{
    {"L", E::ComponentType::Load},
    {"P", E::ComponentType::PassThrough},
    {"S", E::ComponentType::Source}};
  EXPECT_EQ(3, comps.size());
  EXPECT_EQ(expected_comps, comps);
  auto stats = results.get_statistics();
  std::unordered_map<std::string, E::ScenarioStats> expected_stats{
    {"L", E::ScenarioStats{10,0,0,0.0,100.0}},
    {"P", E::ScenarioStats{10,0,0,0.0,100.0}},
    {"S", E::ScenarioStats{10,0,0,0.0,100.0}}};
  EXPECT_EQ(stats.size(), expected_stats.size());
  for (const auto& s_item: expected_stats) {
    const auto& id = s_item.first;
    const auto& expected_stat = s_item.second;
    auto it = stats.find(id);
    ASSERT_TRUE(it != stats.end());
    EXPECT_EQ(expected_stat, it->second) << "id = " << id;
  }
}

TEST(ErinComponents, Test_passthrough_component_with_fragility)
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[loads.load0]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0]]\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "[components.P]\n"
    "type = \"pass_through\"\n"
    "stream = \"electricity\"\n"
    "fragilities = [\"frag01\"]\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.scenario0 = \"load0\"\n"
    "[fragility.frag01]\n"
    "vulnerable_to = \"intensity01\"\n"
    "type = \"linear\"\n"
    "lower_bound = 10.0\n"
    "upper_bound = 20.0\n"
    "[networks.nw0]\n"
    "connections = [[\"S\", \"P\"], [\"P\", \"L\"]]\n"
    "[scenarios.scenario0]\n"
    "time_unit = \"seconds\"\n"
    "duration = 10\n"
    "occurrence_distribution = {type = \"fixed\", value = 0}\n"
    "max_occurrences = 1\n"
    "intensity.intensity01 = 30.0\n"
    "network = \"nw0\"\n";
  namespace E = ::ERIN;
  auto m = E::make_main_from_string(input);
  auto results = m.run("scenario0");
  auto stats = results.get_statistics();
  std::unordered_map<std::string, E::ScenarioStats> expected_stats{
    {"L", E::ScenarioStats{0,10,10,100.0,0.0}},
    {"P", E::ScenarioStats{0,10,10,100.0,0.0}},
    {"S", E::ScenarioStats{10,0,0,0.0,0.0}}};
  EXPECT_EQ(stats.size(), expected_stats.size());
  for (const auto& s_item: expected_stats) {
    const auto& id = s_item.first;
    const auto& expected_stat = s_item.second;
    auto it = stats.find(id);
    ASSERT_TRUE(it != stats.end());
    EXPECT_EQ(expected_stat, it->second) << "id = " << id;
  }
}

TEST(ErinComponents, Test_passthrough_component_with_limits)
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[loads.load0]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0]]\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "[components.P]\n"
    "type = \"pass_through\"\n"
    "stream = \"electricity\"\n"
    "max_outflow = 5.0\n"
    "min_outflow = 0.0\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.scenario0 = \"load0\"\n"
    "[networks.nw0]\n"
    "connections = [[\"S\", \"P\"], [\"P\", \"L\"]]\n"
    "[scenarios.scenario0]\n"
    "time_unit = \"seconds\"\n"
    "duration = 10\n"
    "occurrence_distribution = {type = \"fixed\", value = 0}\n"
    "max_occurrences = 1\n"
    "network = \"nw0\"\n";
  namespace E = ::ERIN;
  auto m = E::make_main_from_string(input);
  auto results = m.run("scenario0");
  ASSERT_TRUE(results.get_is_good());
  auto stats = results.get_statistics();
  std::unordered_map<std::string, E::ScenarioStats> expected_stats{
    // load is aware of unmet requests
    {"L", E::ScenarioStats{0,10,10,50.0,50.0}},
    // pass-through is aware of unmet requests
    {"P", E::ScenarioStats{0,10,10,50.0,50.0}},
    // from source's point of view, it meets all requests
    {"S", E::ScenarioStats{10,0,0,0.0,50.0}}};
  EXPECT_EQ(stats.size(), expected_stats.size());
  for (const auto& s_item: expected_stats) {
    const auto& id = s_item.first;
    const auto& expected_stat = s_item.second;
    auto it = stats.find(id);
    ASSERT_TRUE(it != stats.end());
    EXPECT_EQ(expected_stat, it->second) << "id = " << id;
  }
}

TEST(ErinComponents, Test_that_clone_works_for_passthrough_component)
{
  auto c = ERIN::PassThroughComponent(
      "P", ERIN::StreamType{"electricity"}, ERIN::Limits{0.0,100.0}, {});
  auto p = c.clone();
  EXPECT_EQ(c, dynamic_cast<ERIN::PassThroughComponent&>(*p));
}

TEST(ErinComponents, Test_converter_component_with_fragilities)
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[streams.electricity]\n"
    "type = \"electricity\"\n"
    "[streams.natural_gas]\n"
    "type = \"natural_gas\"\n"
    "[streams.waste_heat]\n"
    "type = \"waste_heat\"\n"
    "[loads.load0]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0]]\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "outflow = \"natural_gas\"\n"
    "[components.C]\n"
    "type = \"converter\"\n"
    "inflow = \"natural_gas\"\n"
    "outflow = \"electricity\"\n"
    "lossflow = \"waste_heat\"\n"
    "constant_efficiency = 0.5\n"
    "fragilities = [\"frag01\"]\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.scenario0 = \"load0\"\n"
    "[fragility.frag01]\n"
    "vulnerable_to = \"intensity01\"\n"
    "type = \"linear\"\n"
    "lower_bound = 10.0\n"
    "upper_bound = 20.0\n"
    "[networks.nw0]\n"
    "connections = [[\"S\", \"C\"], [\"C\", \"L\"]]\n"
    "[scenarios.scenario0]\n"
    "time_unit = \"seconds\"\n"
    "duration = 10\n"
    "occurrence_distribution = {type = \"fixed\", value = 0}\n"
    "max_occurrences = 1\n"
    "intensity.intensity01 = 30.0\n"
    "network = \"nw0\"\n";
  namespace E = ::ERIN;
  auto m = E::make_main_from_string(input);
  const auto& comps = m.get_components();
  const auto& c = comps.at("C");
  ASSERT_TRUE(c->is_fragile());
  const auto& c1 = c->clone();
  ASSERT_TRUE(c1->is_fragile());
  ASSERT_EQ(
      dynamic_cast<E::ConverterComponent&>(*c),
      dynamic_cast<E::ConverterComponent&>(*c1));
  auto results = m.run("scenario0");
  ASSERT_TRUE(results.get_is_good());
  auto stats = results.get_statistics();
  std::unordered_map<std::string, E::ScenarioStats> expected_stats{
    {"L", E::ScenarioStats{0,10,10,100.0,0.0}},
    {"C:inflow", E::ScenarioStats{10,0,0,0.0,0.0}},
    {"C:outflow", E::ScenarioStats{0,10,10,100.0,0.0}},
    {"C:lossflow", E::ScenarioStats{10,0,0,0.0,0.0}},
    {"S", E::ScenarioStats{10,0,0,0.0,0.0}}};
  EXPECT_EQ(stats.size(), expected_stats.size());
  for (const auto& s_item: expected_stats) {
    const auto& id = s_item.first;
    const auto& expected_stat = s_item.second;
    auto it = stats.find(id);
    ASSERT_TRUE(it != stats.end()) << "expected id = " << id;
    EXPECT_EQ(expected_stat, it->second) << "id = " << id;
  }
}

TEST(ErinElements, Test_that_converter_yields_lossflow)
{
  std::unique_ptr<ERIN::FlowElement> c = std::make_unique<ERIN::Converter>(
      "conv",
      ERIN::ComponentType::Converter,
      ERIN::StreamType{"coal"},
      ERIN::StreamType{"electricity"},
      [](ERIN::FlowValueType input) -> ERIN::FlowValueType { return input * 0.5; },
      [](ERIN::FlowValueType output) -> ERIN::FlowValueType { return output * 2.0; });
  // 1. lossflow request of 100,000 comes in at (0,0)
  // 2. inflow request of 10 comes in at (0,1)
  // 3. call delta_int()
  // 4. call output_func() and check values of ys ; do we report the right lossflow?
  ERIN::Time t0{0,0};
  ERIN::Time dt{0,1};
  auto lossflow_request = ERIN::PortValue{ERIN::FlowElement::inport_lossflow_request, 100000.0}; 
  auto outflow_request = ERIN::PortValue{ERIN::FlowElement::inport_outflow_request, 10.0};
  std::vector<ERIN::PortValue> v1 = {lossflow_request};
  std::vector<ERIN::PortValue> v2 = {outflow_request};
  auto dt_next = c->ta();
  EXPECT_EQ(dt_next, ERIN::inf);
  c->delta_ext(t0+dt, v1);
  dt_next = c->ta();
  EXPECT_EQ(dt_next, dt);
  std::vector<ERIN::PortValue> outputs1;
  c->output_func(outputs1);
  c->delta_int();
  dt_next = c->ta();
  // lossflow_achieved
  ASSERT_EQ(1, outputs1.size());
  EXPECT_EQ(ERIN::FlowElement::outport_lossflow_achieved, outputs1[0].port);
  EXPECT_EQ(0.0, outputs1[0].value);
  EXPECT_EQ(dt_next, ERIN::inf);
  c->delta_ext(dt, v2);
  dt_next = c->ta();
  EXPECT_EQ(dt_next, dt);
  std::vector<ERIN::PortValue> outputs2;
  c->output_func(outputs2);
  // inflow_request + lossflow_achieved needs to be reported again
  bool found_inflow_request{false};
  bool found_lossflow_achieved{false};
  for (const auto& p: outputs2) {
    if (p.port == ERIN::FlowElement::outport_inflow_request) {
      if (found_inflow_request) {
        ASSERT_TRUE(false) << "multiple inflow requests found!";
      }
      found_inflow_request = true;
      EXPECT_EQ(20.0, p.value);
    } else if (p.port == ERIN::FlowElement::outport_lossflow_achieved) {
      if (found_lossflow_achieved) {
        ASSERT_TRUE(false) << "multiple lossflow achieved values found!";
      }
      found_lossflow_achieved = true;
      EXPECT_EQ(10.0, p.value);
    } else {
      ASSERT_TRUE(false)
        << "unexpected port found in outputs2: "
        << p.port << " with value: " << p.value;
    }
  }
  ASSERT_TRUE(found_inflow_request) << "no inflow request found";
  ASSERT_TRUE(found_lossflow_achieved) << "no lossflow achieved found";
  ASSERT_EQ(2, outputs2.size());
}

TEST(ErinGraphviz, Test_that_we_can_generate_graphviz)
{
  namespace en = erin::network;
  namespace ep = erin::port;
  std::vector<en::Connection> nw = {
    en::Connection{
      en::ComponentAndPort{"electric_utility", ep::Type::Outflow, 0},
      en::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0}}};
  namespace eg = erin::graphviz;
  std::string expected =
    "digraph ex01_normal_operations {\n"
    "  node [shape=record];\n"
    "  cluster_01_electric [shape=record,label=\"<I0> I(0)|<name> cluster_01_electric\"];\n"
    "  electric_utility [shape=record,label=\"<name> electric_utility|<O0> O(0)\"];\n"
    "  electric_utility:O0 -> cluster_01_electric:I0;\n"
    "}";
  auto actual = eg::network_to_dot(nw, "ex01_normal_operations", false);
  EXPECT_EQ(expected, actual);
}

TEST(ErinBasicsTest, Test_that_we_can_access_version_info_programmatically)
{
  namespace ev = erin::version;
  EXPECT_TRUE(ev::major_version >= 0);
  EXPECT_TRUE(ev::minor_version >= 0);
  EXPECT_TRUE(ev::release_number >= 0);
  std::ostringstream oss;
  oss << ev::major_version << "." << ev::minor_version << "." << ev::release_number;
  EXPECT_EQ(ev::version_string, oss.str());
}

TEST(ErinBasicsTest, Test_that_path_to_filename_works)
{
  namespace eu = erin::utils;
  std::string path0{"e2rin"};
  std::string expected_filename0{"e2rin"};
  EXPECT_EQ(expected_filename0, eu::path_to_filename(path0));
  std::string path1{"./bin/e2rin"};
  std::string expected_filename1{"e2rin"};
  EXPECT_EQ(expected_filename1, eu::path_to_filename(path1));
  std::string path2{".\\bin\\Debug\\e2rin.exe"};
  std::string expected_filename2{"e2rin.exe"};
}

TEST(ErinElements, Test_flow_writer_implementation)
{
  auto fw = ERIN::DefaultFlowWriter();
  auto id = fw.register_id("element", "electricity", true);
  ERIN::RealTimeType t_max{10};
  fw.write_data(id, 0, 0.0, 0.0);
  fw.write_data(id, 0, 10.0, 10.0);
  fw.write_data(id, 0, 10.0, 8.0);
  fw.write_data(id, 5, 5.0, 5.0);
  fw.finalize_at_time(t_max);
  ASSERT_THROW(fw.write_data(id, t_max+1, 10.0, 10.0), std::runtime_error);
  auto actual = fw.get_results();
  std::unordered_map<std::string, std::vector<ERIN::Datum>> expected{
    {
      "element",
      std::vector<ERIN::Datum>{
        ERIN::Datum{0,10.0,8.0},
        ERIN::Datum{5,5.0,5.0},
        ERIN::Datum{10,0.0,0.0}}}};
  EXPECT_EQ(actual, expected);
  fw.clear();
  auto id1 = fw.register_id("electric_load_1:inflow", "electricity", true);
  auto id2 = fw.register_id("diesel_genset_1:outflow", "electricity", true);
  auto id3 = fw.register_id("electric_load_2:inflow", "electricity", true);
  auto id4 = fw.register_id("diesel_genset_2:outflow", "electricity", true);
  auto id5 = fw.register_id("diesel_fuel_tank:outflow", "diesel_fuel", true);
  // start
  fw.write_data(id1, 0, 10.0, 10.0);
  fw.write_data(id2, 0, 10.0, 10.0);
  fw.write_data(id3, 0, 5.0, 5.0);
  fw.write_data(id4, 0, 5.0, 5.0);
  fw.write_data(id5, 0, 30.0, 30.0);
  // 5 seconds
  fw.write_data(id3, 5, 10.0, 10.0);
  fw.write_data(id4, 5, 10.0, 10.0);
  fw.write_data(id5, 5, 40.0, 35.0);
  fw.write_data(id4, 5, 10.0, 7.5);
  fw.write_data(id3, 5, 10.0, 7.5);
  // 10 seconds
  fw.write_data(id3, 10, 5.0, 5.0);
  fw.write_data(id4, 10, 5.0, 5.0);
  fw.write_data(id5, 10, 30.0, 30.0);
  fw.finalize_at_time(10);
  auto actual1 = fw.get_results();
  std::unordered_map<std::string, std::vector<ERIN::Datum>> expected1{
    {
      "electric_load_1:inflow", // id1
      std::vector<ERIN::Datum>{
        ERIN::Datum{0,10.0,10.0},
        ERIN::Datum{5,10.0,10.0},
        ERIN::Datum{10,0.0,0.0}}},
    {
      "diesel_genset_1:outflow", // id2
      std::vector<ERIN::Datum>{
        ERIN::Datum{0,10.0,10.0},
        ERIN::Datum{5,10.0,10.0},
        ERIN::Datum{10,0.0,0.0}}},
    {
      "electric_load_2:inflow", // id3
      std::vector<ERIN::Datum>{
        ERIN::Datum{0,5.0,5.0},
        ERIN::Datum{5,10.0,7.5},
        ERIN::Datum{10,0.0,0.0}}},
    {
      "diesel_genset_2:outflow", // id4
      std::vector<ERIN::Datum>{
        ERIN::Datum{0,5.0,5.0},
        ERIN::Datum{5,10.0,7.5},
        ERIN::Datum{10,0.0,0.0}}},
    {
      "diesel_fuel_tank:outflow", // id5
      std::vector<ERIN::Datum>{
        ERIN::Datum{0,30.0,30.0},
        ERIN::Datum{5,40.0,35.0},
        ERIN::Datum{10,0.0,0.0}}}};
  ASSERT_EQ(5, actual1.size());
  for (const auto& item: actual1) {
    const auto& tag = item.first;
    const auto& actual_val = item.second;
    auto it = expected1.find(tag);
    ASSERT_TRUE(it != expected1.end()) << tag << " not found in expected1!";
    const auto& expected_val = it->second;
    EXPECT_EQ(actual_val, expected_val) << "values not equal for " << tag;
  }
}

TEST(ErinElements, Test_flow_writer_clone)
{
  std::unique_ptr<ERIN::FlowWriter> fw1 = std::make_unique<ERIN::DefaultFlowWriter>();
  auto id = fw1->register_id("element", "stream", true);
  fw1->write_data(id, 0, 10.0, 10.0);
  auto fw2 = fw1->clone();
  fw1->write_data(id, 4, 20.0, 10.0);
  fw1->finalize_at_time(10);
  auto results1 = fw1->get_results();
  fw1->clear();
  fw2->write_data(id, 5, 20.0, 10.0);
  fw2->finalize_at_time(12);
  auto results2 = fw2->get_results();
  fw2->clear();
  // fw1
  std::vector<ERIN::RealTimeType> expected_times1 = {0,4,10};
  auto actual_times1 = ERIN::get_times_from_results_for_component(results1, "element");
  EXPECT_EQ(expected_times1, actual_times1);
  std::vector<ERIN::FlowValueType> expected_achieved_flows1 = {10.0,10.0,0.0};
  auto actual_achieved_flows1 = ERIN::get_actual_flows_from_results_for_component(results1, "element");
  EXPECT_EQ(expected_achieved_flows1, actual_achieved_flows1);
  std::vector<ERIN::FlowValueType> expected_requested_flows1 = {10.0,20.0,0.0};
  auto actual_requested_flows1 = ERIN::get_requested_flows_from_results_for_component(results1, "element");
  EXPECT_EQ(expected_requested_flows1, actual_requested_flows1);
  // fw2
  std::vector<ERIN::RealTimeType> expected_times2 = {0,5,12};
  auto actual_times2 = ERIN::get_times_from_results_for_component(results2, "element");
  EXPECT_EQ(expected_times2, actual_times2);
  std::vector<ERIN::FlowValueType> expected_achieved_flows2 = {10.0,10.0,0.0};
  auto actual_achieved_flows2 = ERIN::get_actual_flows_from_results_for_component(results2, "element");
  EXPECT_EQ(expected_achieved_flows2, actual_achieved_flows2);
  std::vector<ERIN::FlowValueType> expected_requested_flows2 = {10.0,20.0,0.0};
  auto actual_requested_flows2 = ERIN::get_requested_flows_from_results_for_component(results2, "element");
  EXPECT_EQ(expected_requested_flows2, actual_requested_flows2);
}

bool compare_ports(const erin::devs::PortValue& a, const erin::devs::PortValue& b)
{
  return (a.port == b.port) && (a.value == b.value);
};

TEST(ErinDevs, Test_flow_limits_functions)
{
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  using size_type = std::vector<ED::PortValue>::size_type;
  ED::FlowLimits limits{0.0, 100.0};
  ED::FlowLimitsState s0{
    // time, inflow_port, outflow_port
    0, ED::Port{}, ED::Port{},
    // FlowLimits, report_inflow_request, report_outflow_achieved
    limits, false, false};
  auto dt0 = ED::flow_limits_time_advance(s0);
  EXPECT_EQ(dt0, ED::infinity); // ED::infinity is the representation we use for infinity
  auto s1 = ED::flow_limits_external_transition_on_outflow_request(s0, 2, 10.0);
  ED::FlowLimitsState expected_s1{
    2,
    ED::Port{2, 10.0, 10.0, true, false},
    ED::Port{2, 10.0, 10.0, true, false},
    limits, true, false};
  EXPECT_EQ(s1, expected_s1);
  auto dt1 = ED::flow_limits_time_advance(s1);
  EXPECT_EQ(dt1, 0);
  auto ys2 = ED::flow_limits_output_function(s1);
  std::vector<ED::PortValue> expected_ys2 = {
    ED::PortValue{ED::outport_inflow_request, 10.0}};
  ASSERT_EQ(ys2.size(), expected_ys2.size());
  for (size_type i{0}; i < expected_ys2.size(); ++i) {
    const auto& y2 = ys2.at(i);
    const auto& ey2 = expected_ys2.at(i);
    EXPECT_EQ(y2.port, ey2.port) << "i=" << i;
    EXPECT_EQ(y2.value, ey2.value) << "i=" << i;
  }
  auto s2 = ED::flow_limits_internal_transition(s1);
  ED::FlowLimitsState expected_s2{
    2,
    ED::Port{2, 10.0, 10.0},
    ED::Port{2, 10.0, 10.0},
    limits, false, false};
  EXPECT_EQ(s2, expected_s2);
  auto s3 = ED::flow_limits_external_transition_on_inflow_achieved(s2, 0, 8.0);
  ED::FlowLimitsState expected_s3{
    2,
    ED::Port{2,10.0,8.0},
    ED::Port{2,10.0,8.0},
    limits, false, true};
  EXPECT_EQ(s3, expected_s3);
  auto dt3 = ED::flow_limits_time_advance(s3);
  EXPECT_EQ(dt3, 0);
  auto ys3 = ED::flow_limits_output_function(s3);
  std::vector<ED::PortValue> expected_ys3 = {
    ED::PortValue{ED::outport_outflow_achieved, 8.0}};
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys3, expected_ys3, compare_ports));
  auto s4 = ED::flow_limits_internal_transition(s3);
  ED::FlowLimitsState expected_s4{
    2,
    ED::Port{2,10.0,8.0},
    ED::Port{2,10.0,8.0},
    limits, false, false};
  EXPECT_EQ(s4, expected_s4);
  auto dt4 = ED::flow_limits_time_advance(s4);
  EXPECT_EQ(dt4, ED::infinity);
  std::vector<ED::PortValue> xs = {
    ED::PortValue{ED::inport_outflow_request, 200.0}};
  auto s5 = ED::flow_limits_external_transition(s4, 2, xs);
  ED::FlowLimitsState expected_s5{
    4,
    ED::Port{4, 100.0, 100.0},
    ED::Port{4, 200.0, 100.0},
    limits, true, true};
  EXPECT_EQ(s5, expected_s5);
  auto ys5 = ED::flow_limits_output_function(s5);
  std::vector<ED::PortValue> expected_ys5 = {
    ED::PortValue{ED::outport_outflow_achieved, 100.0},
    ED::PortValue{ED::outport_inflow_request, 100.0}};
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys5, expected_ys5, compare_ports));
  auto s6 = ED::flow_limits_internal_transition(s5);
  auto dt6 = ED::flow_limits_time_advance(s6);
  ASSERT_EQ(dt6, ED::infinity);
  std::vector<ED::PortValue> xs6 = {
    ED::PortValue{ED::inport_inflow_achieved, 55.0}};
  auto s7 = ED::flow_limits_external_transition(s6, 0, xs6);
  ED::FlowLimitsState expected_s7{
    4,
    ED::Port{4, 100.0, 55.0},
    ED::Port{4, 200.0, 55.0},
    limits, false, true};
  EXPECT_EQ(s7, expected_s7);
  auto ys7 = ED::flow_limits_output_function(s7);
  std::vector<ED::PortValue> expected_ys7{
    ED::PortValue{ED::outport_outflow_achieved, 55.0}};
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys7, expected_ys7, compare_ports));
}

TEST(ErinBasicsTest, Test_that_compare_vectors_unordered_works)
{
  namespace eu = erin::utils;
  std::vector<int> xs{1,2,3,4};
  std::vector<int> ys{4,3,2,1};
  EXPECT_TRUE(eu::compare_vectors_unordered<int>(xs, ys));
  xs = std::vector<int>{1,2,3};
  ys = std::vector<int>{4,3,2};
  EXPECT_FALSE(eu::compare_vectors_unordered<int>(xs, ys));
  xs = std::vector<int>{1,2,3,4};
  ys = std::vector<int>{4,3,2};
  EXPECT_FALSE(eu::compare_vectors_unordered<int>(xs, ys));
}

TEST(ErinDevs, Test_converter_functions)
{
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  using size_type = std::vector<ED::PortValue>::size_type;
  ED::FlowValueType constant_efficiency{0.25};
  auto s0 = ED::make_converter_state(constant_efficiency);
  std::unique_ptr<ED::ConversionFun> cf =
    std::make_unique<ED::ConstantEfficiencyFun>(constant_efficiency);
  ED::ConverterState expected_s0{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    0, ED::Port{0, 0.0}, ED::Port{0, 0.0}, ED::Port{0, 0.0}, ED::Port{0, 0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    std::move(cf->clone()), false, false, false};
  EXPECT_EQ(s0, expected_s0);
  auto dt0 = ED::converter_time_advance(s0);
  EXPECT_EQ(dt0, ED::infinity);
  std::vector<ED::PortValue> xs0{ED::PortValue{ED::inport_outflow_request, 10.0}};
  auto s1 = ED::converter_external_transition(s0, 2, xs0);
  ED::ConverterState expected_s1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    2, ED::Port{2, 40.0}, ED::Port{2, 10.0}, ED::Port{0, 0.0}, ED::Port{2, 30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    std::move(cf->clone()), true, false, false};
  EXPECT_EQ(expected_s1, s1);
  auto dt1 = ED::converter_time_advance(s1);
  EXPECT_EQ(dt1, 0);
  auto ys1 = ED::converter_output_function(s1);
  std::vector<ED::PortValue> expected_ys1{
    ED::PortValue{ED::outport_inflow_request, 40.0}};
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys1, expected_ys1, compare_ports));
  auto s2 = ED::converter_internal_transition(s1);
  ED::ConverterState expected_s2{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    2, ED::Port{2, 40.0}, ED::Port{2, 10.0}, ED::Port{0, 0.0}, ED::Port{2, 30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    std::move(cf->clone()), false, false, false};
  EXPECT_EQ(expected_s2, s2);
  auto dt2 = ED::converter_time_advance(s2);
  EXPECT_EQ(dt2, ED::infinity);
  std::vector<ED::PortValue> xs2{
    ED::PortValue{ED::inport_inflow_achieved, 20.0}};
  auto s3 = ED::converter_external_transition(s2, 1, xs2);
  ED::ConverterState expected_s3{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    3, ED::Port{3, 40.0, 20.0}, ED::Port{3, 10.0, 5.0}, ED::Port{0, 0.0}, ED::Port{3, 15.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    std::move(cf->clone()), false, true, false};
  EXPECT_EQ(expected_s3, s3);
  auto dt3 = ED::converter_time_advance(s3);
  EXPECT_EQ(dt3, 0);
  auto ys3 = ED::converter_output_function(s3);
  std::vector<ED::PortValue> expected_ys3{
    ED::PortValue{ED::outport_outflow_achieved, 5.0}};
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys3, expected_ys3, compare_ports));
  auto s4 = ED::converter_internal_transition(s3);
  ED::ConverterState expected_s4{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    3, ED::Port{3, 40.0, 20.0}, ED::Port{3, 10.0, 5.0}, ED::Port{0, 0.0}, ED::Port{3, 15.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    std::move(cf->clone()), false, false, false};
  EXPECT_EQ(s4, expected_s4);
  auto dt4 = ED::converter_time_advance(s4);
  EXPECT_EQ(dt4, ED::infinity);
  // Test Confluent Transitions
  std::vector<ED::PortValue> xs1a{
    ED::PortValue{ED::inport_lossflow_request, 2.0}};
  auto s2a = ED::converter_confluent_transition(s1, xs1a);
  ED::ConverterState expected_s2a{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    2, ED::Port{2, 40.0}, ED::Port{2, 10.0}, ED::Port{2, 2.0}, ED::Port{2, 28.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};

  // Test Multiple Events for a Single External Transition
  // Starting from a "Zero" State
  std::vector<ED::PortValue> xs_a{
    ED::PortValue{ED::inport_outflow_request, 10.0}};
  auto s_a = ED::converter_external_transition(s0, 10, xs_a);
  ED::ConverterState expected_s_a{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    10, ED::Port{10, 40.0}, ED::Port{10, 10.0}, ED::Port{0, 0.0}, ED::Port{10, 30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, false, false};
  EXPECT_EQ(s_a, expected_s_a);

  std::vector<ED::PortValue> xs_b{
    ED::PortValue{ED::inport_lossflow_request, 30.0}};
  auto s_b = ED::converter_external_transition(s0, 10, xs_b);
  ED::ConverterState expected_s_b{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    10, ED::Port{0, 0.0}, ED::Port{0, 0.0}, ED::Port{10, 30.0, 0.0}, ED::Port{0, 0.0, 0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, true};
  EXPECT_EQ(s_b, expected_s_b);

  std::vector<ED::PortValue> xs_c{
    ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  ASSERT_THROW(ED::converter_external_transition(s0, 10, xs_c), std::invalid_argument);

  std::vector<ED::PortValue> xs_d{
    ED::PortValue{ED::inport_outflow_request, 10.0},
    ED::PortValue{ED::inport_lossflow_request, 30.0}};
  auto s_d = ED::converter_external_transition(s0, 10, xs_d);
  ED::ConverterState expected_s_d{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    10, ED::Port{10, 40.0}, ED::Port{10, 10.0}, ED::Port{10, 30.0}, ED::Port{0, 0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, false, false};
  EXPECT_EQ(s_d, expected_s_d);

  // ... the below 3 all throw because we're getting an inflow achieved without a request for it
  std::vector<ED::PortValue> xs_e{
    ED::PortValue{ED::inport_outflow_request, 10.0},
    ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  ASSERT_THROW(ED::converter_external_transition(s0, 10, xs_e), std::invalid_argument);

  std::vector<ED::PortValue> xs_f{
    ED::PortValue{ED::inport_lossflow_request, 30.0},
    ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  ASSERT_THROW(ED::converter_external_transition(s0, 10, xs_f), std::invalid_argument);

  std::vector<ED::PortValue> xs_g{
    ED::PortValue{ED::inport_outflow_request, 10.0},
    ED::PortValue{ED::inport_lossflow_request, 30.0},
    ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  ASSERT_THROW(ED::converter_external_transition(s0, 10, xs_g), std::invalid_argument);
  
  // Test Multiple Events for a Single External Transition
  ED::ConverterState s_m{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    2, ED::Port{2, 80.0}, ED::Port{2, 20.0}, ED::Port{0, 0.0}, ED::Port{2, 60.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    std::move(cf->clone()), false, false, false};
  //std::vector<ED::PortValue> xs_a{
  //  ED::PortValue{ED::inport_outflow_request, 10.0}};
  auto s_a1 = ED::converter_external_transition(s_m, 10, xs_a);
  ED::ConverterState expected_s_a1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port{12, 40.0}, ED::Port{12, 10.0}, ED::Port{0, 0.0}, ED::Port{12, 30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, false, false};
  EXPECT_EQ(s_a1, expected_s_a1);

  //std::vector<ED::PortValue> xs_b{
  //  ED::PortValue{ED::inport_lossflow_request, 30.0}};
  auto s_b1 = ED::converter_external_transition(s_m, 10, xs_b);
  ED::ConverterState expected_s_b1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port{2, 80.0}, ED::Port{2, 20.0}, ED::Port{12, 30.0}, ED::Port{12, 30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  EXPECT_EQ(s_b1, expected_s_b1);

  //std::vector<ED::PortValue> xs_c{
  //  ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  auto s_c1 = ED::converter_external_transition(s_m, 10, xs_c);
  ED::ConverterState expected_s_c1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port{12, 80.0, 40.0}, ED::Port{12, 20.0, 10.0}, ED::Port{0, 0.0}, ED::Port{12, 30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, true, false};
  EXPECT_EQ(s_c1, expected_s_c1);

  //std::vector<ED::PortValue> xs_d{
  //  ED::PortValue{ED::inport_outflow_request, 10.0},
  //  ED::PortValue{ED::inport_lossflow_request, 30.0}};
  auto s_d1 = ED::converter_external_transition(s_m, 10, xs_d);
  ED::ConverterState expected_s_d1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port{12, 40.0}, ED::Port{12, 10.0}, ED::Port{12, 30.0}, ED::Port{12, 0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, false, false};
  EXPECT_EQ(s_d1, expected_s_d1);

  //std::vector<ED::PortValue> xs_e{
  //  ED::PortValue{ED::inport_outflow_request, 10.0},
  //  ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  auto s_e1 = ED::converter_external_transition(s_m, 10, xs_e);
  ED::ConverterState expected_s_e1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port{12, 40.0}, ED::Port{12, 10.0}, ED::Port{0, 0.0}, ED::Port{12, 30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  EXPECT_EQ(s_e1, expected_s_e1);

  //std::vector<ED::PortValue> xs_f{
  //  ED::PortValue{ED::inport_lossflow_request, 30.0},
  //  ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  auto s_f1 = ED::converter_external_transition(s_m, 10, xs_f);
  ED::ConverterState expected_s_f1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port{12, 80.0, 40.0}, ED::Port{12, 20.0, 10.0}, ED::Port{12, 30.0}, ED::Port{12, 0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, true, false};
  EXPECT_EQ(s_f1, expected_s_f1);

  //std::vector<ED::PortValue> xs_g{
  //  ED::PortValue{ED::inport_outflow_request, 10.0},
  //  ED::PortValue{ED::inport_lossflow_request, 30.0},
  //  ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  auto s_g1 = ED::converter_external_transition(s_m, 10, xs_g);
  ED::ConverterState expected_s_g1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port{12, 40.0}, ED::Port{12, 10.0}, ED::Port{12, 30.0}, ED::Port{12, 0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  EXPECT_EQ(s_g1, expected_s_g1);
}

TEST(ErinDevs, Test_function_based_efficiency)
{
  namespace ED = erin::devs;
  namespace E = ERIN;
  auto f_in_to_out = [](E::FlowValueType inflow) -> E::FlowValueType {
    return inflow * 0.25;
  };
  auto f_out_to_in = [](E::FlowValueType outflow) -> E::FlowValueType {
    return outflow / 0.25;
  };
  std::unique_ptr<ED::ConversionFun> f =
    std::make_unique<ED::FunctionBasedEfficiencyFun>(f_in_to_out, f_out_to_in);
  EXPECT_EQ(40.0, f->inflow_given_outflow(10.0));
  EXPECT_EQ(10.0, f->outflow_given_inflow(40.0));
}

TEST(ErinDevs, Test_function_based_load)
{
  namespace E = ERIN;
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  auto s0 = ED::make_load_state(
      std::vector<ED::LoadItem>{
        ED::LoadItem{0, 100.0},
        ED::LoadItem{10, 10.0},
        ED::LoadItem{100, 10.0}, // should NOT cause a new event -- same load request.
        ED::LoadItem{200}});
  EXPECT_FALSE(s0.inflow_port.should_propagate_request_at(0));
  EXPECT_EQ(s0.current_index, -1);
  EXPECT_EQ(ED::load_current_time(s0), 0);
  EXPECT_EQ(ED::load_next_time(s0), 0);
  EXPECT_EQ(ED::load_current_request(s0), 0.0);
  EXPECT_EQ(ED::load_current_achieved(s0), 0.0);
  auto dt0 = ED::load_time_advance(s0);
  EXPECT_EQ(dt0, 0);
  auto ys0 = ED::load_output_function(s0);
  std::vector<ED::PortValue> expected_ys0{
    ED::PortValue{ED::outport_inflow_request, 100.0}};
  EXPECT_EQ(ys0.size(), expected_ys0.size());
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys0, expected_ys0, compare_ports));
  auto s1 = ED::load_internal_transition(s0);
  EXPECT_EQ(s1.current_index, 0);
  EXPECT_EQ(ED::load_current_time(s1), 0);
  EXPECT_EQ(ED::load_next_time(s1), 10);
  EXPECT_EQ(ED::load_current_request(s1), 100.0);
  EXPECT_EQ(ED::load_current_achieved(s1), 100.0);
  auto dt1 = ED::load_time_advance(s1);
  EXPECT_EQ(dt1, 10);
  auto ys1 = ED::load_output_function(s1);
  std::vector<ED::PortValue> expected_ys1{
    ED::PortValue{ED::outport_inflow_request, 10.0}};
  EXPECT_EQ(ys1.size(), expected_ys1.size());
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys1, expected_ys1, compare_ports));
  auto s2 = ED::load_internal_transition(s1);
  EXPECT_EQ(s2.current_index, 1);
  EXPECT_EQ(ED::load_current_time(s2), 10);
  EXPECT_EQ(ED::load_next_time(s2), 100);
  EXPECT_EQ(ED::load_current_request(s2), 10.0);
  EXPECT_EQ(ED::load_current_achieved(s2), 10.0);
  auto dt2 = ED::load_time_advance(s2);
  EXPECT_EQ(dt2, 90);
  std::vector<ED::PortValue> xs2{
    ED::PortValue{ED::inport_inflow_achieved, 5.0}};
  auto s3 = ED::load_external_transition(s2, 50, xs2);
  EXPECT_EQ(s3.current_index, 1);
  EXPECT_EQ(ED::load_current_time(s3), 60);
  EXPECT_EQ(ED::load_next_time(s3), 100);
  EXPECT_EQ(ED::load_current_request(s3), 10.0);
  EXPECT_EQ(ED::load_current_achieved(s3), 5.0);
  auto dt3 = ED::load_time_advance(s3);
  EXPECT_EQ(dt3, 40);
  std::vector<ED::PortValue> xs3{
    ED::PortValue{ED::inport_inflow_achieved, 10.0}};
  auto s4 = ED::load_external_transition(s3, 10, xs3);
  EXPECT_EQ(s4.current_index, 1);
  EXPECT_EQ(ED::load_current_time(s4), 70);
  EXPECT_EQ(ED::load_next_time(s4), 100);
  EXPECT_EQ(ED::load_current_request(s4), 10.0);
  EXPECT_EQ(ED::load_current_achieved(s4), 10.0);
  auto dt4 = ED::load_time_advance(s4);
  EXPECT_EQ(dt4, 30);
  auto ys4 = ED::load_output_function(s4);
  // we get no output because the requested load is the same as current -- no
  // change to propagate!
  std::vector<ED::PortValue> expected_ys4{};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys4, expected_ys4, compare_ports));
  auto s5 = ED::load_internal_transition(s4);
  EXPECT_EQ(s5.current_index, 2);
  EXPECT_EQ(ED::load_current_time(s5), 100);
  EXPECT_EQ(ED::load_next_time(s5), 200);
  EXPECT_EQ(ED::load_current_request(s5), 10.0);
  EXPECT_EQ(ED::load_current_achieved(s5), 10.0);
  auto dt5 = ED::load_time_advance(s5);
  EXPECT_EQ(dt5, 100);
  auto ys5 = ED::load_output_function(s5);
  std::vector<ED::PortValue> expected_ys5{
    ED::PortValue{ED::outport_inflow_request, 0.0}};
  EXPECT_EQ(ys5.size(), expected_ys5.size());
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys5, expected_ys5, compare_ports));
  auto s6 = ED::load_internal_transition(s5);
  EXPECT_EQ(s6.current_index, 3);
  EXPECT_EQ(ED::load_current_time(s6), 200);
  EXPECT_EQ(ED::load_next_time(s6), ED::infinity);
  EXPECT_EQ(ED::load_current_request(s6), 0.0);
  EXPECT_EQ(ED::load_current_achieved(s6), 0.0);
  auto dt6 = ED::load_time_advance(s6);
  EXPECT_EQ(dt6, ED::infinity);

  // test confluent update from state 
  std::vector<ED::PortValue> xs5{
    ED::PortValue{ED::inport_inflow_achieved, 8.0}};
  auto s6a = ED::load_confluent_transition(s5, xs5);
  EXPECT_EQ(s6a.current_index, 3);
  EXPECT_EQ(ED::load_current_time(s6a), 200);
  EXPECT_EQ(ED::load_next_time(s6a), ED::infinity);
  EXPECT_EQ(ED::load_current_request(s6a), 0.0);
  EXPECT_EQ(ED::load_current_achieved(s6a), 0.0);

  ASSERT_THROW(
      ED::check_loads(std::vector<ED::LoadItem>{}),
      std::invalid_argument);

  ASSERT_THROW(
      ED::check_loads(std::vector<ED::LoadItem>{
        ED::LoadItem{0,10.0},
        ED::LoadItem{10,0.0}}),
      std::invalid_argument);

  ASSERT_THROW(
      ED::check_loads(std::vector<ED::LoadItem>{
        ED::LoadItem{10,10.0},
        ED::LoadItem{5}}),
      std::invalid_argument);
}

TEST(ErinDevs, Test_function_based_mux)
{
  namespace E = ERIN;
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  int num_inports{3};
  int num_outports{3};
  auto s0 = ED::make_mux_state(num_inports, num_outports);
  auto dt0 = ED::mux_time_advance(s0);
  EXPECT_EQ(dt0, ED::infinity);
  EXPECT_EQ(ED::mux_current_time(s0), 0);
  std::vector<ED::PortValue> xs0{
    ED::PortValue{ED::inport_outflow_request + 0, 100.0}};
  auto s1 = ED::mux_external_transition(s0, 10, xs0);
  EXPECT_TRUE(
      ED::mux_should_report(
        s1.time,
        s1.inflow_ports,
        s1.outflow_ports));
  EXPECT_TRUE(s1.do_report);
  EXPECT_EQ(ED::mux_current_time(s1), 10);
  auto dt1 = ED::mux_time_advance(s1);
  EXPECT_EQ(dt1, 0);
  auto ys1 = ED::mux_output_function(s1);
  std::vector<ED::PortValue> expected_ys1{
    ED::PortValue{ED::outport_inflow_request + 0, 100.0}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys1, expected_ys1, compare_ports));
  auto s2 = ED::mux_internal_transition(s1);
  EXPECT_EQ(ED::mux_current_time(s2), 10);
  auto dt2 = ED::mux_time_advance(s2);
  EXPECT_EQ(dt2, ED::infinity);
  std::vector<ED::PortValue> xs2{
    ED::PortValue{ED::inport_inflow_achieved + 0, 50.0}};
  auto s3 = ED::mux_external_transition(s2, 2, xs2);
  EXPECT_EQ(ED::mux_current_time(s3), 12);
  auto dt3 = ED::mux_time_advance(s3);
  EXPECT_EQ(dt3, 0);
  auto ys3 = ED::mux_output_function(s3);
  std::vector<ED::PortValue> expected_ys3{
    ED::PortValue{ED::outport_inflow_request + 1, 50.0}};
  EXPECT_EQ(ys3.size(), expected_ys3.size());
  //std::cout << "ys3 = " << ERIN::vec_to_string<ED::PortValue>(ys3) << "\n";
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys3, expected_ys3, compare_ports));
  auto s4 = ED::mux_internal_transition(s3);
  auto dt4 = ED::mux_time_advance(s4);
  EXPECT_EQ(dt4, ED::infinity);
  std::vector<ED::PortValue> xs4{
    ED::PortValue{ED::inport_inflow_achieved + 1, 20.0}};
  auto s5 = ED::mux_external_transition(s4, 0, xs4);
  EXPECT_EQ(ED::mux_current_time(s5), 12);
  auto dt5 = ED::mux_time_advance(s5);
  EXPECT_EQ(dt5, 0);
  auto ys5 = ED::mux_output_function(s5);
  std::vector<ED::PortValue> expected_ys5{
    ED::PortValue{ED::outport_inflow_request + 2, 30.0}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys5, expected_ys5, compare_ports));
  std::vector<ED::PortValue> xs5{
    ED::PortValue{ED::inport_inflow_achieved + 2, 20.0}};
  auto s6 = ED::mux_confluent_transition(s5, xs5);
  std::cout << "s6.inflow_ports = " << ERIN::vec_to_string<ED::Port>(s6.inflow_ports) << "\n";
  std::cout << "s6.outflow_ports = " << ERIN::vec_to_string<ED::Port>(s6.outflow_ports) << "\n";
  auto dt6 = ED::mux_time_advance(s6);
  EXPECT_EQ(dt6, 0);
  auto ys6 = ED::mux_output_function(s6);
  std::vector<ED::PortValue> expected_ys6{
    ED::PortValue{ED::outport_outflow_achieved + 0, 90.0}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys6, expected_ys6, compare_ports));
  auto s7 = ED::mux_internal_transition(s6);
  std::cout << "s7.inflow_ports = " << ERIN::vec_to_string<ED::Port>(s7.inflow_ports) << "\n";
  std::cout << "s7.outflow_ports = " << ERIN::vec_to_string<ED::Port>(s7.outflow_ports) << "\n";
  auto dt7 = ED::mux_time_advance(s7);
  EXPECT_EQ(dt7, ED::infinity);
  std::vector<ED::PortValue> xs7{
    ED::PortValue{ED::inport_outflow_request + 1, 100.0}};
  auto s8 = ED::mux_external_transition(s7, 10, xs7);
  EXPECT_EQ(ED::mux_current_time(s8), 22);
  auto dt8 = ED::mux_time_advance(s8);
  EXPECT_EQ(dt8, 0);
  auto ys8 = ED::mux_output_function(s8);
  std::vector<ED::PortValue> expected_ys8{
    ED::PortValue{ED::outport_inflow_request + 0, 200.0},
    ED::PortValue{ED::outport_inflow_request + 1, 0.0},
    ED::PortValue{ED::outport_inflow_request + 2, 0.0},
    ED::PortValue{ED::outport_outflow_achieved + 0, 100.0}};
  std::cout << "expected_ys8 = " << ERIN::vec_to_string<ED::PortValue>(expected_ys8) << "\n";
  std::cout << "ys8          = " << ERIN::vec_to_string<ED::PortValue>(ys8) << "\n";
  std::cout << "s8.inflow_ports = " << ERIN::vec_to_string<ED::Port>(s8.inflow_ports) << "\n";
  std::cout << "s8.outflow_ports = " << ERIN::vec_to_string<ED::Port>(s8.outflow_ports) << "\n";
  EXPECT_EQ(ys8.size(), expected_ys8.size());
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys8, expected_ys8, compare_ports));
}

TEST(ErinDevs, Test_function_based_storage_element)
{
  namespace E = ERIN;
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  ED::FlowValueType capacity{100.0};
  double initial_soc{0.5};
  ASSERT_THROW(ED::storage_make_state(-1.0), std::invalid_argument);
  ASSERT_THROW(ED::storage_make_state(1.0, 2.0), std::invalid_argument);
  ASSERT_THROW(ED::storage_make_state(1.0, -0.5), std::invalid_argument);
  auto s0 = ED::storage_make_state(capacity, initial_soc);
  //auto dt0 = ED::storage_time_advance(s0);
  //EXPECT_EQ(dt0, ED::infinity);
}

int
main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
