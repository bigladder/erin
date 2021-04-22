/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

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
#include "erin/devs/on_off_switch.h"
#include "erin/devs/storage.h"
#include "erin/distribution.h"
#include "erin/erin.h"
#include "erin/fragility.h"
#include "erin/graphviz.h"
#include "erin/port.h"
#include "erin/random.h"
#include "erin/reliability.h"
#include "erin/stream.h"
#include "erin/type.h"
#include "erin/utils.h"
#include "erin/version.h"
#include "erin_test_utils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>

const double tolerance{1e-6};

bool compare_ports(const erin::devs::PortValue& a, const erin::devs::PortValue& b)
{
  return (a.port == b.port) && (a.value == b.value);
};

bool check_times_and_loads(
    const std::unordered_map<std::string, std::vector<ERIN::Datum>>& results,
    const std::vector<ERIN::RealTimeType>& expected_times,
    const std::vector<ERIN::FlowValueType>& expected_loads,
    const std::string& id,
    bool use_requested = false);

bool
check_times_and_loads(
    const std::unordered_map<std::string, std::vector<ERIN::Datum>>& results,
    const std::vector<ERIN::RealTimeType>& expected_times,
    const std::vector<ERIN::FlowValueType>& expected_loads,
    const std::string& id,
    bool use_requested)
{
  namespace E = ERIN;
  auto actual_times = E::get_times_from_results_for_component(results, id);
  bool flag =
    erin_test_utils::compare_vectors_functional<E::RealTimeType>(
        expected_times,
        actual_times);
  std::vector<E::FlowValueType> actual_loads{};
  if (use_requested) {
    actual_loads = E::get_requested_flows_from_results_for_component(results, id);
  }
  else {
    actual_loads = E::get_actual_flows_from_results_for_component(results, id);
  }
  flag = flag && erin_test_utils::compare_vectors_functional<E::FlowValueType>(
      expected_loads, actual_loads);
  if (!flag) {
    if (expected_times.size() < 40) {
      std::cout << "key: " << id
                << " " << (use_requested ? "requested" : "achieved")
                << "\n"
                << "expected_times = "
                << E::vec_to_string<E::RealTimeType>(expected_times) << "\n"
                << "expected_loads = "
                << E::vec_to_string<E::FlowValueType>(expected_loads) << "\n"
                << "actual_times   = "
                << E::vec_to_string<E::RealTimeType>(actual_times) << "\n"
                << (use_requested ? "requested_loads=" : "actual_loads   = ")
                << E::vec_to_string<E::FlowValueType>(actual_loads) << "\n";
    }
    else {
      auto exp_num_times{expected_times.size()};
      auto exp_num_loads{expected_loads.size()};
      auto act_num_times{actual_times.size()};
      auto act_num_loads{actual_loads.size()};
      std::cout << "key: " << id
                << " " << (use_requested ? "requested" : "achieved")
                << "\n"
                << "- expected_times.size(): " << exp_num_times << "\n"
                << "- expected_loads.size(): " << exp_num_loads << "\n"
                << "- actual_times.size(): " << act_num_times << "\n"
                << "- actual_loads.size(): " << act_num_loads << "\n";
      auto sizes = std::vector<decltype(exp_num_times)>{
        exp_num_times, exp_num_loads, act_num_times, act_num_loads};
      auto num{*std::min_element(sizes.begin(), sizes.end())};
      int num_discrepancies{0};
      const int max_reporting{10};
      for (std::size_t idx{0}; idx < num; ++idx) {
        auto t_exp{expected_times[idx]};
        auto t_act{actual_times[idx]};
        auto flow_exp{expected_loads[idx]};
        auto flow_act{actual_loads[idx]};
        if ((t_exp != t_act) || (flow_exp != flow_act)) {
          std::cout << "idx: " << idx
                    << " (t: " << t_act << ")\n";
          ++num_discrepancies;
        }
        if (t_exp != t_act) {
          std::cout << "- time discrepancy\n"
                    << "-- expected-time: " << t_exp << "\n"
                    << "-- actual-time: " << t_act << "\n"
                    ;
          if ((idx > 0) && (idx < (num - 1))) {
            std::cout << "-- expected-times: ["
                      << expected_times[idx-1]
                      << ", <<" << expected_times[idx] << ">>, "
                      << expected_times[idx+1] << "]\n"
                      << "-- actual-times: ["
                      << actual_times[idx-1]
                      << ", <<" << actual_times[idx] << ">>, "
                      << actual_times[idx+1] << "]\n";
          }
        }
        if (flow_exp != flow_act) {
          std::cout << "- flow discrepancy\n"
                    << "-- expected-flow: " << flow_exp << "\n"
                    << "-- actual-flow: " << flow_act << "\n"
                    ;
          if ((idx > 0) && (idx < (num - 1))) {
            std::cout << "-- expected-flows: ["
                      << expected_loads[idx-1]
                      << ", <<" << expected_loads[idx] << ">>, "
                      << expected_loads[idx+1] << "]\n"
                      << "-- actual-flows: ["
                      << actual_loads[idx-1]
                      << ", <<" << actual_loads[idx] << ">>, "
                      << actual_loads[idx+1] << "]\n";
          }
        }
        if (num_discrepancies > max_reporting) {
          break;
        }
      }
    }
  }
  return flag;
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
  const auto li2 = ERIN::LoadItem(4, 0);
  EXPECT_EQ(li1.get_time_advance(li2), 4);
  EXPECT_EQ(li1.time, 0);
  EXPECT_EQ(li1.value, 1.0);
  EXPECT_EQ(li2.time, 4);
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

TEST(ErinBasicsTest, CanRunPowerLimitedSink)
{
  namespace E = ERIN;
  E::RealTimeType t_max{4};
  std::vector<E::RealTimeType> expected_time = {0, 1, 2, 3, t_max};
  std::vector<E::FlowValueType> expected_flow = {50, 50, 40, 0, 0};
  std::string elec{"electrical"};
  std::string limit_id{"lim"};
  auto lim = new E::Source(limit_id, E::ComponentType::Source, elec, 50);
  std::string sink_id{"load"};
  auto sink = new E::Sink(
      sink_id,
      E::ComponentType::Load,
      elec,
      { E::LoadItem{0,160},
        E::LoadItem{1,80},
        E::LoadItem{2,40},
        E::LoadItem{3,0},
        E::LoadItem{t_max,0}});
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
  lim->set_flow_writer(fw);
  lim->set_recording_on();
  sink->set_flow_writer(fw);
  sink->set_recording_on();
  adevs::Digraph<E::FlowValueType, E::Time> network;
  network.couple(
      sink, E::Sink::outport_inflow_request,
      lim, E::FlowLimits::inport_outflow_request);
  network.couple(
      lim, E::FlowLimits::outport_outflow_achieved,
      sink, E::Sink::inport_inflow_achieved);
  adevs::Simulator<E::PortValue, E::Time> sim;
  network.add(&sim);
  while (sim.next_event_time() < E::inf) {
    sim.exec_next_event();
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  ASSERT_TRUE(
      check_times_and_loads(results, expected_time, expected_flow, sink_id));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_time, expected_flow, limit_id));
}

/*
TEST(ErinBasicsTest, CanRunBasicDieselGensetExample)
{
  namespace E = ERIN;
  E::RealTimeType t_max{4};
  const double diesel_generator_efficiency{0.36};
  const std::vector<E::RealTimeType> expected_genset_output_times{
    0, 1, 2, 3, t_max};
  const std::vector<E::FlowValueType> expected_genset_output{
    50, 50, 40, 0, 0};
  auto calc_output_given_input =
    [=](E::FlowValueType input_kW) -> E::FlowValueType {
      return input_kW * diesel_generator_efficiency;
    };
  auto calc_input_given_output =
    [=](E::FlowValueType output_kW) -> E::FlowValueType {
      return output_kW / diesel_generator_efficiency;
    };
  const std::vector<E::FlowValueType> expected_fuel_output{
    calc_input_given_output(50),
    calc_input_given_output(50),
    calc_input_given_output(40),
    calc_input_given_output(0),
    calc_input_given_output(0)
  };
  std::string diesel{"diesel"};
  std::string elec{"electrical"};
  std::string converter_id{"converter"};
  auto genset_tx = new E::Converter(
      converter_id,
      E::ComponentType::Converter,
      diesel,
      elec,
      calc_output_given_input,
      calc_input_given_output
      );
  std::string limit_id{"lim"};
  auto genset_lim = new E::FlowLimits(
      limit_id, E::ComponentType::Converter, elec, 0, 50);
  std::vector<E::LoadItem> load_profile{
    E::LoadItem{0,160},
    E::LoadItem{1,80},
    E::LoadItem{2,40},
    E::LoadItem{3,0},
    E::LoadItem{t_max,0.0}};
  std::vector<E::FlowValueType> requested_loads = {160, 80, 40, 0, 0};
  std::string sink_id{"sink"};
  auto sink = new E::Sink(
      sink_id, E::ComponentType::Load, elec, load_profile);
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();

  genset_tx->set_flow_writer(fw);
  genset_tx->set_recording_on();
  genset_lim->set_flow_writer(fw);
  genset_lim->set_recording_on();
  sink->set_flow_writer(fw);
  sink->set_recording_on();

  adevs::Digraph<E::FlowValueType, E::Time> network;
  network.couple(
      sink, E::Sink::outport_inflow_request,
      genset_lim, E::FlowLimits::inport_outflow_request);
  network.couple(
      genset_lim, E::FlowLimits::outport_inflow_request,
      genset_tx, E::Converter::inport_outflow_request);
  network.couple(
      genset_tx, E::Converter::outport_outflow_achieved,
      genset_lim, E::FlowLimits::inport_inflow_achieved);
  network.couple(
      genset_lim, E::FlowLimits::outport_outflow_achieved,
      sink, E::Sink::inport_inflow_achieved);
  adevs::Simulator<E::PortValue, E::Time> sim;
  network.add(&sim);
  while (sim.next_event_time() < E::inf)
    sim.exec_next_event();
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_genset_output_times, expected_genset_output, limit_id));
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_genset_output_times, requested_loads, limit_id, true));
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_genset_output_times, expected_fuel_output, converter_id + "-inflow"));
}
*/

TEST(ErinBasicsTest, CanRunUsingComponents)
{
  namespace EP = erin::port;
  namespace EN = erin::network;
  namespace E = ERIN;
  const std::string stream_name{"electrical"};
  std::string elec{stream_name};
  auto loads_by_scenario = std::unordered_map<
    std::string, std::vector<E::LoadItem>>(
        {{"bluesky", {
            E::LoadItem{0,160},
            E::LoadItem{1,80},
            E::LoadItem{2,40},
            E::LoadItem{3,0},
            E::LoadItem{4,0}}}});
  const std::string source_id{"electrical_pcc"};
  std::unique_ptr<E::Component> source =
    std::make_unique<E::SourceComponent>(source_id, elec);
  const std::string load_id{"electrical_load"};
  std::unique_ptr<E::Component> load =
    std::make_unique<E::LoadComponent>(
        load_id, elec, loads_by_scenario);
  EN::Connection conn{
    EN::ComponentAndPort{source_id, EP::Type::Outflow, 0},
      EN::ComponentAndPort{load_id, EP::Type::Inflow, 0},
      stream_name};
  std::string scenario_id{"bluesky"};
  adevs::Digraph<E::FlowValueType, E::Time> network;
  auto pes_load = load->add_to_network(network, scenario_id);
  auto pes_source = source->add_to_network(network, scenario_id);
  EN::connect(
      network,
      pes_source.port_map,
      EP::Type::Outflow,
      0,
      pes_load.port_map,
      EP::Type::Inflow,
      0,
      true,
      stream_name);
  adevs::Simulator<E::PortValue, E::Time> sim;
  network.add(&sim);
  bool worked{false};
  int iworked{0};
  while (sim.next_event_time() < E::inf) {
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

TEST(ErinBasicsTest, CanReadComponentsFromToml)
{
  std::stringstream ss{};
  ss << "[components.electric_utility]\n"
        "type = \"source\"\n"
        "# Point of Common Coupling for Electric Utility\n"
        "output_stream = \"electricity\"\n"
        "max_outflow = 10.0\n"
        "min_outflow = 0.0\n"
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
  std::string stream_id{"electricity"};
  std::string scenario_id{"blue_sky"};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> loads_by_id{
    {std::string{"load1"}, {ERIN::LoadItem{0,1.0},ERIN::LoadItem{4,0.0}}}
  };
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> loads{
    {scenario_id, {::ERIN::LoadItem{0,1.0},::ERIN::LoadItem{4,0.0}}}
  };
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>> expected;
  expected.emplace(std::make_pair(
        std::string{"electric_utility"},
        std::make_unique<::ERIN::SourceComponent>(
          std::string{"electric_utility"},
          stream_id,
          10,
          0)));
  expected.emplace(std::make_pair(
      std::string{"cluster_01_electric"},
      std::make_unique<::ERIN::LoadComponent>(
        std::string{"cluster_01_electric"},
        stream_id,
        loads)));
  expected.emplace(std::make_pair(
      std::string{"bus"},
      std::make_unique<::ERIN::MuxerComponent>(
        std::string{"bus"},
        stream_id,
        2,
        1,
        ERIN::MuxerDispatchStrategy::InOrder)));
  auto pt = &t;
  auto actual = pt->read_components(loads_by_id);
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
        "time_rate_pairs = [[0.0,1.0],[4.0,0.0]]\n";
  ERIN::TomlInputReader t{ss};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> expected{
    {std::string{"load1"}, {::ERIN::LoadItem{0,1.0},::ERIN::LoadItem{4,0.0}}}
  };
  auto actual = t.read_loads();
  EXPECT_EQ(expected.size(), actual.size());
  for (auto const& e: expected) {
    const auto a = actual.find(e.first);
    ASSERT_TRUE(a != actual.end());
    EXPECT_EQ(e.second.size(), a->second.size());
    for (std::vector<::ERIN::LoadItem>::size_type i{0}; i < e.second.size(); ++i) {
      EXPECT_EQ(e.second[i].time, a->second[i].time);
      EXPECT_EQ(e.second[i].value, a->second[i].value);
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
        "connections = [[\"electric_utility:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n";
  ERIN::TomlInputReader t{ss};
  std::unordered_map<std::string, std::vector<enw::Connection>> expected{
    { "normal_operations",
      { enw::Connection{
                         enw::ComponentAndPort{
                           "electric_utility", ep::Type::Outflow, 0},
                         enw::ComponentAndPort{
                           "cluster_01_electric", ep::Type::Inflow, 0},
                         "electricity"}}}};
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
    ASSERT_EQ(e_conn.stream, a_conn.stream);
  }
}

TEST(ErinBasicsTest, CanReadScenariosFromTomlForFixedDist)
{
  std::stringstream ss{};
  ss << "[scenarios.blue_sky]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"immediately\"\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n";
  ERIN::TomlInputReader t{ss};
  const std::string scenario_id{"blue_sky"};
  const auto expected_duration
    = static_cast<::ERIN::RealTimeType>(8760 * ::ERIN::seconds_per_hour);
  std::unordered_map<std::string, ERIN::Scenario> expected{{
    scenario_id,
    ERIN::Scenario{
      scenario_id,
      std::string{"normal_operations"},
      expected_duration,
      1,
      0,
      {},
      false}}};
  ERIN::size_type occurrence_distribution_id{0};
  std::unordered_map<std::string,ERIN::size_type> dists{
    {"immediately", occurrence_distribution_id}};
  auto actual = t.read_scenarios(dists);
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
  ERIN::Time dt_expected{1, 0};
  auto scenario = actual.at(scenario_id);
  EXPECT_EQ(scenario.get_max_occurrences(), 1);
  EXPECT_EQ(scenario.get_network_id(), "normal_operations");
  EXPECT_EQ(scenario.get_occurrence_distribution_id(), occurrence_distribution_id);
  EXPECT_EQ(scenario.get_duration(), 8760*3600);
  EXPECT_EQ(scenario.get_name(), "blue_sky");
}

TEST(ErinBasicsTest, CanReadScenariosFromTomlForRandIntDist)
{
  const std::string scenario_id{"blue_sky"};
  std::stringstream ss{};
  ss << "[scenarios." << scenario_id << "]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"1_to_10\"\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n";
  ERIN::TomlInputReader t{ss};
  std::unordered_map<std::string, ERIN::size_type> dists{{"1_to_10", 0}};
  auto actual = t.read_scenarios(dists);
  auto scenario = actual.at(scenario_id);
  EXPECT_EQ(scenario.get_duration(), 8760*3600);
}

TEST(ErinBasicsTest, CanReadScenariosIntensities)
{
  const std::string scenario_id{"class_4_hurricane"};
  std::stringstream ss{};
  ss << "[scenarios." << scenario_id << "]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"immediately\"\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"emergency_operations\"\n"
        "intensity.wind_speed_mph = 156\n"
        "intensity.inundation_depth_ft = 4\n";
  ERIN::TomlInputReader t{ss};
  std::unordered_map<std::string, ERIN::size_type> cds{{"immediately",0}};
  auto scenario_map = t.read_scenarios(cds);
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
        "############################################################\n"
        "[loads.building_electrical]\n"
        "time_unit = \"hours\"\n"
        "rate_unit = \"kW\"\n"
        "time_rate_pairs = [[0.0,1.0],[4.0,0.0]]\n"
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
        "connections=[[\"electric_utility:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
        "############################################################\n"
        "[cds.every_hour]\n"
        "type = \"fixed\"\n"
        "value = 1\n"
        "time_unit = \"hours\"\n"
        "############################################################\n"
        "[scenarios.blue_sky]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"every_hour\"\n"
        "duration = 1\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n";
  ERIN::TomlInputReader r{ss};
  auto si = r.read_simulation_info();
  auto loads = r.read_loads();
  auto components = r.read_components(loads);
  auto networks = r.read_networks();
  std::unordered_map<std::string, ERIN::size_type> cds{
    {"every_hour", 0}};
  auto scenarios = r.read_scenarios(cds);
  std::unordered_map<std::string, std::vector<ERIN::TimeState>>
    reliability_schedule{};
  std::unordered_map<std::string, std::vector<ERIN::RealTimeType>>
    scenario_schedules{
      {"blue_sky", {3600}}};
  ERIN::Main m{
    si, components, networks, scenarios, reliability_schedule,
    scenario_schedules};
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
        "connections = [[\"electric_utility:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
        "############################################################\n"
        "[cds.every_hour]\n"
        "type = \"fixed\"\n"
        "value = 1\n"
        "time_unit = \"hours\"\n"
        "############################################################\n"
        "[scenarios.blue_sky]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"every_hour\"\n"
        "duration = 4\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"";
  ERIN::TomlInputReader r{ss};
  auto si = r.read_simulation_info();
  auto loads = r.read_loads();
  auto components = r.read_components(loads);
  auto networks = r.read_networks();
  std::unordered_map<std::string, ERIN::size_type> cds{{"every_hour", 0}};
  auto scenarios = r.read_scenarios(cds);
  std::unordered_map<std::string, std::vector<ERIN::TimeState>>
    reliability_schedule{};
  std::unordered_map<std::string, std::vector<ERIN::RealTimeType>>
    scenario_schedules{{"blue_sky", {3600}}};
  ERIN::Main m{
    si, components, networks, scenarios, reliability_schedule,
    scenario_schedules};
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
  for (int i{0}; i < N; ++i) {
    loads.emplace_back(ERIN::LoadItem{i, 1.0});
  }
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>>
    loads_by_scenario{{scenario_id, loads}};
  ERIN::SimulationInfo si{};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> loads_by_id{
    {load_id, loads}
  };
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>>
    components;
  components.insert(
      std::make_pair(
        source_id,
        std::make_unique<::ERIN::SourceComponent>(
          source_id, stream_id)));
  components.insert(
      std::make_pair(
        load_id,
        std::make_unique<::ERIN::LoadComponent>(
          load_id,
          stream_id,
          loads_by_scenario)));
  std::unordered_map<
    std::string, std::vector<enw::Connection>> networks{
      { net_id,
        { enw::Connection{
                           enw::ComponentAndPort{
                             source_id, ep::Type::Outflow, 0},
                           enw::ComponentAndPort{
                             load_id, ep::Type::Inflow, 0},
                           stream_id}}}};
  std::unordered_map<std::string, ::ERIN::Scenario> scenarios{
    {
      scenario_id,
      ERIN::Scenario{
        scenario_id,
        net_id,
        1,
        -1,
        0,
        {},
        false
      }}};
  std::unordered_map<std::string, std::vector<ERIN::TimeState>>
    reliability_schedule{};
  std::unordered_map<std::string, std::vector<ERIN::RealTimeType>>
    scenario_schedules{{scenario_id, {0}}};
  ERIN::Main m{
    si, components, networks, scenarios, reliability_schedule,
    scenario_schedules};
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
      { B_id, E::ComponentType::Source}},
    { { A_id, E::PortRole::LoadInflow},
      { B_id, E::PortRole::SourceOutflow}}};
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
  vs = std::vector<E::Datum>{ E::Datum{10,1.0,1.0}, E::Datum{5,0.0,0.0}};
  ASSERT_THROW(E::sum_requested_load(vs), std::invalid_argument);
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
  vs = std::vector<E::Datum>{ E::Datum{10,1.0,1.0}, E::Datum{5,0.0,0.0}};
  ASSERT_THROW(E::sum_achieved_load(vs), std::invalid_argument);
}

TEST(ErinBasicsTest, ScenarioResultsToCSV)
{
  ERIN::RealTimeType start_time{0};
  ERIN::RealTimeType duration{4};
  std::string elec_stream_id{"electrical"};
  ERIN::ScenarioResults out{
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
     {std::string{"B"}, ::ERIN::ComponentType::Source}},
    {{std::string{"A"}, ::ERIN::PortRole::LoadInflow},
     {std::string{"B"}, ::ERIN::PortRole::SourceOutflow}},
  };
  auto actual = out.to_csv(::ERIN::TimeUnits::Seconds);
  std::string expected{
    "time (seconds),A:achieved (kW),A:requested (kW),"
      "B:achieved (kW),B:requested (kW)\n"
    "0,1,1,10,10\n1,0.5,0.5,10,10\n2,0,0,5,5\n4,0,0,0,0\n"};
  EXPECT_EQ(expected, actual);
  duration = 4;
  ERIN::ScenarioResults out2{
    true,
    start_time,
    duration,
    {{std::string{"A"}, {::ERIN::Datum{0,1.0,1.0}}}},
    {{std::string{"A"}, elec_stream_id}},
    {{std::string{"A"}, ::ERIN::ComponentType::Load}},
    {{std::string{"A"}, ::ERIN::PortRole::LoadInflow}},
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
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>>
    loads_by_scenario{{scenario_id, loads}};
  ERIN::SimulationInfo si{};
  std::unordered_map<std::string, std::vector<::ERIN::LoadItem>> loads_by_id{
    {load_id, loads}
  };
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>>
    components;
  components.insert(
      std::make_pair(
        source_id,
        std::make_unique<ERIN::SourceComponent>(
          source_id,
          stream_id)));
  components.insert(
      std::make_pair(
        load_id,
        std::make_unique<ERIN::LoadComponent>(
          load_id,
          stream_id,
          loads_by_scenario)));
  std::unordered_map<
    std::string, std::vector<enw::Connection>> networks{
      { net_id,
        { enw::Connection{
                           enw::ComponentAndPort{source_id, ep::Type::Outflow, 0},
                           enw::ComponentAndPort{load_id, ep::Type::Inflow, 0},
                           stream_id}}}};
  std::unordered_map<std::string, ERIN::Scenario> scenarios{
    {
      scenario_id,
      ERIN::Scenario{
        scenario_id,
        net_id,
        max_time,
        -1,
        0,
        {},
        false
      }}};
  std::unordered_map<std::string, std::vector<ERIN::TimeState>>
    reliability_schedule{};
  ERIN::Main m{si, components, networks, scenarios, reliability_schedule};
  auto actual = m.max_time_for_scenario(scenario_id);
  ERIN::RealTimeType expected = max_time;
  EXPECT_EQ(expected, actual);
}

TEST(ErinBasicsTest, TestScenarioResultsMetrics)
{
  // ## Example 0
  ERIN::RealTimeType start_time{0};
  ERIN::RealTimeType duration{4};
  ERIN::ScenarioResults sr0{
    true,
    start_time,
    duration,
    {{ std::string{"A0"},
       { ERIN::Datum{0,1.0,1.0},
         ERIN::Datum{4,0.0,0.0}}}},
    {{ std::string{"A0"},
       std::string{"electrical"}}},
    {{ std::string{"A0"},
       ERIN::ComponentType::Source}},
    {{ std::string{"A0"},
       ERIN::PortRole::SourceOutflow}},
  };
  // energy_availability
  std::unordered_map<std::string,double> expected0{{"A0",1.0}};
  auto actual0 = sr0.calc_energy_availability();
  erin_test_utils::compare_maps<double>(
      expected0, actual0, "energy_availability_with_sr0");
  // max_downtime
  std::unordered_map<std::string,::ERIN::RealTimeType> expected0_max_downtime{
    {"A0",0}};
  auto actual0_max_downtime = sr0.calc_max_downtime();
  erin_test_utils::compare_maps_exact<::ERIN::RealTimeType>(
      expected0_max_downtime, actual0_max_downtime, "max_downtime_with_sr0");
  // load_not_served
  std::unordered_map<std::string,::ERIN::FlowValueType> expected0_lns{
    {"A0",0.0}};
  auto actual0_lns = sr0.calc_load_not_served();
  erin_test_utils::compare_maps<::ERIN::FlowValueType>(
      expected0_lns, actual0_lns, "load_not_served_with_sr0");
  // energy_usage_by_stream
  std::unordered_map<std::string,::ERIN::FlowValueType> expected0_eubs{
    {"electrical", 4.0}};
  auto actual0_eubs = sr0.calc_energy_usage_by_stream(
      ERIN::ComponentType::Source);
  erin_test_utils::compare_maps<::ERIN::FlowValueType>(
      expected0_eubs, actual0_eubs, "energy_usage_by_stream_with_sr0");
  // ## Example 1
  ERIN::ScenarioResults sr1{
    true,
    start_time,
    duration,
    {{ std::string{"A1"},
       { ERIN::Datum{0,2.0,1.0},
         ERIN::Datum{2,0.5,0.5},
         ERIN::Datum{4,0.0,0.0}}}},
    {{ std::string{"A1"},
       std::string{"electrical"}}},
    {{ std::string{"A1"},
       ERIN::ComponentType::Source}},
    {{ std::string{"A1"},
       ERIN::PortRole::SourceOutflow}},
  };
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
  std::vector<ERIN::Datum> ds{
    { ERIN::Datum{0,1.0,1.0},
      ERIN::Datum{4,0.0,0.0}}
  };
  // RealTimeType uptime;
  // RealTimeType downtime;
  // FlowValueType load_not_served;
  // FlowValueType total_energy;
  ERIN::ScenarioStats expected{
    4,    // RealTimeType uptime
    0,    // RealTimeType downtime
    0,    // RealTimeType max_downtime
    0.0,  // FlowValueType load_not_served
    4.0}; // FlowValueType total_energy
  auto actual = ERIN::calc_scenario_stats(ds);
  EXPECT_EQ(expected.uptime, actual.uptime);
  EXPECT_EQ(expected.downtime, actual.downtime);
  EXPECT_EQ(expected.max_downtime, actual.max_downtime);
  EXPECT_NEAR(expected.load_not_served, actual.load_not_served, tolerance);
  EXPECT_NEAR(expected.total_energy, actual.total_energy, tolerance);
}

TEST(ErinBasicsTest, Test_calc_scenario_stats_for_max_single_event_downtime)
{
  std::vector<ERIN::Datum> ds{
    {
      // time, requested, achieved
      ERIN::Datum{0,1.0,1.0},
      ERIN::Datum{4,1.0,0.0},
      ERIN::Datum{6,1.0,1.0},
      ERIN::Datum{20,1.0,0.0},
      ERIN::Datum{28,1.0,1.0},
      ERIN::Datum{30,0.0,0.0}
    }
  };
  // RealTimeType uptime;
  // RealTimeType downtime;
  // FlowValueType load_not_served;
  // FlowValueType total_energy;
  ERIN::ScenarioStats expected{
    20,    // RealTimeType uptime
    10,    // RealTimeType downtime
    8,    // RealTimeType max_downtime
    10.0,  // FlowValueType load_not_served
    20.0}; // FlowValueType total_energy
  auto actual = ERIN::calc_scenario_stats(ds);
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
  namespace enw = erin::network;
  namespace ep = erin::port;
  std::string scenario_id{"blue_sky"};
  std::string stream_id{"electricity_medium_voltage"};
  std::string source_id{"electric_utility"};
  std::string load_id{"cluster_01_electric"};
  std::string net_id{"normal_operations"};
  const ERIN::RealTimeType scenario_duration_s{10};
  std::vector<::ERIN::LoadItem> loads;
  for (ERIN::RealTimeType i{0}; i < scenario_duration_s; ++i) {
    loads.emplace_back(ERIN::LoadItem{i, 1.0});
  }
  std::unordered_map<std::string, std::vector<ERIN::LoadItem>>
    loads_by_scenario{{scenario_id, loads}};
  const ERIN::RealTimeType max_simulation_time_s{1000LL * 8760LL * 3600LL};
  ERIN::SimulationInfo si{
    "kW", "kJ", ERIN::TimeUnits::Seconds, max_simulation_time_s};
  std::unordered_map<std::string, std::vector<ERIN::LoadItem>> loads_by_id{
    {load_id, loads}
  };
  std::unordered_map<std::string, std::unique_ptr<::ERIN::Component>>
    components;
  components.insert(
      std::make_pair(
        source_id,
        std::make_unique<ERIN::SourceComponent>(
          source_id,
          stream_id)));
  components.insert(
      std::make_pair(
        load_id,
        std::make_unique<ERIN::LoadComponent>(
          load_id,
          stream_id,
          loads_by_scenario)));
  std::unordered_map<
    std::string, std::vector<enw::Connection>> networks{
      { net_id,
        { enw::Connection{
                           { source_id, ep::Type::Outflow, 0},
                           { load_id, ep::Type::Inflow, 0},
                           stream_id}}}};
  erin::distribution::DistributionSystem cds{};
  auto dist_id = cds.add_fixed("every_100_seconds", 100);
  std::unordered_map<std::string, ERIN::Scenario> scenarios{
    {
      scenario_id,
      ERIN::Scenario{
        scenario_id,
        net_id,
        scenario_duration_s,
        1,
        dist_id,
        {},
        false
      }}};
  std::unordered_map<std::string, std::vector<ERIN::TimeState>>
    reliability_schedule{};
  auto rand_fn = []()->double { return 0.5; };
  auto scenario_schedules = ERIN::calc_scenario_schedule(
      max_simulation_time_s, scenarios, cds, rand_fn);
  ERIN::Main m{
    si, components, networks, scenarios, reliability_schedule,
    scenario_schedules};
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
  std::string st{"electricity"};
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
  namespace E = ERIN;
  namespace enw = erin::network;
  namespace ep = erin::port;
  namespace ef = erin::fragility;
  E::SimulationInfo si{};
  const auto elec_id = std::string{"electrical"};
  const auto elec_stream_id = std::string{elec_id};
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
  E::fragility_map fs_pcc, fs_load, fs_gen;
  std::vector<std::unique_ptr<ef::Curve>> vs_pcc, vs_gen;
  vs_pcc.emplace_back(fc_wind->clone());
  vs_gen.emplace_back(fc_inundation->clone());
  fs_pcc.emplace(std::make_pair(intensity_wind_speed, std::move(vs_pcc)));
  fs_gen.emplace(std::make_pair(intensity_flood, std::move(vs_gen)));
  std::vector<E::LoadItem>
    loads{E::LoadItem{0,100.0}, E::LoadItem{100,0.0}};
  std::unordered_map<std::string, std::vector<E::LoadItem>>
    loads_by_scenario{{blue_sky, loads}, {class_4_hurricane, loads}};
  std::unordered_map<std::string, std::unique_ptr<E::Component>> comps;
  comps.emplace(
      std::make_pair(
        pcc_id,
        std::make_unique<E::SourceComponent>(
          pcc_id, elec_stream_id, std::move(fs_pcc))));
  comps.emplace(
      std::make_pair(
        load_id,
        std::make_unique<E::LoadComponent>(
          load_id, elec_stream_id, loads_by_scenario, std::move(fs_load))));
  comps.emplace(
      std::make_pair(
        gen_id,
        std::make_unique<E::SourceComponent>(
          gen_id, elec_stream_id, std::move(fs_gen))));
  std::unordered_map<
  std::string, std::vector<enw::Connection>> networks{
    { normal,
      { enw::Connection{ { pcc_id, ep::Type::Outflow, 0},
                         { load_id, ep::Type::Inflow, 0},
                         elec_id}}},
      { emergency,
        { enw::Connection{ { gen_id, ep::Type::Outflow, 0},
                           { load_id, ep::Type::Inflow, 0},
                           elec_id}}}};
  // test 1: simulate with a fragility curve that never fails
  // - confirm the statistics show load always met
  std::unordered_map<std::string, double> intensities_low{
    { intensity_wind_speed, 0.0},
    { intensity_flood, 0.0}};
  std::unordered_map<std::string, E::Scenario> scenarios_low{
    { blue_sky, // 0
      E::Scenario{blue_sky, normal, 10, 1, 0, {}, false}},
    { class_4_hurricane,
      E::Scenario{
        class_4_hurricane, // 100
        emergency, 10, -1, 0, intensities_low, false}}};
  std::unordered_map<std::string, std::vector<ERIN::RealTimeType>>
    scenario_schedules{
      {blue_sky, {0}},
      {class_4_hurricane, { 100*8760*3600,
                            200*8760*3600,
                            300*8760*3600,
                            400*8760*3600,
                            500*8760*3600,
                            600*8760*3600,
                            700*8760*3600,
                            800*8760*3600,
                            900*8760*3600,
                            1000*8760*3600}}};
  E::Main m_low{
    si, comps, networks, scenarios_low, {}, scenario_schedules};
  auto results_low = m_low.run(class_4_hurricane);
  if (false) {
    std::cout << "results_low:\n";
    for (const auto& pair : results_low.get_results())
      std::cout << "... " << pair.first << ": "
                << E::vec_to_string<E::Datum>(pair.second) << "\n";
  }
  EXPECT_NEAR(
      results_low.calc_energy_availability().at(load_id),
      1.0,
      tolerance);

  // test 2: simulate with a fragility curve that always fails
  // - confirm the statistics show 100% load not served
  std::unordered_map<std::string, double> intensities_high{
    { intensity_wind_speed, 300.0},
    { intensity_flood, 20.0}};
  std::unordered_map<std::string, E::Scenario> scenarios_high{
    { blue_sky,
      E::Scenario{
        blue_sky, normal, 10, 1, 0, {}, false}},
    { class_4_hurricane,
      E::Scenario{
        class_4_hurricane,
        emergency, 10, -1, 0, intensities_high, false}}};
  E::Main m_high{
    si, comps, networks, scenarios_high, {}, scenario_schedules};
  auto results_high = m_high.run(class_4_hurricane);
  if (false) {
    std::cout << "results_high:\n";
    for (const auto& pair : results_high.get_results())
      std::cout << "... " << pair.first << ": "
                << E::vec_to_string<E::Datum>(pair.second) << "\n";
  }
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
  namespace E = ERIN;
  E::RealTimeType t{1};
  EXPECT_EQ(
      E::time_to_seconds(t, E::TimeUnits::Years),
      E::rtt_seconds_per_year);
  EXPECT_EQ(
      E::time_to_seconds(t, E::TimeUnits::Days),
      E::rtt_seconds_per_day);
  EXPECT_EQ(
      E::time_to_seconds(t, E::TimeUnits::Hours),
      E::rtt_seconds_per_hour);
  EXPECT_EQ(
      E::time_to_seconds(t, E::TimeUnits::Minutes),
      E::rtt_seconds_per_minute);
  EXPECT_EQ(
      E::time_to_seconds(t, E::TimeUnits::Seconds),
      1);
}

TEST(ErinBasicsTest, TestMuxerComponent)
{
  namespace E = ERIN;
  namespace enw = erin::network; 
  namespace ep = erin::port;
  const std::string s1_id{"s1"};
  const E::FlowValueType s1_max{12.0};
  const E::FlowValueType s2_max{4.0};
  const std::string s2_id{"s2"};
  const std::string muxer_id{"bus"};
  const std::string l1_id{"l1"};
  const std::string l2_id{"l2"};
  const int num_inflows{2};
  const int num_outflows{2};
  const std::string stream{"electrical"};
  const std::string scenario_id{"blue_sky"};
  const E::RealTimeType t_max{12};
  std::unique_ptr<E::Component> m =
    std::make_unique<E::MuxerComponent>(
        muxer_id, stream, num_inflows, num_outflows,
        E::MuxerDispatchStrategy::Distribute);
  std::unordered_map<std::string, std::vector<E::LoadItem>>
    l1_loads_by_scenario{
      { scenario_id,
        { E::LoadItem{0,10},
          E::LoadItem{t_max, 0.0}}}};
  std::unique_ptr<E::Component> l1 =
    std::make_unique<E::LoadComponent>(
        l1_id,
        stream,
        l1_loads_by_scenario);
  std::unordered_map<std::string, std::vector<E::LoadItem>>
    l2_loads_by_scenario{
      { scenario_id,
        { E::LoadItem{0,0},
          E::LoadItem{5,5},
          E::LoadItem{8,10},
          E::LoadItem{10,5},
          E::LoadItem{t_max,0}}}};
  std::unique_ptr<E::Component> l2 =
    std::make_unique<E::LoadComponent>(
        l2_id,
        stream,
        l2_loads_by_scenario);
  std::unique_ptr<E::Component> s1 =
    std::make_unique<E::SourceComponent>(
        s1_id,
        stream,
        s1_max);
  std::unique_ptr<E::Component> s2 =
    std::make_unique<E::SourceComponent>(
        s2_id,
        stream,
        s2_max);
  std::unordered_map<std::string, std::unique_ptr<E::Component>>
    components;
  components.insert(std::make_pair(muxer_id, std::move(m)));
  components.insert(std::make_pair(l1_id, std::move(l1)));
  components.insert(std::make_pair(l2_id, std::move(l2)));
  components.insert(std::make_pair(s1_id, std::move(s1)));
  components.insert(std::make_pair(s2_id, std::move(s2)));
  adevs::Digraph<E::FlowValueType, E::Time> network;
  const std::vector<enw::Connection> connections{
    {{l1_id, ep::Type::Inflow, 0}, {muxer_id, ep::Type::Outflow, 0}, "electrical"},
    {{l2_id, ep::Type::Inflow, 0}, {muxer_id, ep::Type::Outflow, 1}, "electrical"},
    {{muxer_id, ep::Type::Inflow, 0}, {s1_id, ep::Type::Outflow, 0}, "electrical"},
    {{muxer_id, ep::Type::Inflow, 1}, {s2_id, ep::Type::Outflow, 0}, "electrical"}};
  bool two_way{true};
  auto elements = enw::build(
      scenario_id, network, connections, components,
      {}, []()->double{ return 0.0; },
      two_way);
  std::shared_ptr<E::FlowWriter> fw =
    std::make_shared<E::DefaultFlowWriter>();
  std::vector<E::FlowElement*>::size_type expected_num_elements{5};
  EXPECT_EQ(elements.size(), expected_num_elements);
  for (auto e: elements)
    e->set_flow_writer(fw);
  adevs::Simulator<E::PortValue, E::Time> sim;
  network.add(&sim);
  const auto duration{t_max};
  const int max_no_advance{static_cast<int>(elements.size()) * 10};
  auto is_good = E::run_devs(sim, duration, max_no_advance, "test");
  EXPECT_TRUE(is_good);
  fw->finalize_at_time(t_max);
  auto fw_results = fw->get_results();
  auto fw_stream_ids = fw->get_stream_ids();
  auto fw_comp_types = fw->get_component_types();
  auto fw_port_roles = fw->get_port_roles();
  E::RealTimeType scenario_start_time_s{0};
  auto sr = E::process_single_scenario_results(
      is_good, duration, scenario_start_time_s,
      fw_results, fw_stream_ids, fw_comp_types, fw_port_roles);
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
      for (std::vector<E::Datum>::size_type i{0}; i < n; ++i) {
        const auto& v = vs[i];
        std::cout << k << "[" << i << "]{t=" << v.time
                  << ",r=" << v.requested_value
                  << ",a=" << v.achieved_value << "}\n";
      }
    }
  }
  // bus-inflow(0)
  const std::vector<E::Datum> expected_bus_inflow0{
    E::Datum{0,10.0,10.0},
    E::Datum{5,15.0,12.0},
    E::Datum{8,20.0,12.0},
    E::Datum{10,15.0,12.0},
    E::Datum{t_max,0.0,0.0}};
  const auto n_bus_inflow0 = expected_bus_inflow0.size();
  const auto& actual_bus_inflow0 = results.at("bus-inflow(0)");
  EXPECT_EQ(n_bus_inflow0, actual_bus_inflow0.size());
  using size_type = std::vector<E::Datum>::size_type;
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
  const std::vector<E::Datum> expected_bus_inflow1{
    E::Datum{0,0.0,0.0},
    E::Datum{5,3.0,3.0},
    E::Datum{8,8.0,4.0},
    E::Datum{10,3.0,3.0},
    E::Datum{t_max,0.0,0.0}};
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
  const std::vector<E::Datum> expected_bus_outflow0{
    E::Datum{0,10.0,10.0},
    E::Datum{5,10.0,10.0},
    E::Datum{8,10.0,8.0},
    E::Datum{10,10.0,10.0},
    E::Datum{t_max,0.0,0.0}};
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
  const std::vector<E::Datum> expected_bus_outflow1{
    E::Datum{0,0.0,0.0},
    E::Datum{5,5.0,5.0},
    E::Datum{8,10.0,8.0},
    E::Datum{10,5.0,5.0},
    E::Datum{t_max,0.0,0.0}};
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
  namespace E = ERIN;
  namespace ef = erin::fragility;
  std::string id{"source"};
  const std::string stream{"electricity"};
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

