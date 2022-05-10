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
#include "erin/devs/flow_meter.h"
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
#include <unordered_set>
#include <utility>

constexpr std::size_t comprehensive_test_num_events{ 1'000 };

const double tolerance{ 1e-6 };

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
      auto exp_num_times{ expected_times.size() };
      auto exp_num_loads{ expected_loads.size() };
      auto act_num_times{ actual_times.size() };
      auto act_num_loads{ actual_loads.size() };
      std::cout << "key: " << id
        << " " << (use_requested ? "requested" : "achieved")
        << "\n"
        << "- expected_times.size(): " << exp_num_times << "\n"
        << "- expected_loads.size(): " << exp_num_loads << "\n"
        << "- actual_times.size(): " << act_num_times << "\n"
        << "- actual_loads.size(): " << act_num_loads << "\n";
      auto sizes = std::vector<decltype(exp_num_times)>{
        exp_num_times, exp_num_loads, act_num_times, act_num_loads };
      auto num{ *std::min_element(sizes.begin(), sizes.end()) };
      int num_discrepancies{ 0 };
      const int max_reporting{ 10 };
      for (std::size_t idx{ 0 }; idx < num; ++idx) {
        auto t_exp{ expected_times[idx] };
        auto t_act{ actual_times[idx] };
        auto flow_exp{ expected_loads[idx] };
        auto flow_act{ actual_loads[idx] };
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
              << expected_times[idx - 1]
              << ", <<" << expected_times[idx] << ">>, "
              << expected_times[idx + 1] << "]\n"
              << "-- actual-times: ["
              << actual_times[idx - 1]
              << ", <<" << actual_times[idx] << ">>, "
              << actual_times[idx + 1] << "]\n";
          }
        }
        if (flow_exp != flow_act) {
          std::cout << "- flow discrepancy\n"
            << "-- expected-flow: " << flow_exp << "\n"
            << "-- actual-flow: " << flow_act << "\n"
            ;
          if ((idx > 0) && (idx < (num - 1))) {
            std::cout << "-- expected-flows: ["
              << expected_loads[idx - 1]
              << ", <<" << expected_loads[idx] << ">>, "
              << expected_loads[idx + 1] << "]\n"
              << "-- actual-flows: ["
              << actual_loads[idx - 1]
              << ", <<" << actual_loads[idx] << ">>, "
              << actual_loads[idx + 1] << "]\n";
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
    std::string comp_string_id{ "S" };
    auto comp_id = rc.register_component(comp_string_id);
    rc.link_component_with_failure_mode(comp_id, fm_standard_id);
    ERIN::RealTimeType final_time{ 100 };
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
            E::TimeState{105, true}}} };
    ASSERT_EQ(sch, expected_sch);
    E::RealTimeType scenario_start{ 62 };
    E::RealTimeType scenario_end{ 87 };
    auto clipped_sch = E::clip_schedule_to<std::string>(
      sch, scenario_start, scenario_end);
    std::unordered_map<std::string, std::vector<E::TimeState>>
      expected_clipped_sch{
        { comp_string_id,
          std::vector<E::TimeState>{
            E::TimeState{62 - 62, true},
            E::TimeState{70 - 62, false},
            E::TimeState{75 - 62, true},
            E::TimeState{85 - 62, false}}} };
    ASSERT_EQ(clipped_sch, expected_clipped_sch);
  }

  TEST(ErinBasicsTest, Test_fixed_distribution)
  {
    namespace E = ERIN;
    namespace ED = erin::distribution;
    ED::DistributionSystem dist_sys{};
    E::RealTimeType fixed_dt{ 10 };
    auto dist_id = dist_sys.add_fixed("some_dist", fixed_dt);
    EXPECT_EQ(dist_sys.next_time_advance(dist_id), fixed_dt);
  }

  TEST(ErinBasicsTest, Test_uniform_distribution)
  {
    namespace E = ERIN;
    namespace ED = erin::distribution;
    ED::DistributionSystem dist_sys{};
    E::RealTimeType lower_dt{ 10 };
    E::RealTimeType upper_dt{ 50 };
    auto dist_id = dist_sys.add_uniform("a_uniform_dist", lower_dt, upper_dt);
    double dice_roll_1{ 1.0 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), upper_dt);
    double dice_roll_2{ 0.0 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), lower_dt);
    double dice_roll_3{ 0.5 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), (lower_dt + upper_dt) / 2);
  }

  TEST(ErinBasicsTest, Test_normal_distribution)
  {
    namespace E = ERIN;
    namespace ED = erin::distribution;
    ED::DistributionSystem dist_sys{};
    E::RealTimeType mean{ 1000 };
    E::RealTimeType stddev{ 50 };
    auto dist_id = dist_sys.add_normal("a_normal_dist", mean, stddev);
    double dice_roll_1{ 0.5 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), mean);
    double dice_roll_2{ 0.0 };
    constexpr double sqrt2{ 1.4142'1356'2373'0951 };
    EXPECT_EQ(
      dist_sys.next_time_advance(dist_id, dice_roll_2),
      mean - static_cast<E::RealTimeType>(std::round(3.0 * sqrt2 * stddev)));
    double dice_roll_3{ 1.0 };
    EXPECT_EQ(
      dist_sys.next_time_advance(dist_id, dice_roll_3),
      mean + static_cast<E::RealTimeType>(std::round(3.0 * sqrt2 * stddev)));
    double dice_roll_4{ 0.0 };
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
    std::vector<double> dts{ 0.0, 100.0 };
    std::vector<double> xs{ 0.0, 1.0 };
    auto dist_id = dist_sys.add_quantile_table("a_table_dist_1", xs, dts);
    constexpr double dice_roll_1{ 0.5 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), 50);
    constexpr double dice_roll_2{ 0.0 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), 0);
    constexpr double dice_roll_3{ 1.0 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), 100);
    dts = std::vector{ 5.0, 6.0 };
    dist_id = dist_sys.add_quantile_table("a_table_dist_2", xs, dts);
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), 6);
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), 5);
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), 6);
    dts = std::vector{ 0.0, 400.0, 600.0, 1000.0 };
    xs = std::vector{ 0.0, 0.4, 0.6, 1.0 };
    dist_id = dist_sys.add_quantile_table("a_table_dist_3", xs, dts);
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), 500);
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), 0);
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), 1000);
    constexpr double dice_roll_4{ 0.25 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_4), 250);
    constexpr double dice_roll_5{ 0.75 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_5), 750);
    xs = std::vector{ -20.0, -15.0, -10.0, -5.0, 0.0 };
    dts = std::vector{ 1.0, 2.0, 3.0, 4.0, 5.0 };
    ASSERT_THROW(dist_sys.add_quantile_table("a_table_dist_4", xs, dts), std::invalid_argument);
    xs = std::vector{ 0.0, 0.5, 0.8 };
    dts = std::vector{ 100.0, 200.0, 300.0 };
    ASSERT_THROW(dist_sys.add_quantile_table("a_table_dist_5", xs, dts), std::invalid_argument);
  }

  TEST(ErinBasicsTest, Test_weibull_distribution)
  {
    namespace E = ERIN;
    namespace ED = erin::distribution;
    ED::DistributionSystem dist_sys{};
    double k{ 5.0 };        // shape parameter
    double lambda{ 200.0 }; // scale parameter
    double gamma{ 0.0 };    // location parameter
    auto dist_id = dist_sys.add_weibull("a_weibull_dist", k, lambda, gamma);
    double dice_roll_1{ 0.5 };
    E::RealTimeType ans1{ 186 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_1), ans1);
    double dice_roll_2{ 0.0 };
    E::RealTimeType ans2{ 0 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_2), ans2);
    double dice_roll_3{ 1.0 };
    E::RealTimeType ans3{ 312 };
    EXPECT_EQ(dist_sys.next_time_advance(dist_id, dice_roll_3), ans3);
    double dice_roll_4{ 0.0 };
    gamma = 10.0;
    E::RealTimeType ans4{ static_cast<E::RealTimeType>(gamma) };
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
    std::set<std::string> expected_comp_ids{ "US-inflow", "US-outflow", "US-lossflow", "L" };
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
    ERIN::FlowValueType L_load_not_served{ 5 * 50.0 };
    ERIN::FlowValueType L_total_energy{ 5 * 50.0 + 5 * 100.0 };
    ERIN::RealTimeType L_max_downtime{ 5 };
    auto L_ss = ss_map["L"];
    EXPECT_EQ(L_ss.load_not_served, L_load_not_served);
    EXPECT_EQ(L_ss.total_energy, L_total_energy);
    EXPECT_EQ(L_ss.max_downtime, L_max_downtime);
    ERIN::FlowValueType US_inflow_total_energy{ 5 * 50.0 + 3 * 120.0 + 2 * 100.0 };
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
      "[loads.cooling]\n"
      "time_unit = \"seconds\"\n"
      "rate_unit = \"kW\"\n"
      "time_rate_pairs = [[0.0,60.0],[5.0,144.0],[8.0,120.0],[10.0,0.0]]\n"
      "[components.S]\n"
      "type = \"source\"\n"
      "outflow = \"electricity\"\n"
      "[components.US]\n"
      "type = \"source\"\n"
      "output_stream = \"heat\"\n"
      "[components.L]\n"
      "type = \"load\"\n"
      "input_stream = \"heat\"\n"
      "loads_by_scenario.blue_sky = \"cooling\"\n"
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
      "US", "L", "S", "M-inflow(0)", "M-inflow(1)", "M-outflow" };
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
    ERIN::RealTimeType L_max_downtime{ 0 };
    ERIN::FlowValueType L_total_energy{ (5 * 50.0 + 3 * 120.0 + 2 * 100.0) * (1.0 + (1.0 / 5.0)) };
    ERIN::FlowValueType L_load_not_served{ 0.0 };
    auto L_ss = ss_map["L"];
    EXPECT_EQ(L_ss.max_downtime, L_max_downtime);
    EXPECT_EQ(L_ss.load_not_served, L_load_not_served);
    EXPECT_EQ(L_ss.total_energy, L_total_energy);
  }

  TEST(ErinBasicsTest, Test_muxer_dispatch_strategy)
  {
    namespace ED = erin::devs;
    namespace E = ERIN;
    E::RealTimeType time{ 0 };
    E::FlowValueType outflow_achieved{ 100.0 };
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
    for (std::vector<ED::Port>::size_type idx{ 0 }; idx < outflows.size(); ++idx) {
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
    for (std::size_t idx{ 0 }; idx < outflows_irregular.size(); ++idx) {
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
    for (std::vector<ED::Port>::size_type idx{ 0 }; idx < outflows_dist.size(); ++idx) {
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
    for (std::vector<ED::Port2>::size_type idx{ 0 }; idx < outflows_dist_irregular.size(); ++idx) {
      EXPECT_EQ(expected_outflows_dist_irregular[idx], outflows_dist_irregular[idx].port)
        << "idx = " << idx << "\n";
    }
  }

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
    std::set<std::string> expected_comp_ids{ "L", "S" };
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
    ERIN::RealTimeType L_max_downtime{ 5 * 3600 };
    ERIN::FlowValueType L_load_not_served{ 10.0 * 3600.0 * 1.0 };
    ERIN::FlowValueType L_total_energy{ 40.0 * 3600 * 1.0 - L_load_not_served };
    auto L_ss = ss_map["L"];
    EXPECT_EQ(L_ss.max_downtime, L_max_downtime);
    EXPECT_EQ(L_ss.load_not_served, L_load_not_served);
    EXPECT_EQ(L_ss.total_energy, L_total_energy);
  }

  TEST(ErinBasicsTest, Test_that_port2_works)
  {
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
    p = ED::Port2{ 8,6 };
    update = p.with_achieved(7).port.with_requested(7);
    EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
    // R=9,A=2
    p = ED::Port2{ 2,2 };
    update = p.with_achieved(2).port.with_requested(9);
    EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
    // R=2,A=2
    p = ED::Port2{ 3,2 };
    update = p.with_achieved(2).port.with_requested(3);
    EXPECT_FALSE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
    // {5,5} => A=2 => send A
    p = ED::Port2{ 5,5 };
    update = p.with_achieved(2);
    EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
    EXPECT_TRUE(update.send_update);
    // {5,4} => A=5 => send A
    p = ED::Port2{ 5,4 };
    update = p.with_achieved(5);
    EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
    EXPECT_TRUE(update.send_update);
    // {5,4} => R=4,A=5 => don't send A
    p = ED::Port2{ 5,4 };
    update = p.with_achieved(5).port.with_requested(4);
    EXPECT_FALSE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
    // {5,4} => R=8,A=5 => send A
    p = ED::Port2{ 5,4 };
    update = p.with_achieved(5).port.with_requested(8);
    EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
    // R=4252.38,A=0
    p = ED::Port2{ 2952.38,855.556 };
    update = p.with_achieved(0).port.with_requested(4252.38);
    EXPECT_TRUE(update.port.should_send_achieved(p))
      << "p: " << update.port << "pL: " << p;
  }

  TEST(ErinBasicsTest, Test_interpolate_value)
  {
    namespace E = ERIN;
    namespace EU = erin::utils;
    // #1
    std::vector<E::RealTimeType> ts{ 0, 5, 10, 15 };
    std::vector<E::FlowValueType> fs{ 10.0, 20.0, 30.0, 40.0 };
    E::RealTimeType t{ 2 };
    auto f = EU::interpolate_value(t, ts, fs);
    E::FlowValueType expected_f{ 10.0 };
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
    ts = std::vector<E::RealTimeType>{ 5, 10, 15 };
    fs = std::vector<E::FlowValueType>{ 20.0, 30.0, 40.0 };
    t = 2;
    f = EU::interpolate_value(t, ts, fs);
    expected_f = 0.0;
    EXPECT_EQ(f, expected_f);
  }

  TEST(ErinBasicsTest, Test_integrate_value)
  {
    namespace E = ERIN;
    namespace EU = erin::utils;
    // #1
    std::vector<E::RealTimeType> ts{ 0, 5, 10, 15 };
    std::vector<E::FlowValueType> fs{ 10.0, 20.0, 30.0, 40.0 };
    E::RealTimeType t{ 2 };
    auto g = EU::integrate_value(t, ts, fs);
    E::FlowValueType expected_g{ 20.0 };
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
    ts = std::vector<E::RealTimeType>{ 5, 10, 15 };
    fs = std::vector<E::FlowValueType>{ 20.0, 30.0, 40.0 };
    t = 2;
    g = EU::integrate_value(t, ts, fs);
    expected_g = 0.0;
    EXPECT_EQ(g, expected_g);
  }

  TEST(ErinBasicsTest, Test_store_element_comprehensive)
  {
    namespace E = ERIN;
    namespace EU = erin::utils;
    namespace ED = erin::devs;

    const E::FlowValueType capacity{ 100.0 };
    const E::FlowValueType max_charge_rate{ 10.0 };
    const std::size_t num_events{ comprehensive_test_num_events };
    const bool source_is_limited{ false };
    const E::FlowValueType source_limit{ 20.0 };

    std::string id{ "store" };
    std::string stream_type{ "electricity" };
    auto c = new E::Storage(
      id,
      E::ComponentType::Storage,
      stream_type,
      capacity,
      max_charge_rate);
    std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
    c->set_flow_writer(fw);
    c->set_recording_on();

    std::default_random_engine generator{};
    std::uniform_int_distribution<int> dt_dist(0, 10);
    std::uniform_int_distribution<int> flow_dist(0, 100);

    // inflow and outflow from viewpoint of the component
    std::vector<E::RealTimeType> outflow_times{};
    std::vector<E::FlowValueType> outflow_requests{};
    std::vector<E::LoadItem> outflow_profile{};
    E::RealTimeType t{ 0 };
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      auto flow{ static_cast<E::FlowValueType>(flow_dist(generator)) };
      t += static_cast<E::RealTimeType>(dt_dist(generator));
      outflow_times.emplace_back(t);
      outflow_requests.emplace_back(flow);
      outflow_profile.emplace_back(E::LoadItem{ t, flow });
    }
    auto t_max{ t };
    auto inflow_driver = new E::Source{
      "inflow-to-store",
      E::ComponentType::Source,
      stream_type,
      source_is_limited ? source_limit : ED::supply_unlimited_value };
    inflow_driver->set_flow_writer(fw);
    inflow_driver->set_recording_on();
    auto outflow_driver = new E::Sink{
      "outflow-from-store",
      E::ComponentType::Load,
      stream_type,
      outflow_profile,
      false };
    outflow_driver->set_flow_writer(fw);
    outflow_driver->set_recording_on();
    adevs::Digraph<E::FlowValueType, E::Time> network;
    network.couple(
      outflow_driver, E::Sink::outport_inflow_request,
      c, E::Storage::inport_outflow_request);
    network.couple(
      c, E::Storage::outport_inflow_request,
      inflow_driver, E::Source::inport_outflow_request);
    network.couple(
      inflow_driver, E::Source::outport_outflow_achieved,
      c, E::Storage::inport_inflow_achieved);
    network.couple(
      c, E::Storage::outport_outflow_achieved,
      outflow_driver, E::Sink::inport_inflow_achieved);
    adevs::Simulator<E::PortValue, E::Time> sim{};
    network.add(&sim);
    const std::size_t max_no_advance{ num_events * 4 };
    std::size_t non_advance_count{ 0 };
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
    ASSERT_EQ(results.size(), 7);
    auto inflow_results = results[id + "-inflow"];
    auto outflow_results = results[id + "-outflow"];
    auto storeflow_results = results[id + "-storeflow"];
    auto discharge_results = results[id + "-discharge"];
    std::vector<E::RealTimeType> inflow_ts{};
    std::vector<E::FlowValueType> inflow_fs{};
    std::vector<E::FlowValueType> inflow_fs_req{};
    std::vector<E::RealTimeType> outflow_ts{};
    std::vector<E::FlowValueType> outflow_fs{};
    std::vector<E::FlowValueType> outflow_fs_req{};
    for (const auto& data : results["inflow-to-store"]) {
      inflow_ts.emplace_back(data.time);
      inflow_fs.emplace_back(data.achieved_value);
      inflow_fs_req.emplace_back(data.requested_value);
    }
    for (const auto& data : results["outflow-from-store"]) {
      outflow_ts.emplace_back(data.time);
      outflow_fs.emplace_back(data.achieved_value);
      outflow_fs_req.emplace_back(data.requested_value);
    }
    std::size_t last_idx{ outflow_results.size() - 1 };
    // Note: on the last index, the finalize_at_time(.) method of FlowWriter sets
    // the flows to 0 which causes a discrepancy with the drivers that need not
    // be tested. Therefore, we only go until prior to the last index.
    for (std::size_t idx{ 0 }; idx < last_idx; ++idx) {
      std::ostringstream oss{};
      oss << "idx            : " << idx << "\n";
      const auto& outf_res = outflow_results[idx];
      const auto& time = outf_res.time;
      oss << "time           : " << time << "\n";
      const auto outflow_d{ EU::interpolate_value(time, outflow_ts, outflow_fs) };
      oss << "outflow_results : " << outf_res << "\n";
      oss << "outflow_driver  : " << outflow_d << "\n";
      ASSERT_EQ(outf_res.achieved_value, outflow_d) << oss.str();
      const auto& inf_res = inflow_results[idx];
      const auto inflow_d{ EU::interpolate_value(time, inflow_ts, inflow_fs) };
      oss << "inflow_results: " << inf_res << "\n";
      oss << "inflow_driver : " << inflow_d << "\n";
      ASSERT_EQ(inf_res.achieved_value, inflow_d) << oss.str();
      const auto& str_res = storeflow_results[idx];
      const auto& dis_res = discharge_results[idx];
      auto error{
        inf_res.achieved_value +
        dis_res.achieved_value -
        (str_res.achieved_value + outf_res.achieved_value) };
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

  TEST(ErinBasicsTest, Test_converter_element_comprehensive)
  {
    namespace E = ERIN;
    namespace EU = erin::utils;
    namespace ED = erin::devs;

    constexpr bool do_rounding{ false };
    const E::FlowValueType constant_efficiency{ 0.4 };
    const std::size_t num_events{ comprehensive_test_num_events };
    const bool has_flow_limit{ true };
    const E::FlowValueType flow_limit{ 60.0 };

    auto calc_output_from_input = [constant_efficiency, do_rounding](E::FlowValueType inflow) -> E::FlowValueType {
      auto out{ inflow * constant_efficiency };
      if (do_rounding)
        return std::round(out * 1e6) / 1e6;
      return out;
    };
    auto calc_input_from_output = [constant_efficiency, do_rounding](E::FlowValueType outflow) -> E::FlowValueType {
      auto out{ outflow / constant_efficiency };
      if (do_rounding) {
        return std::round(out * 1e6) / 1e6;
      }
      return out;
    };
    std::string id{ "conv" };
    std::string src_id{ "inflow_at_source" };
    std::string sink_out_id{ "outflow_at_load" };
    std::string sink_loss_id{ "lossflow_at_load" };
    std::string outflow_stream{ "electricity" };
    std::string inflow_stream{ "diesel_fuel" };
    std::string lossflow_stream{ "waste_heat" };
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

    E::RealTimeType t{ 0 };
    E::FlowValueType lossflow_r{ 0.0 };
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      auto dt{ static_cast<E::RealTimeType>(dt_dist(generator)) };
      auto dt2{ static_cast<E::RealTimeType>(dt_dist(generator)) };
      auto outflow_r{ static_cast<E::FlowValueType>(flow_dist(generator)) };
      outflow_load_profile.emplace_back(E::LoadItem{ t, outflow_r });
      auto inflow_r{ calc_input_from_output(outflow_r) };
      auto inflow_a{ inflow_r };
      auto outflow_a{ calc_output_from_input(inflow_a) };
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
        E::LoadItem{ t, lossflow_r });
      if (dt2 > 0) {
        times.emplace_back(t);
        flows_conv_to_out_req.emplace_back(outflow_r);
        auto inflow_r{ calc_input_from_output(outflow_r) };
        flows_src_to_conv_req.emplace_back(inflow_r);
        auto inflow_a{ inflow_r };
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
    auto t_max{ times.back() };
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
        false };
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
      outflow_driver, E::Sink::inport_inflow_achieved);
    network.couple(
      c, E::Converter::outport_outflow_achieved + 1,
      lossflow_driver, E::Converter::inport_inflow_achieved);
    adevs::Simulator<E::PortValue, E::Time> sim{};
    network.add(&sim);
    const std::size_t max_no_advance{ num_events * 4 };
    std::size_t non_advance_count{ 0 };
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
    for (std::size_t idx{ 0 }; idx < results[id + "-inflow"].size(); ++idx) {
      const auto& inflow = results[id + "-inflow"][idx].achieved_value;
      const auto& outflow = results[id + "-outflow"][idx].achieved_value;
      const auto& lossflow = results[id + "-lossflow"][idx].achieved_value;
      const auto& wasteflow = results[id + "-wasteflow"][idx].achieved_value;
      auto error{ inflow - (outflow + lossflow + wasteflow) };
      EXPECT_TRUE(std::abs(error) < 1e-6)
        << "idx:       " << idx << "\n"
        << "inflow:    " << inflow << "\n"
        << "outflow:   " << outflow << "\n"
        << "lossflow:  " << lossflow << "\n"
        << "wasteflow: " << wasteflow << "\n"
        << "error:     " << error << "\n";
    }
  }

  TEST(ErinBasicsTest, Test_mux_element_comprehensive)
  {
    namespace E = ERIN;
    namespace EU = erin::utils;
    namespace ED = erin::devs;

    const int num_inflows{ 3 };
    const int num_outflows{ 3 };
    const auto output_dispatch_strategy = E::MuxerDispatchStrategy::InOrder;
    const std::size_t num_events{ comprehensive_test_num_events };
    const bool use_limited_source{ true };
    const E::FlowValueType source_limit{ 20.0 };

    std::string id{ "mux" };
    std::string stream{ "electricity" };
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
    E::RealTimeType t{ 0 };
    E::RealTimeType t_max{ 0 };
    E::FlowValueType v{ 0.0 };
    for (std::size_t outport_id{ 0 }; outport_id < static_cast<std::size_t>(num_outflows); ++outport_id) {
      t = 0;
      for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
        t += static_cast<E::RealTimeType>(dt_dist(generator));
        v = static_cast<E::FlowValueType>(flow_dist(generator));
        outflow_times[outport_id].emplace_back(t);
        outflow_requests[outport_id].emplace_back(v);
        outflow_load_profiles[outport_id].emplace_back(E::LoadItem{ t, v });
      }
      t_max = std::max(t, t_max);
    }
    adevs::Digraph<E::FlowValueType, E::Time> network;
    std::vector<E::Sink*> outflow_drivers{};
    for (std::size_t outport_id{ 0 }; outport_id < num_outflows; ++outport_id) {
      auto d = new E::Sink{
        "outflow-from-mux(" + std::to_string(outport_id) + ")",
        E::ComponentType::Load,
        stream,
        outflow_load_profiles[outport_id],
        false };
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
    for (std::size_t inport_id{ 0 }; inport_id < num_inflows; ++inport_id) {
      auto d = new E::Source{
        "inflow-to-mux(" + std::to_string(inport_id) + ")",
        E::ComponentType::Source,
        stream,
        use_limited_source ? source_limit : ED::supply_unlimited_value };
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
    const std::size_t max_no_advance{ num_events * 4 };
    std::size_t non_advance_count{ 0 };
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
    for (std::size_t outport_id{ 0 }; outport_id < num_outflows; ++outport_id) {
      outflow_results[outport_id] = results[id + "-outflow(" + std::to_string(outport_id) + ")"];
      for (const auto& data : results["outflow-from-mux(" + std::to_string(outport_id) + ")"]) {
        outflow_tss[outport_id].emplace_back(data.time);
        outflow_fss[outport_id].emplace_back(data.achieved_value);
        outflow_fss_req[outport_id].emplace_back(data.requested_value);
      }
    }
    for (std::size_t inport_id{ 0 }; inport_id < num_inflows; ++inport_id) {
      inflow_results[inport_id] = results[id + "-inflow(" + std::to_string(inport_id) + ")"];
      for (const auto& data : results["inflow-to-mux(" + std::to_string(inport_id) + ")"]) {
        inflow_tss[inport_id].emplace_back(data.time);
        inflow_fss[inport_id].emplace_back(data.achieved_value);
        inflow_fss_req[inport_id].emplace_back(data.requested_value);
      }
    }
    E::RealTimeType time{ 0 };
    for (std::size_t idx{ 0 }; idx < (inflow_results[0].size() - 1); ++idx) {
      std::ostringstream oss{};
      oss << "idx            : " << idx << "\n";
      E::FlowValueType mux_reported_inflow{ 0.0 };
      E::FlowValueType driver_reported_inflow{ 0.0 };
      E::FlowValueType mux_reported_outflow{ 0.0 };
      E::FlowValueType driver_reported_outflow{ 0.0 };
      time = outflow_results[0][idx].time;
      oss << "time           : " << time << "\n";
      for (std::size_t outport_id{ 0 }; outport_id < num_outflows; ++outport_id) {
        ASSERT_EQ(time, outflow_results[outport_id][idx].time) << oss.str();
        auto mux_outflow{ outflow_results[outport_id][idx].achieved_value };
        mux_reported_outflow += mux_outflow;
        auto driver_outflow{
          EU::interpolate_value(
              time,
              outflow_tss[outport_id],
              outflow_fss[outport_id]) };
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
      for (std::size_t inport_id{ 0 }; inport_id < num_inflows; ++inport_id) {
        ASSERT_EQ(time, inflow_results[inport_id][idx].time) << oss.str();
        auto mux_inflow{ inflow_results[inport_id][idx].achieved_value };
        mux_reported_inflow += mux_inflow;
        auto driver_inflow{
          EU::interpolate_value(
              time,
              inflow_tss[inport_id],
              inflow_fss[inport_id]) };
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
      auto error{ mux_reported_inflow - mux_reported_outflow };
      ASSERT_NEAR(error, 0.0, 1e-6) << oss.str();
    }
  }

  TEST(ErinBasicsTest, Test_Port3)
  {
    namespace ED = erin::devs;

    auto p = ED::Port3{};
    ED::FlowValueType r{ 10.0 };
    ED::FlowValueType a{ 10.0 };
    ED::FlowValueType available{ 40.0 };
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
      true,
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
      true,
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
      true,
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

  TEST(ErinBasicsTest, Test_new_port_scheme)
  {
    namespace ED = erin::devs;

    constexpr std::size_t num_events{ 10'000 };
    constexpr double efficiency{ 0.5 };
    constexpr int flow_max{ 100 };

    std::default_random_engine generator{};
    std::uniform_int_distribution<int> flow_dist(0, flow_max);

    auto pout = ED::Port3{};
    auto ploss = ED::Port3{};
    auto pwaste = ED::Port3{};
    auto pin = ED::Port3{};
    auto outflow = ED::Port3{};
    auto inflow = ED::Port3{};
    auto lossflow = ED::Port3{};

    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      double max_inflow{ static_cast<ED::FlowValueType>(flow_dist(generator)) };
      double outflow_req{ static_cast<ED::FlowValueType>(flow_dist(generator)) };
      double lossflow_req{ static_cast<ED::FlowValueType>(flow_dist(generator)) };
      auto outflow_update = outflow.with_requested(outflow_req);
      outflow = outflow_update.port;
      auto lossflow_update = lossflow.with_requested(lossflow_req);
      lossflow = lossflow_update.port;
      auto inflow_update = inflow.with_achieved(std::min(max_inflow, inflow.get_requested()));
      inflow = inflow_update.port;
      while (outflow_update.send_request || lossflow_update.send_request || inflow_update.send_achieved) {
        bool resend_inflow_request{ false };
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
        auto total_lossflow{ pin.get_achieved() - pout.get_achieved() };
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
          - (pout.get_achieved() + ploss.get_achieved() + pwaste.get_achieved()) };
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
        - (outflow.get_achieved() + lossflow.get_achieved() + pwaste.get_achieved()) };
      ASSERT_NEAR(energy_balance_v2, 0.0, 1e-6)
        << "energy_balance_v2: " << energy_balance_v2 << "\n"
        << "inflow: " << inflow << "\n"
        << "outflow: " << outflow << "\n"
        << "lossflow: " << lossflow << "\n"
        << "pwaste: " << pwaste << "\n";
    }
  }

  TEST(ErinBasicsTest, Test_new_port_scheme_v2)
  {
    namespace ED = erin::devs;

    constexpr std::size_t num_events{ 10'000 };
    constexpr int flow_max{ 100 };

    std::default_random_engine generator{};
    std::uniform_int_distribution<int> flow_dist(0, flow_max);

    auto pout = ED::Port3{};
    auto pin = ED::Port3{};
    auto outflow = ED::Port3{};
    auto inflow = ED::Port3{};

    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      double max_inflow{ static_cast<ED::FlowValueType>(flow_dist(generator)) };
      double outflow_req{ static_cast<ED::FlowValueType>(flow_dist(generator)) };
      auto outflow_update = outflow.with_requested(outflow_req);
      outflow = outflow_update.port;
      auto inflow_update = inflow.with_requested_and_available(inflow.get_requested(), max_inflow);
      inflow = inflow_update.port;
      std::size_t no_advance{ 0 };
      std::size_t max_no_advance{ 1'000 };
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
        auto energy_balance{ pin.get_achieved() - pout.get_achieved() };
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
      auto energy_balance_v2{ inflow.get_achieved() - outflow.get_achieved() };
      ASSERT_NEAR(energy_balance_v2, 0.0, 1e-6)
        << "idx: " << idx << "\n"
        << "energy_balance_v2: " << energy_balance_v2 << "\n"
        << "inflow: " << inflow << "\n"
        << "outflow: " << outflow << "\n";
    }
  }

  TEST(ErinBasicsTest, Test_schedule_state_at_time)
  {
    namespace E = ERIN;
    std::vector<E::TimeState> schedule{
      E::TimeState{0, true},
      E::TimeState{10, false},
      E::TimeState{40, true},
      E::TimeState{50, false} };
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
    ERIN::RealTimeType dt{ -1 };
    for (const auto& ts : schedule) {
      if (ts.time >= current_time) {
        dt = ts.time - current_time;
        break;
      }
    }
    return dt;
  }

  TEST(ErinBasicsTest, Test_load_and_source_comprehensive)
  {
    namespace E = ERIN;
    namespace ED = erin::devs;
    namespace EU = erin::utils;
    const std::size_t num_events{ comprehensive_test_num_events };
    const std::vector<bool> has_flow_limit_options{ true, false };
    const E::FlowValueType max_source_outflow{ 50.0 };
    const unsigned seed = 17; // std::chrono::system_clock::now().time_since_epoch().count();

    // std::cout << "seed: " << seed << "\n";
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> dt_dist(0, 10);
    std::uniform_int_distribution<int> flow_dist(0, 100);

    std::string stream{ "stream" };
    std::string source_id{ "source" };
    std::string sink_id{ "sink" };

    for (const auto& has_flow_limit : has_flow_limit_options) {
      std::vector<E::RealTimeType> expected_times{};
      std::vector<E::FlowValueType> expected_flows_req{};
      std::vector<E::FlowValueType> expected_flows_ach{};
      std::vector<E::LoadItem> load_profile{};

      E::RealTimeType t{ 0 };
      for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
        auto new_load{ static_cast<E::FlowValueType>(flow_dist(generator)) };
        load_profile.emplace_back(E::LoadItem{ t, new_load });
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
      for (std::size_t idx{ 0 }; idx < expected_times.size(); ++idx) {
        auto flow_r{ expected_flows_req[idx] };
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

  TEST(ErinBasicsTest, Test_on_off_switch_comprehensive)
  {
    namespace E = ERIN;
    namespace ED = erin::devs;
    namespace EU = erin::utils;
    const std::size_t num_events{ comprehensive_test_num_events };
    const std::size_t num_time_state_transitions{ 1'000 };
    const auto t_end{ static_cast<E::RealTimeType>(num_events * 5) };

    unsigned seed = 17; // std::chrono::system_clock::now().time_since_epoch().count();
    // std::cout << "seed: " << seed << "\n";
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> dt_dist(0, 10);
    std::uniform_int_distribution<int> flow_dist(0, 100);

    std::string stream{ "stream" };
    std::string source_id{ "source" };
    std::string sink_id{ "sink" };
    std::string switch_id{ "switch" };

    std::vector<E::RealTimeType> expected_times{};
    std::vector<E::FlowValueType> expected_flows_req{};
    std::vector<E::FlowValueType> expected_flows_ach{};
    std::vector<E::LoadItem> load_profile{};
    std::vector<E::TimeState> schedule{};

    E::RealTimeType t{ 0 };
    bool flag{ true };
    for (std::size_t idx{ 0 }; idx < num_time_state_transitions; ++idx) {
      schedule.emplace_back(
        E::TimeState{ t, flag });
      flag = !flag;
      t += (static_cast<E::RealTimeType>(dt_dist(generator)) + 1) * 100;
      if (t > t_end) {
        break;
      }
    }
    t = 0;
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      auto new_load{ static_cast<E::FlowValueType>(flow_dist(generator)) };
      load_profile.emplace_back(E::LoadItem{ t, new_load });
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
    for (std::size_t idx{ 0 }; idx < expected_times.size(); ++idx) {
      auto time{ expected_times[idx] };
      auto flow_r{ expected_flows_req[idx] };
      auto flag{ E::schedule_state_at_time(schedule, time) };
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

  TEST(ErinBasicsTest, Test_flow_limits_comprehensive)
  {
    namespace E = ERIN;
    namespace ED = erin::devs;
    namespace EU = erin::utils;

    const std::size_t num_events{ comprehensive_test_num_events };
    const E::FlowValueType max_lim_flow{ 75.0 };
    const E::FlowValueType max_src_flow{ 50.0 };
    const bool source_is_limited{ false };

    unsigned seed = 17; // std::chrono::system_clock::now().time_since_epoch().count();
    // std::cout << "seed: " << seed << "\n";
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> dt_dist(0, 10);
    std::uniform_int_distribution<int> flow_dist(0, 100);

    std::string stream{ "stream" };
    std::string source_id{ "source" };
    std::string sink_id{ "sink" };
    std::string lim_id{ "flow_limits" };

    std::vector<E::RealTimeType> expected_times{};
    std::vector<E::FlowValueType> expected_outflows_req{};
    std::vector<E::FlowValueType> expected_outflows_ach{};
    std::vector<E::FlowValueType> expected_inflows_req{};
    std::vector<E::FlowValueType> expected_inflows_ach{};
    std::vector<E::LoadItem> load_profile{};

    E::RealTimeType t{ 0 };
    t = 0;
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      auto new_load{ static_cast<E::FlowValueType>(flow_dist(generator)) };
      load_profile.emplace_back(E::LoadItem{ t, new_load });
      auto dt = static_cast<E::RealTimeType>(dt_dist(generator));
      if (dt > 0) {
        expected_times.emplace_back(t);
        expected_outflows_req.emplace_back(new_load);
        expected_inflows_req.emplace_back(std::min(new_load, max_lim_flow));
        auto flow_a{ std::min(
          new_load,
          source_is_limited
          ? std::min(max_src_flow, max_lim_flow)
          : max_lim_flow) };
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

  TEST(ErinBasicsTest, Test_flow_limits_function_cases)
  {
    namespace E = ERIN;
    namespace ED = erin::devs;

    const E::FlowValueType upper_limit{ 75.0 };
    const E::FlowValueType lower_limit{ 0.0 };
    const E::RealTimeType t{ 1013 };

    auto xs = std::vector<ED::PortValue>{
      ED::PortValue{ED::inport_inflow_achieved, 30.0},
      ED::PortValue{ED::inport_outflow_request, 26.0} };
    auto lim = ED::FlowLimits{ lower_limit, upper_limit };
    auto s = ED::FlowLimitsState{
      t,
      ED::Port3{50.0, 75.0}, // inflow
      ED::Port3{50.0, 50.0}, // outflow
      lim,
      true,   // report IR
      true };  // report OA
    auto next_s = ED::flow_limits_confluent_transition(s, xs);
    auto expected_next_s = ED::FlowLimitsState{
      t,
      ED::Port3{26.0, 30.0}, // inflow
      ED::Port3{26.0, 26.0}, // outflow
      lim,
      true,   // report IR
      true };  // report OA
    ASSERT_EQ(expected_next_s, next_s);
  }

  TEST(ErinBasicsTest, Test_uncontrolled_source_with_sink_comprehensive)
  {
    namespace E = ERIN;
    namespace EU = erin::utils;
    namespace ED = erin::devs;

    const std::size_t num_events{ comprehensive_test_num_events };

    unsigned seed = 17; // std::chrono::system_clock::now().time_since_epoch().count();
    // std::cout << "seed: " << seed << "\n";
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> dt_dist(0, 10);
    std::uniform_int_distribution<int> flow_dist(0, 100);

    std::string stream{ "stream" };
    std::string source_id{ "source" };
    std::string sink_id{ "sink" };

    std::vector<E::LoadItem> load_profile{};
    std::vector<E::LoadItem> source_profile{};

    E::RealTimeType t{ 0 };
    E::RealTimeType t_max{ 0 };
    std::unordered_set<E::RealTimeType> time_set{};
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      load_profile.emplace_back(
        E::LoadItem{
          t,
          static_cast<E::FlowValueType>(flow_dist(generator)) });
      time_set.emplace(t);
      t += static_cast<E::RealTimeType>(dt_dist(generator));
    }
    t_max = t;
    t = 0;
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      source_profile.emplace_back(
        E::LoadItem{
          t,
          static_cast<E::FlowValueType>(flow_dist(generator)) });
      time_set.emplace(t);
      t += static_cast<E::RealTimeType>(dt_dist(generator));
    }
    t_max = std::max(t_max, t);
    time_set.emplace(t_max);
    auto sink = new E::Sink(
      sink_id,
      E::ComponentType::Load,
      stream,
      load_profile,
      false);
    auto source = new E::UncontrolledSource(
      source_id,
      E::ComponentType::Source,
      stream,
      source_profile);
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

    ASSERT_EQ(results.size(), 4);
    ASSERT_EQ(time_set.size(), results[source_id + "-outflow"].size());
    ASSERT_EQ(time_set.size(), results[sink_id].size());

    for (std::size_t idx{ 0 }; idx < results[sink_id].size(); ++idx) {
      std::ostringstream oss{};
      oss << "idx: " << idx << "\n";
      const auto& src = results[source_id + "-outflow"][idx];
      oss << "src: " << src << "\n";
      const auto& src_in = results[source_id + "-inflow"][idx];
      oss << "src_in: " << src_in << "\n";
      const auto& src_loss = results[source_id + "-lossflow"][idx];
      oss << "src_loss: " << src_loss << "\n";
      const auto& snk = results[sink_id][idx];
      oss << "snk: " << snk << "\n";
      ASSERT_EQ(src.time, snk.time) << oss.str();
      ASSERT_EQ(src.requested_value, snk.requested_value) << oss.str();
      ASSERT_EQ(src.achieved_value, snk.achieved_value) << oss.str();
      ASSERT_TRUE(src.requested_value >= src.achieved_value) << oss.str();
      auto error{
        src_in.achieved_value
        - (src.achieved_value + src_loss.achieved_value) };
      oss << "error: " << error << "\n";
      ASSERT_NEAR(error, 0.0, 1e-6) << oss.str();
    }
  }

  TEST(DevsModelTest, Test_flow_meter_functions)
  {
    namespace E = ERIN;
    namespace ED = erin::devs;

    auto s = ED::flow_meter_make_state();
    auto other = ED::FlowMeterState{};
    ASSERT_EQ(s, other);
    other.report_outflow_achieved = true;
    const ED::FlowValueType inflow_request{ 100.0 };
    const ED::FlowValueType outflow_achieved{ 20.0 };
    other.port = ED::Port3{ inflow_request, outflow_achieved };
    ASSERT_NE(s, other);
    std::ostringstream oss{};
    oss << s;
    ASSERT_EQ(ED::flow_meter_time_advance(s), ED::infinity);
    ASSERT_EQ(ED::flow_meter_time_advance(other), 0);
    auto ys = ED::flow_meter_output_function(other);
    std::vector<ED::PortValue> expected_ys{
      ED::PortValue{ED::outport_outflow_achieved, outflow_achieved} };
    ASSERT_EQ(ys.size(), expected_ys.size());
    ASSERT_EQ(ys[0].port, expected_ys[0].port);
    ASSERT_EQ(ys[0].value, expected_ys[0].value);
    other.report_inflow_request = true;
    other.report_outflow_achieved = false;
    ys = ED::flow_meter_output_function(other);
    expected_ys = std::vector<ED::PortValue>{
      ED::PortValue{ED::outport_inflow_request, inflow_request} };
    ASSERT_EQ(ys.size(), expected_ys.size());
    ASSERT_EQ(ys[0].port, expected_ys[0].port);
    ASSERT_EQ(ys[0].value, expected_ys[0].value);
    expected_ys = std::vector<ED::PortValue>{};
    ys = ED::flow_meter_output_function(s);
    ASSERT_EQ(ys.size(), expected_ys.size());
    auto s1 = ED::flow_meter_internal_transition(other);
    other.report_outflow_achieved = false;
    other.report_inflow_request = false;
    ASSERT_EQ(s1, other);
    const E::FlowValueType outflow_request_2{ 30.0 };
    std::vector<ED::PortValue> xs{
      ED::PortValue{ED::inport_outflow_request, outflow_request_2} };
    E::RealTimeType elapsed{ 5 };
    auto s2 = ED::flow_meter_external_transition(s, elapsed, xs);
    ED::FlowMeterState expected_s2{
      elapsed, ED::Port3{outflow_request_2, 0.0}, true, false };
    ASSERT_EQ(s2, expected_s2);
    xs = std::vector<ED::PortValue>{
      ED::PortValue{ED::inport_inflow_achieved, outflow_request_2} };
    auto s3 = ED::flow_meter_confluent_transition(s2, xs);
    ED::FlowMeterState expected_s3{
      elapsed, ED::Port3{outflow_request_2, outflow_request_2}, false, true };
    ASSERT_EQ(s3, expected_s3);
  }

  TEST(ErinBasicsTest, Test_flow_meter_element_comprehensive)
  {
    namespace E = ERIN;
    namespace EU = erin::utils;
    namespace ED = erin::devs;

    const std::size_t num_events{ comprehensive_test_num_events };

    unsigned seed = 17; // std::chrono::system_clock::now().time_since_epoch().count();
    // std::cout << "seed: " << seed << "\n";
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> dt_dist(0, 10);
    std::uniform_int_distribution<int> flow_dist(0, 100);

    std::string stream{ "stream" };
    const std::string source_id{ "source" };
    const std::string sink_id{ "sink" };
    const std::string meter_id{ "meter" };

    std::vector<E::LoadItem> load_profile{};
    std::vector<E::LoadItem> source_profile{};

    E::RealTimeType t{ 0 };
    E::RealTimeType t_max{ 0 };
    std::unordered_set<E::RealTimeType> time_set{};
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      load_profile.emplace_back(
        E::LoadItem{
          t,
          static_cast<E::FlowValueType>(flow_dist(generator)) });
      time_set.emplace(t);
      t += static_cast<E::RealTimeType>(dt_dist(generator));
    }
    t_max = t;
    t = 0;
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      source_profile.emplace_back(
        E::LoadItem{
          t,
          static_cast<E::FlowValueType>(flow_dist(generator)) });
      time_set.emplace(t);
      t += static_cast<E::RealTimeType>(dt_dist(generator));
    }
    t_max = std::max(t_max, t);
    time_set.emplace(t_max);
    auto sink = new E::Sink(
      sink_id,
      E::ComponentType::Load,
      stream,
      load_profile,
      false);
    auto source = new E::UncontrolledSource(
      source_id,
      E::ComponentType::Source,
      stream,
      source_profile);
    auto meter = new E::FlowMeter(
      meter_id,
      E::ComponentType::PassThrough,
      stream);
    std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
    source->set_flow_writer(fw);
    source->set_recording_on();
    sink->set_flow_writer(fw);
    sink->set_recording_on();
    meter->set_flow_writer(fw);
    meter->set_recording_on();

    adevs::Digraph<E::FlowValueType, E::Time> network{};
    network.couple(
      sink, E::Sink::outport_inflow_request,
      meter, E::FlowMeter::inport_outflow_request);
    network.couple(
      meter, E::FlowMeter::outport_inflow_request,
      source, E::Source::inport_outflow_request);
    network.couple(
      source, E::Source::outport_outflow_achieved,
      meter, E::FlowMeter::inport_inflow_achieved);
    network.couple(
      meter, E::FlowMeter::outport_outflow_achieved,
      sink, E::Sink::inport_inflow_achieved);
    adevs::Simulator<E::PortValue, E::Time> sim{};
    network.add(&sim);
    while (sim.next_event_time() < E::inf) {
      sim.exec_next_event();
    }
    fw->finalize_at_time(t_max);
    auto results = fw->get_results();
    fw->clear();

    ASSERT_EQ(results.size(), 5);
    ASSERT_EQ(time_set.size(), results[source_id + "-outflow"].size());
    ASSERT_EQ(time_set.size(), results[sink_id].size());
    ASSERT_EQ(time_set.size(), results[meter_id].size());

    for (std::size_t idx{ 0 }; idx < results[sink_id].size(); ++idx) {
      std::ostringstream oss{};
      oss << "idx: " << idx << "\n";
      const auto& src = results[source_id + "-outflow"][idx];
      oss << "src: " << src << "\n";
      const auto& src_in = results[source_id + "-inflow"][idx];
      oss << "src_in: " << src_in << "\n";
      const auto& src_loss = results[source_id + "-lossflow"][idx];
      oss << "src_loss: " << src_loss << "\n";
      const auto& snk = results[sink_id][idx];
      oss << "snk: " << snk << "\n";
      const auto& mtr = results[meter_id][idx];
      oss << "mtr: " << mtr << "\n";
      ASSERT_EQ(src.time, snk.time) << oss.str();
      ASSERT_EQ(mtr.time, src.time) << oss.str();
      ASSERT_EQ(src.requested_value, snk.requested_value) << oss.str();
      ASSERT_EQ(src.requested_value, mtr.requested_value) << oss.str();
      ASSERT_EQ(src.achieved_value, snk.achieved_value) << oss.str();
      ASSERT_EQ(mtr.achieved_value, snk.achieved_value) << oss.str();
      ASSERT_TRUE(src.requested_value >= src.achieved_value) << oss.str();
      auto error{
        src_in.achieved_value
        - (src.achieved_value + src_loss.achieved_value) };
      oss << "uncontrolled source energy balance error: " << error << "\n";
      ASSERT_NEAR(error, 0.0, 1e-6) << oss.str();
      error = src.achieved_value - snk.achieved_value;
      oss << "network energy balance error: " << error << "\n";
      ASSERT_NEAR(error, 0.0, 1e-6) << oss.str();
    }
  }

  TEST(DevsModelTest, Test_bad_behavior_for_converter)
  {
    namespace E = ERIN;
    namespace ED = erin::devs;
    // Motivating Example:
    // delta_ext::turbine::Converter
    // - e  = 0
    // - xs = [PortValue{port=0, value=294.118}]
    // - s  = {:t 0, :inflow {:r 294.118, :a 0} :outflow {:r 100, :a 0} :lossflow {:r 100, :a 0} :wasteflow {:r 94.1176, :a 0} :report-ir? 0 :report-oa? 0 :report-la? 0}
    // - s* = {:t 0, :inflow {:r 294.118, :a 294.118} :outflow {:r 100, :a 100} :lossflow {:r 100, :a 100} :wasteflow {:r 94.1176, :a 94.1176} :report-ir? 0 :report-oa? 0 :report-la? 1}
    //
    const double efficiency{ 0.34 };
    auto s = ED::make_converter_state(efficiency);
    const ED::FlowValueType outflow{ 100.0 };
    const ED::FlowValueType lossflow{ 100.0 };
    const ED::FlowValueType inflow{ outflow / efficiency };
    const ED::FlowValueType wasteflow{ inflow - (outflow + lossflow) };
    s.inflow_port = ED::Port3{ inflow, 0.0 };
    s.outflow_port = ED::Port3{ outflow, 0.0 };
    s.lossflow_port = ED::Port3{ lossflow, 0.0 };
    s.wasteflow_port = ED::Port3{ wasteflow, 0.0 };
    s.report_inflow_request = false;
    s.report_lossflow_achieved = false;
    s.report_outflow_achieved = false;
    std::vector<ED::PortValue> xs{
      ED::PortValue{ED::inport_inflow_achieved, inflow} };
    auto s2 = ED::converter_external_transition(s, 0, xs);
    auto expected_s2 = ED::make_converter_state(efficiency);
    expected_s2.time = 0;
    expected_s2.inflow_port = ED::Port3{ inflow, inflow };
    expected_s2.outflow_port = ED::Port3{ outflow, outflow };
    expected_s2.lossflow_port = ED::Port3{ lossflow, lossflow };
    expected_s2.wasteflow_port = ED::Port3{ wasteflow, wasteflow };
    expected_s2.report_inflow_request = false;
    expected_s2.report_outflow_achieved = true;
    expected_s2.report_lossflow_achieved = true;
    ASSERT_EQ(s2.time, expected_s2.time);
    ASSERT_EQ(s2.inflow_port, expected_s2.inflow_port);
    ASSERT_EQ(s2.outflow_port, expected_s2.outflow_port);
    ASSERT_EQ(s2.lossflow_port, expected_s2.lossflow_port);
    ASSERT_EQ(s2.wasteflow_port, expected_s2.wasteflow_port);
    ASSERT_TRUE(s2.report_lossflow_achieved);
    ASSERT_TRUE(s2.report_outflow_achieved);
    ASSERT_FALSE(s2.report_inflow_request);
  }

  TEST(DevsModelTest, Test_mover_element_comprehensive)
  {
    namespace E = ERIN;
    namespace EU = erin::utils;
    namespace ED = erin::devs;

    const std::size_t num_events{ comprehensive_test_num_events };
    const E::FlowValueType mover_cop{ 5.0 };

    unsigned seed = 23; // std::chrono::system_clock::now().time_since_epoch().count();
    // std::cout << "seed: " << seed << "\n";
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> dt_dist(0, 10);
    std::uniform_int_distribution<int> flow_dist(0, 100);

    std::string moved_stream{ "heat" };
    std::string power_stream{ "electricity" };
    const std::string heat_source_id{ "moved_source" };
    const std::string power_source_id{ "power_source" };
    const std::string heat_sink_id{ "heat_sink" };
    const std::string mover_id{ "mover" };

    std::vector<E::LoadItem> load_profile{};
    std::vector<E::LoadItem> heat_source_profile{};
    std::vector<E::LoadItem> power_source_profile{};

    E::RealTimeType t{ 0 };
    E::RealTimeType t_max{ 0 };
    std::unordered_set<E::RealTimeType> time_set{};
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      power_source_profile.emplace_back(
        E::LoadItem{
          t,
          static_cast<E::FlowValueType>(flow_dist(generator)) });
      time_set.emplace(t);
      t += static_cast<E::RealTimeType>(dt_dist(generator));
    }
    t_max = t;
    t = 0;
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      heat_source_profile.emplace_back(
        E::LoadItem{
          t,
          static_cast<E::FlowValueType>(flow_dist(generator)) });
      time_set.emplace(t);
      t += static_cast<E::RealTimeType>(dt_dist(generator));
    }
    t_max = std::max(t_max, t);
    t = 0;
    for (std::size_t idx{ 0 }; idx < num_events; ++idx) {
      load_profile.emplace_back(
        E::LoadItem{
          t,
          static_cast<E::FlowValueType>(flow_dist(generator)) });
      time_set.emplace(t);
      t += static_cast<E::RealTimeType>(dt_dist(generator));
    }
    t_max = std::max(t_max, t);
    time_set.emplace(t_max);
    auto heat_sink = new E::Sink(
      heat_sink_id,
      E::ComponentType::Load,
      moved_stream,
      load_profile,
      false);
    auto heat_source = new E::UncontrolledSource(
      heat_source_id,
      E::ComponentType::Source,
      moved_stream,
      heat_source_profile);
    auto power_source = new E::UncontrolledSource(
      power_source_id,
      E::ComponentType::Source,
      power_stream,
      power_source_profile);
    auto mover = new E::Mover(
      mover_id,
      E::ComponentType::Mover,
      moved_stream,
      power_stream,
      moved_stream,
      mover_cop);
    std::shared_ptr<E::FlowWriter> fw = std::make_shared<E::DefaultFlowWriter>();
    heat_sink->set_flow_writer(fw);
    heat_sink->set_recording_on();
    heat_source->set_flow_writer(fw);
    heat_source->set_recording_on();
    power_source->set_flow_writer(fw);
    power_source->set_recording_on();
    mover->set_flow_writer(fw);
    mover->set_recording_on();

    adevs::Digraph<E::FlowValueType, E::Time> network{};
    network.couple(
      heat_sink, E::Sink::outport_inflow_request,
      mover, E::Mover::inport_outflow_request);
    network.couple(
      mover, E::Mover::outport_inflow_request,
      heat_source, E::UncontrolledSource::inport_outflow_request);
    network.couple(
      mover, E::Mover::outport_inflow_request + 1,
      power_source, E::UncontrolledSource::inport_outflow_request);
    network.couple(
      heat_source, E::UncontrolledSource::outport_outflow_achieved,
      mover, E::Mover::inport_inflow_achieved);
    network.couple(
      power_source, E::UncontrolledSource::outport_outflow_achieved,
      mover, E::Mover::inport_inflow_achieved + 1);
    network.couple(
      mover, E::Mover::outport_outflow_achieved,
      heat_sink, E::Sink::inport_inflow_achieved);
    adevs::Simulator<E::PortValue, E::Time> sim{};
    network.add(&sim);
    while (sim.next_event_time() < E::inf) {
      sim.exec_next_event();
    }
    fw->finalize_at_time(t_max);
    auto results = fw->get_results();
    fw->clear();

    ASSERT_EQ(results.size(), 10);
    ASSERT_EQ(time_set.size(), results[heat_source_id + "-outflow"].size());
    ASSERT_EQ(time_set.size(), results[power_source_id + "-outflow"].size());
    ASSERT_EQ(time_set.size(), results[heat_sink_id].size());
    ASSERT_EQ(time_set.size(), results[mover_id + "-inflow(0)"].size());

    for (std::size_t idx{ 0 }; idx < results[heat_sink_id].size(); ++idx) {
      std::ostringstream oss{};
      oss << "idx: " << idx << "\n";
      const auto& ht_src = results[heat_source_id + "-outflow"][idx];
      oss << "time: " << ht_src.time << "\n";
      oss << "ht_src: " << ht_src << "\n";
      const auto& ht_src_in = results[heat_source_id + "-inflow"][idx];
      oss << "ht_src_in: " << ht_src_in << "\n";
      const auto& ht_src_loss = results[heat_source_id + "-lossflow"][idx];
      oss << "src_loss: " << ht_src_loss << "\n";
      const auto& pw_src = results[power_source_id + "-outflow"][idx];
      oss << "pw_src: " << pw_src << "\n";
      const auto& pw_src_in = results[power_source_id + "-inflow"][idx];
      oss << "pw_src_in: " << pw_src_in << "\n";
      const auto& pw_src_loss = results[power_source_id + "-lossflow"][idx];
      oss << "src_loss: " << pw_src_loss << "\n";
      const auto& ht_snk = results[heat_sink_id][idx];
      oss << "ht_snk: " << ht_snk << "\n";
      const auto& mvr_in0 = results[mover_id + "-inflow(0)"][idx];
      oss << "mvr_in(0): " << mvr_in0 << "\n";
      const auto& mvr_in1 = results[mover_id + "-inflow(1)"][idx];
      oss << "mvr_in(1): " << mvr_in1 << "\n";
      const auto& mvr_out = results[mover_id + "-outflow"][idx];
      oss << "mvr_out: " << mvr_out << "\n";
      ASSERT_EQ(ht_src.time, ht_snk.time) << oss.str();
      ASSERT_EQ(mvr_in0.time, ht_src.time) << oss.str();
      ASSERT_EQ(ht_src.time, pw_src.time) << oss.str();
      ASSERT_EQ(ht_src.requested_value, mvr_in0.requested_value) << oss.str();
      ASSERT_EQ(pw_src.requested_value, mvr_in1.requested_value) << oss.str();
      ASSERT_EQ(ht_snk.requested_value, mvr_out.requested_value) << oss.str();
      ASSERT_EQ(ht_src.achieved_value, mvr_in0.achieved_value) << oss.str();
      ASSERT_EQ(pw_src.achieved_value, mvr_in1.achieved_value) << oss.str();
      ASSERT_EQ(ht_snk.achieved_value, mvr_out.achieved_value) << oss.str();
      auto error{
        mvr_in0.achieved_value + mvr_in1.achieved_value
        - mvr_out.achieved_value };
      oss << "energy balance error on mover: " << error << "\n";
      ASSERT_NEAR(error, 0.0, 1e-6) << oss.str();
    }
  }

  TEST(ErinBasicsTest, Test_mover_cases)
  {
    namespace E = ERIN;
    namespace ED = erin::devs;

    const E::FlowValueType mover_cop{ 5.0 };

    const auto d = ED::make_mover_data(mover_cop);
    auto s = ED::make_mover_state();
    s.inflow0_port = ED::Port3{ 25.8333, 0.0 };
    s.inflow1_port = ED::Port3{ 5.16667, 0.0 };
    s.outflow_port = ED::Port3{ 31.0, 0.0 };
    s.report_inflow0_request = true;
    s.report_inflow1_request = true;
    s.report_outflow_achieved = false;
    std::vector<ED::PortValue> xs{
      ED::PortValue{ED::inport_outflow_request, 74.0},
      ED::PortValue{ED::inport_inflow_achieved, 22.0},
      ED::PortValue{ED::inport_inflow_achieved + 1, 5.0},
    };
    auto s1 = ED::mover_confluent_transition(d, s, xs);
    ASSERT_NEAR(
      s1.inflow0_port.get_achieved() + s1.inflow1_port.get_achieved()
      - s1.outflow_port.get_achieved(), 0.0, 1e-6);
  }


  TEST(ErinBasicsTest, Test_that_we_can_modify_schedule_for_reliability)
  {
    ERIN::RealTimeType max_time{ 2000 };
    auto sch = std::vector<ERIN::TimeState>{ {0,1},{100,0},{200,1},{1000,0},{1100,1} };
    ASSERT_EQ(sch.size(), 5);
    auto new_sch = erin::fragility::modify_schedule_for_fragility(sch, false, false, 0, max_time);
    ASSERT_EQ(sch, new_sch);
    auto actual_1 = erin::fragility::modify_schedule_for_fragility(sch, true, false, 0, max_time);
    auto expected_1 = std::vector<ERIN::TimeState>{ {0,0} };
    ASSERT_EQ(actual_1, expected_1);
    auto actual_2 = erin::fragility::modify_schedule_for_fragility(sch, true, true, 1500, max_time);
    auto expected_2 = std::vector<ERIN::TimeState>{ {0,0},{1500,1} };
    ASSERT_EQ(actual_2, expected_2);
    auto actual_3 = erin::fragility::modify_schedule_for_fragility(sch, true, true, 800, max_time);
    auto expected_3 = std::vector<ERIN::TimeState>{ {0,0},{800,1},{1000,0},{1100,1} };
    ASSERT_EQ(actual_3, expected_3);
    auto actual_4 = erin::fragility::modify_schedule_for_fragility(sch, true, true, 1050, max_time);
    auto expected_4 = std::vector<ERIN::TimeState>{ {0,0},{1050,1} };
    ASSERT_EQ(actual_4, expected_4);
    auto actual_5 = erin::fragility::modify_schedule_for_fragility(sch, true, true, 1000, max_time);
    auto expected_5 = std::vector<ERIN::TimeState>{ {0,0},{1000,1} };
    ASSERT_EQ(actual_5, expected_5);
    auto actual_6 = erin::fragility::modify_schedule_for_fragility(sch, true, true, 150, max_time);
    auto expected_6 = std::vector<ERIN::TimeState>{ {0,0},{150,1},{1000,0},{1100,1} };
    ASSERT_EQ(actual_6, expected_6);
    auto actual_7 = erin::fragility::modify_schedule_for_fragility(sch, true, true, 3000, max_time);
    auto expected_7 = std::vector<ERIN::TimeState>{ {0,0} };
    ASSERT_EQ(actual_7, expected_7);
    auto actual_8 = erin::fragility::modify_schedule_for_fragility(sch, true, true, max_time, max_time);
    auto expected_8 = std::vector<ERIN::TimeState>{ {0,0},{max_time,1} };
    ASSERT_EQ(actual_8, expected_8);
  }


  TEST(ErinBasicsTest, Test_that_we_can_run_fragility_with_repair)
  {
    namespace E = ERIN;
    std::string input =
      "[simulation_info]\n"
      "rate_unit = \"kW\"\n"
      "quantity_unit = \"kJ\"\n"
      "time_unit = \"hours\"\n"
      "max_time = 300\n"
      "############################################################\n"
      "[loads.b1_electric_load]\n"
      "time_unit = \"hours\"\n"
      "rate_unit = \"kW\"\n"
      "time_rate_pairs = [[0.0,10.0],[300.0,0.0]]\n"
      "############################################################\n"
      "[components.electric_utility]\n"
      "type = \"source\"\n"
      "outflow = \"electricity\"\n"
      "fragility_modes = [\"power_line_down_and_repair\"]\n"
      "[components.b1_electric]\n"
      "type = \"load\"\n"
      "inflow = \"electricity\"\n"
      "loads_by_scenario.c4_hurricane = \"b1_electric_load\"\n"
      "############################################################\n"
      "[fragility_mode.power_line_down_and_repair]\n"
      "fragility_curve = \"power_line_down_by_high_wind\"\n"
      "# the repair_dist is optional; if not specified, there is no\n"
      "# repair for the component experiencing a fragility failure\n"
      "repair_dist = \"power_line_repair\"\n"
      "############################################################\n"
      "[fragility_curve.power_line_down_by_high_wind]\n"
      "vulnerable_to = \"wind_speed_mph\"\n"
      "type = \"linear\"\n"
      "lower_bound = 80.0\n"
      "upper_bound = 160.0\n"
      "############################################################\n"
      "[networks.nw]\n"
      "connections = [\n"
      "  [\"electric_utility:OUT(0)\", \"b1_electric:IN(0)\", \"electricity\"]\n"
      "]\n"
      "############################################################\n"
      "[dist.immediately]\n"
      "type = \"fixed\"\n"
      "value = 0\n"
      "time_unit = \"hours\"\n"
      "[dist.power_line_repair]\n"
      "type = \"fixed\"\n"
      "value = 100\n"
      "time_units = \"hours\"\n"
      "############################################################\n"
      "[scenarios.c4_hurricane]\n"
      "time_unit = \"hours\"\n"
      "occurrence_distribution = \"immediately\"\n"
      "duration = 300\n"
      "max_occurrences = 1\n"
      "network = \"nw\"\n"
      "intensity.wind_speed_mph = 180\n";
    auto m = E::make_main_from_string(input);
    auto run_all_results = m.run_all();
    const auto& scenario_results = run_all_results.get_results();
    ASSERT_TRUE(run_all_results.get_is_good());
    const std::size_t num_scenarios{ 1 };
    ASSERT_EQ(scenario_results.size(), num_scenarios);
    const std::string scenario_name{ "c4_hurricane" };
    const auto& scenario_instance_results = scenario_results.at(scenario_name);
    const std::size_t num_scenario_instances{ 1 };
    ASSERT_EQ(scenario_instance_results.size(), num_scenario_instances);
    const auto& inst_results = scenario_instance_results[0];
    const std::size_t num_comps{ 2 };
    const auto& comp_results = inst_results.get_results();
    ASSERT_EQ(comp_results.size(), num_comps);
    const auto& stats = inst_results.get_statistics();
    ASSERT_EQ(stats.size(), num_comps);
    const std::string b1_id{ "b1_electric" };
    const std::string utility_id{ "utility_electric" };
    const auto& b1_stats = stats.at(b1_id);
    const E::FlowValueType flow_request_kW{ 10.0 };
    const E::FlowValueType total_time_s{ 300.0 * 3600.0 };
    const E::FlowValueType uptime_s{ 200.0 * 3600.0 };
    const E::FlowValueType downtime_s{ total_time_s - uptime_s };
    const E::FlowValueType total_energy_delivered_kJ{ uptime_s * flow_request_kW };
    const E::FlowValueType load_not_served_kJ{ downtime_s * flow_request_kW };
    EXPECT_EQ(b1_stats.load_not_served, load_not_served_kJ);
    EXPECT_EQ(b1_stats.downtime, downtime_s);
    EXPECT_EQ(b1_stats.max_downtime, downtime_s);
    EXPECT_EQ(b1_stats.total_energy, total_energy_delivered_kJ);
  }


  TEST(ErinBasicsTest, Test_calculation_of_fragility_schedules)
  {
    namespace E = ERIN;
    namespace EF = erin::fragility;
    const std::string blue_sky_tag{ "blue_sky" };
    const std::string src_comp_tag{ "src" };
    const std::string snk_comp_tag{ "snk" };
    const std::unordered_map<std::string, EF::FragilityMode> fragility_modes{};
    const std::unordered_map<std::string, std::vector<std::int64_t>> scenario_schedules{
      {blue_sky_tag, {0}} };
    const std::unordered_map<
      std::string,
      std::unordered_map<
      std::string,
      std::vector<EF::FailureProbAndRepair>>> failure_probs_by_comp_id_by_scenario_id{
      {blue_sky_tag,
        {
          { src_comp_tag, {
            EF::FailureProbAndRepair{0.5, EF::no_repair_distribution},
            EF::FailureProbAndRepair{0.2, EF::no_repair_distribution}}},
          { snk_comp_tag, {
            EF::FailureProbAndRepair{0.1, EF::no_repair_distribution}}}
        }
      }
    };
    std::function<double()> rand_fn = []() { return 0.4; };
    erin::distribution::DistributionSystem ds{};
    ds.add_fixed("repair_in_24_hours", 24 * 3600);
    const auto fs = EF::calc_fragility_schedules(
      scenario_schedules,
      failure_probs_by_comp_id_by_scenario_id,
      rand_fn,
      ds);
    ASSERT_EQ(fs.size(), scenario_schedules.size());
    const auto& blue_sky_instances = fs.at(blue_sky_tag);
    ASSERT_EQ(
      blue_sky_instances.size(),
      scenario_schedules.at(blue_sky_tag).size());
    const auto& blue_sky_0 = blue_sky_instances[0];
    const auto& fpbc = failure_probs_by_comp_id_by_scenario_id.at(blue_sky_tag);
    ASSERT_EQ(blue_sky_0.size(), fpbc.size());
    const auto& blue_sky_0_src = blue_sky_0.at(src_comp_tag);
    const auto& blue_sky_0_snk = blue_sky_0.at(snk_comp_tag);
    ASSERT_EQ(blue_sky_0_src.scenario_tag, blue_sky_tag);
    ASSERT_EQ(blue_sky_0_snk.scenario_tag, blue_sky_tag);
    ASSERT_EQ(blue_sky_0_src.start_time_s, 0);
    ASSERT_EQ(blue_sky_0_snk.start_time_s, 0);
    ASSERT_EQ(blue_sky_0_src.is_failed, true);
    ASSERT_EQ(blue_sky_0_snk.is_failed, false);
  }


int
  main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