/*
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
        "connections = [[\"electric_utility:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
        "[networks.emergency_operations]\n"
        "connections = [\n"
        "  [\"electric_utility:OUT(0)\", \"bus:IN(0)\", \"electricity\"],\n"
        "  [\"emergency_generator:OUT(0)\", \"bus:IN(1)\", \"electricity\"],\n"
        "  [\"bus:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
        "[cds.immediately]\n"
        "type = \"fixed\"\n"
        "value = 0\n"
        "time_unit = \"hours\"\n"
        "[cds.every_10_years]\n"
        "type = \"fixed\"\n"
        "value = 87600\n"
        "time_unit = \"hours\"\n"
        "[scenarios.blue_sky]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"immediately\"\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n"
        "[scenarios.class_4_hurricane]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"every_10_years\"\n"
        "duration = 336\n"
        "max_occurrences = -1\n"
        "network = \"emergency_operations\"\n"
        "intensity.wind_speed_mph = 156\n"
        "intensity.inundation_depth_ft = 8\n";
  const std::vector<int>::size_type num_comps{4};
  const std::vector<int>::size_type num_networks{2};
  ERIN::TomlInputReader r{ss};
  auto si = r.read_simulation_info();
  auto loads = r.read_loads();
  auto fragilities = r.read_fragility_data();
  ERIN::ReliabilityCoordinator rc{};
  auto components = r.read_components(loads, fragilities, {}, rc);
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
      enw::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0},
      "electricity"}};
  ASSERT_EQ(expected_normal_nw.size(), normal_nw.size());
  ASSERT_EQ(expected_normal_nw, normal_nw);
  const std::vector<enw::Connection> expected_eo{
    enw::Connection{
      enw::ComponentAndPort{"electric_utility", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"bus", ep::Type::Inflow, 0},
      "electricity"},
    enw::Connection{
      enw::ComponentAndPort{"emergency_generator", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"bus", ep::Type::Inflow, 1},
      "electricity"},
    enw::Connection{
      enw::ComponentAndPort{"bus", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0},
      "electricity"}};
  const auto& actual_eo = networks["emergency_operations"];
  ASSERT_EQ(expected_eo.size(), actual_eo.size());
  ASSERT_EQ(expected_eo, actual_eo);
  std::unordered_map<std::string, ERIN::size_type> dists{
    {"immediately", 0},
    {"every_10_years", 1}};
  auto scenarios = r.read_scenarios(dists);
  constexpr ERIN::RealTimeType blue_sky_duration
    = 8760 * ERIN::rtt_seconds_per_hour;
  constexpr int blue_sky_max_occurrence = 1;
  constexpr ERIN::RealTimeType hurricane_duration
    = 336 * ERIN::rtt_seconds_per_hour;
  constexpr int hurricane_max_occurrence = -1;
  const std::unordered_map<std::string, ERIN::Scenario> expected_scenarios{
    { "blue_sky",
      ERIN::Scenario{
        "blue_sky",
        "normal_operations",
        blue_sky_duration, 
        blue_sky_max_occurrence,
        0,
        {},
        false}},
    { "class_4_hurricane",
      ERIN::Scenario{
        "class_4_hurricane",
        "emergency_operations",
        hurricane_duration,
        hurricane_max_occurrence,
        1,
        {{"wind_speed_mph", 156.0}, {"inundation_depth_ft", 8.0}},
        false}}};
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
  std::unordered_map<std::string, std::vector<ERIN::TimeState>>
    reliability_schedule{};
  ERIN::Main m{si, components, networks, scenarios, reliability_schedule};
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
*/

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
        "connections = [[\"electric_utility:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
        "[networks.emergency_operations]\n"
        "connections = [\n"
        "  [\"electric_utility:OUT(0)\", \"bus:IN(0)\", \"electricity\"],\n"
        "  [\"emergency_generator:OUT(0)\", \"bus:IN(1)\", \"electricity\"],\n"
        "  [\"bus:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
        "[dist.immediately]\n"
        "type = \"fixed\"\n"
        "value = 0\n"
        "time_unit = \"hours\"\n"
        "[dist.every_10_years]\n"
        "type = \"fixed\"\n"
        "value = 87600\n"
        "time_unit = \"hours\"\n"
        "[scenarios.blue_sky]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"immediately\"\n"
        "duration = 8760\n"
        "max_occurrences = 1\n"
        "network = \"normal_operations\"\n"
        "[scenarios.class_4_hurricane]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"every_10_years\"\n"
        "duration = 336\n"
        "max_occurrences = -1\n"
        "network = \"emergency_operations\"\n"
        "intensity.wind_speed_mph = 200.0\n"
        "intensity.inundation_depth_ft = 20.0\n";
  const std::vector<int>::size_type num_comps{4};
  const std::vector<int>::size_type num_networks{2};
  ERIN::TomlInputReader r{ss};
  auto si = r.read_simulation_info();
  auto loads = r.read_loads();
  auto fragilities = r.read_fragility_data();
  ERIN::ReliabilityCoordinator rc{};
  auto components = r.read_components(loads, fragilities, {}, rc);
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
    enw::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0},
    "electricity"}};
  ASSERT_EQ(expected_normal_nw.size(), normal_nw.size());
  ASSERT_EQ(expected_normal_nw, normal_nw);
  const std::vector<enw::Connection> expected_eo{
    enw::Connection{
      enw::ComponentAndPort{"electric_utility", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"bus", ep::Type::Inflow, 0},
      "electricity"},
    enw::Connection{
      enw::ComponentAndPort{"emergency_generator", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"bus", ep::Type::Inflow, 1},
      "electricity"},
    enw::Connection{
      enw::ComponentAndPort{"bus", ep::Type::Outflow, 0},
      enw::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0},
      "electricity"}};
  const auto& actual_eo = networks["emergency_operations"];
  ASSERT_EQ(expected_eo.size(), actual_eo.size());
  ASSERT_EQ(expected_eo, actual_eo);
  std::unordered_map<std::string, ERIN::size_type>
    dists{{"immediately",0}, {"every_10_years", 1}};
  auto scenarios = r.read_scenarios(dists);
  constexpr ::ERIN::RealTimeType blue_sky_duration
    = 8760 * ::ERIN::rtt_seconds_per_hour;
  constexpr int blue_sky_max_occurrence = 1;
  constexpr ::ERIN::RealTimeType hurricane_duration
    = 336 * ::ERIN::rtt_seconds_per_hour;
  constexpr int hurricane_max_occurrence = -1;
  const std::unordered_map<std::string, ::ERIN::Scenario> expected_scenarios{
    { "blue_sky",
      ERIN::Scenario{
        "blue_sky",
        "normal_operations",
        blue_sky_duration, 
        blue_sky_max_occurrence,
        0,
        {},
        false}},
    { "class_4_hurricane",
      ERIN::Scenario{
        "class_4_hurricane",
        "emergency_operations",
        hurricane_duration,
        hurricane_max_occurrence,
        1,
        {{"wind_speed_mph", 200.0}, {"inundation_depth_ft", 20.0}},
        false}}};
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
  std::unordered_map<std::string, std::vector<ERIN::TimeState>>
    reliability_schedule{};
  ERIN::Main m{si, components, networks, scenarios, reliability_schedule};
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
  std::unordered_map<std::string, E::PortRole> port_roles{
    { id_cluster_01_electric, E::PortRole::LoadInflow},
    { id_electric_utility, E::PortRole::SourceOutflow}};
  E::RealTimeType scenario_start{0 * hours_to_seconds};
  E::RealTimeType duration{4 * hours_to_seconds};
  E::ScenarioResults sr{
    is_good, scenario_start, duration, data,
    stream_types, comp_types, port_roles};
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
    "blue_sky,1,4,TOTAL (load),,,,,,14400\n"
    "blue_sky,1,4,TOTAL (storage),,,,,,0.0\n"
    "blue_sky,1,4,TOTAL (waste),,,,,,0.0\n"
    "blue_sky,1,4,ENERGY BALANCE (source-(load+storage+waste)),0,,,,,\n"
  };
  auto actual_stats_csv = ar.to_stats_csv();
  EXPECT_EQ(expected_stats_csv, actual_stats_csv);
}

TEST(ErinBasicsTest, ScenarioStatsAddAndAddEq)
{
  ERIN::ScenarioStats a{1,2,2,1.0,1.0};
  ERIN::ScenarioStats b{10,20,10,10.0,10.0};
  ERIN::ScenarioStats expected{11,22,10,11.0,11.0};
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
  std::unordered_map<std::string, E::PortRole> port_roles{
    { id_cluster_01_electric, E::PortRole::LoadInflow},
    { id_electric_utility, E::PortRole::SourceOutflow}};
  E::RealTimeType scenario_start{10 * hours_to_seconds};
  E::RealTimeType duration{4 * hours_to_seconds};
  E::ScenarioResults sr{
    is_good, scenario_start, duration, data,
    stream_types, comp_types, port_roles};
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
  std::unordered_map<std::string, E::PortRole> port_roles{
    { id_cluster_01_electric, E::PortRole::LoadInflow},
    { id_electric_utility, E::PortRole::SourceOutflow}};
  E::RealTimeType scenario_start{10 * hours_to_seconds};
  E::RealTimeType duration{8 * hours_to_seconds};
  E::ScenarioResults sr{
    is_good, scenario_start, duration, data,
    stream_types, comp_types, port_roles};
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

/*
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
    "[loads.default]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[4.0,0.0]]\n"
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
    "connections = [[\"electric_utility:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
    "[networks.emergency_operations]\n"
    "connections = [\n"
    "  [\"electric_utility:OUT(0)\", \"bus:IN(0)\", \"electricity\"],\n"
    "  [\"emergency_generator:OUT(0)\", \"bus:IN(1)\", \"electricity\"],\n"
    "  [\"bus:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[dist.every_10_seconds]\n"
    "type = \"fixed\"\n"
    "value = 10\n"
    "time_unit = \"seconds\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 4\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "[scenarios.class_4_hurricane]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"every_10_seconds\"\n"
    "duration = 4\n"
    "max_occurrences = -1\n"
    "network = \"emergency_operations\"\n"
    "intensity.wind_speed_mph = 156\n"
    "intensity.inundation_depth_ft = 8\n";
  namespace E = ERIN;
  auto m = E::make_main_from_string(input);
  auto results = m.run_all();
  std::vector<std::string> expected_scenario_ids{
    "blue_sky", "class_4_hurricane"};
  std::unordered_map<
    std::string,
    E::size_type> expected_num_results{
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
        auto actual_val = actual_teas.at(i);
        EXPECT_NEAR(expected_teas[i], actual_val, tolerance)
          << scenario_id << ":" << stream_name << "[" << i << "]"
          << " expected_teas=" << expected_teas[i]
          << " actual_teas  =" << actual_val << "\n";
      }
    }
  }
}
*/

/*
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
    "[loads.default]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[4.0,0.0]]\n"
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
    "connections = [[\"electric_utility:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
    "[networks.emergency_operations]\n"
    "connections = [\n"
    "  [\"electric_utility:OUT(0)\", \"bus:IN(0)\", \"electricity\"],\n"
    "  [\"emergency_generator:OUT(0)\", \"bus:IN(1)\", \"electricity\"],\n"
    "  [\"bus:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"seconds\"\n"
    "[dist.every_10_seconds]\n"
    "type = \"fixed\"\n"
    "value = 10\n"
    "time_unit = \"seconds\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 4\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "[scenarios.class_4_hurricane]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"every_10_seconds\"\n"
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
*/

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
    "[loads.default]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[4.0,0.0]]\n"
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
    "connections = [[\"electric_utility:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
    "[networks.emergency_operations]\n"
    "connections = [\n"
    "  [\"electric_utility:OUT(0)\", \"bus:IN(0)\", \"electricity\"],\n"
    "  [\"emergency_generator:OUT(0)\", \"bus:IN(1)\", \"electricity\"],\n"
    "  [\"bus:OUT(0)\", \"cluster_01_electric:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[dist.every_10_hours]\n"
    "type = \"fixed\"\n"
    "value = 10\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 4\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "[scenarios.class_4_hurricane]\n"
    "time_unit = \"hours\"\n"
    "occurrence_distribution = \"every_10_hours\"\n"
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"C", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"C", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"C", std::string{"electricity"}}}, // TODO check this; shouldn't it be "B"?
    std::unordered_map<std::string,E::ComponentType>{
      {"A", E::ComponentType::Load},
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"C", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Load}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"B", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"B", E::PortRole::SourceOutflow}}};
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
      {"C", E::ComponentType::Source}},
    std::unordered_map<std::string,E::PortRole>{
      {"A", E::PortRole::LoadInflow},
      {"C", E::PortRole::SourceOutflow}}};
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
  namespace E = ERIN;
  std::stringstream ss{};
  ss << "[scenarios.a]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"every_10_years\"\n"
        "duration = 10\n"
        "max_occurrences = 1\n"
        "network = \"nw_A\"\n"
        "[scenarios.b]\n"
        "time_unit = \"hours\"\n"
        "occurrence_distribution = \"every_10_hours\"\n"
        "duration = 10\n"
        "max_occurrences = 1\n"
        "network = \"nw_B\"\n";
  E::TomlInputReader t{ss};
  std::unordered_map<std::string, E::size_type>
    dists{{"every_10_years", 0}, {"every_10_hours", 1}};
  auto scenario_map = t.read_scenarios(dists);
  auto a_it = scenario_map.find("a");
  ASSERT_TRUE(a_it != scenario_map.end());
  auto b_it = scenario_map.find("b");
  ASSERT_TRUE(b_it != scenario_map.end());
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
    "max_time = 40\n" + random_line + "\n"
    "[loads.load01]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1.0],[10.0,0.0]]\n"
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
    "connections = [[\"A:OUT(0)\", \"B:IN(0)\", \"electricity\"]]\n"
    "[dist.every_10_years]\n"
    "type = \"fixed\"\n"
    "value = 10\n"
    "time_unit = \"years\"\n"
    "[scenarios.scenario01]\n"
    "time_unit = \"hours\"\n"
    "occurrence_distribution = \"every_10_years\"\n"
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
    "[loads.load01]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1.0],[10.0,0.0]]\n"
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
    "connections = [[\"S:OUT(0)\", \"C:IN(0)\", \"diesel\"], [\"C:OUT(0)\", \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.scenario01]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
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
  // num_components + 3 because for converter we have an input (the one we
  // count) plus output, lossport, and wasteport
  EXPECT_EQ(stats_by_comp_id.size(), expected_num_components + 3);
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
        std::string{"diesel"},
        std::string{"electricity"},
        std::string{"waste_heat"},
        const_eff);
  EXPECT_EQ(expected_conv, conv);
  std::ostringstream oss;
  oss << conv;
  std::string expected_str{
    "ConverterComponent(id=C, component_type=converter, "
                       "input_stream=\"diesel\", "
                       "output_stream=\"electricity\", "
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
    "[loads.electric_load]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0,0.0]]\n"
    "[loads.heating_load]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1.0],[10.0,0.0]]\n"
    "[loads.waste_heat_load]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1000.0],[10.0,0.0]]\n"
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
    "lossflow = \"waste_heat\"\n"
    "constant_efficiency = 0.5\n"
    "dispatch_strategy = \"dump_load\"\n"
    "[networks.nw01]\n"
    "connections = [[\"S:OUT(0)\", \"C0:IN(0)\", \"natural_gas\"], "
    "               [\"C0:OUT(0)\", \"LE:IN(0)\", \"electricity\"], "
    "               [\"C0:OUT(1)\", \"C1:IN(0)\", \"waste_heat\"], "
    "               [\"C1:OUT(0)\", \"LT:IN(0)\", \"district_hot_water\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.scenario01]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
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
  const size_type expected_num_components{5};
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
  ASSERT_THROW(auto _ = p1.with_requested(v2, t0), std::invalid_argument);
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
  if (false) {
    std::cout << "p=" << p << "\n";
    std::cout << "p1=" << p1 << "\n";
    std::cout << "p2=" << p2 << "\n";
  }
  EXPECT_TRUE(p2.should_propagate_request_at(t1));
  EXPECT_FALSE(p2.should_propagate_achieved_at(t1));
  EXPECT_FALSE(p2.should_propagate_request_at(t2));
  EXPECT_FALSE(p2.should_propagate_achieved_at(t2));
  //auto p2 = p1.with_achieved(10.0);
  //EXPECT_EQ(p2.get_requested(), 20.0);
  //EXPECT_EQ(p2.get_achieved(), 10.0);
}

/*
TEST(ErinComponents, Test_passthrough_component)
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.load0]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0,0.0]]\n"
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
    "connections = [[\"S:OUT(0)\", \"P:IN(0)\", \"electricity\"], [\"P:OUT(0)\", \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.scenario0]\n"
    "time_unit = \"seconds\"\n"
    "duration = 10\n"
    "occurrence_distribution = \"immediately\"\n"
    "max_occurrences = 1\n"
    "network = \"nw0\"\n";
  namespace E = ::ERIN;
  auto pt = E::ComponentType::PassThrough;
  EXPECT_EQ("pass_through", E::component_type_to_tag(pt));
  EXPECT_EQ(E::tag_to_component_type("pass_through"), pt);
  auto ptc = E::PassThroughComponent{"my_comp", std::string{"electrical"}};
  auto ptc2 = E::PassThroughComponent{"my_comp", std::string{"electrical"}};
  std::ostringstream oss;
  oss << ptc;
  std::string stream_str{"\"electrical\""};
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
*/

TEST(ErinComponents, Test_passthrough_component_with_fragility)
{
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.load0]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0,0.0]]\n"
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
    "connections = [[\"S:OUT(0)\", \"P:IN(0)\", \"electricity\"], [\"P:OUT(0)\", \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.scenario0]\n"
    "time_unit = \"seconds\"\n"
    "duration = 10\n"
    "occurrence_distribution = \"immediately\"\n"
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
    "[loads.load0]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0,0.0]]\n"
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
    "connections = [[\"S:OUT(0)\", \"P:IN(0)\", \"electricity\"], [\"P:OUT(0)\", \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.scenario0]\n"
    "time_unit = \"seconds\"\n"
    "duration = 10\n"
    "occurrence_distribution = \"immediately\"\n"
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
      "P", std::string{"electricity"}, ERIN::Limits{0.0,100.0}, {});
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
    "[loads.load0]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0,0.0]]\n"
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
    "connections = [[\"S:OUT(0)\", \"C:IN(0)\", \"natural_gas\"], [\"C:OUT(0)\", \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.scenario0]\n"
    "time_unit = \"seconds\"\n"
    "duration = 10\n"
    "occurrence_distribution = \"immediately\"\n"
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
    {"C-inflow", E::ScenarioStats{10,0,0,0.0,0.0}},
    {"C-outflow", E::ScenarioStats{0,10,10,100.0,0.0}},
    {"C-lossflow", E::ScenarioStats{10,0,0,0.0,0.0}},
    {"C-wasteflow", E::ScenarioStats{10,0,0,0.0,0.0}},
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
  namespace E = ERIN;
  namespace EU = erin::utils;
  std::unique_ptr<ERIN::FlowElement> c = std::make_unique<ERIN::Converter>(
      "conv",
      E::ComponentType::Converter,
      std::string{"coal"},
      std::string{"electricity"},
      [](E::FlowValueType input) -> E::FlowValueType { return input * 0.5; },
      [](E::FlowValueType output) -> E::FlowValueType { return output * 2.0; });
  // * external: lossflow request of 100,000 comes in at (0,0)
  // * ta() should be 0
  // * external: outflow request of 10 comes in at (0,1)
  // * ta() should be 0
  // * ys = output(); ys = [{:port :outport_inflow_request :value 20}]
  // * external: inflow achieved of 20 comes in at (0,3)
  // * call output_func() and check values of ys; do we report the right lossflow?
  // * call delta_int()
  E::Time t0{0,0};
  E::Time dt{0,1};
  int inport_lossflow_request{
    E::FlowElement::inport_outflow_request + 1};
  int outport_lossflow_achieved{
    E::FlowElement::outport_outflow_achieved + 1};
  auto lossflow_request = E::PortValue{
    inport_lossflow_request,
    100'000.0,
  }; 
  auto outflow_request = E::PortValue{
    E::FlowElement::inport_outflow_request,
    10.0,
  };
  auto inflow_achieved = E::PortValue{
    E::FlowElement::inport_inflow_achieved,
    20.0,
  };
  std::vector<E::PortValue> v1 = {lossflow_request};
  std::vector<E::PortValue> v2 = {outflow_request};
  std::vector<E::PortValue> v3 = {inflow_achieved};
  auto dt_next = c->ta();
  EXPECT_EQ(dt_next, E::inf);
  c->delta_ext(t0+dt, v1);
  dt_next = c->ta();
  EXPECT_EQ(dt_next, E::inf);
  c->delta_ext(dt, v2);
  dt_next = c->ta();
  EXPECT_EQ(dt_next, dt);
  std::vector<E::PortValue> outputs1{};
  c->output_func(outputs1);
  c->delta_int();
  dt_next = c->ta();
  // lossflow_achieved
  ASSERT_EQ(1, outputs1.size());
  EXPECT_EQ(E::FlowElement::outport_inflow_request, outputs1[0].port);
  EXPECT_EQ(20.0, outputs1[0].value);
  EXPECT_EQ(dt_next, E::inf);
  c->delta_ext(dt, v3);
  dt_next = c->ta();
  EXPECT_EQ(dt_next, dt);
  std::vector<E::PortValue> outputs2{};
  std::vector<E::PortValue> expected_outputs2{
    E::PortValue{E::FlowElement::outport_outflow_achieved, 10.0},
    E::PortValue{outport_lossflow_achieved, 10.0}};
  c->output_func(outputs2);
  ASSERT_EQ(2, outputs2.size());
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<E::PortValue>(
        outputs2, expected_outputs2, compare_ports));
}

TEST(ErinGraphviz, Test_that_we_can_generate_graphviz)
{
  namespace en = erin::network;
  namespace ep = erin::port;
  std::vector<en::Connection> nw = {
  en::Connection{
    en::ComponentAndPort{"electric_utility", ep::Type::Outflow, 0},
    en::ComponentAndPort{"cluster_01_electric", ep::Type::Inflow, 0},
    "electricity"}};
  namespace eg = erin::graphviz;
  std::string expected =
    "digraph ex01_normal_operations {\n"
    "  node [shape=record];\n"
    "  cluster_01_electric [shape=record,label=\"<I0> I(0)|<name> cluster_01_electric\"];\n"
    "  electric_utility [shape=record,label=\"<name> electric_utility|<O0> O(0)\"];\n"
    "  electric_utility:O0:s -> cluster_01_electric:I0:n;\n"
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
  std::string path0{"erin"};
  std::string expected_filename0{"erin"};
  EXPECT_EQ(expected_filename0, eu::path_to_filename(path0));
  std::string path1{"./bin/erin"};
  std::string expected_filename1{"erin"};
  EXPECT_EQ(expected_filename1, eu::path_to_filename(path1));
  std::string path2{".\\bin\\Debug\\erin.exe"};
  std::string expected_filename2{"erin.exe"};
}

TEST(ErinElements, Test_flow_writer_implementation)
{
  auto fw = ERIN::DefaultFlowWriter();
  auto id = fw.register_id(
      "element", "electricity", ERIN::ComponentType::Load, ERIN::PortRole::LoadInflow, true);
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
  auto id1 = fw.register_id(
      "electric_load_1:inflow", "electricity",
      ERIN::ComponentType::Load, ERIN::PortRole::LoadInflow, true);
  auto id2 = fw.register_id(
      "diesel_genset_1:outflow", "electricity",
      ERIN::ComponentType::Converter, ERIN::PortRole::Outflow, true);
  auto id3 = fw.register_id(
      "electric_load_2:inflow", "electricity",
      ERIN::ComponentType::Load, ERIN::PortRole::LoadInflow, true);
  auto id4 = fw.register_id(
      "diesel_genset_2:outflow", "electricity",
      ERIN::ComponentType::Converter, ERIN::PortRole::Outflow, true);
  auto id5 = fw.register_id(
      "diesel_fuel_tank:outflow", "diesel_fuel",
      ERIN::ComponentType::Source, ERIN::PortRole::SourceOutflow, true);
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

TEST(ErinElements, Test_flow_writer)
{
  std::unique_ptr<ERIN::FlowWriter> fw1 = std::make_unique<ERIN::DefaultFlowWriter>();
  auto id = fw1->register_id(
      "element", "stream", ERIN::ComponentType::Load, ERIN::PortRole::LoadInflow, true);
  fw1->write_data(id, 0, 10.0, 10.0);
  fw1->write_data(id, 4, 20.0, 10.0);
  fw1->finalize_at_time(10);
  auto results1 = fw1->get_results();
  fw1->clear();
  std::vector<ERIN::RealTimeType> expected_times1 = {0,4,10};
  auto actual_times1 = ERIN::get_times_from_results_for_component(results1, "element");
  EXPECT_EQ(expected_times1, actual_times1);
  std::vector<ERIN::FlowValueType> expected_achieved_flows1 = {10.0,10.0,0.0};
  auto actual_achieved_flows1 = ERIN::get_actual_flows_from_results_for_component(results1, "element");
  EXPECT_EQ(expected_achieved_flows1, actual_achieved_flows1);
  std::vector<ERIN::FlowValueType> expected_requested_flows1 = {10.0,20.0,0.0};
  auto actual_requested_flows1 = ERIN::get_requested_flows_from_results_for_component(results1, "element");
  EXPECT_EQ(expected_requested_flows1, actual_requested_flows1);
}

/*
TEST(ErinDevs, Test_flow_limits_functions)
{
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  using size_type = std::vector<ED::PortValue>::size_type;
  ED::FlowLimits limits{0.0, 100.0};
  ED::FlowLimitsState s0{
    // time, inflow_port, outflow_port
    0, ED::Port3{}, ED::Port3{},
    // FlowLimits, report_inflow_request, report_outflow_achieved
    limits, false, false};
  auto dt0 = ED::flow_limits_time_advance(s0);
  EXPECT_EQ(dt0, ED::infinity); // ED::infinity is the representation we use for infinity
  auto s1 = ED::flow_limits_external_transition_on_outflow_request(s0, 2, 10.0);
  ED::FlowLimitsState expected_s1{
    2,
    ED::Port3{10.0},
    ED::Port3{10.0},
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
    ED::Port3{10.0},
    ED::Port3{10.0},
    limits, false, false};
  EXPECT_EQ(s2, expected_s2);
  auto s3 = ED::flow_limits_external_transition_on_inflow_achieved(s2, 0, 8.0);
  ED::FlowLimitsState expected_s3{
    2,
    ED::Port3{10.0,8.0},
    ED::Port3{10.0,8.0},
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
    ED::Port3{10.0,8.0},
    ED::Port3{10.0,8.0},
    limits, false, false};
  EXPECT_EQ(s4, expected_s4);
  auto dt4 = ED::flow_limits_time_advance(s4);
  EXPECT_EQ(dt4, ED::infinity);
  std::vector<ED::PortValue> xs = {
    ED::PortValue{ED::inport_outflow_request, 200.0}};
  auto s5 = ED::flow_limits_external_transition(s4, 2, xs);
  ED::FlowLimitsState expected_s5{
    4,
    ED::Port3{100.0, 8.0},
    ED::Port3{200.0, 8.0},
    limits, true, false};
  EXPECT_EQ(s5, expected_s5);
  auto ys5 = ED::flow_limits_output_function(s5);
  std::vector<ED::PortValue> expected_ys5 = {
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
    ED::Port3{100.0, 55.0},
    ED::Port3{200.0, 55.0},
    limits, false, true};
  EXPECT_EQ(s7, expected_s7);
  auto ys7 = ED::flow_limits_output_function(s7);
  std::vector<ED::PortValue> expected_ys7{
    ED::PortValue{ED::outport_outflow_achieved, 55.0}};
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys7, expected_ys7, compare_ports));
}
*/

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
  ED::FlowValueType constant_efficiency{0.25};
  auto s0 = ED::make_converter_state(constant_efficiency);
  std::unique_ptr<ED::ConversionFun> cf =
    std::make_unique<ED::ConstantEfficiencyFun>(constant_efficiency);
  ED::ConverterState expected_s0{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    0, ED::Port3{}, ED::Port3{}, ED::Port3{}, ED::Port3{},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  EXPECT_EQ(s0, expected_s0);
  auto dt0 = ED::converter_time_advance(s0);
  EXPECT_EQ(dt0, ED::infinity);
  std::vector<ED::PortValue> xs0{ED::PortValue{ED::inport_outflow_request, 10.0}};
  auto s1 = ED::converter_external_transition(s0, 2, xs0);
  ED::ConverterState expected_s1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    2, ED::Port3{40.0}, ED::Port3{10.0}, ED::Port3{0.0}, ED::Port3{30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, false, false};
  ASSERT_EQ(expected_s1, s1);
  auto dt1 = ED::converter_time_advance(s1);
  ASSERT_EQ(dt1, 0);
  auto ys1 = ED::converter_output_function(s1);
  std::vector<ED::PortValue> expected_ys1{
    ED::PortValue{ED::outport_inflow_request, 40.0}};
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys1, expected_ys1, compare_ports));
  auto s2 = ED::converter_internal_transition(s1);
  ED::ConverterState expected_s2{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    2, ED::Port3{40.0}, ED::Port3{10.0}, ED::Port3{0.0}, ED::Port3{30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  ASSERT_EQ(expected_s2, s2);
  auto dt2 = ED::converter_time_advance(s2);
  ASSERT_EQ(dt2, ED::infinity);
  std::vector<ED::PortValue> xs2{
    ED::PortValue{ED::inport_inflow_achieved, 20.0}};
  auto s3 = ED::converter_external_transition(s2, 1, xs2);
  ED::ConverterState expected_s3{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    3, ED::Port3{40.0, 20.0}, ED::Port3{10.0, 5.0}, ED::Port3{0.0}, ED::Port3{30.0, 15.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, true, false};
  ASSERT_EQ(expected_s3, s3);
  auto dt3 = ED::converter_time_advance(s3);
  ASSERT_EQ(dt3, 0);
  auto ys3 = ED::converter_output_function(s3);
  std::vector<ED::PortValue> expected_ys3{
    ED::PortValue{ED::outport_outflow_achieved, 5.0}};
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys3, expected_ys3, compare_ports));
  auto s4 = ED::converter_internal_transition(s3);
  ED::ConverterState expected_s4{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    3, ED::Port3{40.0, 20.0}, ED::Port3{10.0, 5.0}, ED::Port3{0.0}, ED::Port3{30.0, 15.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  ASSERT_EQ(s4, expected_s4);
  auto dt4 = ED::converter_time_advance(s4);
  ASSERT_EQ(dt4, ED::infinity);
  // Test Confluent Transitions
  const int inport_lossflow_request{ED::inport_outflow_request + 1};
  std::vector<ED::PortValue> xs1a{
    ED::PortValue{inport_lossflow_request, 2.0}};
  auto s2a = ED::converter_confluent_transition(s1, xs1a);
  ED::ConverterState expected_s2a{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    2, ED::Port3{40.0}, ED::Port3{10.0}, ED::Port3{2.0}, ED::Port3{28.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  ASSERT_EQ(s2a, expected_s2a);

  // Test Multiple Events for a Single External Transition
  // Starting from a "Zero" State
  std::vector<ED::PortValue> xs_a{
    ED::PortValue{ED::inport_outflow_request, 10.0}};
  auto s_a = ED::converter_external_transition(s0, 10, xs_a);
  ED::ConverterState expected_s_a{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    10, ED::Port3{40.0}, ED::Port3{10.0}, ED::Port3{0.0}, ED::Port3{30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, false, false};
  ASSERT_EQ(s_a, expected_s_a);

  std::vector<ED::PortValue> xs_b{
    ED::PortValue{inport_lossflow_request, 30.0}};
  auto s_b = ED::converter_external_transition(s0, 10, xs_b);
  ED::ConverterState expected_s_b{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    10, ED::Port3{0.0}, ED::Port3{0.0}, ED::Port3{30.0, 0.0}, ED::Port3{0.0, 0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  ASSERT_EQ(s_b, expected_s_b);

  // setting up an achieved more than requested situation
  std::cout << "1\n";
  std::vector<ED::PortValue> xs_c{
    ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  auto some_s = ED::converter_external_transition(s_a, 10, xs_c);
  ASSERT_FALSE(some_s.report_inflow_request);

  std::vector<ED::PortValue> xs_d{
    ED::PortValue{ED::inport_outflow_request, 10.0},
    ED::PortValue{inport_lossflow_request, 30.0}};
  auto s_d = ED::converter_external_transition(s_a, 10, xs_d);
  ED::ConverterState expected_s_d{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    20, ED::Port3{40.0}, ED::Port3{10.0}, ED::Port3{30.0}, ED::Port3{0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  EXPECT_EQ(s_d, expected_s_d);

  // ... we get an outflow request and somehow we get an overrequest at the
  // same moment; it all works out
  std::vector<ED::PortValue> xs_e{
    ED::PortValue{ED::inport_outflow_request, 10.0},
    ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  some_s = ED::converter_external_transition(s_a, 10, xs_e);
  EXPECT_FALSE(some_s.report_inflow_request);

  // a lossflow port cannot drive an inflow request. Therefore, inflow is going
  // to get rerequested at 0 and lossflow request denied
  std::vector<ED::PortValue> xs_f{
    ED::PortValue{inport_lossflow_request, 30.0},
    ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  some_s = ED::converter_external_transition(s_a, 10, xs_f);
  EXPECT_FALSE(some_s.report_inflow_request);
  EXPECT_TRUE(some_s.report_outflow_achieved);
  EXPECT_TRUE(some_s.report_lossflow_achieved);
  EXPECT_EQ(some_s.inflow_port.get_requested(), 40.0);
  EXPECT_EQ(some_s.lossflow_port.get_achieved(), 30.0);

  // inflow, outflow, and lossflow just happen to be in sync. OK.
  std::vector<ED::PortValue> xs_g{
    ED::PortValue{ED::inport_outflow_request, 10.0},
    ED::PortValue{inport_lossflow_request, 30.0},
    ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  some_s = ED::converter_external_transition(s_a, 10, xs_g);
  EXPECT_FALSE(some_s.report_inflow_request);
  EXPECT_TRUE(some_s.report_outflow_achieved);
  EXPECT_TRUE(some_s.report_lossflow_achieved);
  EXPECT_EQ(some_s.inflow_port.get_requested(), 40.0);
  EXPECT_EQ(some_s.lossflow_port.get_achieved(), 30.0);
  EXPECT_EQ(some_s.outflow_port.get_achieved(), 10.0);
  
  // Test Multiple Events for a Single External Transition
  ED::ConverterState s_m{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    2, ED::Port3{80.0}, ED::Port3{20.0}, ED::Port3{0.0}, ED::Port3{60.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  //std::vector<ED::PortValue> xs_a{
  //  ED::PortValue{ED::inport_outflow_request, 10.0}};
  auto s_a1 = ED::converter_external_transition(s_m, 10, xs_a);
  ED::ConverterState expected_s_a1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port3{40.0}, ED::Port3{10.0}, ED::Port3{0.0}, ED::Port3{30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, false, false};
  EXPECT_EQ(s_a1, expected_s_a1);

  //std::vector<ED::PortValue> xs_b{
  //  ED::PortValue{ED::inport_lossflow_request, 30.0}};
  auto s_b1 = ED::converter_external_transition(s_m, 10, xs_b);
  ED::ConverterState expected_s_b1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port3{80.0}, ED::Port3{20.0}, ED::Port3{30.0}, ED::Port3{30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, false, false};
  EXPECT_EQ(s_b1, expected_s_b1);

  //std::vector<ED::PortValue> xs_c{
  //  ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  auto s_c1 = ED::converter_external_transition(s_m, 10, xs_c);
  ED::ConverterState expected_s_c1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port3{80.0, 40.0}, ED::Port3{20.0, 10.0}, ED::Port3{0.0, 0.0}, ED::Port3{60.0, 30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, true, false};
  EXPECT_EQ(s_c1, expected_s_c1);

  //std::vector<ED::PortValue> xs_d{
  //  ED::PortValue{ED::inport_outflow_request, 10.0},
  //  ED::PortValue{ED::inport_lossflow_request, 30.0}};
  auto s_d1 = ED::converter_external_transition(s_m, 10, xs_d);
  ED::ConverterState expected_s_d1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port3{40.0}, ED::Port3{10.0}, ED::Port3{30.0}, ED::Port3{0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, false, false};
  EXPECT_EQ(s_d1, expected_s_d1);

  //std::vector<ED::PortValue> xs_e{
  //  ED::PortValue{ED::inport_outflow_request, 10.0},
  //  ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  auto s_e1 = ED::converter_external_transition(s_m, 10, xs_e);
  ED::ConverterState expected_s_e1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port3{40.0, 40.0}, ED::Port3{10.0, 10.0}, ED::Port3{0.0}, ED::Port3{30.0, 30.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, true, false};
  EXPECT_EQ(s_e1, expected_s_e1);

  //std::vector<ED::PortValue> xs_f{
  //  ED::PortValue{ED::inport_lossflow_request, 30.0},
  //  ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  auto s_f1 = ED::converter_external_transition(s_m, 10, xs_f);
  ED::ConverterState expected_s_f1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port3{80.0, 40.0}, ED::Port3{20.0, 10.0}, ED::Port3{30.0, 30.0}, ED::Port3{30.0, 0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), false, true, true};
  EXPECT_EQ(s_f1, expected_s_f1);

  //std::vector<ED::PortValue> xs_g{
  //  ED::PortValue{ED::inport_outflow_request, 10.0},
  //  ED::PortValue{ED::inport_lossflow_request, 30.0},
  //  ED::PortValue{ED::inport_inflow_achieved, 40.0}};
  auto s_g1 = ED::converter_external_transition(s_m, 10, xs_g);
  ED::ConverterState expected_s_g1{
    // time, inflow_port, outflow_port, lossflow_port, wasteflow_port
    12, ED::Port3{40.0, 40.0}, ED::Port3{10.0, 10.0}, ED::Port3{30.0, 30.0}, ED::Port3{0.0},
    // std::unique_ptr<ConversionFun>, report_inflow_request, report_outflow_achieved, report_lossflow_achieved
    cf->clone(), true, true, true};
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
  auto d = ED::make_load_data(
      std::vector<ED::LoadItem>{
        ED::LoadItem{0, 100.0},
        ED::LoadItem{10, 10.0},
        ED::LoadItem{100, 10.0}, // should NOT cause a new event -- same load request.
        ED::LoadItem{200, 0.0}});
  auto s0 = ED::make_load_state();
  EXPECT_EQ(s0.current_index, -1);
  EXPECT_EQ(ED::load_current_time(s0), 0);
  EXPECT_EQ(ED::load_next_time(d, s0), 0);
  EXPECT_EQ(ED::load_current_request(s0), 0.0);
  EXPECT_EQ(ED::load_current_achieved(s0), 0.0);
  auto dt0 = ED::load_time_advance(d, s0);
  EXPECT_EQ(dt0, 0);
  auto ys0 = ED::load_output_function(d, s0);
  std::vector<ED::PortValue> expected_ys0{
    ED::PortValue{ED::outport_inflow_request, 100.0}};
  EXPECT_EQ(ys0.size(), expected_ys0.size());
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys0, expected_ys0, compare_ports));
  auto s1 = ED::load_internal_transition(d, s0);
  EXPECT_EQ(s1.current_index, 0);
  EXPECT_EQ(ED::load_current_time(s1), 0);
  EXPECT_EQ(ED::load_next_time(d, s1), 10);
  EXPECT_EQ(ED::load_current_request(s1), 100.0);
  EXPECT_EQ(ED::load_current_achieved(s1), 0.0);
  auto dt1 = ED::load_time_advance(d, s1);
  EXPECT_EQ(dt1, 10);
  auto ys1 = ED::load_output_function(d, s1);
  std::vector<ED::PortValue> expected_ys1{
    ED::PortValue{ED::outport_inflow_request, 10.0}};
  EXPECT_EQ(ys1.size(), expected_ys1.size());
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys1, expected_ys1, compare_ports));
  auto s2 = ED::load_internal_transition(d, s1);
  EXPECT_EQ(s2.current_index, 1);
  EXPECT_EQ(ED::load_current_time(s2), 10);
  EXPECT_EQ(ED::load_next_time(d, s2), 100);
  EXPECT_EQ(ED::load_current_request(s2), 10.0);
  EXPECT_EQ(ED::load_current_achieved(s2), 0.0);
  auto dt2 = ED::load_time_advance(d, s2);
  EXPECT_EQ(dt2, 90);
  std::vector<ED::PortValue> xs2{
    ED::PortValue{ED::inport_inflow_achieved, 5.0}};
  auto s3 = ED::load_external_transition(s2, 50, xs2);
  EXPECT_EQ(s3.current_index, 1);
  EXPECT_EQ(ED::load_current_time(s3), 60);
  EXPECT_EQ(ED::load_next_time(d, s3), 100);
  EXPECT_EQ(ED::load_current_request(s3), 10.0);
  EXPECT_EQ(ED::load_current_achieved(s3), 5.0);
  auto dt3 = ED::load_time_advance(d, s3);
  EXPECT_EQ(dt3, 40);
  std::vector<ED::PortValue> xs3{
    ED::PortValue{ED::inport_inflow_achieved, 10.0}};
  auto s4 = ED::load_external_transition(s3, 10, xs3);
  EXPECT_EQ(s4.current_index, 1);
  EXPECT_EQ(ED::load_current_time(s4), 70);
  EXPECT_EQ(ED::load_next_time(d, s4), 100);
  EXPECT_EQ(ED::load_current_request(s4), 10.0);
  EXPECT_EQ(ED::load_current_achieved(s4), 10.0);
  auto dt4 = ED::load_time_advance(d, s4);
  EXPECT_EQ(dt4, 30);
  auto ys4 = ED::load_output_function(d, s4);
  // we get no output because the requested load is the same as current -- no
  // change to propagate!
  std::vector<ED::PortValue> expected_ys4{};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys4, expected_ys4, compare_ports));
  auto s5 = ED::load_internal_transition(d, s4);
  EXPECT_EQ(s5.current_index, 2);
  EXPECT_EQ(ED::load_current_time(s5), 100);
  EXPECT_EQ(ED::load_next_time(d, s5), 200);
  EXPECT_EQ(ED::load_current_request(s5), 10.0);
  EXPECT_EQ(ED::load_current_achieved(s5), 10.0);
  auto dt5 = ED::load_time_advance(d, s5);
  EXPECT_EQ(dt5, 100);
  auto ys5 = ED::load_output_function(d, s5);
  std::vector<ED::PortValue> expected_ys5{
    ED::PortValue{ED::outport_inflow_request, 0.0}};
  EXPECT_EQ(ys5.size(), expected_ys5.size());
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys5, expected_ys5, compare_ports));
  auto s6 = ED::load_internal_transition(d, s5);
  EXPECT_EQ(s6.current_index, 3);
  EXPECT_EQ(ED::load_current_time(s6), 200);
  EXPECT_EQ(ED::load_next_time(d, s6), ED::infinity);
  EXPECT_EQ(ED::load_current_request(s6), 0.0);
  EXPECT_EQ(ED::load_current_achieved(s6), 0.0);
  EXPECT_EQ(s6.inflow_port.get_actual_achieved(), 10.0);
  auto dt6 = ED::load_time_advance(d, s6);
  EXPECT_EQ(dt6, ED::infinity);

  // test confluent update from state 
  std::vector<ED::PortValue> xs5{
    ED::PortValue{ED::inport_inflow_achieved, 8.0}};
  auto s6a = ED::load_confluent_transition(d, s5, xs5);
  EXPECT_EQ(s6a.current_index, 3);
  EXPECT_EQ(ED::load_current_time(s6a), 200);
  EXPECT_EQ(ED::load_next_time(d, s6a), ED::infinity);
  EXPECT_EQ(ED::load_current_request(s6a), 0.0);
  EXPECT_EQ(ED::load_current_achieved(s6a), 0.0);
  EXPECT_EQ(s6a.inflow_port.get_actual_achieved(), 8.0);

  ASSERT_THROW(
      ED::check_loads(std::vector<ED::LoadItem>{}),
      std::invalid_argument);
}

/*
// TODO: delete this test; too hard to follow and maintain
TEST(ErinDevs, Test_function_based_mux)
{
  namespace E = ERIN;
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  int num_inports{3};
  int num_outports{3};
  // by default, uses the distribute strategy
  auto s0 = ED::make_mux_state(num_inports, num_outports);
  auto dt0 = ED::mux_time_advance(s0);
  EXPECT_EQ(dt0, ED::infinity);
  EXPECT_EQ(ED::mux_current_time(s0), 0);
  std::vector<ED::PortValue> xs0{
    ED::PortValue{ED::inport_outflow_request + 0, 100.0}};
  auto s1 = ED::mux_external_transition(s0, 10, xs0);
  if (false) {
    std::cout << "@t = " << s1.time << ": xs="
              << ERIN::vec_to_string<ED::PortValue>(xs0) << "\n";
  }
  EXPECT_TRUE(
      ED::mux_should_report(s1.report_irs, s1.report_oas));
  EXPECT_EQ(ED::mux_current_time(s1), 10);
  auto dt1 = ED::mux_time_advance(s1);
  EXPECT_EQ(dt1, 0);
  auto ys1 = ED::mux_output_function(s1);
  std::vector<ED::PortValue> expected_ys1{
    ED::PortValue{ED::outport_inflow_request + 0, 100.0},
    ED::PortValue{ED::outport_outflow_achieved + 0, 100.0}
  };
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys1, expected_ys1, compare_ports))
    << "s0: " << s0 << "\n"
    << "xs0: " << ERIN::vec_to_string<ED::PortValue>(xs0) << "\n"
    << "s1: " << s1 << "\n"
    << "ys1: " << ERIN::vec_to_string<ED::PortValue>(ys1) << "\n"
    << "expected_ys1: " << ERIN::vec_to_string<ED::PortValue>(expected_ys1) << "\n";
  auto s2 = ED::mux_internal_transition(s1);
  EXPECT_EQ(ED::mux_current_time(s2), 10);
  auto dt2 = ED::mux_time_advance(s2);
  EXPECT_EQ(dt2, ED::infinity);
  std::vector<ED::PortValue> xs2{
    ED::PortValue{ED::inport_inflow_achieved + 0, 50.0}};
  auto s3 = ED::mux_external_transition(s2, 2, xs2);
  if (false) {
    std::cout << "@t = " << s3.time << ": xs="
              << ERIN::vec_to_string<ED::PortValue>(xs2) << "\n";
  }
  EXPECT_EQ(ED::mux_current_time(s3), 12);
  auto dt3 = ED::mux_time_advance(s3);
  EXPECT_EQ(dt3, 0);
  auto ys3 = ED::mux_output_function(s3);
  std::vector<ED::PortValue> expected_ys3{
    ED::PortValue{ED::outport_inflow_request + 1, 50.0}};
  EXPECT_EQ(ys3.size(), expected_ys3.size());
  if (false) {
    std::cout << "ys3 = " << ERIN::vec_to_string<ED::PortValue>(ys3) << "\n";
  }
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys3, expected_ys3, compare_ports));
  auto s4 = ED::mux_internal_transition(s3);
  auto dt4 = ED::mux_time_advance(s4);
  EXPECT_EQ(dt4, ED::infinity);
  std::vector<ED::PortValue> xs4{
    ED::PortValue{ED::inport_inflow_achieved + 1, 20.0}};
  auto s5 = ED::mux_external_transition(s4, 0, xs4);
  if (false) {
    std::cout << "@t = " << s5.time << ": xs="
              << ERIN::vec_to_string<ED::PortValue>(xs4) << "\n";
  }
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
  if (false) {
    std::cout << "s6.inflow_ports = " << ERIN::vec_to_string<ED::Port3>(s6.inflow_ports) << "\n";
    std::cout << "s6.outflow_ports = " << ERIN::vec_to_string<ED::Port3>(s6.outflow_ports) << "\n";
  }
  auto dt6 = ED::mux_time_advance(s6);
  EXPECT_EQ(dt6, 0);
  auto ys6 = ED::mux_output_function(s6);
  std::vector<ED::PortValue> expected_ys6{
    ED::PortValue{ED::outport_outflow_achieved + 0, 90.0}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys6, expected_ys6, compare_ports));
  auto s7 = ED::mux_internal_transition(s6);
  if (false) {
    std::cout << "s7.inflow_ports = " << ERIN::vec_to_string<ED::Port3>(s7.inflow_ports) << "\n";
    std::cout << "s7.outflow_ports = " << ERIN::vec_to_string<ED::Port3>(s7.outflow_ports) << "\n";
  }
  auto dt7 = ED::mux_time_advance(s7);
  EXPECT_EQ(dt7, ED::infinity);
  std::vector<ED::PortValue> xs7{
    ED::PortValue{ED::inport_outflow_request + 1, 100.0}};
  auto s8 = ED::mux_external_transition(s7, 10, xs7);
  if (false) {
    std::cout << "@t = " << s8.time << ": xs="
              << ERIN::vec_to_string<ED::PortValue>(xs7) << "\n";
  }
  EXPECT_EQ(ED::mux_current_time(s8), 22);
  auto dt8 = ED::mux_time_advance(s8);
  EXPECT_EQ(dt8, 0);
  auto ys8 = ED::mux_output_function(s8);
  std::vector<ED::PortValue> expected_ys8{
    ED::PortValue{ED::outport_inflow_request + 0, 200.0},
    ED::PortValue{ED::outport_inflow_request + 1, 150.0},
    ED::PortValue{ED::outport_inflow_request + 2, 130.0},
    ED::PortValue{ED::outport_outflow_achieved + 0, 45.0},
    ED::PortValue{ED::outport_outflow_achieved + 1, 45.0},
  };
  if (false) {
    std::cout << "expected_ys8 = " << ERIN::vec_to_string<ED::PortValue>(expected_ys8) << "\n";
    std::cout << "ys8          = " << ERIN::vec_to_string<ED::PortValue>(ys8) << "\n";
    std::cout << "s8.inflow_ports = " << ERIN::vec_to_string<ED::Port3>(s8.inflow_ports) << "\n";
    std::cout << "s8.outflow_ports = " << ERIN::vec_to_string<ED::Port3>(s8.outflow_ports) << "\n";
  }
  ASSERT_EQ(ys8.size(), expected_ys8.size());
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys8, expected_ys8, compare_ports));
  auto s9 = ED::mux_internal_transition(s8);
  std::vector<ED::PortValue> xs9{
    ED::PortValue{ED::inport_inflow_achieved + 0, 100.0}};
  auto s10 = ED::mux_external_transition(s9, 0, xs9);
  if (false) {
    std::cout << "@t = " << s10.time << ": xs="
              << ERIN::vec_to_string<ED::PortValue>(xs9) << "\n";
  }
  EXPECT_EQ(ED::mux_current_time(s10), 22);
  auto dt10 = ED::mux_time_advance(s10);
  EXPECT_EQ(dt10, 0);
  auto ys10 = ED::mux_output_function(s10);
  std::vector<ED::PortValue> expected_ys10{
    ED::PortValue{ED::outport_inflow_request + 1, 100.0},
    ED::PortValue{ED::outport_inflow_request + 2, 80.0},
    ED::PortValue{ED::outport_outflow_achieved + 0, 70.0},
    ED::PortValue{ED::outport_outflow_achieved + 1, 70.0},
  };
  if (false) {
    std::cout << "s9.inflow_ports   = " << ERIN::vec_to_string<ED::Port3>(s8.inflow_ports) << "\n";
    std::cout << "s8.outflow_ports  = " << ERIN::vec_to_string<ED::Port3>(s8.outflow_ports) << "\n";
    std::cout << "s9.inflow_ports   = " << ERIN::vec_to_string<ED::Port3>(s9.inflow_ports) << "\n";
    std::cout << "s9.outflow_ports  = " << ERIN::vec_to_string<ED::Port3>(s9.outflow_ports) << "\n";
    std::cout << "expected_ys10     = " << ERIN::vec_to_string<ED::PortValue>(expected_ys10) << "\n";
    std::cout << "ys10              = " << ERIN::vec_to_string<ED::PortValue>(ys10) << "\n";
    std::cout << "s10.time          = " << s10.time << "\n";
    std::cout << "s10.inflow_ports  = " << ERIN::vec_to_string<ED::Port3>(s10.inflow_ports) << "\n";
    std::cout << "s10.outflow_ports = " << ERIN::vec_to_string<ED::Port3>(s10.outflow_ports) << "\n";
  }
  ASSERT_EQ(ys10.size(), expected_ys10.size());
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys10, expected_ys10, compare_ports));
}
*/

TEST(ErinDevs, Test_function_based_storage_element)
{
  namespace E = ERIN;
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  ED::FlowValueType capacity{100.0};
  ED::FlowValueType max_charge_rate{1.0};
  double initial_soc{0.5};
  ASSERT_THROW(ED::storage_make_data(-1.0, 1.0), std::invalid_argument);
  ASSERT_THROW(ED::storage_make_data(0.0, 1.0), std::invalid_argument);
  ASSERT_THROW(ED::storage_make_data(2.0, 0.0), std::invalid_argument);
  ASSERT_THROW(ED::storage_make_data(2.0, -1.0), std::invalid_argument);
  auto data = ED::storage_make_data(capacity, max_charge_rate);
  ASSERT_THROW(ED::storage_make_state(data, -1.0), std::invalid_argument);
  ASSERT_THROW(ED::storage_make_state(data, 1.1), std::invalid_argument);
  auto s0 = ED::storage_make_state(data, initial_soc);
  auto dt0 = ED::storage_time_advance(data, s0);
  EXPECT_EQ(dt0, 0);
  EXPECT_EQ(ED::storage_current_time(s0), 0);
  EXPECT_EQ(ED::storage_current_soc(s0), initial_soc);
  auto ys0 = ED::storage_output_function(data, s0);
  std::vector<ED::PortValue> expected_ys0{
    ED::PortValue{ED::outport_inflow_request, max_charge_rate}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys0, expected_ys0, compare_ports));
  auto s1 = ED::storage_internal_transition(data, s0);
  if (false) {
    std::cout << "s0 = " << s0 << "\n";
    std::cout << "s1 = " << s1 << "\n";
  }
  auto dt1 = ED::storage_time_advance(data, s1);
  EXPECT_EQ(dt1, 50);
  EXPECT_EQ(ED::storage_current_time(s1), 0);
  EXPECT_EQ(ED::storage_current_soc(s1), initial_soc);
  auto ys1 = ED::storage_output_function(data, s1);
  std::vector<ED::PortValue> expected_ys1{
    ED::PortValue{ED::outport_inflow_request, 0.0}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys1, expected_ys1, compare_ports));
  if (false) {
    std::cout << "ys1 = " << E::vec_to_string<ED::PortValue>(ys1) << "\n";
    std::cout << "expected_ys1 = "
              << E::vec_to_string<ED::PortValue>(expected_ys1) << "\n";
  }
  auto s2 = ED::storage_internal_transition(data, s1);
  auto dt2 = ED::storage_time_advance(data, s2);
  EXPECT_EQ(dt2, ED::infinity);
  EXPECT_EQ(ED::storage_current_time(s2), 50);
  EXPECT_EQ(ED::storage_current_soc(s2), 1.0);
  if (false) {
    std::cout << "s1 = " << s1 << "\n";
    std::cout << "s2 = " << s2 << "\n";
    std::cout << "data = " << data << "\n";
  }
  std::vector<ED::PortValue> xs2{
    ED::PortValue{ED::inport_outflow_request, max_charge_rate}};
  auto s3 = ED::storage_external_transition(data, s2, 10, xs2);
  auto dt3 = ED::storage_time_advance(data, s3);
  if (false) {
    std::cout << "s2 = " << s2 << "\n";
    std::cout << "s3 = " << s3 << "\n";
  }
  EXPECT_EQ(dt3, 0);
  EXPECT_EQ(ED::storage_current_time(s3), 60);
  EXPECT_EQ(ED::storage_current_soc(s3), 1.0);
  auto ys3 = ED::storage_output_function(data, s3);
  std::vector<ED::PortValue> expected_ys3{
    ED::PortValue{ED::outport_inflow_request, max_charge_rate}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys3, expected_ys3, compare_ports));
  if (false) {
    std::cout << "ys3 = " << E::vec_to_string<ED::PortValue>(ys3) << "\n";
    std::cout << "expected_ys3 = "
              << E::vec_to_string<ED::PortValue>(expected_ys3) << "\n";
  }
  auto s4 = ED::storage_internal_transition(data, s3);
  auto dt4 = ED::storage_time_advance(data, s4);
  EXPECT_EQ(dt4, ED::infinity);
  EXPECT_EQ(ED::storage_current_time(s3), 60);
  EXPECT_EQ(ED::storage_current_soc(s3), 1.0);
  if (false) {
    std::cout << "s3 = " << s3 << "\n";
    std::cout << "s4 = " << s4 << "\n";
  }
  std::vector<ED::PortValue> xs4{
    ED::PortValue{ED::inport_outflow_request, 2 * max_charge_rate}};
  auto s5 = ED::storage_external_transition(data, s4, 20, xs4);
  auto dt5 = ED::storage_time_advance(data, s5);
  EXPECT_EQ(dt5, 100);
  EXPECT_EQ(ED::storage_current_time(s5), 80);
  EXPECT_EQ(ED::storage_current_soc(s5), 1.0);
  auto ys5 = ED::storage_output_function(data, s5);
  std::vector<ED::PortValue> expected_ys5{
    ED::PortValue{ED::outport_outflow_achieved, max_charge_rate}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys5, expected_ys5, compare_ports));
  if (false) {
    std::cout << "s4           = " << s4 << "\n";
    std::cout << "s5           = " << s5 << "\n";
    std::cout << "ys5          = " << E::vec_to_string<ED::PortValue>(ys5) << "\n";
    std::cout << "expected_ys5 = " << E::vec_to_string<ED::PortValue>(expected_ys5) << "\n";
  }
  auto s6 = ED::storage_internal_transition(data, s5);
  auto dt6 = ED::storage_time_advance(data, s6);
  EXPECT_EQ(dt6, ED::infinity);
  EXPECT_EQ(ED::storage_current_time(s6), 180);
  EXPECT_EQ(ED::storage_current_soc(s6), 0.0);
  std::vector<ED::PortValue> xs6{
    ED::PortValue{ED::inport_inflow_achieved, 0.5 * max_charge_rate}};
  auto s7 = ED::storage_external_transition(data, s6, 15, xs6);
  auto dt7 = ED::storage_time_advance(data, s7);
  EXPECT_EQ(dt7, 0);
  EXPECT_EQ(ED::storage_current_time(s7), 195);
  EXPECT_EQ(ED::storage_current_soc(s7), 0.0);
  auto ys7 = ED::storage_output_function(data, s7);
  std::vector<ED::PortValue> expected_ys7{
    ED::PortValue{ED::outport_outflow_achieved, 0.5 * max_charge_rate}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys7, expected_ys7, compare_ports));
  auto s8 = ED::storage_internal_transition(data, s7);
  auto dt8 = ED::storage_time_advance(data, s8);
  EXPECT_EQ(dt8, ED::infinity);
  EXPECT_EQ(ED::storage_current_time(s8), 195);
  EXPECT_EQ(ED::storage_current_soc(s8), 0.0);
  std::vector<ED::PortValue> xs8{
    ED::PortValue{ED::inport_outflow_request, 0.0}};
  auto s9 = ED::storage_external_transition(data, s8, 5, xs8);
  auto dt9 = ED::storage_time_advance(data, s9);
  EXPECT_EQ(dt9, 200);
  EXPECT_EQ(ED::storage_current_time(s9), 200);
  EXPECT_EQ(ED::storage_current_soc(s9), 0.0);
  if (false) {
    std::cout << "s6 = " << s6 << "\n";
    std::cout << "s7 = " << s7 << "\n";
    std::cout << "s8 = " << s8 << "\n";
    std::cout << "s9 = " << s9 << "\n";
  }
  auto ys9 = ED::storage_output_function(data, s9);
  std::vector<ED::PortValue> expected_ys9{
    ED::PortValue{ED::outport_inflow_request, 0.0}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys9, expected_ys9, compare_ports));
  std::vector<ED::PortValue> xs9{
    ED::PortValue{ED::inport_outflow_request, 2.5 * max_charge_rate}};
  auto s10 = ED::storage_confluent_transition(data, s9, xs9);
  auto dt10 = ED::storage_time_advance(data, s10);
  EXPECT_EQ(dt10, 0);
  EXPECT_EQ(ED::storage_current_time(s10), 400);
  EXPECT_EQ(ED::storage_current_soc(s10), 1.0);
  if (false) {
    std::cout << "s9  = " << s9 << "\n";
    std::cout << "s10 = " << s10 << "\n";
  }
  auto ys10 = ED::storage_output_function(data, s10);
  std::vector<ED::PortValue> expected_ys10{
    ED::PortValue{ED::outport_inflow_request, max_charge_rate}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys10, expected_ys10, compare_ports));
  auto s11 = ED::storage_internal_transition(data, s10);
  auto dt11 = ED::storage_time_advance(data, s11);
  EXPECT_EQ(dt11, 66);
  EXPECT_EQ(ED::storage_current_time(s11), 400);
  EXPECT_EQ(ED::storage_current_soc(s11), 1.0);
  auto ys11 = ED::storage_output_function(data, s11);
  std::vector<ED::PortValue> expected_ys11{
    ED::PortValue{ED::outport_outflow_achieved, 1.0}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys10, expected_ys10, compare_ports));
  auto s12 = ED::storage_internal_transition(data, s11);
  auto dt12 = ED::storage_time_advance(data, s12);
  EXPECT_EQ(dt12, 1);
  EXPECT_EQ(ED::storage_current_time(s12), 466);
  EXPECT_TRUE(std::abs(ED::storage_current_soc(s12) - 0.01) < E::flow_value_tolerance);
  auto ys12 = ED::storage_output_function(data, s12);
  std::vector<ED::PortValue> expected_ys12{
    ED::PortValue{ED::outport_outflow_achieved, 1.0}};
  EXPECT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys12, expected_ys12, compare_ports));
  if (false) {
    std::cout << "ys12          = " << E::vec_to_string<ED::PortValue>(ys12) << "\n";
    std::cout << "expected_ys12 = " << E::vec_to_string<ED::PortValue>(expected_ys12) << "\n";
  }
  auto s13 = ED::storage_internal_transition(data, s12);
  auto dt13 = ED::storage_time_advance(data, s13);
  EXPECT_EQ(dt13, ED::infinity);
  EXPECT_EQ(ED::storage_current_time(s13), 467);
  EXPECT_EQ(ED::storage_current_soc(s13), 0.0);
  if (false) {
    std::cout << "s12 = " << s12 << "\n";
    std::cout << "s13 = " << s13 << "\n";
  }
}

TEST(ErinBasicsTest, Test_standalone_sink_with_port_logging)
{
  namespace E = ERIN;
  std::string st{"electrical"};
  E::RealTimeType t_max{3};
  std::string id{"load"};
  auto sink = new E::Sink(
      id,
      E::ComponentType::Load,
      st,
      {E::LoadItem{0,100},
       E::LoadItem{1,10},
       E::LoadItem{2,0},
       E::LoadItem{t_max,0}});
  std::shared_ptr<E::FlowWriter> fw =
    std::make_shared<E::DefaultFlowWriter>();
  sink->set_recording_on();
  sink->set_flow_writer(fw);
  adevs::Simulator<E::PortValue, E::Time> sim{};
  sim.add(sink);
  while (sim.next_event_time() < E::inf) {
    sim.exec_next_event();
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  std::vector<E::RealTimeType> expected_times = {0, 1, 2, 3};
  std::vector<E::FlowValueType> expected_loads_achieved = {0, 0, 0, 0};
  std::vector<E::FlowValueType> expected_loads_requested = {100, 10, 0, 0};
  bool use_requested{true};
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_times, expected_loads_requested, id, use_requested))
    << "key: " << id << "\n";
  use_requested = false;
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_times, expected_loads_achieved, id, use_requested))
    << "key: " << id << "\n";
}

/*
TEST(ErinBasicsTest, Test_sink_and_flow_limits_with_port_logging)
{
  namespace E = ERIN;
  std::string st{"electrical"};
  E::RealTimeType t_max{3};
  std::string sink_id{"sink"};
  auto sink = new E::Sink(
      sink_id,
      E::ComponentType::Load,
      st,
      {E::LoadItem{0,100},
       E::LoadItem{1,10},
       E::LoadItem{2,0},
       E::LoadItem{t_max,0}});
  std::string limits_id{"limits"};
  E::FlowValueType lower_limit{0.0};
  E::FlowValueType upper_limit{50.0};
  auto limits = new E::FlowLimits(
      limits_id,
      E::ComponentType::Source,
      st,
      lower_limit,
      upper_limit);
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
  sink->set_flow_writer(fw);
  sink->set_recording_on();
  limits->set_flow_writer(fw);
  limits->set_recording_on();
  adevs::Digraph<E::FlowValueType, E::Time> network;
  network.couple(
      sink, E::Sink::outport_inflow_request,
      limits, E::FlowLimits::inport_outflow_request);
  network.couple(
      limits, E::FlowLimits::outport_outflow_achieved,
      sink, E::Sink::inport_inflow_achieved);
  adevs::Simulator<E::PortValue, E::Time> sim{};
  network.add(&sim);
  while (sim.next_event_time() < E::inf) {
    sim.exec_next_event();
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  std::vector<E::RealTimeType> expected_times = {0, 1, 2, 3};
  std::vector<E::FlowValueType> expected_loads = {50, 10, 0, 0};
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_loads, sink_id)
      ) << "key: " << sink_id;
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_loads, limits_id)
      ) << "key: " << limits_id;
}
*/

TEST(ErinBasicsTest, Test_sink_and_converter_with_port_logging)
{
  namespace E = ERIN;
  std::string st_out{"electrical"};
  std::string st_in{"natural_gas"};
  E::RealTimeType t_max{3};
  std::string sink_id{"sink"};
  auto sink = new E::Sink(
      sink_id,
      E::ComponentType::Load,
      st_out,
      {E::LoadItem{0,100},
       E::LoadItem{1,10},
       E::LoadItem{2,0},
       E::LoadItem{t_max,0}});
  std::string converter_id{"converter"};
  E::FlowValueType constant_efficiency{0.5};
  std::function<E::FlowValueType(E::FlowValueType)> outflow_given_inflow =
    [constant_efficiency](E::FlowValueType inflow) -> E::FlowValueType {
      return inflow * constant_efficiency;
    };
  std::function<E::FlowValueType(E::FlowValueType)> inflow_given_outflow =
    [constant_efficiency](E::FlowValueType outflow) -> E::FlowValueType {
      return outflow / constant_efficiency;
    };
  auto converter = new E::Converter(
      converter_id,
      E::ComponentType::Converter,
      st_in,
      st_out,
      outflow_given_inflow,
      inflow_given_outflow);
  std::string src_id{"natural_gas_tank"};
  auto src = new E::Source(
      src_id,
      E::ComponentType::Source,
      st_in);
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
  sink->set_flow_writer(fw);
  sink->set_recording_on();
  converter->set_flow_writer(fw);
  converter->set_recording_on();
  adevs::Digraph<E::FlowValueType, E::Time> network;
  network.couple(
      sink, E::Sink::outport_inflow_request,
      converter, E::Converter::inport_outflow_request);
  network.couple(
      converter, E::Source::outport_inflow_request,
      src, E::Source::inport_outflow_request);
  network.couple(
      src, E::Source::outport_outflow_achieved,
      converter, E::Converter::inport_inflow_achieved);
  network.couple(
      converter, E::Converter::outport_outflow_achieved,
      sink, E::Sink::inport_inflow_achieved);
  adevs::Simulator<E::PortValue, E::Time> sim{};
  network.add(&sim);
  while (sim.next_event_time() < E::inf) {
    sim.exec_next_event();
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  // Sink recorded data
  std::vector<E::RealTimeType> expected_times = {0, 1, 2, 3};
  std::vector<E::FlowValueType> expected_loads = {100.0, 10.0, 0.0, 0.0};
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_loads, sink_id)
      ) << "key: " << sink_id;
  // Converter recorded data -- outflow
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_times, expected_loads,
        converter_id + "-outflow")
      ) << "key: " << converter_id;
  // ... -- inflow
  std::vector<E::FlowValueType> expected_loads_inflow = {200.0, 20.0, 0.0, 0.0};
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_times, expected_loads_inflow,
        converter_id + "-inflow")
      ) << "key: " << converter_id << "-inflow";
  // ... -- wasteflow
  std::vector<E::FlowValueType> expected_loads_wasteflow = {100.0, 10.0, 0.0, 0.0};
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_times, expected_loads_wasteflow,
        converter_id + "-wasteflow")
      ) << "key: " << converter_id << "-wasteflow";
  // ... -- lossflow
  std::vector<E::FlowValueType> expected_loads_lossflow = {0.0, 0.0, 0.0, 0.0};
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_times, expected_loads_lossflow,
        converter_id + "-lossflow")
      ) << "key: " << converter_id << "-lossflow";
}

/*
//TODO: re-enable this after mux has been translated to Port3
TEST(ErinBasicsTest, Test_sink_and_mux_and_limits_with_port_logging)
{
  namespace E = ERIN;
  std::string st{"electrical"};
  E::RealTimeType t_max{3};
  std::string sink0_id{"sink0"};
  std::string sink1_id{"sink1"};
  auto sink0 = new E::Sink(
      sink0_id,
      E::ComponentType::Load,
      st,
      {E::LoadItem{0,100},
       E::LoadItem{1,10},
       E::LoadItem{2,0},
       E::LoadItem{t_max,0}});
  auto sink1 = new E::Sink(
      sink1_id,
      E::ComponentType::Load,
      st,
      {E::LoadItem{0,100},
       E::LoadItem{1,10},
       E::LoadItem{2,0},
       E::LoadItem{t_max,0}});
  std::string mux_id{"mux"};
  int num_inflows{2};
  int num_outflows{2};
  auto mux = new E::Mux(
      mux_id,
      E::ComponentType::Muxer,
      st,
      num_inflows,
      num_outflows,
      E::MuxerDispatchStrategy::InOrder);
  std::string limit0_id{"limit0"};
  E::FlowValueType lower_limit0{0.0};
  E::FlowValueType upper_limit0{50.0};
  auto limit0 = new E::FlowLimits(
      limit0_id,
      E::ComponentType::Source,
      st,
      lower_limit0,
      upper_limit0);
  std::string limit1_id{"limit1"};
  E::FlowValueType lower_limit1{0.0};
  E::FlowValueType upper_limit1{20.0};
  auto limit1 = new E::FlowLimits(
      limit1_id,
      E::ComponentType::Source,
      st,
      lower_limit1,
      upper_limit1);
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
  sink0->set_flow_writer(fw);
  sink0->set_recording_on();
  sink1->set_flow_writer(fw);
  sink1->set_recording_on();
  mux->set_flow_writer(fw);
  mux->set_recording_on();
  limit0->set_flow_writer(fw);
  limit0->set_recording_on();
  limit1->set_flow_writer(fw);
  limit1->set_recording_on();
  adevs::Digraph<E::FlowValueType, E::Time> network;
  network.couple(
      sink0, E::Sink::outport_inflow_request,
      mux, E::Mux::inport_outflow_request);
  network.couple(
      sink1, E::Sink::outport_inflow_request,
      mux, E::Mux::inport_outflow_request + 1);
  network.couple(
      mux, E::Mux::outport_inflow_request,
      limit0, E::FlowLimits::inport_outflow_request);
  network.couple(
      mux, E::Mux::outport_inflow_request + 1,
      limit1, E::FlowLimits::inport_outflow_request);
  network.couple(
      limit0, E::FlowLimits::outport_outflow_achieved,
      mux, E::Mux::inport_inflow_achieved);
  network.couple(
      limit1, E::FlowLimits::outport_outflow_achieved,
      mux, E::Mux::inport_inflow_achieved + 1);
  network.couple(
      mux, E::Mux::outport_outflow_achieved,
      sink0, E::Sink::inport_inflow_achieved);
  network.couple(
      mux, E::Mux::outport_outflow_achieved + 1,
      sink1, E::Sink::inport_inflow_achieved);
  adevs::Simulator<E::PortValue, E::Time> sim{};
  network.add(&sim);
  while (sim.next_event_time() < E::inf)
    sim.exec_next_event();
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  // Sink0 recorded data
  std::vector<E::RealTimeType> expected_times = {0, 1, 2, 3};
  std::vector<E::FlowValueType> expected_loads0 = {70.0, 10.0, 0.0, 0.0};
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_loads0, sink0_id));
  // Sink1 recorded data
  std::vector<E::FlowValueType> expected_loads1 = {0.0, 10.0, 0.0, 0.0};
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_loads1, sink1_id));
  // Mux recorded data -- outflow0
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_loads0, mux_id + "-outflow(0)"));
  // Mux recorded data -- outflow1
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_loads1, mux_id + "-outflow(1)"));
  // Mux recorded data -- inflow0
  std::vector<E::FlowValueType> expected_inflows0 = {50.0, 20.0, 0.0, 0.0};
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_inflows0, mux_id + "-inflow(0)"));
  // Mux recorded data -- inflow1
  std::vector<E::FlowValueType> expected_inflows1 = {20.0, 0.0, 0.0, 0.0};
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_inflows1, mux_id + "-inflow(1)"));
  // FlowLimits0
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_times, expected_inflows0, limit0_id));
  // FlowLimits1
  ASSERT_TRUE(
      check_times_and_loads(
        results, expected_times, expected_inflows1, limit1_id));
}
*/

/*
TEST(ErinBasicsTest, Test_example_8)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"hours\"\n"
    "max_time = 10\n"
    "[loads.building_electrical]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0,0.0]]\n"
    "[components.electric_source]\n"
    "type = \"source\"\n"
    "max_outflow = 5.0\n"
    "outflow = \"electricity\"\n"
    "[components.electric_battery]\n"
    "type = \"store\"\n"
    "outflow = \"electricity\"\n"
    "inflow = \"electricity\"\n"
    "max_inflow = 5.0\n"
    "capacity_unit = \"kWh\"\n"
    "capacity = 20.0\n"
    "[components.electric_load]\n"
    "type = \"load\"\n"
    "inflow = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"building_electrical\"\n"
    "[networks.normal_operations]\n"
    "connections = [\n"
    "  [\"electric_source:OUT(0)\", \"electric_battery:IN(0)\", \"electricity\"],\n"
    "  [\"electric_battery:OUT(0)\", \"electric_load:IN(0)\", \"electricity\"],\n"
    "]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"hours\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n";
  auto m = E::make_main_from_string(input);
  const auto& si = m.get_sim_info();
  EXPECT_EQ(si.get_time_units(), E::TimeUnits::Hours);
  EXPECT_EQ(si.get_max_time(), 10);
  EXPECT_EQ(si.get_max_time_in_seconds(), 36000);
  EXPECT_EQ(m.max_time_for_scenario("blue_sky"), 36000);
  const auto& comps = m.get_components();
  EXPECT_EQ(comps.size(), 3);
  const auto& nws = m.get_networks();
  EXPECT_EQ(nws.size(), 1);
  auto results = m.run_all();
  EXPECT_TRUE(results.get_is_good());
  auto srs = results.get_results();
  EXPECT_EQ(srs.size(), 1);
  EXPECT_EQ(results.number_of_scenarios(), 1);
  const auto& scenario_ids = results.get_scenario_ids();
  EXPECT_EQ(scenario_ids.size(), 1);
  EXPECT_EQ(scenario_ids[0], "blue_sky");
  const auto& blue_sky_sr = srs["blue_sky"];
  EXPECT_EQ(blue_sky_sr.size(), 1);
  const auto& bsr = blue_sky_sr[0];
  EXPECT_TRUE(bsr.get_is_good());
  const auto& blue_sky_data = bsr.get_results();
  if constexpr (false) {
    for (const auto& pair : blue_sky_data) {
      std::cout << "item: " << pair.first << "\n";
      for (const auto& x : pair.second) {
        std::cout << "- " << x << "\n";
      }
    }
  }
  const auto& d0_load = blue_sky_data.at("electric_load")[0];
  const E::Datum d0_load_expected{0, 10.0, 10.0};
  EXPECT_EQ(d0_load, d0_load_expected);
  const auto& d1_load = blue_sky_data.at("electric_load")[1];
  const E::Datum d1_load_expected{14400, 10.0, 5.0};
  EXPECT_EQ(d1_load, d1_load_expected);
  const auto& d2_load = blue_sky_data.at("electric_load")[2];
  const E::Datum d2_load_expected{36000, 0.0, 0.0};
  EXPECT_EQ(d2_load, d2_load_expected);

  const auto& d0_battery = blue_sky_data.at("electric_battery-inflow")[0];
  const E::Datum d0_battery_expected{0, 5.0, 5.0};
  EXPECT_EQ(d0_battery, d0_battery_expected);
  const auto& d1_battery = blue_sky_data.at("electric_battery-inflow")[1];
  const E::Datum d1_battery_expected{14400, 5.0, 5.0};
  EXPECT_EQ(d1_battery, d1_battery_expected);
  const auto& d2_battery = blue_sky_data.at("electric_battery-inflow")[2];
  const E::Datum d2_battery_expected{36000, 0.0, 0.0};
  EXPECT_EQ(d2_battery, d2_battery_expected);

  const auto& d0_battery_out = blue_sky_data.at("electric_battery-outflow")[0];
  const E::Datum d0_battery_out_expected{0, 10.0, 10.0};
  EXPECT_EQ(d0_battery_out, d0_battery_out_expected);
  const auto& d1_battery_out = blue_sky_data.at("electric_battery-outflow")[1];
  const E::Datum d1_battery_out_expected{14400, 10.0, 5.0};
  EXPECT_EQ(d1_battery_out, d1_battery_out_expected);
  const auto& d2_battery_out = blue_sky_data.at("electric_battery-outflow")[2];
  const E::Datum d2_battery_out_expected{36000, 0.0, 0.0};
  EXPECT_EQ(d2_battery_out, d2_battery_out_expected);

  const auto& d0_source = blue_sky_data.at("electric_source")[0];
  const E::Datum d0_source_expected{0, 5.0, 5.0};
  EXPECT_EQ(d0_source, d0_source_expected);
  const auto& d1_source = blue_sky_data.at("electric_source")[1];
  const E::Datum d1_source_expected{14400, 5.0, 5.0};
  EXPECT_EQ(d1_source, d1_source_expected);
  const auto& d2_source = blue_sky_data.at("electric_source")[2];
  const E::Datum d2_source_expected{36000, 0.0, 0.0};
  EXPECT_EQ(d2_source, d2_source_expected);
}
*/

TEST(ErinBasicsTest, Test_that_we_can_create_an_energy_balance)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"hours\"\n"
    "max_time = 10\n"
    "[loads.LP1]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,10.0],[10.0,0.0]]\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "outflow = \"natural_gas\"\n"
    "[components.C]\n"
    "type = \"converter\"\n"
    "inflow = \"natural_gas\"\n"
    "outflow = \"electricity\"\n"
    "lossflow = \"waste_heat\"\n"
    "constant_efficiency = 0.5\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "inflow = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"LP1\"\n"
    "[networks.normal_operations]\n"
    "connections = [\n"
    "  [\"S:OUT(0)\", \"C:IN(0)\", \"natural_gas\"],\n"
    "  [\"C:OUT(0)\", \"L:IN(0)\", \"electricity\"],\n"
    "]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"hours\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n";
  auto m = E::make_main_from_string(input);
  auto results = m.run_all();
  auto stats = results.to_stats_csv();
  // Updated Stats CSV Output Header:
  // scenario id,number of occurrences,total time in scenario (hours),
  // +source component id,+source port,+receiving component id,+receiving port,-component id,type,+role,stream,energy availability,max downtime (hours),load not served (kJ)
  // [,<stream> energy used (kJ)]+
  // role is one of source, load, circulatory, and waste
  std::string expected{
    "scenario id,number of occurrences,total time in scenario (hours),component id,type,stream,energy availability,max downtime (hours),load not served (kJ),"
    "electricity energy used (kJ),natural_gas energy used (kJ),waste_heat energy used (kJ)\n"
    "blue_sky,1,10,C-inflow,converter,natural_gas,1,0,0,0.0,720000,0.0\n"
    "blue_sky,1,10,C-lossflow,converter,waste_heat,1,0,0,0.0,0.0,0\n"
    "blue_sky,1,10,C-outflow,converter,electricity,1,0,0,360000,0.0,0.0\n"
    "blue_sky,1,10,C-wasteflow,converter,waste_heat,1,0,0,0.0,0.0,360000\n"
    "blue_sky,1,10,L,load,electricity,1,0,0,360000,0.0,0.0\n"
    "blue_sky,1,10,S,source,natural_gas,1,0,0,0.0,720000,0.0\n"
    "blue_sky,1,10,TOTAL (source),,,,,,0.0,720000,0.0\n"
    "blue_sky,1,10,TOTAL (load),,,,,,360000,0.0,0.0\n"
    "blue_sky,1,10,TOTAL (storage),,,,,,0.0,0.0,0.0\n"
    "blue_sky,1,10,TOTAL (waste),,,,,,0.0,0.0,360000\n"
    "blue_sky,1,10,ENERGY BALANCE (source-(load+storage+waste)),0,,,,,,,\n"
  };
  EXPECT_EQ(stats, expected);
}

TEST(ErinBasicsTest, Test_port_role_to_and_from_string_roundtrip)
{
  namespace E = ERIN;
  std::vector<E::PortRole> roles{
    E::PortRole::Inflow,
    E::PortRole::LoadInflow,
    E::PortRole::WasteInflow,
    E::PortRole::Outflow,
    E::PortRole::SourceOutflow,
  };
  for (const auto& role : roles) {
    auto role_tag = E::port_role_to_tag(role);
    auto role_2 = E::tag_to_port_role(role_tag);
    EXPECT_EQ(role_2, role);
  }
}

/*
TEST(ErinBasicsTest, Test_energy_balance_on_devs_mux)
{
  namespace ED = erin::devs;
  std::vector<double> achieved_for_input_0{1.0,0.5,0.1, 0.0};
  std::vector<double> achieved_for_input_1{1.0,0.5,0.1, 0.0};
  std::vector<double> request_for_output_0{10.0,100.0,5.0};
  std::vector<double> request_for_output_1{10.0,100.0,5.0};
  ED::RealTimeType dt{0};
  int num_checks{0};
  for (const auto ach_in_0 : achieved_for_input_0) {
    for (const auto ach_in_1 : achieved_for_input_1) {
      for (const auto req_out_0 : request_for_output_0) {
        for (const auto req_out_1 : request_for_output_1) {
          ++num_checks;
          std::ostringstream oss{};
          oss << "num_checks=" << num_checks << "\n"
              << "ach_in_0=" << ach_in_0 << "\n"
              << "ach_in_1=" << ach_in_1 << "\n"
              << "req_out_0=" << req_out_0 << "\n"
              << "req_out_1=" << req_out_1 << "\n";
          std::string setup = oss.str();
          auto s0 = ED::make_mux_state(2, 2, ED::MuxerDispatchStrategy::InOrder);
          double requested{req_out_0 + req_out_1};
          double achieved{requested}; // initial assumption
          std::vector<ED::PortValue> requested_pvs{
            ED::PortValue{ED::inport_outflow_request + 0, req_out_0},
            ED::PortValue{ED::inport_outflow_request + 1, req_out_1},
          };
          auto s1 = ED::mux_external_transition(s0, dt, requested_pvs);
          auto dt1 = ED::mux_time_advance(s1);
          auto ys1 = ED::mux_output_function(s1);
          ASSERT_EQ(ys1.size(), 1);
          EXPECT_EQ(ys1[0].port, ED::outport_inflow_request + 0);
          EXPECT_NEAR(ys1[0].value, requested, 1e-6);
          EXPECT_NEAR(ED::mux_get_outflow_request(s1), ED::mux_get_inflow_request(s1), 1e-6) << setup;
          EXPECT_NEAR(ED::mux_get_outflow_achieved(s1), ED::mux_get_inflow_achieved(s1), 1e-6) << setup;
          EXPECT_NEAR(ED::mux_get_outflow_request(s1), requested, 1e-6) << setup;
          EXPECT_NEAR(ED::mux_get_outflow_achieved(s1), requested, 1e-6) << setup;
          // response from inport 1 -- may or may not be enough
          std::vector<ED::PortValue> achieved_pvs{
            ED::PortValue{ED::inport_inflow_achieved + 0, requested * ach_in_0},
          };
          achieved = (requested * ach_in_0);
          auto s2 = mux_external_transition(s1, dt, achieved_pvs);
          std::string setup_i0 = std::string{"after response from inflow 0\n"} + setup;
          //EXPECT_NEAR(ED::mux_get_outflow_request(s2), ED::mux_get_inflow_request(s2), 1e-6) << setup_i0;
          EXPECT_NEAR(ED::mux_get_outflow_achieved(s2), ED::mux_get_inflow_achieved(s2), 1e-6) << setup_i0;
          EXPECT_NEAR(ED::mux_get_outflow_request(s2), requested, 1e-6) << setup_i0;
          // note: still requested below because we immediately request the difference to port 1 and assume we achieve it
          EXPECT_NEAR(ED::mux_get_outflow_achieved(s2), requested, 1e-6) << setup_i0;
          auto dt2 = mux_time_advance(s2);
          if (ach_in_0 == 1.0) {
            EXPECT_EQ(dt2, ED::infinity);
          }
          else {
            // didn't get enough... request from inport 1
            std::string setup_i1 = std::string{"after response from inflow 1\n"} + setup;
            EXPECT_EQ(dt2, 0);
            auto ys2 = mux_output_function(s2);
            ASSERT_EQ(ys2.size(), 1);
            EXPECT_EQ(ys2[0].port, ED::outport_inflow_request + 1);
            EXPECT_NEAR(ys2[0].value, requested - achieved, 1e-6);
            achieved = achieved + ((requested - achieved) * ach_in_1);
            achieved_pvs = std::vector{
              ED::PortValue{ED::inport_inflow_achieved + 1, (requested - (requested * ach_in_0)) * ach_in_1},
            };
            auto s3 = mux_external_transition(s2, dt, achieved_pvs);
            auto dt3 = mux_time_advance(s3);
            if (ach_in_1 == 1.0) {
              EXPECT_EQ(dt3, ED::infinity);
              EXPECT_NEAR(ED::mux_get_outflow_achieved(s3), ED::mux_get_inflow_achieved(s3), 1e-6) << setup_i1;
              EXPECT_NEAR(ED::mux_get_outflow_request(s3), requested, 1e-6) << setup_i1;
              // note: still requested below because we requested the difference to port 1 and achieved it
              // note: normally, we woudn't hear back from port 1 since it achieved its request...
              EXPECT_NEAR(ED::mux_get_outflow_achieved(s3), requested, 1e-6) << setup_i1;
            }
            else {
              // didn't meet loads; inform downstream
              auto ys3 = mux_output_function(s3);
              if (achieved >= req_out_0) {
                // only need to inform output 2 of a problem
                ASSERT_EQ(ys3.size(), 1);
                EXPECT_EQ(ys3[0].port, ED::outport_outflow_achieved + 1);
                EXPECT_NEAR(ys3[0].value, achieved - req_out_0, 1e-6);
              }
              else {
                ASSERT_EQ(ys3.size(), 2);
                EXPECT_EQ(ys3[0].port, ED::outport_outflow_achieved + 0);
                EXPECT_NEAR(ys3[0].value, achieved, 1e-6);
                EXPECT_EQ(ys3[1].port, ED::outport_outflow_achieved + 1);
                EXPECT_NEAR(ys3[1].value, 0.0, 1e-6);
              }
              achieved = (requested * ach_in_0) + ((requested - (requested * ach_in_0)) * ach_in_1);
              EXPECT_NEAR(ED::mux_get_outflow_achieved(s3), ED::mux_get_inflow_achieved(s3), 1e-6) << setup_i1;
              EXPECT_NEAR(ED::mux_get_outflow_request(s3), requested, 1e-6) << setup_i1;
              // note: all ports reported in and we are deficient so reporting...
              EXPECT_NEAR(ED::mux_get_outflow_achieved(s3), achieved, 1e-6) << setup_i1;
            }
          }
        }
      }
    }
  }
}

TEST(ErinBasicsTest, Test_energy_balance_on_mux_element)
{
  namespace E = ERIN;
  std::vector<double> achieved_for_input_0{1.0,0.5,0.1, 0.0};
  std::vector<double> achieved_for_input_1{1.0,0.5,0.1, 0.0};
  std::vector<double> request_for_output_0{10.0,100.0,5.0};
  std::vector<double> request_for_output_1{10.0,100.0,5.0};
  E::RealTimeType dt{0};
  int num_checks{0};
  for (const auto ach_in_0 : achieved_for_input_0) {
    for (const auto ach_in_1 : achieved_for_input_1) {
      for (const auto req_out_0 : request_for_output_0) {
        for (const auto req_out_1 : request_for_output_1) {
          ++num_checks;
          std::ostringstream oss{};
          oss << "num_checks=" << num_checks << "\n"
              << "ach_in_0=" << ach_in_0 << "\n"
              << "ach_in_1=" << ach_in_1 << "\n"
              << "req_out_0=" << req_out_0 << "\n"
              << "req_out_1=" << req_out_1 << "\n";
          std::string setup = oss.str();
          std::unique_ptr<E::FlowElement> p = std::make_unique<E::Mux>(
              "mux",
              E::ComponentType::Muxer,
              "electricity",
              2,
              2,
              E::MuxerDispatchStrategy::InOrder);
          double requested{req_out_0 + req_out_1};
          double achieved{requested}; // initial assumption
          std::vector<E::PortValue> xs0{
            E::PortValue{E::FlowElement::inport_outflow_request + 0, req_out_0},
            E::PortValue{E::FlowElement::inport_outflow_request + 1, req_out_1},
          };
          std::vector<E::PortValue> xs1{
            E::PortValue{
              E::FlowElement::inport_inflow_achieved + 0,
              requested * ach_in_0},
          };
          std::vector<E::PortValue> xs2{
            E::PortValue{
              E::FlowElement::inport_inflow_achieved + 1,
              (requested - (requested * ach_in_0)) * ach_in_1},
          };
          auto dt_logical_advance = E::Time{0, 1};
          // Exercise Begin
          auto dt0 = p->ta();
          ASSERT_EQ(dt0, E::inf) << setup;
          p->delta_ext(dt_logical_advance, xs0);
          auto dt1 = p->ta();
          ASSERT_EQ(dt1, dt_logical_advance) << setup;
          std::vector<E::PortValue> ys1{};
          p->output_func(ys1);
          ASSERT_EQ(ys1.size(), 1) << setup;
          ASSERT_EQ(ys1[0].port, E::FlowElement::outport_inflow_request + 0) << setup;
          ASSERT_NEAR(ys1[0].value, requested, 1e-6) << setup;
          p->delta_int();
          auto dt2 = p->ta();
          ASSERT_EQ(dt2, E::inf) << setup;
          if (ach_in_0 == 1.0) {
            continue;
          }
          p->delta_ext(dt_logical_advance, xs1);
          auto dt3 = p->ta();
          ASSERT_EQ(dt3, dt_logical_advance) << setup;
          std::vector<E::PortValue> ys2{};
          p->output_func(ys2);
          ASSERT_EQ(ys2.size(), 1) << setup;
          ASSERT_EQ(ys2[0].port, E::FlowElement::outport_inflow_request + 1) << setup;
          ASSERT_NEAR(ys2[0].value, requested * (1.0 - ach_in_0), 1e-6) << setup;
          p->delta_int();
          auto dt4 = p->ta();
          ASSERT_EQ(dt4, E::inf) << setup;
          if (ach_in_1 == 1.0) {
            continue;
          }
          p->delta_ext(dt_logical_advance, xs2);
          achieved = requested * (ach_in_0 + ((1.0 - ach_in_0) * ach_in_1));
          auto dt5 = p->ta();
          ASSERT_EQ(dt5, dt_logical_advance) << setup;
          std::vector<E::PortValue> ys3{};
          p->output_func(ys3);
          if (achieved >= req_out_0) {
            ASSERT_EQ(ys3.size(), 1) << setup;
            ASSERT_EQ(ys3[0].port, E::FlowElement::outport_outflow_achieved + 1) << setup;
            ASSERT_NEAR(ys3[0].value, achieved - req_out_0, 1e-6) << setup;

          }
          else {
            ASSERT_EQ(ys3.size(), 2) << setup;
            ASSERT_EQ(ys3[0].port, E::FlowElement::outport_outflow_achieved + 0) << setup;
            ASSERT_NEAR(ys3[0].value, achieved, 1e-6) << setup;
            ASSERT_EQ(ys3[1].port, E::FlowElement::outport_outflow_achieved + 1) << setup;
            ASSERT_NEAR(ys3[1].value, 0.0, 1e-6) << setup;
          }
          p->delta_int();
          auto dt6 = p->ta();
          ASSERT_EQ(dt6, E::inf) << setup;
        }
      }
    }
  }
}

TEST(ErinBasicsTest, Test_energy_balance_on_mux_with_replay)
{
  namespace ED = erin::devs;
  auto s0 = ED::make_mux_state(3, 2, ED::MuxerDispatchStrategy::InOrder);
  auto dt0 = ED::mux_time_advance(s0);
  ASSERT_EQ(dt0, ED::infinity);
  std::vector<ED::PortValue> xs0{
    ED::PortValue{ED::inport_outflow_request + 0, 2000.0},
    ED::PortValue{ED::inport_outflow_request + 1, 2000.0},
  };
  auto s1 = ED::mux_external_transition(s0, 0, xs0);
  EXPECT_NEAR(ED::mux_get_outflow_request(s1), 4000.0, 1e-6);
  EXPECT_NEAR(ED::mux_get_outflow_achieved(s1), ED::mux_get_inflow_achieved(s1), 1e-6);
  auto dt1 = ED::mux_time_advance(s1);
  ASSERT_EQ(dt1, 0);
  auto ys1 = ED::mux_output_function(s1);
  ASSERT_EQ(ys1.size(), 1);
  ASSERT_EQ(ys1[0].port, ED::outport_inflow_request + 0);
  ASSERT_EQ(ys1[0].value, 4000.0);
  auto s2 = ED::mux_internal_transition(s1);
  EXPECT_NEAR(ED::mux_get_outflow_request(s2), 4000.0, 1e-6);
  EXPECT_NEAR(ED::mux_get_outflow_achieved(s2), ED::mux_get_inflow_achieved(s2), 1e-6);
  auto dt2 = ED::mux_time_advance(s2);
  ASSERT_EQ(dt2, ED::infinity);
  std::vector<ED::PortValue> xs2{
    ED::PortValue{ED::inport_inflow_achieved + 0, 2228.57},
  };
  auto s3 = ED::mux_external_transition(s2, 0, xs2);
  EXPECT_NEAR(ED::mux_get_outflow_request(s3), 4000.0, 1e-6);
  EXPECT_NEAR(ED::mux_get_outflow_achieved(s3), ED::mux_get_inflow_achieved(s3), 1e-6);
  auto dt3 = ED::mux_time_advance(s3);
  ASSERT_EQ(dt3, 0);
  auto ys3 = ED::mux_output_function(s3);
  ASSERT_EQ(ys3.size(), 1);
  ASSERT_EQ(ys3[0].port, ED::outport_inflow_request + 1);
  ASSERT_NEAR(ys3[0].value, 1771.43, 0.005);
  std::vector<ED::PortValue> xs3{
    ED::PortValue{ED::inport_inflow_achieved + 0, 1448.57},
  };
  auto s4 = ED::mux_confluent_transition(s3, xs3);
  EXPECT_NEAR(ED::mux_get_outflow_request(s4), 4000.0, 1e-6);
  EXPECT_NEAR(ED::mux_get_outflow_achieved(s4), ED::mux_get_inflow_achieved(s4), 1e-6);
  auto dt4 = ED::mux_time_advance(s4);
  ASSERT_EQ(dt4, 0);
}
*/

TEST(ErinBasicsTest, Test_that_we_can_calculate_reliability_schedule)
{
  auto f = []()->double { return 0.5; };
  ERIN::ReliabilityCoordinator rc{};
  auto cds = erin::distribution::DistributionSystem{};
  std::int64_t final_time{10};
  auto reliability_schedule_1 = rc.calc_reliability_schedule(f, cds, final_time);
  EXPECT_EQ(reliability_schedule_1.size(), 0);
  auto failure_id = cds.add_fixed("f", 5);
  auto repair_id = cds.add_fixed("r", 1);
  auto fm_id = rc.add_failure_mode(
      "standard failure",
      failure_id,
      repair_id);
  auto comp_id = rc.register_component("c");
  rc.link_component_with_failure_mode(comp_id, fm_id);
  auto reliability_schedule = rc.calc_reliability_schedule(f, cds, final_time);
  EXPECT_EQ(reliability_schedule.size(), 1);
  std::vector<ERIN::TimeState> expected{
    ERIN::TimeState{0, true},
    ERIN::TimeState{5, false},
    ERIN::TimeState{6, true},
    ERIN::TimeState{11, false},
  };
  EXPECT_EQ(reliability_schedule.at(0), expected);
}

TEST(ErinBasicsTest, Test_that_reliability_works_on_source_component)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.default]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[10.0,0.0]]\n"
    "[dist.break]\n"
    "type = \"fixed\"\n"
    "value = 5\n"
    "time_unit = \"seconds\"\n"
    "[dist.repair]\n"
    "type = \"fixed\"\n"
    "value = 2\n"
    "time_unit = \"seconds\"\n"
    "[failure_mode.standard]\n"
    "failure_dist = \"break\"\n"
    "repair_dist = \"repair\"\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "max_outflow = 100.0\n"
    "failure_modes = [\"standard\"]\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "[networks.normal_operations]\n"
    "connections = [[\"S:OUT(0)\", \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "calculate_reliability = true\n";
  auto f = []()->double { return 0.5; };
  erin::distribution::DistributionSystem cds{};
  E::ReliabilityCoordinator rc{};
  auto id_break = cds.add_fixed("break", 5);
  auto id_repair = cds.add_fixed("repair", 2);
  auto id_fm = rc.add_failure_mode(
          "standard",
          id_break,
          id_repair);
  auto id_S = rc.register_component("S");
  rc.link_component_with_failure_mode(id_S, id_fm);
  std::int64_t final_time{10};
  auto expected_sch =
    rc.calc_reliability_schedule_by_component_tag(f, cds, final_time);
  auto m = E::make_main_from_string(input);
  auto sch = m.get_reliability_schedule();
  EXPECT_EQ(sch.size(), expected_sch.size());
  EXPECT_EQ(sch, expected_sch);
  auto out = m.run_all();
  EXPECT_TRUE(out.get_is_good());
  auto results = out.get_results();
  ASSERT_EQ(results.size(), 1);
  auto blue_sky_it = results.find("blue_sky");
  ASSERT_TRUE(blue_sky_it != results.end());
  const auto& raw_bs_data = results["blue_sky"];
  ASSERT_EQ(raw_bs_data.size(), 1);
  const auto& bs_scenario_results = raw_bs_data.at(0);
  ASSERT_TRUE(bs_scenario_results.get_is_good());
  const auto& bs_data = bs_scenario_results.get_results();
  std::unordered_map<std::string, std::vector<ERIN::Datum>>
    expected_results{
      { std::string{"S"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"L"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}}};
  ASSERT_EQ(expected_results.size(), bs_data.size());
  EXPECT_EQ(expected_results, bs_data);
}

TEST(ErinBasicsTest, Test_that_reliability_works_on_load_component)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.default]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[10.0,0.0]]\n"
    "[dist.break]\n"
    "type = \"fixed\"\n"
    "value = 5\n"
    "time_unit = \"seconds\"\n"
    "[dist.repair]\n"
    "type = \"fixed\"\n"
    "value = 2\n"
    "time_unit = \"seconds\"\n"
    "[failure_mode.standard]\n"
    "failure_dist = \"break\"\n"
    "repair_dist = \"repair\"\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "max_outflow = 100.0\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "failure_modes = [\"standard\"]\n"
    "[networks.normal_operations]\n"
    "connections = [[\"S:OUT(0)\", \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "calculate_reliability = true\n";
  auto rand_fn = []()->double { return 0.5; };
  erin::distribution::DistributionSystem cds{};
  E::ReliabilityCoordinator rc{};
  auto id_break = cds.add_fixed("break", 5);
  auto id_repair = cds.add_fixed("repair", 2);
  auto id_fm = rc.add_failure_mode(
          "standard",
          id_break,
          id_repair);
  auto id_L = rc.register_component("L");
  rc.link_component_with_failure_mode(id_L, id_fm);
  std::int64_t final_time{10};
  auto expected_sch =
    rc.calc_reliability_schedule_by_component_tag(rand_fn, cds, final_time);
  auto m = E::make_main_from_string(input);
  auto sch = m.get_reliability_schedule();
  EXPECT_EQ(sch.size(), expected_sch.size());
  EXPECT_EQ(sch, expected_sch);
  auto out = m.run_all();
  EXPECT_TRUE(out.get_is_good());
  auto results = out.get_results();
  ASSERT_EQ(results.size(), 1);
  auto blue_sky_it = results.find("blue_sky");
  ASSERT_TRUE(blue_sky_it != results.end());
  const auto& raw_bs_data = results["blue_sky"];
  ASSERT_EQ(raw_bs_data.size(), 1);
  const auto& bs_scenario_results = raw_bs_data.at(0);
  ASSERT_TRUE(bs_scenario_results.get_is_good());
  const auto& bs_data = bs_scenario_results.get_results();
  std::unordered_map<std::string, std::vector<ERIN::Datum>>
    expected_results{
      { std::string{"S"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"L"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}}};
  ASSERT_EQ(expected_results.size(), bs_data.size());
  EXPECT_EQ(expected_results, bs_data);
}

/*
TEST(ErinBasicsTest, Test_that_reliability_works_on_mux_component)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.default]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[10.0,0.0]]\n"
    "[dist.break]\n"
    "type = \"fixed\"\n"
    "value = 5\n"
    "time_unit = \"seconds\"\n"
    "[dist.repair]\n"
    "type = \"fixed\"\n"
    "value = 2\n"
    "time_unit = \"seconds\"\n"
    "[failure_mode.standard]\n"
    "failure_dist = \"break\"\n"
    "repair_dist = \"repair\"\n"
    "[components.S1]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "max_outflow = 50.0\n"
    "[components.S2]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "[components.L1]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "[components.L2]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "[components.M]\n"
    "type = \"muxer\"\n"
    "num_inflows = 2\n"
    "num_outflows = 2\n"
    "stream = \"electricity\"\n"
    "dispatch_strategy = \"in_order\"\n"
    "failure_modes = [\"standard\"]\n"
    "[networks.normal_operations]\n"
    "connections = [\n"
    "    [\"S1:OUT(0)\",  \"M:IN(0)\", \"electricity\"],\n"
    "    [\"S2:OUT(0)\",  \"M:IN(1)\", \"electricity\"],\n"
    "    [ \"M:OUT(0)\", \"L1:IN(0)\", \"electricity\"],\n"
    "    [ \"M:OUT(1)\", \"L2:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "calculate_reliability = true\n";
  auto m = E::make_main_from_string(input);
  auto out = m.run_all();
  EXPECT_TRUE(out.get_is_good());
  auto results = out.get_results();
  ASSERT_EQ(results.size(), 1);
  auto blue_sky_it = results.find("blue_sky");
  ASSERT_TRUE(blue_sky_it != results.end());
  const auto& raw_bs_data = results["blue_sky"];
  ASSERT_EQ(raw_bs_data.size(), 1);
  const auto& bs_scenario_results = raw_bs_data.at(0);
  ASSERT_TRUE(bs_scenario_results.get_is_good());
  const auto& bs_data = bs_scenario_results.get_results();
  std::unordered_map<std::string, std::vector<ERIN::Datum>>
    expected_results{
      { std::string{"S1"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 200.0, 50.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 200.0, 50.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"S2"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 150.0, 150.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 150.0, 150.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"M-inflow(0)"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 200.0, 50.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 200.0, 50.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"M-inflow(1)"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 150.0, 150.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 150.0, 150.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"M-outflow(0)"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"M-outflow(1)"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"L1"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"L2"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}}};
  ASSERT_EQ(expected_results.size(), bs_data.size());
  if (false) {
    for (const auto& item : bs_data) {
      std::cout << item.first << "\n";
      for (const auto& d : item.second) {
        std::cout << "  " << d << "\n";
      }
    }
  }
  EXPECT_EQ(expected_results, bs_data);
}
*/

/*
TEST(ErinBasicsTest, Test_that_reliability_works_on_converter_component)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.default]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[10.0,0.0]]\n"
    "[dist.break]\n"
    "type = \"fixed\"\n"
    "value = 5\n"
    "time_unit = \"seconds\"\n"
    "[dist.repair]\n"
    "type = \"fixed\"\n"
    "value = 2\n"
    "time_unit = \"seconds\"\n"
    "[failure_mode.standard]\n"
    "failure_dist = \"break\"\n"
    "repair_dist = \"repair\"\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"natural_gas\"\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "[components.C]\n"
    "type = \"converter\"\n"
    "input_stream = \"natural_gas\"\n"
    "output_stream = \"electricity\"\n"
    "lossflow = \"waste_heat\"\n"
    "constant_efficiency = 0.5\n"
    "failure_modes = [\"standard\"]\n"
    "[networks.normal_operations]\n"
    "connections = [\n"
    "    [\"S:OUT(0)\",  \"C:IN(0)\", \"natural_gas\"],\n"
    "    [\"C:OUT(0)\",  \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "calculate_reliability = true\n";
  auto m = E::make_main_from_string(input);
  auto out = m.run_all();
  EXPECT_TRUE(out.get_is_good());
  auto results = out.get_results();
  ASSERT_EQ(results.size(), 1);
  auto blue_sky_it = results.find("blue_sky");
  ASSERT_TRUE(blue_sky_it != results.end());
  const auto& raw_bs_data = results["blue_sky"];
  ASSERT_EQ(raw_bs_data.size(), 1);
  const auto& bs_scenario_results = raw_bs_data.at(0);
  ASSERT_TRUE(bs_scenario_results.get_is_good());
  const auto& bs_data = bs_scenario_results.get_results();
  std::unordered_map<std::string, std::vector<ERIN::Datum>>
    expected_results{
      { std::string{"S"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 200.0, 200.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 200.0, 200.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"C-inflow"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 200.0, 200.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 200.0, 200.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"C-outflow"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"C-lossflow"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 0.0, 0.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 0.0, 0.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"C-wasteflow"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"L"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}}};
  ASSERT_EQ(expected_results.size(), bs_data.size());
  if (false) {
    std::vector<std::string> keys{};
    std::cout << "actual keys:\n";
    for (const auto& item : bs_data) {
      std::cout << "- " << item.first << "\n";
    }
    std::cout << "expected keys:\n";
    for (const auto& item : expected_results) {
      const auto& k = item.first;
      std::cout << "- " << k << "\n";
      keys.emplace_back(k);
      auto it = bs_data.find(k);
      ASSERT_TRUE(it != bs_data.end()) << "expected key '" << k << "' not found in bs_data\n";
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& k : keys) {
      std::cout << k << ":\n";
      std::cout << "- EXPECTED:\n";
      for (const auto& d : expected_results[k]) {
        std::cout << "  " << d << "\n";
      }
      std::cout << "- ACTUAL:\n";
      auto it = bs_data.find(k);
      ASSERT_TRUE(it != bs_data.end());
      for (const auto& d : it->second) {
        std::cout << "  " << d << "\n";
      }
    }
  }
  EXPECT_EQ(expected_results, bs_data);
}
*/

/*
TEST(ErinBasicsTest, Test_that_reliability_works_on_pass_through_component)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.default]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[10.0,0.0]]\n"
    "[dist.break]\n"
    "type = \"fixed\"\n"
    "value = 5\n"
    "time_unit = \"seconds\"\n"
    "[dist.repair]\n"
    "type = \"fixed\"\n"
    "value = 2\n"
    "time_unit = \"seconds\"\n"
    "[failure_mode.standard]\n"
    "failure_dist = \"break\"\n"
    "repair_dist = \"repair\"\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "[components.P]\n"
    "type = \"pass_through\"\n"
    "stream = \"electricity\"\n"
    "failure_modes = [\"standard\"]\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "[networks.normal_operations]\n"
    "connections = [\n"
    "    [\"S:OUT(0)\",  \"P:IN(0)\", \"electricity\"],\n"
    "    [\"P:OUT(0)\",  \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "calculate_reliability = true\n";
  auto m = E::make_main_from_string(input);
  auto out = m.run_all();
  EXPECT_TRUE(out.get_is_good());
  auto results = out.get_results();
  ASSERT_EQ(results.size(), 1);
  auto blue_sky_it = results.find("blue_sky");
  ASSERT_TRUE(blue_sky_it != results.end());
  const auto& raw_bs_data = results["blue_sky"];
  ASSERT_EQ(raw_bs_data.size(), 1);
  const auto& bs_scenario_results = raw_bs_data.at(0);
  ASSERT_TRUE(bs_scenario_results.get_is_good());
  const auto& bs_data = bs_scenario_results.get_results();
  std::unordered_map<std::string, std::vector<ERIN::Datum>>
    expected_results{
      { std::string{"S"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"P"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"L"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{10, 0.0, 0.0}}}};
  ASSERT_EQ(expected_results.size(), bs_data.size());
  if (false) {
    std::vector<std::string> keys{};
    std::cout << "actual keys:\n";
    for (const auto& item : bs_data) {
      std::cout << "- " << item.first << "\n";
    }
    std::cout << "expected keys:\n";
    for (const auto& item : expected_results) {
      const auto& k = item.first;
      std::cout << "- " << k << "\n";
      keys.emplace_back(k);
      auto it = bs_data.find(k);
      ASSERT_TRUE(it != bs_data.end()) << "expected key '" << k << "' not found in bs_data\n";
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& k : keys) {
      std::cout << k << ":\n";
      std::cout << "- EXPECTED:\n";
      for (const auto& d : expected_results[k]) {
        std::cout << "  " << d << "\n";
      }
      std::cout << "- ACTUAL:\n";
      auto it = bs_data.find(k);
      ASSERT_TRUE(it != bs_data.end());
      for (const auto& d : it->second) {
        std::cout << "  " << d << "\n";
      }
    }
  }
  EXPECT_EQ(expected_results, bs_data);
}
*/

/*
TEST(ErinBasicsTest, Test_that_reliability_works_on_storage_component)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.default]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[10.0,0.0]]\n"
    "[dist.break]\n"
    "type = \"fixed\"\n"
    "value = 5\n"
    "time_unit = \"seconds\"\n"
    "[dist.repair]\n"
    "type = \"fixed\"\n"
    "value = 2\n"
    "time_unit = \"seconds\"\n"
    "[failure_mode.standard]\n"
    "failure_dist = \"break\"\n"
    "repair_dist = \"repair\"\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "[components.BATTERY]\n"
    "type = \"store\"\n"
    "outflow = \"electricity\"\n"
    "inflow = \"electricity\"\n"
    "max_inflow = 50.0\n"
    "capacity_unit = \"kJ\"\n"
    "capacity = 300.0\n"
    "failure_modes = [\"standard\"]\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "[networks.normal_operations]\n"
    "connections = [\n"
    "    [\"S:OUT(0)\",  \"BATTERY:IN(0)\", \"electricity\"],\n"
    "    [\"BATTERY:OUT(0)\", \"L:IN(0)\", \"electricity\"]]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"normal_operations\"\n"
    "calculate_reliability = true\n";
  auto m = E::make_main_from_string(input);
  auto out = m.run_all();
  EXPECT_TRUE(out.get_is_good());
  auto results = out.get_results();
  ASSERT_EQ(results.size(), 1);
  auto blue_sky_it = results.find("blue_sky");
  ASSERT_TRUE(blue_sky_it != results.end());
  const auto& raw_bs_data = results["blue_sky"];
  ASSERT_EQ(raw_bs_data.size(), 1);
  const auto& bs_scenario_results = raw_bs_data.at(0);
  ASSERT_TRUE(bs_scenario_results.get_is_good());
  const auto& bs_data = bs_scenario_results.get_results();
  std::unordered_map<std::string, std::vector<ERIN::Datum>>
    expected_results{
      { std::string{"S"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 50.0, 50.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 50.0, 50.0},
          ERIN::Datum{8, 50.0, 50.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"BATTERY-inflow"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 50.0, 50.0},
          ERIN::Datum{5, 50.0, 0.0},
          ERIN::Datum{7, 50.0, 50.0},
          ERIN::Datum{8, 50.0, 50.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"BATTERY-outflow"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{8, 100.0, 50.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"BATTERY-storeflow"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 0.0, 0.0},
          ERIN::Datum{5, 50.0, 0.0},
          ERIN::Datum{7, 0.0, 0.0},
          ERIN::Datum{8, 0.0, 0.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"BATTERY-discharge"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 50.0, 50.0},
          ERIN::Datum{5, 0.0, 0.0},
          ERIN::Datum{7, 50.0, 50.0},
          ERIN::Datum{8, 50.0, 0.0},
          ERIN::Datum{10, 0.0, 0.0}}},
      { std::string{"L"},
        std::vector<ERIN::Datum>{
          ERIN::Datum{0, 100.0, 100.0},
          ERIN::Datum{5, 100.0, 0.0},
          ERIN::Datum{7, 100.0, 100.0},
          ERIN::Datum{8, 100.0, 50.0},
          ERIN::Datum{10, 0.0, 0.0}}}};
  ASSERT_EQ(expected_results.size(), bs_data.size());
  if (false) {
    std::vector<std::string> keys{};
    std::cout << "actual keys:\n";
    for (const auto& item : bs_data) {
      std::cout << "- " << item.first << "\n";
    }
    std::cout << "expected keys:\n";
    for (const auto& item : expected_results) {
      const auto& k = item.first;
      std::cout << "- " << k << "\n";
      keys.emplace_back(k);
      auto it = bs_data.find(k);
      ASSERT_TRUE(it != bs_data.end()) << "expected key '" << k << "' not found in bs_data\n";
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& k : keys) {
      std::cout << k << ":\n";
      std::cout << "- EXPECTED:\n";
      for (const auto& d : expected_results[k]) {
        std::cout << "  " << d << "\n";
      }
      std::cout << "- ACTUAL:\n";
      auto it = bs_data.find(k);
      ASSERT_TRUE(it != bs_data.end());
      for (const auto& d : it->second) {
        std::cout << "  " << d << "\n";
      }
    }
  }
  EXPECT_EQ(expected_results, bs_data);
}
*/

TEST(ErinBasicsTest, Test_adjusting_reliability_schedule)
{
  namespace E = ERIN;
  auto rand_fn = []()->double { return 0.5; };
  erin::distribution::DistributionSystem cds{};
  E::ReliabilityCoordinator rc{};
  auto dist_break_id = cds.add_fixed("break", 10);
  auto dist_repair_id = cds.add_fixed("repair", 5);
  auto fm_standard_id = rc.add_failure_mode(
      "standard", dist_break_id, dist_repair_id);
  std::string comp_string_id{"S"};
  auto comp_id = rc.register_component(comp_string_id);
  rc.link_component_with_failure_mode(comp_id, fm_standard_id);
  ERIN::RealTimeType final_time{100};
  auto sch = rc.calc_reliability_schedule_by_component_tag(rand_fn, cds, final_time);
  std::unordered_map<std::string, std::vector<E::TimeState>>
    expected_sch{
      { comp_string_id,
        std::vector<E::TimeState>{
          E::TimeState{0,  true},
          E::TimeState{10, false},
          E::TimeState{15, true},
          E::TimeState{25, false},
          E::TimeState{30, true},
          E::TimeState{40, false},
          E::TimeState{45, true},
          E::TimeState{55, false},
          E::TimeState{60, true},
          E::TimeState{70, false},
          E::TimeState{75, true},
          E::TimeState{85, false},
          E::TimeState{90, true},
          E::TimeState{100, false},
          E::TimeState{105, true}}}};
  ASSERT_EQ(sch, expected_sch);
  E::RealTimeType scenario_start{62};
  E::RealTimeType scenario_end{87};
  auto clipped_sch = E::clip_schedule_to<std::string>(
      sch, scenario_start, scenario_end);
  std::unordered_map<std::string, std::vector<E::TimeState>>
    expected_clipped_sch{
      { comp_string_id,
        std::vector<E::TimeState>{
          E::TimeState{62-62, true},
          E::TimeState{70-62, false},
          E::TimeState{75-62, true},
          E::TimeState{85-62, false}}}};
  ASSERT_EQ(clipped_sch, expected_clipped_sch);
}

/*
TEST(ErinBasicsTest, Test_that_switch_element_works)
{
  namespace ED = erin::devs;
  namespace E = ERIN;
  std::vector<E::TimeState> schedule{
    E::TimeState{0, true},
    E::TimeState{5, false},
    E::TimeState{7, true},
    E::TimeState{12, false},
    E::TimeState{14, true}};
  ED::OnOffSwitchData expected_data{
    std::vector<E::RealTimeType>{0,5,7,12,14},
    std::vector<bool>{true,false,true,false,true},
    std::vector<E::RealTimeType>::size_type{5}
  };
  auto data = ED::make_on_off_switch_data(schedule);
  EXPECT_EQ(data, expected_data);
  ED::OnOffSwitchState expected_s{
    0,
    true,
    1,
    ED::Port3{},
    ED::Port3{},
    false,
    false
  };
  auto s = ED::make_on_off_switch_state(data);
  EXPECT_EQ(s, expected_s);
  auto dt = ED::on_off_switch_time_advance(data, s);
  EXPECT_EQ(dt, 5);
  std::vector<E::PortValue> xs{
    E::PortValue{ED::inport_outflow_request, 100.0}};
  s = ED::on_off_switch_external_transition(s, 1, xs);
  EXPECT_NE(s, expected_s);
  EXPECT_EQ(s.time, 1);
  dt = ED::on_off_switch_time_advance(data, s);
  EXPECT_EQ(dt, 0);
  std::vector<ED::PortValue> expected_ys{
    E::PortValue{ED::outport_inflow_request, 100.0}
  };
  auto ys = ED::on_off_switch_output_function(data, s);
  ASSERT_EQ(expected_ys.size(), ys.size());
  EXPECT_EQ(expected_ys[0].port, ys[0].port);
  EXPECT_EQ(expected_ys[0].value, ys[0].value);
  s = ED::on_off_switch_internal_transition(data, s);
  dt = ED::on_off_switch_time_advance(data, s);
  EXPECT_EQ(dt, 4);
  ys = ED::on_off_switch_output_function(data, s);
  expected_ys = std::vector<ED::PortValue>{};
  ASSERT_EQ(expected_ys.size(), ys.size())
    << "s: " << s << "\n"
    << "expected_ys: " << E::vec_to_string<E::PortValue>(expected_ys) << "\n"
    << "ys: " << E::vec_to_string<E::PortValue>(ys) << "\n";
  ASSERT_TRUE(
      erin::utils::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys, expected_ys, compare_ports));
  s = ED::on_off_switch_internal_transition(data, s);
  ASSERT_EQ(s.time, 5);
  dt = ED::on_off_switch_time_advance(data, s);
  ASSERT_EQ(dt, 0);
  ys = ED::on_off_switch_output_function(data, s);
  expected_ys = std::vector<ED::PortValue>{
    ED::PortValue{ED::outport_inflow_request, 100.0},
  };
  ASSERT_TRUE(
      erin::utils::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys, expected_ys, compare_ports));
  s = ED::on_off_switch_internal_transition(data, s);
  ASSERT_EQ(s.time, 7);
  dt = ED::on_off_switch_time_advance(data, s);
  ASSERT_EQ(dt, 5);
  ys = ED::on_off_switch_output_function(data, s);
  expected_ys = std::vector<ED::PortValue>{
    ED::PortValue{ED::outport_inflow_request, 0.0},
  };
  ASSERT_TRUE(
      erin::utils::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys, expected_ys, compare_ports));
  s = ED::on_off_switch_internal_transition(data, s);
  ASSERT_EQ(s.time, 12);
  dt = ED::on_off_switch_time_advance(data, s);
  ASSERT_EQ(dt, 2);
  ys = ED::on_off_switch_output_function(data, s);
  expected_ys = std::vector<ED::PortValue>{
    ED::PortValue{ED::outport_inflow_request, 100.0},
    ED::PortValue{ED::outport_outflow_achieved, 100.0},
  };
  ASSERT_TRUE(
      erin::utils::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys, expected_ys, compare_ports));
  s = ED::on_off_switch_internal_transition(data, s);
  EXPECT_EQ(s.time, 14);
  dt = ED::on_off_switch_time_advance(data, s);
  ASSERT_EQ(dt, ED::infinity);
}
*/

TEST(ErinBasicsTest, Test_fixed_distribution)
{
  namespace E = ERIN;
  namespace ED = erin::distribution;
  ED::DistributionSystem dist_sys{};
  E::RealTimeType fixed_dt{10};
  auto dist_id = dist_sys.add_fixed("some_dist", fixed_dt);
  EXPECT_EQ(dist_sys.next_time_advance(dist_id), fixed_dt);
}

TEST(ErinBasicsTest, Test_uniform_distribution)
{
  namespace E = ERIN;
  namespace ED = erin::distribution;
  ED::DistributionSystem dist_sys{};
  E::RealTimeType lower_dt{10};
  E::RealTimeType upper_dt{50};
  auto dist_id = dist_sys.add_uniform("a_uniform_dist", lower_dt, upper_dt);
  double dice_roll_1{1.0};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), upper_dt);
  double dice_roll_2{0.0};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), lower_dt);
  double dice_roll_3{0.5};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), (lower_dt + upper_dt) / 2);
}

TEST(ErinBasicsTest, Test_normal_distribution)
{
  namespace E = ERIN;
  namespace ED = erin::distribution;
  ED::DistributionSystem dist_sys{};
  E::RealTimeType mean{1000};
  E::RealTimeType stddev{50};
  auto dist_id = dist_sys.add_normal("a_normal_dist", mean, stddev);
  double dice_roll_1{0.5};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), mean);
  double dice_roll_2{0.0};
  constexpr double sqrt2{1.4142'1356'2373'0951};
  EXPECT_EQ(
      dist_sys.next_time_advance(dist_id, dice_roll_2),
      mean - static_cast<E::RealTimeType>(std::round(3.0 * sqrt2 * stddev)));
  double dice_roll_3{1.0};
  EXPECT_EQ(
      dist_sys.next_time_advance(dist_id, dice_roll_3),
      mean + static_cast<E::RealTimeType>(std::round(3.0 * sqrt2 * stddev)));
  double dice_roll_4{0.0};
  mean = 10;
  dist_id = dist_sys.add_normal("a_normal_dist_v2", mean, stddev);
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_4), 0);
}

TEST(ErinBasicsTest, Test_quantile_table_distribution)
{
  namespace E = ERIN;
  namespace ED = erin::distribution;
  ED::DistributionSystem dist_sys{};
  // ys are times; always increasing
  // xs are "dice roll" values [0.0, 1.0]; always increasing
  std::vector<double> dts{0.0, 100.0};
  std::vector<double> xs{0.0, 1.0};
  auto dist_id = dist_sys.add_quantile_table("a_table_dist_1", xs, dts);
  constexpr double dice_roll_1{0.5};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), 50);
  constexpr double dice_roll_2{0.0};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), 0);
  constexpr double dice_roll_3{1.0};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), 100);
  dts = std::vector{5.0, 6.0};
  dist_id = dist_sys.add_quantile_table("a_table_dist_2", xs, dts);
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), 6);
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), 5);
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), 6);
  dts = std::vector{0.0, 400.0, 600.0, 1000.0};
  xs = std::vector{0.0, 0.4, 0.6, 1.0};
  dist_id = dist_sys.add_quantile_table("a_table_dist_3", xs, dts);
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), 500);
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), 0);
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), 1000);
  constexpr double dice_roll_4{0.25};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_4), 250);
  constexpr double dice_roll_5{0.75};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_5), 750);
  xs = std::vector{-20.0, -15.0, -10.0, -5.0, 0.0};
  dts = std::vector{1.0, 2.0, 3.0, 4.0, 5.0};
  ASSERT_THROW(dist_sys.add_quantile_table("a_table_dist_4", xs, dts), std::invalid_argument);
  xs = std::vector{0.0, 0.5, 0.8};
  dts = std::vector{100.0, 200.0, 300.0};
  ASSERT_THROW(dist_sys.add_quantile_table("a_table_dist_5", xs, dts), std::invalid_argument);
}

TEST(ErinBasicsTest, Test_weibull_distribution)
{
  namespace E = ERIN;
  namespace ED = erin::distribution;
  ED::DistributionSystem dist_sys{};
  double k{5.0};        // shape parameter
  double lambda{200.0}; // scale parameter
  double gamma{0.0};    // location parameter
  auto dist_id = dist_sys.add_weibull("a_weibull_dist", k, lambda, gamma);
  double dice_roll_1{0.5};
  E::RealTimeType ans1{186};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), ans1);
  double dice_roll_2{0.0};
  E::RealTimeType ans2{0};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), ans2);
  double dice_roll_3{1.0};
  E::RealTimeType ans3{312};
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), ans3);
  double dice_roll_4{0.0};
  gamma = 10.0;
  E::RealTimeType ans4{static_cast<E::RealTimeType>(gamma)};
  dist_id = dist_sys.add_weibull("a_normal_dist_v2", k, lambda, gamma);
  EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_4), ans4);
}

TEST(ErinBasicsTest, Test_uncontrolled_source)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.default]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,100.0],[10.0,0.0]]\n"
    "[loads.supply]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,50.0],[5.0,120.0],[8.0,100.0],[10.0,0.0]]\n"
    "[components.US]\n"
    "type = \"uncontrolled_source\"\n"
    "output_stream = \"electricity\"\n"
    "supply_by_scenario.blue_sky = \"supply\"\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"default\"\n"
    "[networks.nw]\n"
    "connections = [\n"
    "    [\"US:OUT(0)\",  \"L:IN(0)\", \"electricity\"],\n"
    "    ]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"nw\"\n";
  auto m = E::make_main_from_string(input);
  auto out = m.run_all();
  EXPECT_TRUE(out.get_is_good());
  auto results_map = out.get_results();
  ASSERT_EQ(1, results_map.size());
  const auto& bs_res = results_map["blue_sky"];
  ASSERT_EQ(1, bs_res.size());
  const auto& bs_res0 = bs_res[0];
  const auto& rez = bs_res0.get_results();
  std::set<std::string> expected_comp_ids{"US-inflow", "US-outflow", "US-lossflow", "L"};
  ASSERT_EQ(expected_comp_ids.size(), rez.size());
  if (false) {
    for (const auto& item : rez) {
      std::cout << item.first << ":\n";
      for (const auto& d : item.second) {
        std::cout << "  " << d << "\n";
      }
    }
  }
  const auto& comp_ids = bs_res0.get_component_ids();
  std::set<std::string> actual_comp_ids{};
  for (const auto& id : comp_ids) {
    actual_comp_ids.emplace(id);
  }
  ASSERT_EQ(actual_comp_ids.size(), expected_comp_ids.size());
  EXPECT_EQ(actual_comp_ids, expected_comp_ids);
  auto ss_map = bs_res0.get_statistics();
  ERIN::FlowValueType L_load_not_served{5*50.0};
  ERIN::FlowValueType L_total_energy{5*50.0 + 5*100.0};
  ERIN::RealTimeType L_max_downtime{5};
  auto L_ss = ss_map["L"];
  EXPECT_EQ(L_ss.load_not_served, L_load_not_served);
  EXPECT_EQ(L_ss.total_energy, L_total_energy);
  EXPECT_EQ(L_ss.max_downtime, L_max_downtime);
  ERIN::FlowValueType US_inflow_total_energy{5*50.0 + 3*120.0 + 2*100.0};
  auto USin_ss = ss_map["US-inflow"];
  EXPECT_EQ(USin_ss.total_energy, US_inflow_total_energy);
}

TEST(ErinBasicsTest, Test_mover_element_addition)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"seconds\"\n"
    "max_time = 10\n"
    "[loads.environment]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1000.0],[10.0,0.0]]\n"
    "[loads.cooling]\n"
    "time_unit = \"seconds\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,50.0],[5.0,120.0],[8.0,100.0],[10.0,0.0]]\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "outflow = \"electricity\"\n"
    "[components.US]\n"
    "type = \"uncontrolled_source\"\n"
    "output_stream = \"heat\"\n"
    "supply_by_scenario.blue_sky = \"cooling\"\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"heat\"\n"
    "loads_by_scenario.blue_sky = \"environment\"\n"
    "[components.M]\n"
    "type = \"mover\"\n"
    "inflow0 = \"heat\"\n"
    "inflow1 = \"electricity\"\n"
    "outflow = \"heat\"\n"
    "COP = 5.0\n"
    "[networks.nw]\n"
    "connections = [\n"
    "    [\"US:OUT(0)\",  \"M:IN(0)\", \"heat\"],\n"
    "    [\"S:OUT(0)\",  \"M:IN(1)\", \"electricity\"],\n"
    "    [\"M:OUT(0)\",  \"L:IN(0)\", \"heat\"],\n"
    "    ]\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"seconds\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 10\n"
    "max_occurrences = 1\n"
    "network = \"nw\"\n";
  auto m = E::make_main_from_string(input);
  auto out = m.run_all();
  EXPECT_TRUE(out.get_is_good());
  auto results_map = out.get_results();
  ASSERT_EQ(1, results_map.size());
  const auto& bs_res = results_map["blue_sky"];
  ASSERT_EQ(1, bs_res.size());
  const auto& bs_res0 = bs_res[0];
  const auto& rez = bs_res0.get_results();
  std::set<std::string> expected_comp_ids{
    "US-inflow", "US-outflow", "US-lossflow",
    "L", "S", "M-inflow(0)", "M-inflow(1)", "M-outflow"};
  ASSERT_EQ(expected_comp_ids.size(), rez.size());
  if (false) {
    for (const auto& item : rez) {
      std::cout << item.first << ":\n";
      for (const auto& d : item.second) {
        std::cout << "  " << d << "\n";
      }
    }
  }
  const auto& comp_ids = bs_res0.get_component_ids();
  std::set<std::string> actual_comp_ids{};
  for (const auto& id : comp_ids) {
    actual_comp_ids.emplace(id);
  }
  ASSERT_EQ(actual_comp_ids.size(), expected_comp_ids.size());
  EXPECT_EQ(actual_comp_ids, expected_comp_ids);
  auto ss_map = bs_res0.get_statistics();
  ERIN::RealTimeType L_max_downtime{10};
  ERIN::FlowValueType L_total_energy{(5*50.0 + 3*120.0 + 2*100.0)*(1.0 + (1.0 / 5.0))};
  ERIN::FlowValueType L_load_not_served{10*1000.0 - L_total_energy};
  auto L_ss = ss_map["L"];
  EXPECT_EQ(L_ss.max_downtime, L_max_downtime);
  EXPECT_EQ(L_ss.load_not_served, L_load_not_served);
  EXPECT_EQ(L_ss.total_energy, L_total_energy);
}

TEST(ErinBasicsTest, Test_muxer_dispatch_strategy)
{
  namespace ED = erin::devs;
  namespace E = ERIN;
  E::RealTimeType time{0};
  E::FlowValueType outflow_achieved{100.0};
  std::vector<ED::Port3> outflow_ports{
    ED::Port3{50.0, 0.0},
    ED::Port3{50.0, 0.0},
    ED::Port3{50.0, 0.0},
    ED::Port3{50.0, 0.0},
  };
  std::vector<ED::PortUpdate3> expected_outflows{
    ED::PortUpdate3{ED::Port3{50.0, 50.0}, false, true},
    ED::PortUpdate3{ED::Port3{50.0, 50.0}, false, true},
    ED::PortUpdate3{ED::Port3{50.0, 0.0}, false, false},
    ED::PortUpdate3{ED::Port3{50.0, 0.0}, false, false},
  };
  auto outflows = ED::distribute_inflow_to_outflow_in_order(
      outflow_ports, outflow_achieved);
  ASSERT_EQ(expected_outflows.size(), outflows.size());
  for (std::vector<ED::Port>::size_type idx{0}; idx < outflows.size(); ++idx) {
    EXPECT_EQ(expected_outflows[idx], outflows[idx])
      << "idx = " << idx << "\n";
  }
  std::vector<ED::Port3> outflow_ports_irregular{
    ED::Port3{50.0, 0.0},
    ED::Port3{10.0, 0.0},
    ED::Port3{90.0, 0.0},
    ED::Port3{50.0, 0.0},
  };
  auto outflows_irregular = ED::distribute_inflow_to_outflow_in_order(
      outflow_ports_irregular, outflow_achieved);
  std::vector<ED::PortUpdate3> expected_outflows_irregular{
    ED::PortUpdate3{ED::Port3{50.0, 50.0}, false, true},
    ED::PortUpdate3{ED::Port3{10.0, 10.0}, false, true},
    ED::PortUpdate3{ED::Port3{90.0, 40.0}, false, true},
    ED::PortUpdate3{ED::Port3{50.0, 0.0}, false, false},
  };
  ASSERT_EQ(expected_outflows_irregular.size(), outflows_irregular.size());
  for (std::size_t idx{0}; idx < outflows_irregular.size(); ++idx) {
    EXPECT_EQ(expected_outflows_irregular[idx], outflows_irregular[idx])
      << "idx = " << idx << "\n";
  }

  std::vector<ED::Port3> expected_outflows_dist{
    ED::Port3{50.0, 25.0},
    ED::Port3{50.0, 25.0},
    ED::Port3{50.0, 25.0},
    ED::Port3{50.0, 25.0},
  };
  auto outflows_dist = ED::distribute_inflow_to_outflow_evenly(
      outflow_ports, outflow_achieved);
  ASSERT_EQ(expected_outflows_dist.size(), outflows_dist.size());
  for (std::vector<ED::Port>::size_type idx{0}; idx < outflows_dist.size(); ++idx) {
    EXPECT_EQ(expected_outflows_dist[idx], outflows_dist[idx].port)
      << "idx = " << idx << "\n";
  }
  auto outflows_dist_irregular = ED::distribute_inflow_to_outflow_evenly(
      outflow_ports_irregular, outflow_achieved);
  std::vector<ED::Port3> expected_outflows_dist_irregular{
    ED::Port3{50.0, 30.0},
    ED::Port3{10.0, 10.0},
    ED::Port3{90.0, 30.0},
    ED::Port3{50.0, 30.0},
  };
  ASSERT_EQ(expected_outflows_dist_irregular.size(), outflows_dist_irregular.size());
  for (std::vector<ED::Port2>::size_type idx{0}; idx < outflows_dist_irregular.size(); ++idx) {
    EXPECT_EQ(expected_outflows_dist_irregular[idx], outflows_dist_irregular[idx].port)
      << "idx = " << idx << "\n";
  }
}

/*
TEST(ErinBasicsTest, Test_reliability_schedule)
{
  namespace E = ERIN;
  std::string input =
    "[simulation_info]\n"
    "rate_unit = \"kW\"\n"
    "quantity_unit = \"kJ\"\n"
    "time_unit = \"hours\"\n"
    "max_time = 40\n"
    "############################################################\n"
    "[loads.building_electrical]\n"
    "time_unit = \"hours\"\n"
    "rate_unit = \"kW\"\n"
    "time_rate_pairs = [[0.0,1.0],[40.0,0.0]]\n"
    "############################################################\n"
    "[components.S]\n"
    "type = \"source\"\n"
    "output_stream = \"electricity\"\n"
    "failure_modes = [\"fm\"]\n"
    "[components.L]\n"
    "type = \"load\"\n"
    "input_stream = \"electricity\"\n"
    "loads_by_scenario.blue_sky = \"building_electrical\"\n"
    "############################################################\n"
    "[dist.every_10]\n"
    "type = \"fixed\"\n"
    "value = 10\n"
    "time_unit = \"hours\"\n"
    "[dist.every_5]\n"
    "type = \"fixed\"\n"
    "value = 5\n"
    "time_unit = \"hours\"\n"
    "############################################################\n"
    "[failure_mode.fm]\n"
    "failure_dist = \"every_10\"\n"
    "repair_dist = \"every_5\"\n"
    "############################################################\n"
    "[networks.nw]\n"
    "connections = [[\"S:OUT(0)\", \"L:IN(0)\", \"electricity\"]]\n"
    "############################################################\n"
    "[dist.immediately]\n"
    "type = \"fixed\"\n"
    "value = 0\n"
    "time_unit = \"hours\"\n"
    "############################################################\n"
    "[scenarios.blue_sky]\n"
    "time_unit = \"hours\"\n"
    "occurrence_distribution = \"immediately\"\n"
    "duration = 40\n"
    "max_occurrences = 1\n"
    "network = \"nw\"\n"
    "calculate_reliability = true\n";
  auto m = E::make_main_from_string(input);
  auto out = m.run_all();
  EXPECT_TRUE(out.get_is_good());
  auto results_map = out.get_results();
  ASSERT_EQ(1, results_map.size());
  const auto& bs_res = results_map["blue_sky"];
  ASSERT_EQ(1, bs_res.size());
  const auto& bs_res0 = bs_res[0];
  const auto& rez = bs_res0.get_results();
  std::set<std::string> expected_comp_ids{"L", "S"};
  ASSERT_EQ(expected_comp_ids.size(), rez.size());
  const auto& comp_ids = bs_res0.get_component_ids();
  std::set<std::string> actual_comp_ids{};
  for (const auto& id : comp_ids) {
    actual_comp_ids.emplace(id);
  }
  ASSERT_EQ(actual_comp_ids.size(), expected_comp_ids.size());
  EXPECT_EQ(actual_comp_ids, expected_comp_ids);
  auto ss_map = bs_res0.get_statistics();
  //0--10,15--25,30--40
  ERIN::RealTimeType L_max_downtime{5*3600};
  ERIN::FlowValueType L_load_not_served{10.0*3600.0*1.0};
  ERIN::FlowValueType L_total_energy{40.0*3600*1.0 - L_load_not_served};
  auto L_ss = ss_map["L"];
  EXPECT_EQ(L_ss.max_downtime, L_max_downtime);
  EXPECT_EQ(L_ss.load_not_served, L_load_not_served);
  EXPECT_EQ(L_ss.total_energy, L_total_energy);
}
*/

/*
// TODO: re-enable after we determine mux works right
TEST(ErinBasicsTest, Test_request_ports_intelligently) {
  namespace E = ERIN;
  namespace ED = erin::devs;
  auto inports = std::vector<ED::Port3>{
    ED::Port3{}, ED::Port3{}, ED::Port3{}
  };
  E::FlowValueType total_request{30.0};
  E::FlowValueType remaining_request{total_request};
  auto inports_returned = ED::request_inflows_intelligently(
      inports, remaining_request);
  ASSERT_EQ(inports_returned.size(), 3);
  EXPECT_EQ(inports_returned[0].port.get_requested(), 30.0);
  EXPECT_EQ(inports_returned[1].port.get_requested(), 0.0);
  EXPECT_EQ(inports_returned[2].port.get_requested(), 0.0);
  EXPECT_EQ(inports_returned[0].port.get_achieved(), 0.0);
  EXPECT_EQ(inports_returned[1].port.get_achieved(), 0.0);
  EXPECT_EQ(inports_returned[2].port.get_achieved(), 0.0);
  inports = std::vector<ED::Port3>{
    ED::Port3{30.0, 5.0},
    ED::Port3{25.0, 5.0},
    ED::Port3{20.0, 10.0}
  };
  remaining_request = 25.0;
  inports_returned = ED::request_inflows_intelligently(
      inports, remaining_request);
  ASSERT_EQ(inports_returned.size(), 3);
  EXPECT_EQ(inports_returned[0].port.get_requested(), 25.0);
  EXPECT_EQ(inports_returned[1].port.get_requested(), 20.0);
  EXPECT_EQ(inports_returned[2].port.get_requested(), 15.0);
  EXPECT_EQ(inports_returned[0].port.get_achieved(), 5.0);
  EXPECT_EQ(inports_returned[1].port.get_achieved(), 5.0);
  EXPECT_EQ(inports_returned[2].port.get_achieved(), 10.0);
}
*/

/*
TEST(ErinBasicsTest, Test_that_on_off_switch_carries_request_through_on_repair) {
  namespace E = ERIN;
  namespace ED = erin::devs;
  const std::vector<E::TimeState> time_states{{0, true}, {100, false}, {103, true}};
  // 1. request powerflow through at time 0 of 1 kW
  // 2. at time equals 100, the request from upstream should be 0 kW and downstream should be notified of 0 kW achieved
  // 3. at time equals 103, the request should return to 1 kW upstream and downstream should be notified of 1 kW achieved
  auto d = ED::make_on_off_switch_data(time_states);
  auto s0 = ED::make_on_off_switch_state(d);
  EXPECT_EQ(s0.time, 0);
  EXPECT_EQ(s0.state, true);
  auto dt0 = ED::on_off_switch_time_advance(d, s0);
  EXPECT_EQ(dt0, 100);
  auto s1 = ED::on_off_switch_external_transition(s0, 0, std::vector<E::PortValue>{{ED::inport_outflow_request, 1.0}});
  EXPECT_EQ(s1.time, 0);
  EXPECT_EQ(s1.state, true);
  auto dt1 = ED::on_off_switch_time_advance(d, s1);
  EXPECT_EQ(dt1, 0);
  auto ys0 = ED::on_off_switch_output_function(d, s1);
  ASSERT_EQ(ys0.size(), 1);
  EXPECT_EQ(ys0[0].port, ED::outport_inflow_request);
  EXPECT_EQ(ys0[0].value, 1.0);
  auto s2 = ED::on_off_switch_internal_transition(d, s1);
  EXPECT_EQ(s2.time, 0);
  EXPECT_EQ(s2.state, true);
  auto dt2 = ED::on_off_switch_time_advance(d, s2);
  EXPECT_EQ(dt2, 100);
  auto ys2 = ED::on_off_switch_output_function(d, s2);
  ASSERT_EQ(ys2.size(), 1);
  EXPECT_EQ(ys0[0].port, ED::outport_inflow_request);
  EXPECT_EQ(ys0[0].value, 1.0);
}
*/

TEST(ErinBasicsTest, Test_that_port2_works) {
  namespace ED = erin::devs;
  auto p = ED::Port2{};
  // R=10,A=5
  auto update = p.with_requested(10);
  update = update.port.with_achieved(5);
  EXPECT_TRUE(update.send_update);
  EXPECT_EQ(update.send_update, update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // R=5,A=6
  p = update.port;
  update = p.with_achieved(6).port.with_requested(5);
  EXPECT_FALSE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // R=6,A=4
  p = update.port;
  update = p.with_achieved(4).port.with_requested(6);
  EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // R=4;R=4,A=2
  p = update.port.with_requested(4).port;
  update = p.with_achieved(2).port.with_requested(4);
  EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // R=3,A=2
  p = update.port;
  update = p.with_achieved(2).port.with_requested(3);
  EXPECT_FALSE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // R=7,A=7
  p = ED::Port2{8,6};
  update = p.with_achieved(7).port.with_requested(7);
  EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // R=9,A=2
  p = ED::Port2{2,2};
  update = p.with_achieved(2).port.with_requested(9);
  EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // R=2,A=2
  p = ED::Port2{3,2};
  update = p.with_achieved(2).port.with_requested(3);
  EXPECT_FALSE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // {5,5} => A=2 => send A
  p = ED::Port2{5,5};
  update = p.with_achieved(2);
  EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  EXPECT_TRUE(update.send_update);
  // {5,4} => A=5 => send A
  p = ED::Port2{5,4};
  update = p.with_achieved(5);
  EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  EXPECT_TRUE(update.send_update);
  // {5,4} => R=4,A=5 => don't send A
  p = ED::Port2{5,4};
  update = p.with_achieved(5).port.with_requested(4);
  EXPECT_FALSE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // {5,4} => R=8,A=5 => send A
  p = ED::Port2{5,4};
  update = p.with_achieved(5).port.with_requested(8);
  EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  // R=4252.38,A=0
  p = ED::Port2{2952.38,855.556};
  update = p.with_achieved(0).port.with_requested(4252.38);
  EXPECT_TRUE(update.port.should_send_achieved(p))
    << "p: " << update.port << "pL: " << p;
}

/*
TEST(ErinBasicsTest, Test_energy_balance_on_converter) {
  namespace E = ERIN;
  namespace ED = erin::devs;
  auto s = ED::make_converter_state(0.5);
  std::default_random_engine generator{};
  std::uniform_int_distribution<int> request_dist(0, 100);
  std::uniform_int_distribution<int> achieved_diff_dist(1, 99);
  std::size_t num_times{1'000};
  double tol{1e-6};
  for (std::size_t idx{0}; idx < num_times; idx++) {
    E::RealTimeType e{0};
    auto out_req = static_cast<E::FlowValueType>(request_dist(generator));
    auto in_req = out_req * 2.0;
    auto waste_req = in_req - out_req;
    decltype(waste_req) loss_req = 0.0;
    auto diff = static_cast<E::FlowValueType>(achieved_diff_dist(generator));
    auto in_ach = (out_req * 2.0) - (diff - 50.0);
    if (in_ach < 0.0) {
      in_ach = 0.0;
    }
    auto out_ach = in_ach * 0.5;
    auto waste_ach = in_ach - out_ach;
    decltype(waste_ach) loss_ach = 0.0;
    // ... quiescent ...
    // xs -> ext transition, outflow request
    std::vector<ED::PortValue> xs{{ED::inport_outflow_request, out_req}};
    auto s1 = ED::converter_external_transition(s, e, xs);
    ASSERT_NEAR(s1.inflow_port.get_requested(), in_req, tol)
      << "idx: " << idx << "\n"
      << "s: " << s << "\n"
      << "s1: " << s1 << "\n"
      << "e: " << e << "\n"
      << "xs: " << E::vec_to_string<E::PortValue>(xs) << "\n"
      << "in_req: " << in_req << "\n";
    ASSERT_NEAR(s1.outflow_port.get_requested(), out_req, tol)
      << "idx: " << idx << "\n"
      << "s: " << s << "\n"
      << "s1: " << s1 << "\n"
      << "e: " << e << "\n"
      << "xs: " << E::vec_to_string<E::PortValue>(xs) << "\n"
      << "in_req: " << in_req << "\n";
    ASSERT_NEAR(s1.lossflow_port.get_requested(), 0.0, tol)
      << "idx: " << idx << "\n"
      << "s: " << s << "\n"
      << "s1: " << s1 << "\n"
      << "e: " << e << "\n"
      << "xs: " << E::vec_to_string<E::PortValue>(xs) << "\n"
      << "in_req: " << in_req << "\n";
    ASSERT_NEAR(s1.wasteflow_port.get_requested(), waste_req, tol)
      << "idx: " << idx << "\n"
      << "s: " << s << "\n"
      << "s1: " << s1 << "\n"
      << "e: " << e << "\n"
      << "xs: " << E::vec_to_string<E::PortValue>(xs) << "\n"
      << "in_req: " << in_req << "\n";
    // ta -> dt == 0
    auto dt = ED::converter_time_advance(s1);
    ASSERT_EQ(dt, (out_req > 0.0) ? 0 : ED::infinity)
      << "idx      = " << idx << "\n"
      << "out_req  = " << out_req << "\n"
      << "in_req   = " << in_req << "\n"
      << "in_ach   = " << in_ach << "\n"
      << "out_ach  = " << out_ach << "\n"
      << "loss_req = " << loss_req << "\n"
      << "loss_ach = " << loss_ach << "\n";
    if (dt == ED::infinity) {
      continue;
    }
    // output -> ys, inflow_request
    std::vector<ED::PortValue> expected_ys{{ED::outport_inflow_request, in_req}};
    auto ys = ED::converter_output_function(s1);
    ASSERT_EQ(ys.size(), 1)
      << "idx         = " << idx << "\n"
      << "ys          = " << E::vec_to_string<E::PortValue>(ys) << "\n"
      << "expected_ys = " << E::vec_to_string<E::PortValue>(expected_ys) << "\n"
      << "out_req     = " << out_req << "\n"
      << "in_req      = " << in_req << "\n"
      << "in_ach      = " << in_ach << "\n"
      << "out_ach     = " << out_ach << "\n"
      << "loss_req    = " << loss_req << "\n"
      << "loss_ach    = " << loss_ach << "\n";
    EXPECT_EQ(ys[0].port, expected_ys[0].port);
    EXPECT_NEAR(ys[0].value, expected_ys[0].value, tol);
    // internal transition
    auto s2 = ED::converter_internal_transition(s1);
    // ta -> dt == infinity
    auto dt2 = ED::converter_time_advance(s2);
    EXPECT_EQ(dt2, ED::infinity);
    // xs -> ext transition, inflow achieved
    std::vector<ED::PortValue> xs2{{ED::inport_inflow_achieved, in_ach}};
    auto s3 = ED::converter_external_transition(s2, e, xs2);
    // ta -> dt == 0
    auto dt3 = ED::converter_time_advance(s3);
    if (dt3 == ED::infinity) {
      continue;
    }
    // output -> ys, outflow_achieved
    auto ys3 = ED::converter_output_function(s3);
    ASSERT_EQ(ys3.size(), 1)
      << "idx         = " << idx << "\n"
      << "s3          = " << s3 << "\n"
      << "ys3         = " << E::vec_to_string<E::PortValue>(ys3) << "\n"
      << "in_req      = " << in_req << "\n"
      << "in_ach      = " << in_ach << "\n"
      << "out_req     = " << out_req << "\n"
      << "out_ach     = " << out_ach << "\n"
      << "loss_req    = " << loss_req << "\n"
      << "loss_ach    = " << loss_ach << "\n"
      << "waste_req   = " << waste_req << "\n"
      << "waste_ach   = " << waste_ach << "\n";
    if (in_ach > in_req) {
      ASSERT_EQ(ys3[0].port, ED::outport_inflow_request);
      ASSERT_NEAR(ys3[0].value, in_req, tol);
    }
    else if (in_ach <= in_req) {
      ASSERT_EQ(ys3[0].port, ED::outport_outflow_achieved);
      ASSERT_NEAR(ys3[0].value, out_ach, tol);
    }
    // internal transition
    auto s4 = ED::converter_internal_transition(s3);
    std::ostringstream oss{};
    oss << "idx: " << idx << "\n"
        << "s2: " << s2 << "\n"
        << "s3: " << s3 << "\n"
        << "in_req: " << in_req << "\n"
        << "in_ach: " << in_ach << "\n"
        << "out_req: " << out_req << "\n"
        << "out_ach: " << out_ach << "\n"
        << "loss_req: " << loss_req << "\n"
        << "loss_ach: " << loss_ach << "\n"
        << "waste_req: " << waste_req << "\n"
        << "waste_ach: " << waste_ach << "\n";
    ASSERT_NEAR(s4.inflow_port.get_requested(), in_req, tol) << oss.str();
    ASSERT_NEAR(s4.outflow_port.get_requested(), out_req, tol) << oss.str();
    ASSERT_NEAR(s4.lossflow_port.get_requested(), 0.0, tol) << oss.str();
    ASSERT_NEAR(s4.wasteflow_port.get_requested(), waste_req, tol) << oss.str();
    ASSERT_NEAR(s4.inflow_port.get_achieved(), in_ach, tol) << oss.str();
    ASSERT_NEAR(s4.outflow_port.get_achieved(), (in_ach > in_req) ? 0.0 : out_ach, tol) << oss.str();
    ASSERT_NEAR(s4.lossflow_port.get_achieved(), 0.0, tol) << oss.str();
    ASSERT_NEAR(s4.wasteflow_port.get_achieved(), (in_ach > in_req) ? in_ach : waste_ach, tol) << oss.str();
    // ta -> dt == infinity
    auto dt4 = ED::converter_time_advance(s4);
    ASSERT_EQ(dt4, ED::infinity);
  }
}
*/

/*
TEST(ErinBasicsTest, Test_energy_balance_on_flow_limits) {
  namespace E = ERIN;
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  E::FlowValueType upper_limit{10.0};
  auto s = ED::make_flow_limits_state(0.0, upper_limit);
  std::default_random_engine generator{};
  std::uniform_int_distribution<int> request_dist(0, 100);
  std::uniform_int_distribution<int> achieved_diff_dist(1, 99);
  std::size_t num_times{1000};
  double tol{1e-6};
  for (std::size_t idx{0}; idx < num_times; idx++) {
    E::RealTimeType e{0};
    auto out_req = static_cast<E::FlowValueType>(request_dist(generator));
    auto in_req = std::min(out_req, upper_limit);
    auto diff = static_cast<E::FlowValueType>(achieved_diff_dist(generator));
    auto in_ach = out_req - (diff - 50.0);
    if (in_ach < 0.0) {
      in_ach = 0.0;
    }
    auto out_ach = std::min(in_ach, std::min(upper_limit, in_req));
    // ... quiescent ...
    // xs -> ext transition, outflow request
    std::vector<ED::PortValue> xs{{ED::inport_outflow_request, out_req}};
    auto s1 = ED::flow_limits_external_transition(s, e, xs);
    EXPECT_NEAR(s1.inflow_port.get_requested(), in_req, tol);
    EXPECT_NEAR(s1.outflow_port.get_requested(), out_req, tol);
    // ta -> dt == 0
    auto dt = ED::flow_limits_time_advance(s1);
    EXPECT_EQ(dt, (out_req > 0.0) ? 0 : ED::infinity)
      << "idx      = " << idx << "\n"
      << "out_req  = " << out_req << "\n"
      << "in_req   = " << in_req << "\n"
      << "in_ach   = " << in_ach << "\n"
      << "out_ach  = " << out_ach << "\n";
    if (dt == ED::infinity) {
      continue;
    }
    // output -> ys, inflow_request
    std::vector<ED::PortValue> expected_ys{{ED::outport_inflow_request, in_req}};
    if (out_req > upper_limit) {
      expected_ys.emplace_back(E::PortValue{ED::outport_outflow_achieved, upper_limit});
    }
    auto ys = ED::flow_limits_output_function(s1);
    ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys, expected_ys, compare_ports))
      << "idx         = " << idx << "\n"
      << "ys          = " << E::vec_to_string<E::PortValue>(ys) << "\n"
      << "expected_ys = " << E::vec_to_string<E::PortValue>(expected_ys) << "\n"
      << "out_req     = " << out_req << "\n"
      << "in_req      = " << in_req << "\n"
      << "in_ach      = " << in_ach << "\n"
      << "out_ach     = " << out_ach << "\n";
    // internal transition
    auto s2 = ED::flow_limits_internal_transition(s1);
    // ta -> dt == infinity
    auto dt2 = ED::flow_limits_time_advance(s2);
    EXPECT_EQ(dt2, ED::infinity);
    // xs -> ext transition, inflow achieved
    std::vector<ED::PortValue> xs2{{ED::inport_inflow_achieved, in_ach}};
    auto s3 = ED::flow_limits_external_transition(s2, e, xs2);
    // ta -> dt == 0
    auto dt3 = ED::flow_limits_time_advance(s3);
    EXPECT_EQ(dt3, (in_req == in_ach) ? ED::infinity : 0);
    if (dt3 == ED::infinity) {
      continue;
    }
    // output -> ys, outflow_achieved
    auto ys3 = ED::flow_limits_output_function(s3);
    std::vector<ED::PortValue> expected_ys3{};
    if (in_ach > in_req) {
      expected_ys3.emplace_back(ED::PortValue{
          ED::outport_inflow_request, in_req});
    }
    if (in_ach < in_req) {
      expected_ys3.emplace_back(ED::PortValue{
          ED::outport_outflow_achieved, out_ach});
    }
    ASSERT_EQ(ys3.size(), expected_ys3.size())
      << "idx         = " << idx << "\n"
      << "ys          = " << E::vec_to_string<E::PortValue>(ys3) << "\n"
      << "out_req     = " << out_req << "\n"
      << "in_req      = " << in_req << "\n"
      << "in_ach      = " << in_ach << "\n"
      << "out_ach     = " << out_ach << "\n";
    ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<ED::PortValue>(
        ys3, expected_ys3, compare_ports))
      << "idx         = " << idx << "\n"
      << "ys          = " << E::vec_to_string<E::PortValue>(ys3) << "\n"
      << "expected_ys = " << E::vec_to_string<E::PortValue>(expected_ys3) << "\n"
      << "out_req     = " << out_req << "\n"
      << "in_req      = " << in_req << "\n"
      << "in_ach      = " << in_ach << "\n"
      << "out_ach     = " << out_ach << "\n";
    // internal transition
    auto s4 = ED::flow_limits_internal_transition(s3);
    ASSERT_NEAR(s4.inflow_port.get_requested(), in_req, tol);
    ASSERT_NEAR(s4.outflow_port.get_requested(), out_req, tol);
    ASSERT_NEAR(s4.inflow_port.get_achieved(), (in_ach > in_req) ? in_req : in_ach, tol);
    ASSERT_NEAR(s4.outflow_port.get_achieved(), out_ach, tol);
    // ta -> dt == infinity
    auto dt4 = ED::flow_limits_time_advance(s4);
    EXPECT_EQ(dt4, ED::infinity);
  }
}
*/

TEST(ErinBasicsTest, Test_driver_element_for_internal_transitions) {
  namespace E = ERIN;
  namespace EU = erin::utils;
  int outport{2000};
  int inport{0};
  auto d = E::Driver("driver", outport, inport, {0, 10, 200}, {10.0, 0.0, 20.0}, true);
  auto dt = d.ta().real;
  EXPECT_EQ(dt, 0);
  std::vector<E::PortValue> ys{};
  std::vector<E::PortValue> expected_ys{E::PortValue{outport, 10.0}};
  d.output_func(ys);
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<E::PortValue>(
        ys, expected_ys, compare_ports))
    << "ys          = " << E::vec_to_string<E::PortValue>(ys) << "\n"
    << "expected_ys = " << E::vec_to_string<E::PortValue>(expected_ys) << "\n";
  d.delta_int();
  dt = d.ta().real;
  EXPECT_EQ(dt, 10);
  ys.clear();
  expected_ys = std::vector<E::PortValue>{E::PortValue{outport, 0.0}};
  d.output_func(ys);
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<E::PortValue>(
        ys, expected_ys, compare_ports))
    << "ys          = " << E::vec_to_string<E::PortValue>(ys) << "\n"
    << "expected_ys = " << E::vec_to_string<E::PortValue>(expected_ys) << "\n";
  d.delta_int();
  dt = d.ta().real;
  EXPECT_EQ(dt, 190);
  ys.clear();
  expected_ys = std::vector<E::PortValue>{E::PortValue{outport, 20.0}};
  d.output_func(ys);
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<E::PortValue>(
        ys, expected_ys, compare_ports));
  d.delta_int();
  EXPECT_EQ(d.ta(), E::inf);
  auto times = d.get_times();
  auto flows = d.get_flows();
  ASSERT_EQ(times.size(), 3);
  ASSERT_EQ(flows.size(), 3);
  EXPECT_EQ(times[0], 0);
  EXPECT_EQ(times[1], 10);
  EXPECT_EQ(times[2], 200);
  EXPECT_EQ(flows[0], 10.0);
  EXPECT_EQ(flows[1], 0.0);
  EXPECT_EQ(flows[2], 20.0);
}

TEST(ErinBasicsTest, Test_driver_element_for_external_transitions) {
  namespace E = ERIN;
  namespace EU = erin::utils;
  int outport{0};
  int inport{1};
  auto d = E::Driver("driver", outport, inport, {0, 10, 200}, {10.0, 0.0, 20.0}, true);
  auto dt = d.ta().real;
  EXPECT_EQ(dt, 0);
  std::vector<E::PortValue> ys{};
  std::vector<E::PortValue> expected_ys{E::PortValue{outport, 10.0}};
  d.output_func(ys);
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<E::PortValue>(
        ys, expected_ys, compare_ports));
  ys.clear();
  d.delta_int();

  // first external event
  std::vector<E::PortValue> xs{E::PortValue{inport, 5.0}};
  d.delta_ext(E::Time{5, 1}, xs);

  dt = d.ta().real;
  EXPECT_EQ(dt, 5);
  d.output_func(ys);
  ys.clear();
  expected_ys = std::vector<E::PortValue>{E::PortValue{outport, 0.0}};
  d.output_func(ys);
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<E::PortValue>(
        ys, expected_ys, compare_ports));
  d.delta_int();

  // second external event
  xs = std::vector<E::PortValue>{E::PortValue{inport, 50.0}};
  d.delta_ext(E::Time{25, 1}, xs);

  dt = d.ta().real;
  EXPECT_EQ(dt, 0);
  ys.clear();
  expected_ys = std::vector<E::PortValue>{E::PortValue{outport, 0.0}};
  d.output_func(ys);
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<E::PortValue>(
        ys, expected_ys, compare_ports));
  d.delta_int();

  dt = d.ta().real;
  EXPECT_EQ(dt, 165);
  ys.clear();
  expected_ys = std::vector<E::PortValue>{E::PortValue{outport, 20.0}};
  d.output_func(ys);
  ASSERT_TRUE(
      EU::compare_vectors_unordered_with_fn<E::PortValue>(
        ys, expected_ys, compare_ports));
  d.delta_int();
  EXPECT_EQ(d.ta(), E::inf);

  auto times = d.get_times();
  auto flows = d.get_flows();
  ASSERT_EQ(times.size(), 5);
  ASSERT_EQ(flows.size(), 5);
  EXPECT_EQ(times[0], 0);
  EXPECT_EQ(times[1], 5);
  EXPECT_EQ(times[2], 10);
  EXPECT_EQ(times[3], 35);
  EXPECT_EQ(times[4], 200);
  EXPECT_EQ(flows[0], 10.0);
  EXPECT_EQ(flows[1], 5.0);
  EXPECT_EQ(flows[2], 0.0);
  EXPECT_EQ(flows[3], 0.0);
  EXPECT_EQ(flows[4], 20.0);
}

TEST(ErinBasicsTest, Test_driver_element_comprehensive) {
  namespace E = ERIN;
  namespace EU = erin::utils;
  std::default_random_engine generator{};
  std::uniform_int_distribution<int> dt_dist(0, 10);
  std::uniform_int_distribution<int> flow_dist(0, 100);

  // inflow and outflow from viewpoint of the drivers
  std::vector<E::RealTimeType> inflow_times{};
  std::vector<E::FlowValueType> inflow_requests{};
  std::vector<E::RealTimeType> outflow_times{};
  std::vector<E::FlowValueType> outflow_availables{};
  std::size_t num_events{100};
  E::RealTimeType t{0};
  for (std::size_t idx{0}; idx < num_events; ++idx) {
    t += static_cast<E::RealTimeType>(dt_dist(generator));
    inflow_times.emplace_back(t);
    inflow_requests.emplace_back(static_cast<E::FlowValueType>(flow_dist(generator)));
    t += static_cast<E::RealTimeType>(dt_dist(generator));
    outflow_times.emplace_back(t);
    outflow_availables.emplace_back(static_cast<E::FlowValueType>(flow_dist(generator)));
  }
  auto t_max{t};
  auto inflow_driver = new E::Driver{
    "inflow-driver",
    E::Driver::outport_inflow_request,
    E::Driver::inport_inflow_achieved,
    inflow_times,
    inflow_requests,
    true};
  auto outflow_driver = new E::Driver{
    "outflow-driver",
    E::Driver::outport_outflow_achieved,
    E::Driver::inport_outflow_request,
    outflow_times,
    outflow_availables,
    false};
  adevs::Digraph<E::FlowValueType, E::Time> network;
    network.couple(
        inflow_driver, E::Driver::outport_inflow_request,
        outflow_driver, E::Driver::inport_outflow_request);
    network.couple(
        outflow_driver, E::Driver::outport_outflow_achieved,
        inflow_driver, E::Driver::inport_inflow_achieved);
  adevs::Simulator<E::PortValue, E::Time> sim{};
  network.add(&sim);
  const std::size_t max_no_advance{num_events * 4};
  std::size_t non_advance_count{0};
  for (
      auto time = sim.now(), t_next = sim.next_event_time();
      ((t_next < E::inf) && (t_next.real <= t_max));
      sim.exec_next_event(), time = t_next, t_next = sim.next_event_time()) {
    if (t_next.real == time.real) {
      ++non_advance_count;
    }
    else {
      non_advance_count = 0;
    }
    if (non_advance_count >= max_no_advance) {
      std::ostringstream oss{};
      oss << "ERROR: non_advance_count > max_no_advance:\n"
          << "non_advance_count: " << non_advance_count << "\n"
          << "max_no_advance   : " << max_no_advance << "\n"
          << "time.real        : " << time.real << " seconds\n"
          << "time.logical     : " << time.logical << "\n";
      ASSERT_TRUE(false) << oss.str();
      break;
    }
  }
  auto inflow_ts = inflow_driver->get_times();
  auto inflow_fs = inflow_driver->get_flows();
  auto outflow_ts = outflow_driver->get_times();
  auto outflow_fs = outflow_driver->get_flows();
  ASSERT_EQ(inflow_ts.size(), inflow_fs.size());
  ASSERT_EQ(outflow_ts.size(), outflow_fs.size());
  ASSERT_EQ(inflow_ts.size(), outflow_ts.size());
  for (std::size_t idx{0}; idx < inflow_ts.size(); ++idx) {
    const auto& t = inflow_ts[idx];
    const auto& f = inflow_fs[idx];
    auto outflow = EU::interpolate_value(t, outflow_ts, outflow_fs);
    ASSERT_EQ(f, outflow)
      << "idx    = " << idx << "\n"
      << "t      = " << t << "\n"
      << "inflow = " << f << "\n"
      << "outflow= " << outflow << "\n";
  }
}

TEST(ErinBasicsTest, Test_interpolate_value) {
  namespace E = ERIN;
  namespace EU = erin::utils;
  // #1
  std::vector<E::RealTimeType> ts{0, 5, 10, 15};
  std::vector<E::FlowValueType> fs{10.0, 20.0, 30.0, 40.0};
  E::RealTimeType t{2};
  auto f = EU::interpolate_value(t, ts, fs);
  E::FlowValueType expected_f{10.0};
  EXPECT_EQ(f, expected_f);
  // #2
  t = 0;
  f = EU::interpolate_value(t, ts, fs);
  expected_f = 10.0;
  EXPECT_EQ(f, expected_f);
  // #3
  t = 5;
  f = EU::interpolate_value(t, ts, fs);
  expected_f = 20.0;
  EXPECT_EQ(f, expected_f);
  // #4
  t = 20;
  f = EU::interpolate_value(t, ts, fs);
  expected_f = 40.0;
  EXPECT_EQ(f, expected_f);
  // #5
  ts = std::vector<E::RealTimeType>{5, 10, 15};
  fs = std::vector<E::FlowValueType>{20.0, 30.0, 40.0};
  t = 2;
  f = EU::interpolate_value(t, ts, fs);
  expected_f = 0.0;
  EXPECT_EQ(f, expected_f);
}

TEST(ErinBasicsTest, Test_integrate_value) {
  namespace E = ERIN;
  namespace EU = erin::utils;
  // #1
  std::vector<E::RealTimeType> ts{0, 5, 10, 15};
  std::vector<E::FlowValueType> fs{10.0, 20.0, 30.0, 40.0};
  E::RealTimeType t{2};
  auto g = EU::integrate_value(t, ts, fs);
  E::FlowValueType expected_g{20.0};
  EXPECT_EQ(g, expected_g);
  // #2
  t = 0;
  g = EU::integrate_value(t, ts, fs);
  expected_g = 0.0;
  EXPECT_EQ(g, expected_g);
  // #3
  t = 5;
  g = EU::integrate_value(t, ts, fs);
  expected_g = 50.0;
  EXPECT_EQ(g, expected_g);
  // #4
  t = 20;
  g = EU::integrate_value(t, ts, fs);
  expected_g = 500.0; // 50.0 + 100.0 + 150.0 + 200.0
  EXPECT_EQ(g, expected_g);
  // #5
  ts = std::vector<E::RealTimeType>{5, 10, 15};
  fs = std::vector<E::FlowValueType>{20.0, 30.0, 40.0};
  t = 2;
  g = EU::integrate_value(t, ts, fs);
  expected_g = 0.0;
  EXPECT_EQ(g, expected_g);
}

TEST(ErinBasicsTest, Test_store_element_comprehensive) {
  namespace E = ERIN;
  namespace EU = erin::utils;

  E::FlowValueType capacity{100.0};
  E::FlowValueType max_charge_rate{10.0};
  std::size_t num_events{1000};

  std::string id{"store"};
  std::string stream_type{"electricity"};
  auto c = new E::Storage(
      id,
      E::ComponentType::Storage,
      stream_type,
      capacity,
      max_charge_rate);
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
  c->set_flow_writer(fw);

  std::default_random_engine generator{};
  std::uniform_int_distribution<int> dt_dist(0, 10);
  std::uniform_int_distribution<int> flow_dist(0, 100);

  // inflow and outflow from viewpoint of the component
  std::vector<E::RealTimeType> inflow_times{};
  std::vector<E::FlowValueType> inflow_achieveds{};
  std::vector<E::RealTimeType> outflow_times{};
  std::vector<E::FlowValueType> outflow_requests{};
  E::RealTimeType t{0};
  for (std::size_t idx{0}; idx < num_events; ++idx) {
    t += static_cast<E::RealTimeType>(dt_dist(generator));
    outflow_times.emplace_back(t);
    outflow_requests.emplace_back(static_cast<E::FlowValueType>(flow_dist(generator)));
    t += static_cast<E::RealTimeType>(dt_dist(generator));
    inflow_times.emplace_back(t);
    inflow_achieveds.emplace_back(static_cast<E::FlowValueType>(flow_dist(generator)));
  }
  auto t_max{t};
  auto inflow_driver = new E::Driver{
    "inflow-to-store",
    E::Driver::outport_outflow_achieved,
    E::Driver::inport_outflow_request,
    inflow_times,
    inflow_achieveds,
    false};
  auto outflow_driver = new E::Driver{
    "outflow-from-store",
    E::Driver::outport_inflow_request,
    E::Driver::inport_inflow_achieved,
    outflow_times,
    outflow_requests,
    true};
  adevs::Digraph<E::FlowValueType, E::Time> network;
  network.couple(
      outflow_driver, E::Driver::outport_inflow_request,
      c, E::Driver::inport_outflow_request);
  network.couple(
      c, E::Driver::outport_inflow_request,
      inflow_driver, E::Driver::inport_outflow_request);
  network.couple(
      inflow_driver, E::Driver::outport_outflow_achieved,
      c, E::Driver::inport_inflow_achieved);
  network.couple(
      c, E::Driver::outport_outflow_achieved,
      outflow_driver, E::Driver::inport_inflow_achieved);
  c->set_recording_on();
  adevs::Simulator<E::PortValue, E::Time> sim{};
  network.add(&sim);
  const std::size_t max_no_advance{num_events * 4};
  std::size_t non_advance_count{0};
  for (
      auto time = sim.now(), t_next = sim.next_event_time();
      ((t_next < E::inf) && (t_next.real <= t_max));
      sim.exec_next_event(), time = t_next, t_next = sim.next_event_time()) {
    if (t_next.real == time.real) {
      ++non_advance_count;
    }
    else {
      non_advance_count = 0;
    }
    if (non_advance_count >= max_no_advance) {
      std::ostringstream oss{};
      oss << "ERROR: non_advance_count > max_no_advance:\n"
          << "non_advance_count: " << non_advance_count << "\n"
          << "max_no_advance   : " << max_no_advance << "\n"
          << "time.real        : " << time.real << " seconds\n"
          << "time.logical     : " << time.logical << "\n";
      ASSERT_TRUE(false) << oss.str();
      break;
    }
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  EXPECT_EQ(results.size(), 4);
  auto inflow_results = results[id + "-inflow"];
  auto outflow_results = results[id + "-outflow"];
  auto storeflow_results = results[id + "-storeflow"];
  auto discharge_results = results[id + "-discharge"];
  auto inflow_ts = inflow_driver->get_times();
  auto inflow_fs = inflow_driver->get_flows();
  auto outflow_ts = outflow_driver->get_times();
  auto outflow_fs = outflow_driver->get_flows();
  std::size_t last_idx{outflow_results.size() - 1};
  // Note: on the last index, the finalize_at_time(.) method of FlowWriter sets
  // the flows to 0 which causes a discrepancy with the drivers that need not
  // be tested. Therefore, we only go until prior to the last index.
  for (std::size_t idx{0}; idx < last_idx; ++idx) {
    std::ostringstream oss{};
    oss << "idx            : " << idx << "\n";
    const auto& outf_res = outflow_results[idx];
    const auto& time = outf_res.time;
    oss << "time           : " << time << "\n";
    const auto outflow_d{EU::interpolate_value(time, outflow_ts, outflow_fs)};
    oss << "outflow_results : " << outf_res << "\n";
    oss << "outflow_driver  : " << outflow_d << "\n";
    ASSERT_EQ(outf_res.achieved_value, outflow_d) << oss.str();
    const auto& inf_res = inflow_results[idx]; 
    const auto inflow_d{EU::interpolate_value(time, inflow_ts, inflow_fs)};
    oss << "inflow_results: " << inf_res << "\n";
    oss << "inflow_driver : " << inflow_d << "\n";
    ASSERT_EQ(inf_res.achieved_value, inflow_d) << oss.str();
    const auto& str_res = storeflow_results[idx];
    const auto& dis_res = discharge_results[idx];
    auto error{
      inf_res.achieved_value +
      dis_res.achieved_value -
      (str_res.achieved_value + outf_res.achieved_value)};
    oss << "storeflow      : " << str_res << "\n"
        << "discharge      : " << dis_res << "\n"
        << "Energy Balance : " << error << "\n";
    ASSERT_NEAR(error, 0.0, 1e-6) << oss.str();
    const auto e_inflow = EU::integrate_value(time, inflow_results);
    const auto e_outflow = EU::integrate_value(time, outflow_results);
    const auto e_inflow_d = EU::integrate_value(time, inflow_ts, inflow_fs);
    const auto e_outflow_d = EU::integrate_value(time, outflow_ts, outflow_fs);
    oss << "E_inflow       : " << e_inflow << "\n"
        << "E_inflow (drive: " << e_inflow_d << "\n"
        << "E_outflow      : " << e_outflow << "\n"
        << "E_outflow (driv: " << e_outflow_d << "\n";
    ASSERT_NEAR(e_inflow, e_inflow_d, 1e-6) << oss.str();
    ASSERT_NEAR(e_outflow, e_outflow_d, 1e-6) << oss.str();
  }
}

TEST(ErinBasicsTest, Test_converter_element_comprehensive) {
  namespace E = ERIN;
  namespace EU = erin::utils;
  namespace ED = erin::devs;

  const bool do_rounding{false};
  const E::FlowValueType constant_efficiency{0.4};
  const std::size_t num_events{10'000};
  const bool has_flow_limit{true};
  const E::FlowValueType flow_limit{60.0};

  auto calc_output_from_input = [constant_efficiency, do_rounding](E::FlowValueType inflow) -> E::FlowValueType {
    auto out{inflow * constant_efficiency};
    if (do_rounding)
      return std::round(out * 1e6) / 1e6;
    return out;
  };
  auto calc_input_from_output = [constant_efficiency, do_rounding](E::FlowValueType outflow) -> E::FlowValueType {
    auto out{outflow / constant_efficiency};
    if (do_rounding) {
      return std::round(out * 1e6) / 1e6;
    }
    return out;
  };
  std::string id{"conv"};
  std::string src_id{"inflow_at_source"};
  std::string sink_out_id{"outflow_at_load"};
  std::string sink_loss_id{"lossflow_at_load"};
  std::string outflow_stream{"electricity"};
  std::string inflow_stream{"diesel_fuel"};
  std::string lossflow_stream{"waste_heat"};
  auto c = new E::Converter(
          id,
          E::ComponentType::Converter,
          inflow_stream,
          outflow_stream,
          calc_output_from_input,
          calc_input_from_output,
          lossflow_stream);
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
  c->set_flow_writer(fw);
  c->set_recording_on();

  std::default_random_engine generator{};
  std::uniform_int_distribution<int> dt_dist(0, 10);
  std::uniform_int_distribution<int> flow_dist(0, 100);

  // inflow and outflow from viewpoint of the component
  std::vector<E::RealTimeType> times{};
  std::vector<E::FlowValueType> flows_src_to_conv_req{};
  std::vector<E::FlowValueType> flows_src_to_conv_ach{};
  std::vector<E::FlowValueType> flows_conv_to_out_req{};
  std::vector<E::FlowValueType> flows_conv_to_out_ach{};
  std::vector<E::FlowValueType> flows_conv_to_loss_req{};
  std::vector<E::FlowValueType> flows_conv_to_loss_ach{};
  std::vector<E::LoadItem> lossflow_load_profile{};
  std::vector<E::LoadItem> outflow_load_profile{};

  E::RealTimeType t{0};
  E::FlowValueType lossflow_r{0.0};
  for (std::size_t idx{0}; idx < num_events; ++idx) {
    auto dt{static_cast<E::RealTimeType>(dt_dist(generator))};
    auto dt2{static_cast<E::RealTimeType>(dt_dist(generator))};
    auto outflow_r{static_cast<E::FlowValueType>(flow_dist(generator))};
    outflow_load_profile.emplace_back(E::LoadItem{t, outflow_r});
    auto inflow_r{calc_input_from_output(outflow_r)};
    auto inflow_a{inflow_r};
    auto outflow_a{calc_output_from_input(inflow_a)};
    if (dt > 0) {
      times.emplace_back(t);
      flows_conv_to_out_req.emplace_back(outflow_r);
      flows_src_to_conv_req.emplace_back(inflow_r);
      if (has_flow_limit) {
        inflow_a = std::min(flow_limit, inflow_r);
        outflow_a = calc_output_from_input(inflow_a);
      }
      flows_src_to_conv_ach.emplace_back(inflow_a);
      flows_conv_to_out_ach.emplace_back(outflow_a);
      flows_conv_to_loss_req.emplace_back(lossflow_r);
      flows_conv_to_loss_ach.emplace_back(
          std::min(lossflow_r, inflow_a - outflow_a));
    }
    t += dt;
    lossflow_r = static_cast<E::FlowValueType>(flow_dist(generator));
    lossflow_load_profile.emplace_back(
      E::LoadItem{t, lossflow_r});
    if (dt2 > 0) {
      times.emplace_back(t);
      flows_conv_to_out_req.emplace_back(outflow_r);
      auto inflow_r{calc_input_from_output(outflow_r)};
      flows_src_to_conv_req.emplace_back(inflow_r);
      auto inflow_a{inflow_r};
      if (has_flow_limit) {
        inflow_a = std::min(flow_limit, inflow_r);
        outflow_a = calc_output_from_input(inflow_a);
      }
      flows_src_to_conv_ach.emplace_back(inflow_a);
      flows_conv_to_out_ach.emplace_back(outflow_a);
      flows_conv_to_loss_req.emplace_back(lossflow_r);
      flows_conv_to_loss_ach.emplace_back(
          std::min(lossflow_r, inflow_a - outflow_a));
    }
    t += dt2;
  }
  auto t_max{times.back()};
  flows_src_to_conv_req.back() = 0.0;
  flows_src_to_conv_ach.back() = 0.0;
  flows_conv_to_out_req.back() = 0.0;
  flows_conv_to_out_ach.back() = 0.0;
  flows_conv_to_loss_req.back() = 0.0;
  flows_conv_to_loss_ach.back() = 0.0;
  ASSERT_EQ(flows_src_to_conv_req.size(), times.size());
  ASSERT_EQ(flows_src_to_conv_ach.size(), times.size());
  ASSERT_EQ(flows_conv_to_out_req.size(), times.size());
  ASSERT_EQ(flows_conv_to_out_ach.size(), times.size());
  ASSERT_EQ(flows_conv_to_loss_req.size(), times.size());
  ASSERT_EQ(flows_conv_to_loss_ach.size(), times.size());
  auto inflow_driver = new E::Source(
      src_id,
      E::ComponentType::Source,
      inflow_stream,
      has_flow_limit ? flow_limit : ED::supply_unlimited_value);
  inflow_driver->set_flow_writer(fw);
  inflow_driver->set_recording_on();
  auto lossflow_driver = new E::Sink(
      sink_loss_id,
      E::ComponentType::Load,
      lossflow_stream,
      lossflow_load_profile,
      false);
  lossflow_driver->set_flow_writer(fw);
  lossflow_driver->set_recording_on();
  auto outflow_driver = new E::Sink{
      sink_out_id,
      E::ComponentType::Load,
      outflow_stream,
      outflow_load_profile,
      false};
  outflow_driver->set_flow_writer(fw);
  outflow_driver->set_recording_on();
  adevs::Digraph<E::FlowValueType, E::Time> network{};
  network.couple(
      outflow_driver, E::Sink::outport_inflow_request,
      c, E::Converter::inport_outflow_request);
  network.couple(
      lossflow_driver, E::Sink::outport_inflow_request,
      c, E::Converter::inport_outflow_request + 1);
  network.couple(
      c, E::Converter::outport_inflow_request,
      inflow_driver, E::Source::inport_outflow_request);
  network.couple(
      inflow_driver, E::Source::outport_outflow_achieved,
      c, E::Converter::inport_inflow_achieved);
  network.couple(
      c, E::Converter::outport_outflow_achieved,
      outflow_driver, E::Driver::inport_inflow_achieved);
  network.couple(
      c, E::Converter::outport_outflow_achieved + 1,
      lossflow_driver, E::Converter::inport_inflow_achieved);
  adevs::Simulator<E::PortValue, E::Time> sim{};
  network.add(&sim);
  const std::size_t max_no_advance{num_events * 4};
  std::size_t non_advance_count{0};
  for (
      auto time = sim.now(), t_next = sim.next_event_time();
      ((t_next < E::inf) && (t_next.real <= t_max));
      sim.exec_next_event(), time = t_next, t_next = sim.next_event_time()) {
    if (t_next.real == time.real) {
      ++non_advance_count;
    }
    else {
      non_advance_count = 0;
    }
    if (non_advance_count >= max_no_advance) {
      std::ostringstream oss{};
      oss << "ERROR: non_advance_count > max_no_advance:\n"
          << "non_advance_count: " << non_advance_count << "\n"
          << "max_no_advance   : " << max_no_advance << "\n"
          << "time.real        : " << time.real << " seconds\n"
          << "time.logical     : " << time.logical << "\n";
      ASSERT_TRUE(false) << oss.str();
      break;
    }
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  //auto inflow_results = results[id + "-inflow"];
  //auto outflow_results = results[id + "-outflow"];
  //auto lossflow_results = results[id + "-lossflow"];
  //auto wasteflow_results = results[id + "-wasteflow"];
  // REQUESTED FLOWS
  // sinks/sources
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_src_to_conv_req, src_id, true));
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_conv_to_out_req, sink_out_id, true));
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_conv_to_loss_req, sink_loss_id, true));
  // converter
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_src_to_conv_req, id + "-inflow", true));
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_conv_to_out_req, id + "-outflow", true));
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_conv_to_loss_req, id + "-lossflow", true));
  // ACHIEVED FLOWS
  // sinks/sources
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_src_to_conv_ach, src_id, false));
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_conv_to_out_ach, sink_out_id, false));
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_conv_to_loss_ach, sink_loss_id, false));
  // converter
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_src_to_conv_ach, id + "-inflow", false));
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_conv_to_out_ach, id + "-outflow", false));
  ASSERT_TRUE(
      check_times_and_loads(results, times, flows_conv_to_loss_ach, id + "-lossflow", false));
  for (std::size_t idx{0}; idx < results[id + "-inflow"].size(); ++idx) {
    const auto& inflow = results[id + "-inflow"][idx].achieved_value;
    const auto& outflow = results[id + "-outflow"][idx].achieved_value;
    const auto& lossflow = results[id + "-lossflow"][idx].achieved_value;
    const auto& wasteflow = results[id + "-wasteflow"][idx].achieved_value;
    auto error{inflow - (outflow + lossflow + wasteflow)};
    EXPECT_TRUE(std::abs(error) < 1e-6)
      << "idx:       " << idx << "\n"
      << "inflow:    " << inflow << "\n"
      << "outflow:   " << outflow << "\n"
      << "lossflow:  " << lossflow << "\n"
      << "wasteflow: " << wasteflow << "\n"
      << "error:     " << error << "\n";
  }
}

TEST(ErinBasicsTest, Test_mux_element_comprehensive) {
  namespace E = ERIN;
  namespace EU = erin::utils;
  namespace ED = erin::devs;

  const int num_inflows{3};
  const int num_outflows{3};
  const auto output_dispatch_strategy = E::MuxerDispatchStrategy::InOrder;
  const std::size_t num_events{1'000};
  const bool use_limited_source{true};
  const E::FlowValueType source_limit{20.0};

  std::string id{"mux"};
  std::string stream{"electricity"};
  auto c = new E::Mux(
      id,
      E::ComponentType::Muxer,
      stream,
      num_inflows,
      num_outflows,
      output_dispatch_strategy);
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
  c->set_flow_writer(fw);
  c->set_recording_on();

  std::default_random_engine generator{};
  std::uniform_int_distribution<int> dt_dist(0, 10);
  std::uniform_int_distribution<int> flow_dist(0, 100);

  // std::vector<E::LoadItem> load_profile{};
  // inflow and outflow from viewpoint of the mux component
  std::vector<std::vector<E::RealTimeType>> outflow_times(num_outflows);
  std::vector<std::vector<E::FlowValueType>> outflow_requests(num_outflows);
  std::vector<std::vector<E::LoadItem>> outflow_load_profiles(num_outflows);
  E::RealTimeType t{0};
  E::RealTimeType t_max{0};
  E::FlowValueType v{0.0};
  for (std::size_t outport_id{0}; outport_id < static_cast<std::size_t>(num_outflows); ++outport_id) {
    t = 0;
    for (std::size_t idx{0}; idx < num_events; ++idx) {
      t += static_cast<E::RealTimeType>(dt_dist(generator));
      v = static_cast<E::FlowValueType>(flow_dist(generator));
      outflow_times[outport_id].emplace_back(t);
      outflow_requests[outport_id].emplace_back(v);
      outflow_load_profiles[outport_id].emplace_back(E::LoadItem{t, v});
    }
    t_max = std::max(t, t_max);
  }
  adevs::Digraph<E::FlowValueType, E::Time> network;
  std::vector<E::Sink*> outflow_drivers{};
  for (std::size_t outport_id{0}; outport_id < num_outflows; ++outport_id) {
    auto d = new E::Sink{
      "outflow-from-mux(" + std::to_string(outport_id) + ")",
      E::ComponentType::Load,
      stream,
      outflow_load_profiles[outport_id],
      false};
    d->set_flow_writer(fw);
    d->set_recording_on();
    outflow_drivers.emplace_back(d);
    network.couple(
        d, E::Sink::outport_inflow_request,
        c, E::Mux::inport_outflow_request + outport_id);
    network.couple(
        c, E::Mux::outport_outflow_achieved + outport_id,
        d, E::Sink::inport_inflow_achieved);
  }
  std::vector<E::Source*> inflow_drivers{};
  for (std::size_t inport_id{0}; inport_id < num_inflows; ++inport_id) {
    auto d = new E::Source{
      "inflow-to-mux(" + std::to_string(inport_id) + ")",
      E::ComponentType::Source,
      stream,
      use_limited_source ? source_limit : ED::supply_unlimited_value};
    d->set_flow_writer(fw);
    d->set_recording_on();
    inflow_drivers.emplace_back(d);
    network.couple(
        c, E::Mux::outport_inflow_request + inport_id,
        d, E::Source::inport_outflow_request);
    network.couple(
        d, E::Source::outport_outflow_achieved,
        c, E::Mux::inport_inflow_achieved + inport_id);
  }
  adevs::Simulator<E::PortValue, E::Time> sim{};
  network.add(&sim);
  const std::size_t max_no_advance{num_events * 4};
  std::size_t non_advance_count{0};
  for (
      auto time = sim.now(), t_next = sim.next_event_time();
      ((t_next < E::inf) && (t_next.real <= t_max));
      sim.exec_next_event(), time = t_next, t_next = sim.next_event_time()) {
    if (t_next.real == time.real) {
      ++non_advance_count;
    }
    else {
      non_advance_count = 0;
    }
    if (non_advance_count >= max_no_advance) {
      std::ostringstream oss{};
      oss << "ERROR: non_advance_count > max_no_advance:\n"
          << "non_advance_count: " << non_advance_count << "\n"
          << "max_no_advance   : " << max_no_advance << "\n"
          << "time.real        : " << time.real << " seconds\n"
          << "time.logical     : " << time.logical << "\n";
      ASSERT_TRUE(false) << oss.str();
      break;
    }
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();
  ASSERT_EQ(results.size(), static_cast<std::size_t>((num_inflows + num_outflows) * 2));
  std::vector<std::vector<E::Datum>> inflow_results(num_inflows);
  std::vector<std::vector<E::Datum>> outflow_results(num_outflows);
  std::vector<std::vector<E::RealTimeType>> inflow_tss(num_inflows);
  std::vector<std::vector<E::FlowValueType>> inflow_fss(num_inflows);
  std::vector<std::vector<E::FlowValueType>> inflow_fss_req(num_inflows);
  std::vector<std::vector<E::RealTimeType>> outflow_tss(num_outflows);
  std::vector<std::vector<E::FlowValueType>> outflow_fss(num_outflows);
  std::vector<std::vector<E::FlowValueType>> outflow_fss_req(num_outflows);
  for (std::size_t outport_id{0}; outport_id < num_outflows; ++outport_id) {
    outflow_results[outport_id] = results[id + "-outflow(" + std::to_string(outport_id) + ")"];
    for (const auto& data : results["outflow-from-mux(" + std::to_string(outport_id) + ")"]) {
      outflow_tss[outport_id].emplace_back(data.time);
      outflow_fss[outport_id].emplace_back(data.achieved_value);
      outflow_fss_req[outport_id].emplace_back(data.requested_value);
    }
  }
  for (std::size_t inport_id{0}; inport_id < num_inflows; ++inport_id) {
    inflow_results[inport_id] = results[id + "-inflow(" + std::to_string(inport_id) + ")"];
    for (const auto& data : results["inflow-to-mux(" + std::to_string(inport_id) + ")"]) {
      inflow_tss[inport_id].emplace_back(data.time);
      inflow_fss[inport_id].emplace_back(data.achieved_value);
      inflow_fss_req[inport_id].emplace_back(data.requested_value);
    }
  }
  E::RealTimeType time{0};
  for (std::size_t idx{0}; idx < (inflow_results[0].size() - 1); ++idx) {
    std::ostringstream oss{};
    oss << "idx            : " << idx << "\n";
    E::FlowValueType mux_reported_inflow{0.0};
    E::FlowValueType driver_reported_inflow{0.0};
    E::FlowValueType mux_reported_outflow{0.0};
    E::FlowValueType driver_reported_outflow{0.0};
    time = outflow_results[0][idx].time;
    oss << "time           : " << time << "\n";
    for (std::size_t outport_id{0}; outport_id < num_outflows; ++outport_id) {
      ASSERT_EQ(time, outflow_results[outport_id][idx].time) << oss.str();
      auto mux_outflow{outflow_results[outport_id][idx].achieved_value};
      mux_reported_outflow += mux_outflow;
      auto driver_outflow{
        EU::interpolate_value(
            time,
            outflow_tss[outport_id],
            outflow_fss[outport_id])};
      driver_reported_outflow += driver_outflow;
      ASSERT_EQ(mux_outflow, driver_outflow)
        << oss.str()
        << "outport_id = " << outport_id << "\n"
        << "mux_outflow = " << mux_outflow << "\n"
        << "driver_outflow = " << driver_outflow << "\n"
        << "outflow_tss[outport_id] = " << E::vec_to_string<E::RealTimeType>(outflow_tss[outport_id], 20) << "\n"
        << "outflow_fss[outport_id] = " << E::vec_to_string<E::FlowValueType>(outflow_fss[outport_id], 20) << "\n";
    }
    oss << "mux_reported_outflow = " << mux_reported_outflow << "\n"
        << "driver_reported_outflow = " << driver_reported_outflow << "\n";
    ASSERT_EQ(mux_reported_outflow, driver_reported_outflow) << oss.str();
    for (std::size_t inport_id{0}; inport_id < num_inflows; ++inport_id) {
      ASSERT_EQ(time, inflow_results[inport_id][idx].time) << oss.str();
      auto mux_inflow{inflow_results[inport_id][idx].achieved_value};
      mux_reported_inflow += mux_inflow;
      auto driver_inflow{
        EU::interpolate_value(
            time,
            inflow_tss[inport_id],
            inflow_fss[inport_id])};
      driver_reported_inflow += driver_inflow;
      ASSERT_EQ(mux_inflow, driver_inflow)
        << oss.str()
        << "inport_id = " << inport_id << "\n"
        << "mux_inflow = " << mux_inflow << "\n"
        << "driver_inflow = " << driver_inflow << "\n";
    }
    oss << "mux_reported_inflow = " << mux_reported_inflow << "\n"
        << "driver_reported_inflow = " << driver_reported_inflow << "\n";
    ASSERT_EQ(mux_reported_inflow, driver_reported_inflow) << oss.str();
    auto error{mux_reported_inflow - mux_reported_outflow};
    ASSERT_NEAR(error, 0.0, 1e-6) << oss.str();
  }
}

TEST(ErinBasicsTest, Test_Port3) {
  namespace ED = erin::devs;

  auto p = ED::Port3{};
  ED::FlowValueType r{10.0};
  ED::FlowValueType a{10.0};
  ED::FlowValueType available{40.0};
  auto update = p.with_requested(r);
  auto expected_update = ED::PortUpdate3{
    ED::Port3{r,0.0},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
  p = update.port;
  update = p.with_achieved(a);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    false,
    true,
  };
  ASSERT_EQ(update, expected_update);
  r = 20.0;
  p = update.port;
  update = p.with_requested(r);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
  p = update.port;
  update = p.with_achieved(a);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    false,
    false,
  };
  r = 5.0;
  p = update.port; 
  update = p.with_requested(r);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
  a = 20.0;
  p = update.port;
  update = p.with_achieved(a);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
  a = 5.0;
  p = update.port;
  update = p.with_achieved(a);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    false,
    true,
  };
  ASSERT_EQ(update, expected_update);
  r = 20.0;
  p = update.port; 
  update = p.with_requested(r);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
  a = 10.0;
  p = update.port; 
  update = p.with_achieved(a);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    false,
    true,
  };
  ASSERT_EQ(update, expected_update);
  a = 20.0;
  p = update.port; 
  update = p.with_achieved(a);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    false,
    true,
  };
  ASSERT_EQ(update, expected_update);
  r = 8.0;
  p = update.port;
  update = p.with_requested(r);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
  a = 15.0;
  p = update.port; 
  update = p.with_achieved(a);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
  a = 8.0;
  p = update.port; 
  update = p.with_achieved(a);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    false,
    true,
  };
  ASSERT_EQ(update, expected_update);
  r = 10.0;
  p = update.port;
  update = p.with_requested_and_available(r, available);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, r},
    true,
    true,
  };
  ASSERT_EQ(update, expected_update);
  r = 50.0;
  p = update.port;
  update = p.with_requested_and_available(r, available);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, available},
    true,
    true,
  };
  ASSERT_EQ(update, expected_update);
  r = 40.0;
  p = update.port;
  update = p.with_requested_and_available(r, available);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, r},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
  r = 30.0;
  a = 35.0;
  p = update.port;
  update = p.with_requested_and_achieved(r, a);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
  r = 35.0;
  p = update.port;
  update = p.with_requested(r);
  expected_update = ED::PortUpdate3{
    ED::Port3{r, a},
    true,
    false,
  };
  ASSERT_EQ(update, expected_update);
}

TEST(ErinBasicsTest, Test_new_port_scheme) {
  namespace ED = erin::devs;

  constexpr std::size_t num_events{10'000};
  constexpr double efficiency{0.5};
  constexpr int flow_max{100};

  std::default_random_engine generator{};
  std::uniform_int_distribution<int> flow_dist(0, flow_max);

  auto pout = ED::Port3{};
  auto ploss = ED::Port3{};
  auto pwaste = ED::Port3{};
  auto pin = ED::Port3{};
  auto outflow = ED::Port3{};
  auto inflow = ED::Port3{};
  auto lossflow = ED::Port3{};

  for (std::size_t idx{0}; idx < num_events; ++idx) {
    double max_inflow{static_cast<ED::FlowValueType>(flow_dist(generator))};
    double outflow_req{static_cast<ED::FlowValueType>(flow_dist(generator))};
    double lossflow_req{static_cast<ED::FlowValueType>(flow_dist(generator))};
    auto outflow_update = outflow.with_requested(outflow_req);
    outflow = outflow_update.port;
    auto lossflow_update = lossflow.with_requested(lossflow_req);
    lossflow = lossflow_update.port;
    auto inflow_update = inflow.with_achieved(std::min(max_inflow, inflow.get_requested()));
    inflow = inflow_update.port;
    while (outflow_update.send_request || lossflow_update.send_request || inflow_update.send_achieved) {
      bool resend_inflow_request{false};
      if (outflow_update.send_request) {
        pout = pout.with_requested(outflow.get_requested()).port;
      }
      if (lossflow_update.send_request) {
        ploss = ploss.with_requested(lossflow.get_requested()).port;
      }
      if (inflow_update.send_achieved) {
        auto pin_update = pin.with_achieved(inflow.get_achieved());
        pin = pin_update.port;
        resend_inflow_request = pin_update.send_request;
      }
      auto pin_update = pin.with_requested(pout.get_requested() / efficiency);
      pin = pin_update.port;
      auto pout_update = pout.with_achieved(pin.get_achieved() * efficiency);
      pout = pout_update.port;
      auto total_lossflow{pin.get_achieved() - pout.get_achieved()};
      auto ploss_update = ploss.with_achieved(std::min(ploss.get_requested(), total_lossflow));
      ploss = ploss_update.port;
      pwaste = ED::Port3{ total_lossflow - ploss.get_achieved(), total_lossflow - ploss.get_achieved() };
      if (pin_update.send_request || resend_inflow_request) {
        inflow_update = inflow.with_requested_and_available(pin.get_requested(), max_inflow);
        inflow = inflow_update.port;
      }
      else {
        inflow_update.port = inflow;
        inflow_update.send_request = false;
        inflow_update.send_achieved = false;
      }
      if (ploss_update.send_achieved) {
        lossflow_update = lossflow.with_achieved(ploss.get_achieved());
        lossflow = lossflow_update.port;
      }
      else {
        lossflow_update.port = lossflow;
        lossflow_update.send_request = false;
        lossflow_update.send_achieved = false;
      }
      if (pout_update.send_achieved) {
        outflow_update = outflow.with_achieved(pout.get_achieved());
        outflow = outflow_update.port;
      }
      else {
        outflow_update.port = outflow;
        outflow_update.send_request = false;
        outflow_update.send_achieved = false;
      }
      auto energy_balance{
        pin.get_achieved()
        - (pout.get_achieved() + ploss.get_achieved() + pwaste.get_achieved())};
      ASSERT_NEAR(energy_balance, 0.0, 1e-6)
        << "energy_balance: " << energy_balance << "\n"
        << "pin: " << pin << "\n"
        << "pout: " << pout << "\n"
        << "ploss: " << ploss << "\n"
        << "pwaste: " << pwaste << "\n";
    }
    ASSERT_EQ(outflow.get_requested(), pout.get_requested());
    ASSERT_EQ(inflow.get_requested(), pin.get_requested());
    ASSERT_EQ(lossflow.get_requested(), ploss.get_requested());
    ASSERT_EQ(outflow.get_achieved(), pout.get_achieved());
    ASSERT_EQ(inflow.get_achieved(), pin.get_achieved());
    ASSERT_EQ(lossflow.get_achieved(), ploss.get_achieved());
    auto energy_balance_v2{
      inflow.get_achieved()
      - (outflow.get_achieved() + lossflow.get_achieved() + pwaste.get_achieved())};
    ASSERT_NEAR(energy_balance_v2, 0.0, 1e-6)
      << "energy_balance_v2: " << energy_balance_v2 << "\n"
      << "inflow: " << inflow << "\n"
      << "outflow: " << outflow << "\n"
      << "lossflow: " << lossflow << "\n"
      << "pwaste: " << pwaste << "\n";
  }
}

TEST(ErinBasicsTest, Test_new_port_scheme_v2) {
  namespace ED = erin::devs;

  constexpr std::size_t num_events{10'000};
  constexpr int flow_max{100};

  std::default_random_engine generator{};
  std::uniform_int_distribution<int> flow_dist(0, flow_max);

  auto pout = ED::Port3{};
  auto pin = ED::Port3{};
  auto outflow = ED::Port3{};
  auto inflow = ED::Port3{};

  for (std::size_t idx{0}; idx < num_events; ++idx) {
    double max_inflow{static_cast<ED::FlowValueType>(flow_dist(generator))};
    double outflow_req{static_cast<ED::FlowValueType>(flow_dist(generator))};
    auto outflow_update = outflow.with_requested(outflow_req);
    outflow = outflow_update.port;
    auto inflow_update = inflow.with_requested_and_available(inflow.get_requested(), max_inflow);
    inflow = inflow_update.port;
    std::size_t no_advance{0};
    std::size_t max_no_advance{1000};
    while (outflow_update.send_request || inflow_update.send_achieved) {
      ++no_advance;
      if (no_advance > max_no_advance) {
        ASSERT_TRUE(false)
          << "idx: " << idx << "\n"
          << "no_advance: " << no_advance << "\n"
          << "inflow: " << inflow << "\n"
          << "outflow: " << outflow << "\n"
          << "pin: " << pin << "\n"
          << "pout: " << pout << "\n"
          << "max_inflow: " << max_inflow << "\n"
          << "outflow_req: " << outflow_req << "\n"
          ;
      }
      if (outflow_update.send_request) {
        pout = pout.with_requested(outflow.get_requested()).port;
      }
      auto pin_update = pin.with_requested(pout.get_requested());
      if (inflow_update.send_achieved) {
        pin_update = pin.with_requested_and_achieved(pout.get_requested(), inflow.get_achieved());
      }
      pin = pin_update.port;
      auto pout_update = pout.with_achieved(pin.get_achieved());
      pout = pout_update.port;
      if (pin_update.send_request) {
        inflow_update = inflow.with_requested_and_available(pin.get_requested(), max_inflow);
        inflow = inflow_update.port;
      }
      else {
        inflow_update.port = inflow;
        inflow_update.send_request = false;
        inflow_update.send_achieved = false;
      }
      if (pout_update.send_achieved) {
        outflow_update = outflow.with_achieved(pout.get_achieved());
        outflow = outflow_update.port;
      }
      else {
        outflow_update.port = outflow;
        outflow_update.send_request = false;
        outflow_update.send_achieved = false;
      }
      auto energy_balance{pin.get_achieved() - pout.get_achieved()};
      ASSERT_NEAR(energy_balance, 0.0, 1e-6)
        << "idx: " << idx << "\n"
        << "energy_balance: " << energy_balance << "\n"
        << "pin: " << pin << "\n"
        << "pout: " << pout << "\n";
    }
    ASSERT_EQ(outflow.get_requested(), pout.get_requested());
    ASSERT_EQ(inflow.get_requested(), pin.get_requested());
    ASSERT_EQ(outflow.get_achieved(), pout.get_achieved());
    ASSERT_EQ(inflow.get_achieved(), pin.get_achieved());
    auto energy_balance_v2{inflow.get_achieved() - outflow.get_achieved()};
    ASSERT_NEAR(energy_balance_v2, 0.0, 1e-6)
      << "idx: " << idx << "\n"
      << "energy_balance_v2: " << energy_balance_v2 << "\n"
      << "inflow: " << inflow << "\n"
      << "outflow: " << outflow << "\n";
  }
}

TEST(ErinBasicsTest, Test_schedule_state_at_time) {
  namespace E = ERIN;
  std::vector<E::TimeState> schedule{
    E::TimeState{0, true},
    E::TimeState{10, false},
    E::TimeState{40, true},
    E::TimeState{50, false}};
  ASSERT_TRUE(E::schedule_state_at_time(schedule, -100));
  ASSERT_TRUE(E::schedule_state_at_time(schedule, 0));
  ASSERT_TRUE(E::schedule_state_at_time(schedule, 40));
  ASSERT_TRUE(E::schedule_state_at_time(schedule, 42));
  ASSERT_FALSE(E::schedule_state_at_time(schedule, 10));
  ASSERT_FALSE(E::schedule_state_at_time(schedule, 12));
  ASSERT_FALSE(E::schedule_state_at_time(schedule, 60));
  ASSERT_FALSE(E::schedule_state_at_time(schedule, 600));
}

ERIN::RealTimeType
time_to_next_schedule_change(
    const std::vector<ERIN::TimeState>& schedule,
    ERIN::RealTimeType current_time)
{
  ERIN::RealTimeType dt{-1};
  for (const auto& ts : schedule) {
    if (ts.time >= current_time) {
      dt = ts.time - current_time;
      break;
    }
  }
  return dt;
}

TEST(ErinBasicsTest, Test_load_and_source_comprehensive) {
  namespace E = ERIN;
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  const std::size_t num_events{10'000};
  const std::vector<bool> has_flow_limit_options{true, false};
  const E::FlowValueType max_source_outflow{50.0};
  const unsigned seed = 17; // std::chrono::system_clock::now().time_since_epoch().count();
  
  std::cout << "seed: " << seed << "\n";
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> dt_dist(0, 10);
  std::uniform_int_distribution<int> flow_dist(0, 100);

  std::string stream{"stream"};
  std::string source_id{"source"};
  std::string sink_id{"sink"};

  for (const auto& has_flow_limit : has_flow_limit_options) {
    std::vector<E::RealTimeType> expected_times{};
    std::vector<E::FlowValueType> expected_flows_req{};
    std::vector<E::FlowValueType> expected_flows_ach{};
    std::vector<E::LoadItem> load_profile{};

    E::RealTimeType t{0};
    for (std::size_t idx{0}; idx < num_events; ++idx) {
      auto new_load{static_cast<E::FlowValueType>(flow_dist(generator))};
      load_profile.emplace_back(E::LoadItem{t, new_load});
      auto dt = static_cast<E::RealTimeType>(dt_dist(generator));
      if (dt > 0) {
        expected_times.emplace_back(t);
        expected_flows_req.emplace_back(new_load);
      }
      t += dt;
    }
    expected_flows_req.back() = 0.0;
    auto t_max = expected_times.back();
    ASSERT_EQ(expected_times.size(), expected_flows_req.size());
    for (std::size_t idx{0}; idx < expected_times.size(); ++idx) {
      auto flow_r{expected_flows_req[idx]};
      if (has_flow_limit && (flow_r > max_source_outflow)) {
        expected_flows_ach.emplace_back(max_source_outflow);
      }
      else {
        expected_flows_ach.emplace_back(flow_r);
      }
    }
    ASSERT_EQ(expected_times.size(), expected_flows_ach.size());
    auto sink = new E::Sink(
        sink_id,
        E::ComponentType::Load,
        stream,
        load_profile,
        false);
    auto source = new E::Source(
        source_id,
        E::ComponentType::Source,
        stream,
        has_flow_limit ? max_source_outflow : ED::supply_unlimited_value);
    std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
    source->set_flow_writer(fw);
    source->set_recording_on();
    sink->set_flow_writer(fw);
    sink->set_recording_on();

    adevs::Digraph<E::FlowValueType, E::Time> network{};
    network.couple(
        sink, E::Sink::outport_inflow_request,
        source, E::Source::inport_outflow_request);
    network.couple(
        source, E::Source::outport_outflow_achieved,
        sink, E::Sink::inport_inflow_achieved);
    adevs::Simulator<E::PortValue, E::Time> sim{};
    network.add(&sim);
    while (sim.next_event_time() < E::inf) {
      sim.exec_next_event();
    }
    fw->finalize_at_time(t_max);
    auto results = fw->get_results();
    fw->clear();

    ASSERT_TRUE(
        check_times_and_loads(results, expected_times, expected_flows_req, sink_id, true));
    ASSERT_TRUE(
        check_times_and_loads(results, expected_times, expected_flows_req, source_id, true));
    ASSERT_TRUE(
        check_times_and_loads(results, expected_times, expected_flows_ach, sink_id, false));
    ASSERT_TRUE(
        check_times_and_loads(results, expected_times, expected_flows_ach, source_id, false));
  }
}

TEST(ErinBasicsTest, Test_on_off_switch_comprehensive) {
  namespace E = ERIN;
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  const std::size_t num_events{10'000};
  const std::size_t num_time_state_transitions{1'000};
  const auto t_end{static_cast<E::RealTimeType>(num_events * 5)};

  unsigned seed = 17; // std::chrono::system_clock::now().time_since_epoch().count();
  std::cout << "seed: " << seed << "\n";
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> dt_dist(0, 10);
  std::uniform_int_distribution<int> flow_dist(0, 100);

  std::string stream{"stream"};
  std::string source_id{"source"};
  std::string sink_id{"sink"};
  std::string switch_id{"switch"};

  std::vector<E::RealTimeType> expected_times{};
  std::vector<E::FlowValueType> expected_flows_req{};
  std::vector<E::FlowValueType> expected_flows_ach{};
  std::vector<E::LoadItem> load_profile{};
  std::vector<E::TimeState> schedule{};

  E::RealTimeType t{0};
  bool flag{true};
  for (std::size_t idx{0}; idx < num_time_state_transitions; ++idx) {
    schedule.emplace_back(
        E::TimeState{t, flag});
    flag = !flag;
    t += (static_cast<E::RealTimeType>(dt_dist(generator)) + 1) * 100;
    if (t > t_end) {
      break;
    }
  }
  t = 0;
  for (std::size_t idx{0}; idx < num_events; ++idx) {
    auto new_load{static_cast<E::FlowValueType>(flow_dist(generator))};
    load_profile.emplace_back(E::LoadItem{t, new_load});
    auto dt = static_cast<E::RealTimeType>(dt_dist(generator));
    auto dt_sch = time_to_next_schedule_change(schedule, t);
    if (dt > 0) {
      expected_times.emplace_back(t);
      expected_flows_req.emplace_back(new_load);
      if ((dt_sch > 0) && (dt_sch < dt) && (dt_sch < (t_end - t))) {
        expected_times.emplace_back(t + dt_sch);
        expected_flows_req.emplace_back(new_load);
        t += dt_sch;
        dt -= dt_sch;
      }
    }
    t += dt;
    if (t > t_end) {
      break;
    }
  }
  expected_flows_req.back() = 0.0;
  auto t_max = expected_times.back();
  ASSERT_EQ(expected_times.size(), expected_flows_req.size());
  for (std::size_t idx{0}; idx < expected_times.size(); ++idx) {
    auto time{expected_times[idx]};
    auto flow_r{expected_flows_req[idx]};
    auto flag{E::schedule_state_at_time(schedule, time)};
    if (flag) {
      expected_flows_ach.emplace_back(flow_r);
    }
    else {
      expected_flows_ach.emplace_back(0.0);
    }
  }
  ASSERT_EQ(expected_times.size(), expected_flows_ach.size());
  auto sink = new E::Sink(
      sink_id,
      E::ComponentType::Load,
      stream,
      load_profile,
      false);
  auto on_off_switch = new E::OnOffSwitch(
      switch_id,
      E::ComponentType::PassThrough,
      stream,
      schedule);
  auto source = new E::Source(
      source_id, E::ComponentType::Source, stream);
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
  source->set_flow_writer(fw);
  source->set_recording_on();
  sink->set_flow_writer(fw);
  sink->set_recording_on();
  on_off_switch->set_flow_writer(fw);
  on_off_switch->set_recording_on();

  adevs::Digraph<E::FlowValueType, E::Time> network{};
  network.couple(
      sink, E::Sink::outport_inflow_request,
      on_off_switch, E::OnOffSwitch::inport_outflow_request);
  network.couple(
      on_off_switch, E::OnOffSwitch::outport_inflow_request,
      source, E::Source::inport_outflow_request);
  network.couple(
      source, E::Source::outport_outflow_achieved,
      on_off_switch, E::OnOffSwitch::inport_inflow_achieved);
  network.couple(
      on_off_switch, E::OnOffSwitch::outport_outflow_achieved,
      sink, E::Sink::inport_inflow_achieved);
  adevs::Simulator<E::PortValue, E::Time> sim{};
  network.add(&sim);
  while (sim.next_event_time() < E::inf) {
    sim.exec_next_event();
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();

  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_flows_req, sink_id, true));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_flows_req, switch_id, true));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_flows_ach, source_id, true));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_flows_ach, sink_id, false));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_flows_ach, switch_id, false));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_flows_ach, source_id, false));
}

TEST(ErinBasicsTest, Test_flow_limits_comprehensive) {
  namespace E = ERIN;
  namespace ED = erin::devs;
  namespace EU = erin::utils;
  const std::size_t num_events{10'000};
  const E::FlowValueType max_lim_flow{75.0};
  const E::FlowValueType max_src_flow{50.0};
  const bool source_is_limited{false};

  unsigned seed = 17; // std::chrono::system_clock::now().time_since_epoch().count();
  std::cout << "seed: " << seed << "\n";
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> dt_dist(0, 10);
  std::uniform_int_distribution<int> flow_dist(0, 100);

  std::string stream{"stream"};
  std::string source_id{"source"};
  std::string sink_id{"sink"};
  std::string lim_id{"flow_limits"};

  std::vector<E::RealTimeType> expected_times{};
  std::vector<E::FlowValueType> expected_outflows_req{};
  std::vector<E::FlowValueType> expected_outflows_ach{};
  std::vector<E::FlowValueType> expected_inflows_req{};
  std::vector<E::FlowValueType> expected_inflows_ach{};
  std::vector<E::LoadItem> load_profile{};

  E::RealTimeType t{0};
  t = 0;
  for (std::size_t idx{0}; idx < num_events; ++idx) {
    auto new_load{static_cast<E::FlowValueType>(flow_dist(generator))};
    load_profile.emplace_back(E::LoadItem{t, new_load});
    auto dt = static_cast<E::RealTimeType>(dt_dist(generator));
    if (dt > 0) {
      expected_times.emplace_back(t);
      expected_outflows_req.emplace_back(new_load);
      expected_inflows_req.emplace_back(std::min(new_load, max_lim_flow));
      auto flow_a{std::min(
        new_load,
        source_is_limited
        ? std::min(max_src_flow, max_lim_flow)
        : max_lim_flow)};
      expected_inflows_ach.emplace_back(flow_a);
      expected_outflows_ach.emplace_back(flow_a);
    }
    t += dt;
  }
  expected_outflows_req.back() = 0.0;
  expected_outflows_ach.back() = 0.0;
  expected_inflows_req.back() = 0.0;
  expected_inflows_ach.back() = 0.0;
  auto t_max = expected_times.back();
  ASSERT_EQ(expected_times.size(), expected_outflows_req.size());
  ASSERT_EQ(expected_times.size(), expected_outflows_ach.size());
  ASSERT_EQ(expected_times.size(), expected_inflows_req.size());
  ASSERT_EQ(expected_times.size(), expected_inflows_ach.size());
  auto sink = new E::Sink(
      sink_id,
      E::ComponentType::Load,
      stream,
      load_profile,
      false);
  auto lim = new E::FlowLimits(
      lim_id,
      E::ComponentType::PassThrough,
      stream,
      0.0,
      max_lim_flow);
  auto source = new E::Source(
      source_id,
      E::ComponentType::Source,
      stream,
      source_is_limited ? max_src_flow : ED::supply_unlimited_value);
  std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
  source->set_flow_writer(fw);
  source->set_recording_on();
  sink->set_flow_writer(fw);
  sink->set_recording_on();
  lim->set_flow_writer(fw);
  lim->set_recording_on();

  adevs::Digraph<E::FlowValueType, E::Time> network{};
  network.couple(
      sink, E::Sink::outport_inflow_request,
      lim, E::FlowLimits::inport_outflow_request);
  network.couple(
      lim, E::FlowLimits::outport_inflow_request,
      source, E::Source::inport_outflow_request);
  network.couple(
      source, E::Source::outport_outflow_achieved,
      lim, E::FlowLimits::inport_inflow_achieved);
  network.couple(
      lim, E::FlowLimits::outport_outflow_achieved,
      sink, E::Sink::inport_inflow_achieved);
  adevs::Simulator<E::PortValue, E::Time> sim{};
  network.add(&sim);
  while (sim.next_event_time() < E::inf) {
    sim.exec_next_event();
  }
  fw->finalize_at_time(t_max);
  auto results = fw->get_results();
  fw->clear();

  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_outflows_req, sink_id, true));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_outflows_req, lim_id, true));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_inflows_ach, source_id, true));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_outflows_ach, sink_id, false));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_outflows_ach, lim_id, false));
  ASSERT_TRUE(
      check_times_and_loads(results, expected_times, expected_inflows_ach, source_id, false));
}

TEST(ErinBasicsTest, Test_flow_limits_function_cases) {
  namespace E = ERIN;
  namespace ED = erin::devs;

  const E::FlowValueType upper_limit{75.0};
  const E::FlowValueType lower_limit{0.0};
  const E::RealTimeType t{1013};
  
  auto xs = std::vector<ED::PortValue>{
    ED::PortValue{ED::inport_inflow_achieved, 30.0},
    ED::PortValue{ED::inport_outflow_request, 26.0}};
  auto lim = ED::FlowLimits{lower_limit, upper_limit};
  auto s = ED::FlowLimitsState{
    t,
    ED::Port3{50.0, 75.0}, // inflow
    ED::Port3{50.0, 50.0}, // outflow
    lim,
    true,   // report IR
    true};  // report OA
  auto next_s = ED::flow_limits_confluent_transition(s, xs);
  auto expected_next_s = ED::FlowLimitsState{
    t,
    ED::Port3{26.0, 30.0}, // inflow
    ED::Port3{26.0, 26.0}, // outflow
    lim,
    true,   // report IR
    true};  // report OA
  ASSERT_EQ(expected_next_s, next_s);
}

int
main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
