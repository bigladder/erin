// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <iomanip>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "erin/all.h"

using namespace erin;

static double round_it(double n, unsigned int places = 2)
{
    double mult = std::pow(10.0, (double)places);
    return std::round(n * mult) / mult;
}

static auto kW_as_W = [](double p_kW) -> uint32_t
{ return static_cast<uint32_t>(std::round(p_kW * 1000.0)); };
static auto hours_as_seconds = [](double h) -> double { return h * 3600.0; };
static auto kWh_as_J = [](double kWh) -> double { return kWh * 3'600'000.0; };

TEST(Erin, Test1)
{
    Model m = {};
    auto src_id = Model_AddConstantSource(m, 100);
    auto load_id = Model_AddConstantLoad(m, 10);
    auto src_to_load_conn = Model_AddConnection(m, src_id, 0, load_id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].time_s, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].flows.size(), 1) << "size of flows must equal 1";

    auto src_to_load_result = ModelResults_GetFlowForConnection(m, src_to_load_conn, 0.0, results);
    EXPECT_TRUE(src_to_load_result.has_value()) << "connection result should have a value";
    EXPECT_EQ(src_to_load_result.value().actual_W, 10) << "actual value must equal 10";
    EXPECT_EQ(src_to_load_result.value().available_W, 100) << "available must equal 100";
    EXPECT_EQ(src_to_load_result.value().requested_W, 10) << "requested must equal 10";
}

TEST(Erin, Test2)
{
    Model m = {};
    auto src_id = Model_AddConstantSource(m, 100);
    auto load_id = Model_AddConstantLoad(m, 10);
    auto conv_id = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto src_to_conv_conn = Model_AddConnection(m, src_id, 0, conv_id.id, 0);
    auto conv_to_load_conn = Model_AddConnection(m, conv_id.id, 0, load_id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].time_s, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].flows.size(), 3) << "size of flows must equal 3";

    auto src_to_conv_results = ModelResults_GetFlowForConnection(m, src_to_conv_conn, 0.0, results);
    EXPECT_TRUE(src_to_conv_results.has_value()) << "source to converter must have results";
    EXPECT_EQ(src_to_conv_results.value().requested_W, 20) << "requested must equal 20";
    EXPECT_EQ(src_to_conv_results.value().actual_W, 20) << "actual value must equal 20";
    EXPECT_EQ(src_to_conv_results.value().available_W, 100) << "available must equal 100";

    auto conv_to_load_results =
        ModelResults_GetFlowForConnection(m, conv_to_load_conn, 0.0, results);
    EXPECT_TRUE(conv_to_load_results.has_value()) << "converter to load must have results";
    EXPECT_EQ(conv_to_load_results.value().requested_W, 10) << "requested must equal 10";
    EXPECT_EQ(conv_to_load_results.value().actual_W, 10) << "actual value must equal 10";
    EXPECT_EQ(conv_to_load_results.value().available_W, 50) << "available must equal 50";

    auto conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv_id.waste_connection_id, 0.0, results);
    EXPECT_TRUE(conv_to_waste_results.has_value()) << "converter to waste must have results";
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 10) << "requested must equal 10";
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 10) << "actual value must equal 10";
    EXPECT_EQ(conv_to_waste_results.value().available_W, 10) << "available must equal 10";
}

TEST(Erin, Test3)
{
    Model m = {};
    auto src_id = Model_AddConstantSource(m, 100);
    auto load1_id = Model_AddConstantLoad(m, 10);
    auto load2_id = Model_AddConstantLoad(m, 2);
    auto conv_id = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto src_to_conv_conn = Model_AddConnection(m, src_id, 0, conv_id.id, 0);
    auto conv_to_load1_conn = Model_AddConnection(m, conv_id.id, 0, load1_id, 0);
    auto conv_to_load2_conn = Model_AddConnection(m, conv_id.id, 1, load2_id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].time_s, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].flows.size(), 4) << "size of flows must equal 4";

    auto src_to_conv_results = ModelResults_GetFlowForConnection(m, src_to_conv_conn, 0.0, results);
    EXPECT_TRUE(src_to_conv_results.has_value()) << "source to converter must have results";
    EXPECT_EQ(src_to_conv_results.value().requested_W, 20) << "requested must equal 20";
    EXPECT_EQ(src_to_conv_results.value().actual_W, 20) << "actual value must equal 20";
    EXPECT_EQ(src_to_conv_results.value().available_W, 100) << "available must equal 100";

    auto conv_to_load1_results =
        ModelResults_GetFlowForConnection(m, conv_to_load1_conn, 0.0, results);
    EXPECT_TRUE(conv_to_load1_results.has_value()) << "converter to load1 must have results";
    EXPECT_EQ(conv_to_load1_results.value().requested_W, 10) << "requested must equal 10";
    EXPECT_EQ(conv_to_load1_results.value().actual_W, 10) << "actual value must equal 10";
    EXPECT_EQ(conv_to_load1_results.value().available_W, 50) << "available must equal 50";

    auto conv_to_load2_results =
        ModelResults_GetFlowForConnection(m, conv_to_load2_conn, 0.0, results);
    EXPECT_TRUE(conv_to_load2_results.has_value()) << "conv to load2 must have results";
    EXPECT_EQ(conv_to_load2_results.value().requested_W, 2) << "requested must equal 2";
    EXPECT_EQ(conv_to_load2_results.value().actual_W, 2) << "actual value must equal 2";
    EXPECT_EQ(conv_to_load2_results.value().available_W, 10) << "available must equal 10";

    auto conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv_id.waste_connection_id, 0.0, results);
    EXPECT_TRUE(conv_to_waste_results.has_value()) << "conv to waste must have results";
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 8) << "requested must equal 8";
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 8) << "actual value must equal 8";
    EXPECT_EQ(conv_to_waste_results.value().available_W, 8) << "available must equal 8";
}

TEST(Erin, Test3A)
{
    Model m {};
    auto src_id = Model_AddConstantSource(m, 100);
    auto load1_id = Model_AddConstantLoad(m, 10);
    auto load2_id = Model_AddConstantLoad(m, 2);
    auto conv_id = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto conv_to_load2_conn = Model_AddConnection(m, conv_id.id, 1, load2_id, 0);
    auto conv_to_load1_conn = Model_AddConnection(m, conv_id.id, 0, load1_id, 0);
    auto src_to_conv_conn = Model_AddConnection(m, src_id, 0, conv_id.id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].time_s, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].flows.size(), 4) << "size of flows must equal 4";

    auto src_to_conv_results = ModelResults_GetFlowForConnection(m, src_to_conv_conn, 0.0, results);
    EXPECT_TRUE(src_to_conv_results.has_value()) << "source to converter must have results";
    EXPECT_EQ(src_to_conv_results.value().requested_W, 20) << "requested must equal 20";
    EXPECT_EQ(src_to_conv_results.value().actual_W, 20) << "actual value must equal 20";
    EXPECT_EQ(src_to_conv_results.value().available_W, 100) << "available must equal 100";

    auto conv_to_load1_results =
        ModelResults_GetFlowForConnection(m, conv_to_load1_conn, 0.0, results);
    EXPECT_TRUE(conv_to_load1_results.has_value() && "converter to load1 must have results");
    EXPECT_EQ(conv_to_load1_results.value().requested_W, 10) << "requested must equal 10";
    EXPECT_EQ(conv_to_load1_results.value().actual_W, 10) << "actual value must equal 10";
    EXPECT_EQ(conv_to_load1_results.value().available_W, 50) << "available must equal 50";

    auto conv_to_load2_results =
        ModelResults_GetFlowForConnection(m, conv_to_load2_conn, 0.0, results);
    EXPECT_TRUE(conv_to_load2_results.has_value()) << "conv to load2 must have results";
    EXPECT_EQ(conv_to_load2_results.value().requested_W, 2) << "requested must equal 2";
    EXPECT_EQ(conv_to_load2_results.value().actual_W, 2) << "actual value must equal 2";
    EXPECT_EQ(conv_to_load2_results.value().available_W, 10) << "available must equal 10";

    auto conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv_id.waste_connection_id, 0.0, results);
    EXPECT_TRUE(conv_to_waste_results.has_value()) << "conv to waste must have results";
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 8) << "requested must equal 8";
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 8) << "actual value must equal 8";
    EXPECT_EQ(conv_to_waste_results.value().available_W, 8) << "available must equal 8";
}

TEST(Erin, Test4)
{
    std::vector<TimeAndAmount> times_and_loads = {};
    times_and_loads.push_back({0.0, 10});
    times_and_loads.push_back({3600.0, 200});
    Model m = {};
    m.final_time_s = 3600.0;
    auto src_id = Model_AddConstantSource(m, 100);
    auto load_id = Model_AddScheduleBasedLoad(m, times_and_loads);
    auto src_to_load_conn = Model_AddConnection(m, src_id, 0, load_id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 2) << "output must have a size of 2";
    EXPECT_EQ(results[0].time_s, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].flows.size(), 1) << "size of flows[0] must equal 1";

    auto src_to_load_results_0 =
        ModelResults_GetFlowForConnection(m, src_to_load_conn, 0.0, results);
    EXPECT_TRUE(src_to_load_results_0.has_value())
        << "source to load must have results at time=0.0";
    EXPECT_EQ(src_to_load_results_0.value().requested_W, 10) << "requested must equal 10";
    EXPECT_EQ(src_to_load_results_0.value().actual_W, 10) << "actual value must equal 10";
    EXPECT_EQ(src_to_load_results_0.value().available_W, 100) << "available must equal 100";
    EXPECT_EQ(results[1].time_s, 3600.0) << "time must equal 3600.0";
    EXPECT_EQ(results[1].flows.size(), 1) << "size of flows[1] must equal 1";

    auto src_to_load_results_3600 =
        ModelResults_GetFlowForConnection(m, src_to_load_conn, 3600.0, results);
    EXPECT_TRUE(src_to_load_results_3600.has_value())
        << "source to load must have results at time=3600.0";
    EXPECT_EQ(src_to_load_results_3600.value().requested_W, 200) << "requested must equal 200";
    EXPECT_EQ(src_to_load_results_3600.value().actual_W, 100) << "actual value must equal 100";
    EXPECT_EQ(src_to_load_results_3600.value().available_W, 100) << "available must equal 100";
}

TEST(Erin, Test5)
{
    std::vector<TimeAndAmount> times_and_loads = {};
    Model m = {};
    auto src_id = Model_AddConstantSource(m, 100);
    auto load1_id = Model_AddConstantLoad(m, 10);
    auto load2_id = Model_AddConstantLoad(m, 7);
    auto load3_id = Model_AddConstantLoad(m, 5);
    auto conv1 = Model_AddConstantEfficiencyConverter(m, 1, 4);
    auto conv2 = Model_AddConstantEfficiencyConverter(m, 1, 4);
    auto conv3 = Model_AddConstantEfficiencyConverter(m, 1, 4);
    auto src_to_conv1_conn = Model_AddConnection(m, src_id, 0, conv1.id, 0);
    auto conv1_to_load1_conn = Model_AddConnection(m, conv1.id, 0, load1_id, 0);
    auto conv1_to_conv2_conn = Model_AddConnection(m, conv1.id, 1, conv2.id, 0);
    auto conv2_to_load2_conn = Model_AddConnection(m, conv2.id, 0, load2_id, 0);
    auto conv2_to_conv3_conn = Model_AddConnection(m, conv2.id, 1, conv3.id, 0);
    auto conv3_to_load3_conn = Model_AddConnection(m, conv3.id, 0, load3_id, 0);
    auto results = Simulate(m, false);
    auto src_to_conv1_results =
        ModelResults_GetFlowForConnection(m, src_to_conv1_conn, 0.0, results);
    auto conv1_to_load1_results =
        ModelResults_GetFlowForConnection(m, conv1_to_load1_conn, 0.0, results);
    auto conv1_to_conv2_results =
        ModelResults_GetFlowForConnection(m, conv1_to_conv2_conn, 0.0, results);
    auto conv2_to_load2_results =
        ModelResults_GetFlowForConnection(m, conv2_to_load2_conn, 0.0, results);
    auto conv2_to_conv3_results =
        ModelResults_GetFlowForConnection(m, conv2_to_conv3_conn, 0.0, results);
    auto conv3_to_load3_results =
        ModelResults_GetFlowForConnection(m, conv3_to_load3_conn, 0.0, results);
    EXPECT_EQ(src_to_conv1_results.value().actual_W, 40) << "src to conv1 should flow 40";
    EXPECT_EQ(conv1_to_load1_results.value().actual_W, 10) << "conv1 to load1 should flow 10";
    EXPECT_EQ(conv1_to_conv2_results.value().actual_W, 28) << "conv1 to conv2 should flow 28";
    EXPECT_EQ(conv2_to_load2_results.value().actual_W, 7) << "conv1 to conv2 should flow 7";
    EXPECT_EQ(conv2_to_conv3_results.value().actual_W, 20) << "conv2 to conv3 should flow 21";
    EXPECT_EQ(conv3_to_load3_results.value().actual_W, 5) << "conv3 to load3 should flow 5";
}

TEST(Erin, Test6)
{
    Model m = {};
    auto src1_id = Model_AddConstantSource(m, 10);
    auto src2_id = Model_AddConstantSource(m, 50);
    auto load1_id = Model_AddConstantLoad(m, 10);
    auto load2_id = Model_AddConstantLoad(m, 80);
    auto mux_id = Model_AddMux(m, 2, 2);
    auto src1_to_mux_conn = Model_AddConnection(m, src1_id, 0, mux_id, 0);
    auto src2_to_mux_conn = Model_AddConnection(m, src2_id, 0, mux_id, 1);
    auto mux_to_load1_conn = Model_AddConnection(m, mux_id, 0, load1_id, 0);
    auto mux_to_load2_conn = Model_AddConnection(m, mux_id, 1, load2_id, 0);
    auto results = Simulate(m, false);
    auto src1_to_mux_results = ModelResults_GetFlowForConnection(m, src1_to_mux_conn, 0.0, results);
    EXPECT_EQ(src1_to_mux_results.value().actual_W, 10) << "src1 -> mux expected actual flow of 10";

    auto src2_to_mux_results = ModelResults_GetFlowForConnection(m, src2_to_mux_conn, 0.0, results);
    EXPECT_EQ(src2_to_mux_results.value().actual_W, 50) << "src2 -> mux expected actual flow of 50";

    auto mux_to_load1_results =
        ModelResults_GetFlowForConnection(m, mux_to_load1_conn, 0.0, results);
    EXPECT_EQ(mux_to_load1_results.value().actual_W, 10)
        << "mux -> load1 expected actual flow of 10";

    auto mux_to_load2_results =
        ModelResults_GetFlowForConnection(m, mux_to_load2_conn, 0.0, results);
    EXPECT_EQ(mux_to_load2_results.value().actual_W, 50)
        << "mux -> load2 expected actual flow of 50";
}

TEST(Erin, Test7)
{
    Model m = {};
    m.final_time_s = 10.0;
    auto src_id = Model_AddConstantSource(m, 0);
    auto store_id = Model_AddStore(m, 100, 10, 10, 0, 100);
    auto load_id = Model_AddConstantLoad(m, 10);
    auto src_to_store_conn = Model_AddConnection(m, src_id, 0, store_id, 0);
    auto store_to_load_conn = Model_AddConnection(m, store_id, 0, load_id, 0);
    auto results = Simulate(m, false);

    auto src_to_store_results =
        ModelResults_GetFlowForConnection(m, src_to_store_conn, 0.0, results);
    EXPECT_EQ(src_to_store_results.value().actual_W, 0) << "src to store should be providing 0";
    EXPECT_EQ(src_to_store_results.value().requested_W, 10) << "src to store request is 10";
    EXPECT_EQ(src_to_store_results.value().available_W, 0) << "src to store available is 0";

    auto store_to_load_results =
        ModelResults_GetFlowForConnection(m, store_to_load_conn, 0.0, results);
    EXPECT_TRUE(store_to_load_results.has_value())
        << "should have results for store to load connection";
    EXPECT_EQ(store_to_load_results.value().actual_W, 10) << "store to load should be providing 10";
    EXPECT_EQ(store_to_load_results.value().requested_W, 10)
        << "store to load should be requesting 10";
    EXPECT_EQ(store_to_load_results.value().available_W, 10)
        << "store to load available should be 10";
    EXPECT_EQ(results.size(), 2) << "there should be two time events in results";
    EXPECT_TRUE((results[1].time_s > 10.0 - 1e-6) && (results[1].time_s < 10.0 + 1e-6))
        << "time should be 10";

    auto src_to_store_results_at_10 =
        ModelResults_GetFlowForConnection(m, src_to_store_conn, 10.0, results);
    EXPECT_EQ(src_to_store_results_at_10.value().actual_W, 0)
        << "src to store should be providing 0";
    EXPECT_EQ(src_to_store_results_at_10.value().requested_W, 20) << "src to store request is 20";
    EXPECT_EQ(src_to_store_results_at_10.value().available_W, 0) << "src to store available is 0";

    auto store_to_load_results_at_10 =
        ModelResults_GetFlowForConnection(m, store_to_load_conn, 10.0, results);
    EXPECT_TRUE(store_to_load_results_at_10.has_value())
        << "should have results for store to load connection";
    EXPECT_EQ(store_to_load_results_at_10.value().actual_W, 0)
        << "store to load should be providing 0";
    EXPECT_EQ(store_to_load_results_at_10.value().requested_W, 10)
        << "store to load should be requesting 10";
    EXPECT_EQ(store_to_load_results_at_10.value().available_W, 0)
        << "store to load available should be 0";
}

TEST(Erin, Test8)
{
    Model m = {};
    m.final_time_s = 20.0;
    auto src_id = Model_AddConstantSource(m, 5);
    auto store_id = Model_AddStore(m, 100, 10, 10, 0, 100);
    auto load_id = Model_AddConstantLoad(m, 10);
    auto src_to_store_conn = Model_AddConnection(m, src_id, 0, store_id, 0);
    auto store_to_load_conn = Model_AddConnection(m, store_id, 0, load_id, 0);
    auto results = Simulate(m, false);
    auto src_to_store_results =
        ModelResults_GetFlowForConnection(m, src_to_store_conn, 0.0, results);
    EXPECT_EQ(src_to_store_results.value().actual_W, 5) << "src to store should be providing 5";
    EXPECT_EQ(src_to_store_results.value().requested_W, 10) << "src to store request is 10";
    EXPECT_EQ(src_to_store_results.value().available_W, 5) << "src to store available is 5";

    auto store_to_load_results =
        ModelResults_GetFlowForConnection(m, store_to_load_conn, 0.0, results);
    EXPECT_TRUE(store_to_load_results.has_value())
        << "should have results for store to load connection";
    EXPECT_EQ(store_to_load_results.value().actual_W, 10) << "store to load should be providing 10";
    EXPECT_EQ(store_to_load_results.value().requested_W, 10)
        << "store to load should be requesting 10";
    EXPECT_EQ(store_to_load_results.value().available_W, 15)
        << "store to load available should be 15";
    EXPECT_EQ(results.size(), 2) << "there should be two time events in results";
    EXPECT_TRUE((results[1].time_s > 20.0 - 1e-6) && (results[1].time_s < 20.0 + 1e-6))
        << "time should be 20";

    auto src_to_store_results_at_20 =
        ModelResults_GetFlowForConnection(m, src_to_store_conn, 20.0, results);
    EXPECT_EQ(src_to_store_results_at_20.value().actual_W, 5)
        << "src to store should be providing 5";
    EXPECT_EQ(src_to_store_results_at_20.value().requested_W, 20) << "src to store request is 20";
    EXPECT_EQ(src_to_store_results_at_20.value().available_W, 5) << "src to store available is 5";

    auto store_to_load_results_at_20 =
        ModelResults_GetFlowForConnection(m, store_to_load_conn, 20.0, results);
    EXPECT_TRUE(store_to_load_results_at_20.has_value())
        << "should have results for store to load connection";
    EXPECT_EQ(store_to_load_results_at_20.value().actual_W, 5)
        << "store to load should be providing 5";
    EXPECT_EQ(store_to_load_results_at_20.value().requested_W, 10)
        << "store to load should be requesting 10";
    EXPECT_EQ(store_to_load_results_at_20.value().available_W, 5)
        << "store to load available should be 5";
}

TEST(Erin, Test9)
{
    std::vector<TimeAndAmount> times_and_loads = {};
    times_and_loads.push_back({0.0, 20});
    times_and_loads.push_back({5.0, 5});
    times_and_loads.push_back({10.0, 15});
    Model m = {};
    m.final_time_s = 25.0;
    auto src_id = Model_AddConstantSource(m, 10);
    auto store_id = Model_AddStore(m, 100, 10, 10, 80, 100);
    auto load_id = Model_AddScheduleBasedLoad(m, times_and_loads);
    auto src_to_store_conn = Model_AddConnection(m, src_id, 0, store_id, 0);
    auto store_to_load_conn = Model_AddConnection(m, store_id, 0, load_id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 5) << "expected 5 time steps";
    EXPECT_EQ(round_it(results[0].time_s), 0.0) << "expect first time is 0.0";
    EXPECT_EQ(round_it(results[1].time_s), 2.0) << "expect second time is 2.0";
    EXPECT_EQ(round_it(results[2].time_s), 5.0) << "expect third time is 5.0";
    EXPECT_EQ(round_it(results[3].time_s), 10.0) << "expect fourth time is 10.0";
    EXPECT_EQ(round_it(results[4].time_s), 25.0) << "expect fifth time is 25.0";

    auto src_to_store_results_at_0 =
        ModelResults_GetFlowForConnection(m, src_to_store_conn, 0.0, results);
    auto store_to_load_results_at_0 =
        ModelResults_GetFlowForConnection(m, store_to_load_conn, 0.0, results);
    auto store_amount_0 = ModelResults_GetStoreState(m, store_id, 0.0, results);
    EXPECT_EQ(src_to_store_results_at_0.value().actual_W, 10);
    EXPECT_EQ(src_to_store_results_at_0.value().requested_W, 20);
    EXPECT_EQ(src_to_store_results_at_0.value().available_W, 10);
    EXPECT_EQ(store_to_load_results_at_0.value().actual_W, 20);
    EXPECT_EQ(store_to_load_results_at_0.value().requested_W, 20);
    EXPECT_EQ(store_to_load_results_at_0.value().available_W, 20);
    EXPECT_EQ(store_amount_0.value(), 100);

    auto src_to_store_results_at_2 =
        ModelResults_GetFlowForConnection(m, src_to_store_conn, 2.0, results);
    auto store_to_load_results_at_2 =
        ModelResults_GetFlowForConnection(m, store_to_load_conn, 2.0, results);
    auto store_amount_2 = ModelResults_GetStoreState(m, store_id, 2.0, results);
    EXPECT_EQ(src_to_store_results_at_2.value().actual_W, 10);
    EXPECT_EQ(src_to_store_results_at_2.value().requested_W, 30);
    EXPECT_EQ(src_to_store_results_at_2.value().available_W, 10);
    EXPECT_EQ(store_to_load_results_at_2.value().actual_W, 20);
    EXPECT_EQ(store_to_load_results_at_2.value().requested_W, 20);
    EXPECT_EQ(store_to_load_results_at_2.value().available_W, 20);
    EXPECT_EQ(store_amount_2.value(), 80);

    auto src_to_store_results_at_5 =
        ModelResults_GetFlowForConnection(m, src_to_store_conn, 5.0, results);
    auto store_to_load_results_at_5 =
        ModelResults_GetFlowForConnection(m, store_to_load_conn, 5.0, results);
    auto store_amount_5 = ModelResults_GetStoreState(m, store_id, 5.0, results);
    EXPECT_EQ(src_to_store_results_at_5.value().actual_W, 10);
    EXPECT_EQ(src_to_store_results_at_5.value().requested_W, 15);
    EXPECT_EQ(src_to_store_results_at_5.value().available_W, 10);
    EXPECT_EQ(store_to_load_results_at_5.value().actual_W, 5);
    EXPECT_EQ(store_to_load_results_at_5.value().requested_W, 5);
    EXPECT_EQ(store_to_load_results_at_5.value().available_W, 20);
    EXPECT_EQ(store_amount_5.value(), 50);

    auto src_to_store_results_at_10 =
        ModelResults_GetFlowForConnection(m, src_to_store_conn, 10.0, results);
    auto store_to_load_results_at_10 =
        ModelResults_GetFlowForConnection(m, store_to_load_conn, 10.0, results);
    auto store_amount_10 = ModelResults_GetStoreState(m, store_id, 10.0, results);
    EXPECT_EQ(src_to_store_results_at_10.value().actual_W, 10);
    EXPECT_EQ(src_to_store_results_at_10.value().requested_W, 25);
    EXPECT_EQ(src_to_store_results_at_10.value().available_W, 10);
    EXPECT_EQ(store_to_load_results_at_10.value().actual_W, 15);
    EXPECT_EQ(store_to_load_results_at_10.value().requested_W, 15);
    EXPECT_EQ(store_to_load_results_at_10.value().available_W, 20);
    EXPECT_EQ(store_amount_10.value(), 75);

    auto src_to_store_results_at_25 =
        ModelResults_GetFlowForConnection(m, src_to_store_conn, 25.0, results);
    auto store_to_load_results_at_25 =
        ModelResults_GetFlowForConnection(m, store_to_load_conn, 25.0, results);
    auto store_amount_25 = ModelResults_GetStoreState(m, store_id, 25.0, results);
    EXPECT_EQ(src_to_store_results_at_25.value().actual_W, 10);
    EXPECT_EQ(src_to_store_results_at_25.value().requested_W, 25);
    EXPECT_EQ(src_to_store_results_at_25.value().available_W, 10);
    EXPECT_EQ(store_to_load_results_at_25.value().actual_W, 10);
    EXPECT_EQ(store_to_load_results_at_25.value().requested_W, 15);
    EXPECT_EQ(store_to_load_results_at_25.value().available_W, 10);
    EXPECT_EQ(store_amount_25.value(), 0);
}

TEST(Erin, Test10)
{
    std::vector<TimeAndAmount> times_and_loads = {};
    times_and_loads.push_back({0.0, 20});
    times_and_loads.push_back({5.0, 5});
    times_and_loads.push_back({10.0, 15});
    Model m = {};
    m.final_time_s = 12.5;
    auto src1_id = Model_AddConstantSource(m, 20);
    auto src2_id = Model_AddConstantSource(m, 5);
    auto store_id = Model_AddStore(m, 100, 10, 10, 80, 100);
    auto mux_id = Model_AddMux(m, 2, 2);
    auto conv = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto load1_id = Model_AddConstantLoad(m, 20);
    auto load2_id = Model_AddScheduleBasedLoad(m, times_and_loads);
    auto load3_id = Model_AddConstantLoad(m, 5);
    auto src1_to_mux0_port0_conn = Model_AddConnection(m, src1_id, 0, mux_id, 0);
    auto src2_to_store_conn = Model_AddConnection(m, src2_id, 0, store_id, 0);
    auto store_to_mux0_port1_conn = Model_AddConnection(m, store_id, 0, mux_id, 1);
    auto mux0_port0_to_load1_conn = Model_AddConnection(m, mux_id, 0, load1_id, 0);
    auto mux0_port1_to_conv_conn = Model_AddConnection(m, mux_id, 1, conv.id, 0);
    auto conv_to_load2_conn = Model_AddConnection(m, conv.id, 0, load2_id, 0);
    auto conv_to_load3_conn = Model_AddConnection(m, conv.id, 1, load3_id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 5) << "expect 5 events";

    // time = 0.0
    double t = 0.0;
    size_t results_idx = 0;
    EXPECT_EQ(results[results_idx].time_s, t);
    auto conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 3);
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 3);
    EXPECT_EQ(conv_to_waste_results.value().available_W, 3);

    auto src1_to_mux_results =
        ModelResults_GetFlowForConnection(m, src1_to_mux0_port0_conn, 0.0, results);
    EXPECT_EQ(src1_to_mux_results.value().actual_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().available_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().requested_W, 60);

    auto src2_to_store_results =
        ModelResults_GetFlowForConnection(m, src2_to_store_conn, 0.0, results);
    EXPECT_EQ(src2_to_store_results.value().actual_W, 5);
    EXPECT_EQ(src2_to_store_results.value().available_W, 5);
    EXPECT_EQ(src2_to_store_results.value().requested_W, 40);

    auto store_to_mux_results =
        ModelResults_GetFlowForConnection(m, store_to_mux0_port1_conn, 0.0, results);
    EXPECT_EQ(store_to_mux_results.value().actual_W, 15);
    EXPECT_EQ(store_to_mux_results.value().available_W, 15);
    EXPECT_EQ(store_to_mux_results.value().requested_W, 40);

    auto mux_to_load1_results =
        ModelResults_GetFlowForConnection(m, mux0_port0_to_load1_conn, 0.0, results);
    EXPECT_EQ(mux_to_load1_results.value().actual_W, 20);
    EXPECT_EQ(mux_to_load1_results.value().available_W, 20);
    EXPECT_EQ(mux_to_load1_results.value().requested_W, 20);

    auto mux_to_conv_results =
        ModelResults_GetFlowForConnection(m, mux0_port1_to_conv_conn, 0.0, results);
    EXPECT_EQ(mux_to_conv_results.value().actual_W, 15);
    EXPECT_EQ(mux_to_conv_results.value().available_W, 15);
    EXPECT_EQ(mux_to_conv_results.value().requested_W, 40);

    auto conv_to_load2_results =
        ModelResults_GetFlowForConnection(m, conv_to_load2_conn, 0.0, results);
    EXPECT_EQ(conv_to_load2_results.value().actual_W, 7);
    EXPECT_EQ(conv_to_load2_results.value().available_W, 7);
    EXPECT_EQ(conv_to_load2_results.value().requested_W, 20);

    auto conv_to_load3_results =
        ModelResults_GetFlowForConnection(m, conv_to_load3_conn, 0.0, results);
    EXPECT_EQ(conv_to_load3_results.value().actual_W, 5);
    EXPECT_EQ(conv_to_load3_results.value().available_W, 8);
    EXPECT_EQ(conv_to_load3_results.value().requested_W, 5);

    auto store_amount = ModelResults_GetStoreState(m, store_id, 0.0, results);
    EXPECT_EQ(store_amount.value(), 100);

    // time = 2.0
    t = 2.0;
    results_idx = 1;
    EXPECT_EQ(results[results_idx].time_s, t);

    conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 3);
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 3);
    EXPECT_EQ(conv_to_waste_results.value().available_W, 3);

    src1_to_mux_results = ModelResults_GetFlowForConnection(m, src1_to_mux0_port0_conn, t, results);
    EXPECT_EQ(src1_to_mux_results.value().actual_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().available_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().requested_W, 60);

    src2_to_store_results = ModelResults_GetFlowForConnection(m, src2_to_store_conn, t, results);
    EXPECT_EQ(src2_to_store_results.value().actual_W, 5);
    EXPECT_EQ(src2_to_store_results.value().available_W, 5);
    EXPECT_EQ(src2_to_store_results.value().requested_W, 50);

    store_to_mux_results =
        ModelResults_GetFlowForConnection(m, store_to_mux0_port1_conn, t, results);
    EXPECT_EQ(store_to_mux_results.value().actual_W, 15);
    EXPECT_EQ(store_to_mux_results.value().available_W, 15);
    EXPECT_EQ(store_to_mux_results.value().requested_W, 40);

    mux_to_load1_results =
        ModelResults_GetFlowForConnection(m, mux0_port0_to_load1_conn, t, results);
    EXPECT_EQ(mux_to_load1_results.value().actual_W, 20);
    EXPECT_EQ(mux_to_load1_results.value().available_W, 20);
    EXPECT_EQ(mux_to_load1_results.value().requested_W, 20);

    mux_to_conv_results = ModelResults_GetFlowForConnection(m, mux0_port1_to_conv_conn, t, results);
    EXPECT_EQ(mux_to_conv_results.value().actual_W, 15);
    EXPECT_EQ(mux_to_conv_results.value().available_W, 15);
    EXPECT_EQ(mux_to_conv_results.value().requested_W, 40);

    conv_to_load2_results = ModelResults_GetFlowForConnection(m, conv_to_load2_conn, t, results);
    EXPECT_EQ(conv_to_load2_results.value().actual_W, 7);
    EXPECT_EQ(conv_to_load2_results.value().available_W, 7);
    EXPECT_EQ(conv_to_load2_results.value().requested_W, 20);

    conv_to_load3_results = ModelResults_GetFlowForConnection(m, conv_to_load3_conn, t, results);
    EXPECT_EQ(conv_to_load3_results.value().actual_W, 5);
    EXPECT_EQ(conv_to_load3_results.value().available_W, 8);
    EXPECT_EQ(conv_to_load3_results.value().requested_W, 5);

    store_amount = ModelResults_GetStoreState(m, store_id, t, results);
    EXPECT_EQ(store_amount.value(), 80);

    // time = 5.0
    t = 5.0;
    results_idx = 2;
    EXPECT_EQ(results[results_idx].time_s, t);

    conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 0);
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 0);
    EXPECT_EQ(conv_to_waste_results.value().available_W, 0);

    src1_to_mux_results = ModelResults_GetFlowForConnection(m, src1_to_mux0_port0_conn, t, results);
    EXPECT_EQ(src1_to_mux_results.value().actual_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().available_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().requested_W, 30);

    src2_to_store_results = ModelResults_GetFlowForConnection(m, src2_to_store_conn, t, results);
    EXPECT_EQ(src2_to_store_results.value().actual_W, 5);
    EXPECT_EQ(src2_to_store_results.value().available_W, 5);
    EXPECT_EQ(src2_to_store_results.value().requested_W, 20);

    store_to_mux_results =
        ModelResults_GetFlowForConnection(m, store_to_mux0_port1_conn, t, results);
    EXPECT_EQ(store_to_mux_results.value().actual_W, 10);
    EXPECT_EQ(store_to_mux_results.value().available_W, 15);
    EXPECT_EQ(store_to_mux_results.value().requested_W, 10);

    mux_to_load1_results =
        ModelResults_GetFlowForConnection(m, mux0_port0_to_load1_conn, t, results);
    EXPECT_EQ(mux_to_load1_results.value().actual_W, 20);
    EXPECT_EQ(mux_to_load1_results.value().available_W, 25);
    EXPECT_EQ(mux_to_load1_results.value().requested_W, 20);

    mux_to_conv_results = ModelResults_GetFlowForConnection(m, mux0_port1_to_conv_conn, t, results);
    EXPECT_EQ(mux_to_conv_results.value().actual_W, 10);
    EXPECT_EQ(mux_to_conv_results.value().available_W, 10);
    EXPECT_EQ(mux_to_conv_results.value().requested_W, 10);

    conv_to_load2_results = ModelResults_GetFlowForConnection(m, conv_to_load2_conn, t, results);
    EXPECT_EQ(conv_to_load2_results.value().actual_W, 5);
    EXPECT_EQ(conv_to_load2_results.value().available_W, 5);
    EXPECT_EQ(conv_to_load2_results.value().requested_W, 5);

    conv_to_load3_results = ModelResults_GetFlowForConnection(m, conv_to_load3_conn, t, results);
    EXPECT_EQ(conv_to_load3_results.value().actual_W, 5);
    EXPECT_EQ(conv_to_load3_results.value().available_W, 5);
    EXPECT_EQ(conv_to_load3_results.value().requested_W, 5);

    store_amount = ModelResults_GetStoreState(m, store_id, t, results);
    EXPECT_EQ(store_amount.value(), 50);

    // time = 10.0
    t = 10.0;
    results_idx = 3;
    EXPECT_EQ(results[results_idx].time_s, t);

    conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 3);
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 3);
    EXPECT_EQ(conv_to_waste_results.value().available_W, 3);

    src1_to_mux_results = ModelResults_GetFlowForConnection(m, src1_to_mux0_port0_conn, t, results);
    EXPECT_EQ(src1_to_mux_results.value().actual_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().available_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().requested_W, 50);

    src2_to_store_results = ModelResults_GetFlowForConnection(m, src2_to_store_conn, t, results);
    EXPECT_EQ(src2_to_store_results.value().actual_W, 5);
    EXPECT_EQ(src2_to_store_results.value().available_W, 5);
    EXPECT_EQ(src2_to_store_results.value().requested_W, 40);

    store_to_mux_results =
        ModelResults_GetFlowForConnection(m, store_to_mux0_port1_conn, t, results);
    EXPECT_EQ(store_to_mux_results.value().actual_W, 15);
    EXPECT_EQ(store_to_mux_results.value().available_W, 15);
    EXPECT_EQ(store_to_mux_results.value().requested_W, 30);

    mux_to_load1_results =
        ModelResults_GetFlowForConnection(m, mux0_port0_to_load1_conn, t, results);
    EXPECT_EQ(mux_to_load1_results.value().actual_W, 20);
    EXPECT_EQ(mux_to_load1_results.value().available_W, 20);
    EXPECT_EQ(mux_to_load1_results.value().requested_W, 20);

    mux_to_conv_results = ModelResults_GetFlowForConnection(m, mux0_port1_to_conv_conn, t, results);
    EXPECT_EQ(mux_to_conv_results.value().actual_W, 15);
    EXPECT_EQ(mux_to_conv_results.value().available_W, 15);
    EXPECT_EQ(mux_to_conv_results.value().requested_W, 30);

    conv_to_load2_results = ModelResults_GetFlowForConnection(m, conv_to_load2_conn, t, results);
    EXPECT_EQ(conv_to_load2_results.value().actual_W, 7);
    EXPECT_EQ(conv_to_load2_results.value().available_W, 7);
    EXPECT_EQ(conv_to_load2_results.value().requested_W, 15);

    conv_to_load3_results = ModelResults_GetFlowForConnection(m, conv_to_load3_conn, t, results);
    EXPECT_EQ(conv_to_load3_results.value().actual_W, 5);
    EXPECT_EQ(conv_to_load3_results.value().available_W, 8);
    EXPECT_EQ(conv_to_load3_results.value().requested_W, 5);

    store_amount = ModelResults_GetStoreState(m, store_id, t, results);
    EXPECT_EQ(store_amount.value(), 25);

    // time = 12.5
    t = 12.5;
    results_idx = 4;
    EXPECT_EQ(results[results_idx].time_s, t);

    conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 0);
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 0);
    EXPECT_EQ(conv_to_waste_results.value().available_W, 0);

    src1_to_mux_results = ModelResults_GetFlowForConnection(m, src1_to_mux0_port0_conn, t, results);
    EXPECT_EQ(src1_to_mux_results.value().actual_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().available_W, 20);
    EXPECT_EQ(src1_to_mux_results.value().requested_W, 50);

    src2_to_store_results = ModelResults_GetFlowForConnection(m, src2_to_store_conn, t, results);
    EXPECT_EQ(src2_to_store_results.value().actual_W, 5);
    EXPECT_EQ(src2_to_store_results.value().available_W, 5);
    EXPECT_EQ(src2_to_store_results.value().requested_W, 40);

    store_to_mux_results =
        ModelResults_GetFlowForConnection(m, store_to_mux0_port1_conn, t, results);
    EXPECT_EQ(store_to_mux_results.value().actual_W, 5);
    EXPECT_EQ(store_to_mux_results.value().available_W, 5);
    EXPECT_EQ(store_to_mux_results.value().requested_W, 30);

    mux_to_load1_results =
        ModelResults_GetFlowForConnection(m, mux0_port0_to_load1_conn, t, results);
    EXPECT_EQ(mux_to_load1_results.value().actual_W, 20);
    EXPECT_EQ(mux_to_load1_results.value().available_W, 20);
    EXPECT_EQ(mux_to_load1_results.value().requested_W, 20);

    mux_to_conv_results = ModelResults_GetFlowForConnection(m, mux0_port1_to_conv_conn, t, results);
    EXPECT_EQ(mux_to_conv_results.value().actual_W, 5);
    EXPECT_EQ(mux_to_conv_results.value().available_W, 5);
    EXPECT_EQ(mux_to_conv_results.value().requested_W, 30);

    conv_to_load2_results = ModelResults_GetFlowForConnection(m, conv_to_load2_conn, t, results);
    EXPECT_EQ(conv_to_load2_results.value().actual_W, 2);
    EXPECT_EQ(conv_to_load2_results.value().available_W, 2);
    EXPECT_EQ(conv_to_load2_results.value().requested_W, 15);

    conv_to_load3_results = ModelResults_GetFlowForConnection(m, conv_to_load3_conn, t, results);
    EXPECT_EQ(conv_to_load3_results.value().actual_W, 3);
    EXPECT_EQ(conv_to_load3_results.value().available_W, 3);
    EXPECT_EQ(conv_to_load3_results.value().requested_W, 5);

    store_amount = ModelResults_GetStoreState(m, store_id, t, results);
    EXPECT_EQ(store_amount.value(), 0);
}

TEST(Erin, Test11)
{
    // create a model of src->conv->load and place a reliability dist on conv
    // ensure the component goes down and comes back up (i.e., is repaired)
    Model m = {};
    m.random_function = []() { return 0.4; };
    m.final_time_s = 50.0;
    auto src_id = Model_AddConstantSource(m, 100);
    auto load_id = Model_AddConstantLoad(m, 10);
    auto conv_id = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto src_to_conv_conn = Model_AddConnection(m, src_id, 0, conv_id.id, 0);
    auto conv_to_load_conn = Model_AddConnection(m, conv_id.id, 0, load_id, 0);
    auto fixed_dist_id = Model_AddFixedReliabilityDistribution(m, 10.0);
    Model_AddFailureModeToComponent(m, conv_id.id, fixed_dist_id, fixed_dist_id);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 6) << "Expect 6 times: 0.0, 10.0, 20.0, 30.0, 40.0, 50.0";

    double t = 0.0;
    auto src_to_conv_results = ModelResults_GetFlowForConnection(m, src_to_conv_conn, t, results);
    EXPECT_EQ(src_to_conv_results.value().actual_W, 20) << "src -> conv actual should be 20";
    EXPECT_EQ(src_to_conv_results.value().requested_W, 20) << "src -> conv requested should be 20";
    EXPECT_EQ(src_to_conv_results.value().available_W, 100)
        << "src -> conv available should be 100";

    auto conv_to_load_results = ModelResults_GetFlowForConnection(m, conv_to_load_conn, t, results);
    EXPECT_EQ(conv_to_load_results.value().actual_W, 10) << "conv -> load actual should be 10";
    EXPECT_EQ(conv_to_load_results.value().requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(conv_to_load_results.value().available_W, 50)
        << "conv -> load available should be 50";

    auto conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv_id.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 10) << "conv -> waste actual should be 10";
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 10)
        << "conv -> waste requested should be 10";
    EXPECT_EQ(conv_to_waste_results.value().available_W, 10)
        << "conv -> waste available should be 10";

    // time = 10.0, failed
    t = 10.0;
    src_to_conv_results = ModelResults_GetFlowForConnection(m, src_to_conv_conn, t, results);
    EXPECT_EQ(src_to_conv_results.value().actual_W, 0) << "src -> conv actual should be 0";
    EXPECT_EQ(src_to_conv_results.value().requested_W, 0) << "src -> conv requested should be 0";
    EXPECT_EQ(src_to_conv_results.value().available_W, 100)
        << "src -> conv available should be 100";

    conv_to_load_results = ModelResults_GetFlowForConnection(m, conv_to_load_conn, t, results);
    EXPECT_EQ(conv_to_load_results.value().actual_W, 0) << "conv -> load actual should be 0";
    EXPECT_EQ(conv_to_load_results.value().requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(conv_to_load_results.value().available_W, 0) << "conv -> load available should be 0";

    conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv_id.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 0) << "conv -> waste actual should be 0";
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 0)
        << "conv -> waste requested should be 0";
    EXPECT_EQ(conv_to_waste_results.value().available_W, 0)
        << "conv -> waste available should be 0";

    // time = 20.0, fixed/restored
    t = 20.0;
    src_to_conv_results = ModelResults_GetFlowForConnection(m, src_to_conv_conn, t, results);
    EXPECT_EQ(src_to_conv_results.value().actual_W, 20) << "src -> conv actual should be 20";
    EXPECT_EQ(src_to_conv_results.value().requested_W, 20) << "src -> conv requested should be 20";
    EXPECT_EQ(src_to_conv_results.value().available_W, 100)
        << "src -> conv available should be 100";

    conv_to_load_results = ModelResults_GetFlowForConnection(m, conv_to_load_conn, t, results);
    EXPECT_EQ(conv_to_load_results.value().actual_W, 10) << "conv -> load actual should be 10";
    EXPECT_EQ(conv_to_load_results.value().requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(conv_to_load_results.value().available_W, 50) << "conv -> load available should be 0";

    conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv_id.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 10) << "conv -> waste actual should be 10";
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 10)
        << "conv -> waste requested should be 10";
    EXPECT_EQ(conv_to_waste_results.value().available_W, 10)
        << "conv -> waste available should be 10";

    // time = 30.0, failed
    t = 30.0;
    src_to_conv_results = ModelResults_GetFlowForConnection(m, src_to_conv_conn, t, results);
    EXPECT_EQ(src_to_conv_results.value().actual_W, 0) << "src -> conv actual should be 0";
    EXPECT_EQ(src_to_conv_results.value().requested_W, 0) << "src -> conv requested should be 0";
    EXPECT_EQ(src_to_conv_results.value().available_W, 100)
        << "src -> conv available should be 100";

    conv_to_load_results = ModelResults_GetFlowForConnection(m, conv_to_load_conn, t, results);
    EXPECT_EQ(conv_to_load_results.value().actual_W, 0) << "conv -> load actual should be 0";
    EXPECT_EQ(conv_to_load_results.value().requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(conv_to_load_results.value().available_W, 0) << "conv -> load available should be 0";

    conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv_id.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 0) << "conv -> waste actual should be 0";
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 0)
        << "conv -> waste requested should be 0";
    EXPECT_EQ(conv_to_waste_results.value().available_W, 0)
        << "conv -> waste available should be 0";

    // time = 40.0, fixed/restored
    t = 40.0;
    src_to_conv_results = ModelResults_GetFlowForConnection(m, src_to_conv_conn, t, results);
    EXPECT_EQ(src_to_conv_results.value().actual_W, 20) << "src -> conv actual should be 20";
    EXPECT_EQ(src_to_conv_results.value().requested_W, 20) << "src -> conv requested should be 20";
    EXPECT_EQ(src_to_conv_results.value().available_W, 100)
        << "src -> conv available should be 100";

    conv_to_load_results = ModelResults_GetFlowForConnection(m, conv_to_load_conn, t, results);
    EXPECT_EQ(conv_to_load_results.value().actual_W, 10) << "conv -> load actual should be 10";
    EXPECT_EQ(conv_to_load_results.value().requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(conv_to_load_results.value().available_W, 50) << "conv -> load available should be 0";

    conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv_id.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 10) << "conv -> waste actual should be 10";
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 10)
        << "conv -> waste requested should be 10";
    EXPECT_EQ(conv_to_waste_results.value().available_W, 10)
        << "conv -> waste available should be 10";

    // time = 50.0, failed
    t = 50.0;
    src_to_conv_results = ModelResults_GetFlowForConnection(m, src_to_conv_conn, t, results);
    EXPECT_EQ(src_to_conv_results.value().actual_W, 0) << "src -> conv actual should be 0";
    EXPECT_EQ(src_to_conv_results.value().requested_W, 0) << "src -> conv requested should be 0";
    EXPECT_EQ(src_to_conv_results.value().available_W, 100)
        << "src -> conv available should be 100";

    conv_to_load_results = ModelResults_GetFlowForConnection(m, conv_to_load_conn, t, results);
    EXPECT_EQ(conv_to_load_results.value().actual_W, 0) << "conv -> load actual should be 0";
    EXPECT_EQ(conv_to_load_results.value().requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(conv_to_load_results.value().available_W, 0) << "conv -> load available should be 0";

    conv_to_waste_results =
        ModelResults_GetFlowForConnection(m, conv_id.waste_connection_id, t, results);
    EXPECT_EQ(conv_to_waste_results.value().actual_W, 0) << "conv -> waste actual should be 0";
    EXPECT_EQ(conv_to_waste_results.value().requested_W, 0)
        << "conv -> waste requested should be 0";
    EXPECT_EQ(conv_to_waste_results.value().available_W, 0)
        << "conv -> waste available should be 0";
}

TEST(Erin, Test12)
{
    // Add a schedule-based source (availability, uncontrolled source)
    // NOTE: it would be good to have a waste connection so that the component
    // always "spills" (ullage) when not all available is used.
    Model m = {};
    m.random_function = []() { return 0.4; };
    m.final_time_s = 20.0;
    std::vector<TimeAndAmount> source_availability {};
    source_availability.reserve(5);
    source_availability.push_back(TimeAndAmount {0, 10});
    source_availability.push_back(TimeAndAmount {10, 8});
    source_availability.push_back(TimeAndAmount {20, 12});
    auto src_id = Model_AddScheduleBasedSource(m, source_availability);
    auto load_id = Model_AddConstantLoad(m, 10);
    auto src_to_load_conn = Model_AddConnection(m, src_id.id, 0, load_id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 3) << "should have 3 time results";
    EXPECT_EQ(results[0].time_s, 0.0);
    EXPECT_EQ(results[1].time_s, 10.0);
    EXPECT_EQ(results[2].time_s, 20.0);
    double t = 0.0;
    auto src_to_load_results = ModelResults_GetFlowForConnection(m, src_to_load_conn, t, results);
    EXPECT_EQ(src_to_load_results.value().actual_W, 10);
    EXPECT_EQ(src_to_load_results.value().available_W, 10);
    EXPECT_EQ(src_to_load_results.value().requested_W, 10);
    auto src_to_waste_results =
        ModelResults_GetFlowForConnection(m, src_id.waste_connection_id, t, results);
    EXPECT_EQ(src_to_waste_results.value().actual_W, 0);
    EXPECT_EQ(src_to_waste_results.value().available_W, 0);
    EXPECT_EQ(src_to_waste_results.value().requested_W, 0);
    t = 10.0;
    src_to_load_results = ModelResults_GetFlowForConnection(m, src_to_load_conn, t, results);
    EXPECT_EQ(src_to_load_results.value().actual_W, 8);
    EXPECT_EQ(src_to_load_results.value().available_W, 8);
    EXPECT_EQ(src_to_load_results.value().requested_W, 10);
    src_to_waste_results =
        ModelResults_GetFlowForConnection(m, src_id.waste_connection_id, t, results);
    EXPECT_EQ(src_to_waste_results.value().actual_W, 0);
    EXPECT_EQ(src_to_waste_results.value().available_W, 0);
    EXPECT_EQ(src_to_waste_results.value().requested_W, 0);
    t = 20.0;
    src_to_load_results = ModelResults_GetFlowForConnection(m, src_to_load_conn, t, results);
    EXPECT_EQ(src_to_load_results.value().actual_W, 10);
    EXPECT_EQ(src_to_load_results.value().available_W, 12);
    EXPECT_EQ(src_to_load_results.value().requested_W, 10);
    src_to_waste_results =
        ModelResults_GetFlowForConnection(m, src_id.waste_connection_id, t, results);
    EXPECT_EQ(src_to_waste_results.value().actual_W, 2);
    EXPECT_EQ(src_to_waste_results.value().available_W, 2);
    EXPECT_EQ(src_to_waste_results.value().requested_W, 2);
}

TEST(Erin, Test13)
{
    // SIMULATION INFO and INITIALIZATION
    Model m = {};
    m.random_function = []() { return 0.4; };
    m.final_time_s = hours_as_seconds(48.0);
    // LOADS
    std::vector<TimeAndAmount> elecLoad {};
    elecLoad.reserve(49);
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(0.0), kW_as_W(187.47)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(1.0), kW_as_W(146.271)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(2.0), kW_as_W(137.308)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(3.0), kW_as_W(170.276)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(4.0), kW_as_W(139.068)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(5.0), kW_as_W(171.944)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(6.0), kW_as_W(140.051)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(7.0), kW_as_W(173.406)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(8.0), kW_as_W(127.54)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(9.0), kW_as_W(135.751)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(10.0), kW_as_W(95.195)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(11.0), kW_as_W(107.644)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(12.0), kW_as_W(81.227)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(13.0), kW_as_W(98.928)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(14.0), kW_as_W(80.134)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(15.0), kW_as_W(97.222)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(16.0), kW_as_W(81.049)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(17.0), kW_as_W(114.29)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(18.0), kW_as_W(102.652)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(19.0), kW_as_W(125.672)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(20.0), kW_as_W(105.254)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(21.0), kW_as_W(125.047)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(22.0), kW_as_W(104.824)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(23.0), kW_as_W(126.488)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(24.0), kW_as_W(107.094)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(25.0), kW_as_W(135.559)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(26.0), kW_as_W(115.588)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(27.0), kW_as_W(137.494)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(28.0), kW_as_W(115.386)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(29.0), kW_as_W(133.837)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(30.0), kW_as_W(113.812)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(31.0), kW_as_W(343.795)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(32.0), kW_as_W(284.121)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(33.0), kW_as_W(295.434)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(34.0), kW_as_W(264.364)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(35.0), kW_as_W(247.33)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(36.0), kW_as_W(235.89)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(37.0), kW_as_W(233.43)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(38.0), kW_as_W(220.77)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(39.0), kW_as_W(213.825)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(40.0), kW_as_W(210.726)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(41.0), kW_as_W(223.706)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(42.0), kW_as_W(219.193)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(43.0), kW_as_W(186.31)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(44.0), kW_as_W(185.658)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(45.0), kW_as_W(173.137)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(46.0), kW_as_W(172.236)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(47.0), kW_as_W(47.676)});
    elecLoad.push_back(TimeAndAmount {hours_as_seconds(48.0), kW_as_W(48.952)});
    std::vector<TimeAndAmount> heat_load {};
    heat_load.reserve(49);
    heat_load.push_back(TimeAndAmount {hours_as_seconds(0.0), kW_as_W(29.60017807)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(1.0), kW_as_W(16.70505099)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(2.0), kW_as_W(16.99812206)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(3.0), kW_as_W(23.4456856)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(4.0), kW_as_W(17.5842642)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(5.0), kW_as_W(23.73875667)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(6.0), kW_as_W(17.87733527)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(7.0), kW_as_W(24.03182774)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(8.0), kW_as_W(17.87733527)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(9.0), kW_as_W(23.4456856)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(10.0), kW_as_W(16.41197992)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(11.0), kW_as_W(18.75654848)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(12.0), kW_as_W(14.36048243)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(13.0), kW_as_W(16.11890885)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(14.0), kW_as_W(10.55055852)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(15.0), kW_as_W(13.77434029)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(16.0), kW_as_W(9.37827424)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(17.0), kW_as_W(13.18819815)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(18.0), kW_as_W(9.37827424)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(19.0), kW_as_W(13.48126922)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(20.0), kW_as_W(9.67134531)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(21.0), kW_as_W(12.30898494)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(22.0), kW_as_W(10.55055852)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(23.0), kW_as_W(13.48126922)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(24.0), kW_as_W(9.67134531)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(25.0), kW_as_W(13.48126922)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(26.0), kW_as_W(12.30898494)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(27.0), kW_as_W(14.06741136)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(28.0), kW_as_W(12.30898494)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(29.0), kW_as_W(13.48126922)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(30.0), kW_as_W(10.84362959)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(31.0), kW_as_W(4.10299498)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(32.0), kW_as_W(45.71908692)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(33.0), kW_as_W(38.97845231)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(34.0), kW_as_W(33.11703091)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(35.0), kW_as_W(26.96253844)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(36.0), kW_as_W(24.32489881)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(37.0), kW_as_W(22.85954346)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(38.0), kW_as_W(26.66946737)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(39.0), kW_as_W(29.89324914)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(40.0), kW_as_W(26.66946737)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(41.0), kW_as_W(24.32489881)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(42.0), kW_as_W(27.25560951)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(43.0), kW_as_W(26.66946737)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(44.0), kW_as_W(22.85954346)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(45.0), kW_as_W(21.10111704)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(46.0), kW_as_W(18.46347741)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(47.0), kW_as_W(0.0)});
    heat_load.push_back(TimeAndAmount {hours_as_seconds(48.0), kW_as_W(3.22378177)});
    std::vector<TimeAndAmount> pv_avail {};
    pv_avail.reserve(49);
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(0.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(1.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(2.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(3.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(4.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(5.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(6.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(7.0), kW_as_W(14.36)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(8.0), kW_as_W(671.759)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(9.0), kW_as_W(1265.933)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(10.0), kW_as_W(1583.21)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(11.0), kW_as_W(1833.686)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(12.0), kW_as_W(1922.872)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(13.0), kW_as_W(1749.437)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(14.0), kW_as_W(994.715)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(15.0), kW_as_W(468.411)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(16.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(17.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(18.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(19.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(20.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(21.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(22.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(23.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(24.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(25.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(26.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(27.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(28.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(29.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(30.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(31.0), kW_as_W(10.591)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(32.0), kW_as_W(693.539)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(33.0), kW_as_W(1191.017)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(34.0), kW_as_W(1584.868)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(35.0), kW_as_W(1820.692)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(36.0), kW_as_W(1952.869)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(37.0), kW_as_W(1799.1)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(38.0), kW_as_W(1067.225)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(39.0), kW_as_W(396.023)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(40.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(41.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(42.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(43.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(44.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(45.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(46.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(47.0), kW_as_W(0.0)});
    pv_avail.push_back(TimeAndAmount {hours_as_seconds(48.0), kW_as_W(0.0)});
    // COMPONENTS
    auto pv_array_id = Model_AddScheduleBasedSource(m, pv_avail);
    auto elec_util_id = Model_AddConstantSource(m, kW_as_W(10.0));
    auto battery_id = Model_AddStore(m,
                                     static_cast<uint64_t>(kWh_as_J(100.0)),
                                     static_cast<uint64_t>(kW_as_W(10.0)),
                                     static_cast<uint64_t>(kW_as_W(1'000.0)),
                                     static_cast<uint64_t>(kWh_as_J(80.0)),
                                     static_cast<uint64_t>(kWh_as_J(100.0)));
    auto elec_source_mux_id = Model_AddMux(m, 2, 1);
    auto elec_supply_mux_id = Model_AddMux(m, 2, 2);
    auto ng_util_id = Model_AddConstantSource(m, std::numeric_limits<uint32_t>::max());
    auto ng_source_mux_id = Model_AddMux(m, 1, 2);
    auto ng_to_elec_conv_id = Model_AddConstantEfficiencyConverter(m, 42, 100);
    auto elec_heat_pump_conv_id = Model_AddMover(m, 3.5);
    auto ng_heater_conv_id = Model_AddConstantEfficiencyConverter(m, 98, 100);
    auto heating_supply_mux_id = Model_AddMux(m, 3, 1);
    auto elec_load_id = Model_AddScheduleBasedLoad(m, elecLoad);
    auto heat_load_id = Model_AddScheduleBasedLoad(m, heat_load);
    // NETWORK / CONNECTIONS
    // - electricity
    Model_AddConnection(m, pv_array_id.id, 0, elec_source_mux_id, 0);
    Model_AddConnection(m, elec_util_id, 0, elec_source_mux_id, 1);
    Model_AddConnection(m, elec_source_mux_id, 0, battery_id, 0);
    Model_AddConnection(m, battery_id, 0, elec_supply_mux_id, 0);
    Model_AddConnection(m, ng_to_elec_conv_id.id, 0, elec_supply_mux_id, 1);
    Model_AddConnection(m, elec_supply_mux_id, 0, elec_load_id, 0);
    Model_AddConnection(m, elec_supply_mux_id, 1, elec_heat_pump_conv_id.id, 0);
    // - natural gas
    Model_AddConnection(m, ng_util_id, 0, ng_source_mux_id, 0);
    Model_AddConnection(m, ng_source_mux_id, 0, ng_to_elec_conv_id.id, 0);
    Model_AddConnection(m, ng_source_mux_id, 1, ng_heater_conv_id.id, 0);
    // - heating
    Model_AddConnection(m, ng_to_elec_conv_id.id, 1, heating_supply_mux_id, 0);
    Model_AddConnection(m, ng_heater_conv_id.id, 0, heating_supply_mux_id, 1);
    Model_AddConnection(m, elec_heat_pump_conv_id.id, 0, heating_supply_mux_id, 2);
    Model_AddConnection(m, heating_supply_mux_id, 0, heat_load_id, 0);
    Simulate(m, false);
}

TEST(Erin, Test14)
{
    Model m = {};
    m.random_function = []() { return 0.4; };
    m.final_time_s = 4.0;
    std::vector<TimeAndAmount> available_power {
        {0.0, 50},
        {2.0, 10},
    };
    auto src01_id = Model_AddConstantSource(m, 50);
    auto src02_id = Model_AddScheduleBasedSource(m, available_power);
    auto mux_id = Model_AddMux(m, 2, 1);
    auto load_id = Model_AddConstantLoad(m, 100);
    Model_AddConnection(m, src01_id, 0, mux_id, 0);
    Model_AddConnection(m, src02_id.id, 0, mux_id, 1);
    Model_AddConnection(m, mux_id, 0, load_id, 0);
    Simulate(m, false);
}

TEST(Erin, Test15)
{
    Model m = {};
    m.random_function = []() { return 0.4; };
    m.final_time_s = 2.0;
    std::vector<TimeAndAmount> load_one {
        {0.0, 50},
        {2.0, 10},
    };
    auto src01_id = Model_AddConstantSource(m, 1'000);
    auto src02_id = Model_AddConstantSource(m, 1'000);
    auto conv_id = Model_AddConstantEfficiencyConverter(m, 1, 4);
    auto mux_id = Model_AddMux(m, 2, 1);
    auto load01_id = Model_AddScheduleBasedLoad(m, load_one);
    auto load02_id = Model_AddConstantLoad(m, 100);
    auto src1_to_conv_conn = Model_AddConnection(m, src01_id, 0, conv_id.id, 0);
    auto conv_to_load_conn = Model_AddConnection(m, conv_id.id, 0, load01_id, 0);
    auto conv_loss_to_mux_conn = Model_AddConnection(m, conv_id.id, 1, mux_id, 0);
    auto src2_to_mux_conn = Model_AddConnection(m, src02_id, 0, mux_id, 1);
    auto mux_to_load_conn = Model_AddConnection(m, mux_id, 0, load02_id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 2);

    double t = 0.0;
    auto src1_to_conv_results = ModelResults_GetFlowForConnection(m, src1_to_conv_conn, t, results);
    EXPECT_TRUE(src1_to_conv_results.has_value());
    EXPECT_EQ(src1_to_conv_results.value().actual_W, 200);
    EXPECT_EQ(src1_to_conv_results.value().requested_W, 200);
    EXPECT_EQ(src1_to_conv_results.value().available_W, 1'000);

    auto conv_to_load_results = ModelResults_GetFlowForConnection(m, conv_to_load_conn, t, results);
    EXPECT_TRUE(conv_to_load_results.has_value());
    EXPECT_EQ(conv_to_load_results.value().actual_W, 50);
    EXPECT_EQ(conv_to_load_results.value().requested_W, 50);
    EXPECT_EQ(conv_to_load_results.value().available_W, 250);

    auto conv_loss_to_mux_results =
        ModelResults_GetFlowForConnection(m, conv_loss_to_mux_conn, t, results);
    EXPECT_TRUE(conv_loss_to_mux_results.has_value());
    EXPECT_EQ(conv_loss_to_mux_results.value().actual_W, 100);
    EXPECT_EQ(conv_loss_to_mux_results.value().requested_W, 100);
    EXPECT_EQ(conv_loss_to_mux_results.value().available_W, 150);

    auto src2_to_mux_results = ModelResults_GetFlowForConnection(m, src2_to_mux_conn, t, results);
    EXPECT_TRUE(src2_to_mux_results.has_value());
    EXPECT_EQ(src2_to_mux_results.value().actual_W, 0);
    EXPECT_EQ(src2_to_mux_results.value().requested_W, 0);
    EXPECT_EQ(src2_to_mux_results.value().available_W, 1'000);

    auto mux_to_load_results = ModelResults_GetFlowForConnection(m, mux_to_load_conn, t, results);
    EXPECT_TRUE(mux_to_load_results.has_value());
    EXPECT_EQ(mux_to_load_results.value().actual_W, 100);
    EXPECT_EQ(mux_to_load_results.value().requested_W, 100);
    EXPECT_EQ(mux_to_load_results.value().available_W, 1'150);

    t = 2.0;
    src1_to_conv_results = ModelResults_GetFlowForConnection(m, src1_to_conv_conn, t, results);
    EXPECT_TRUE(src1_to_conv_results.has_value());
    EXPECT_EQ(src1_to_conv_results.value().actual_W, 40);
    EXPECT_EQ(src1_to_conv_results.value().requested_W, 40);
    EXPECT_EQ(src1_to_conv_results.value().available_W, 1'000);

    conv_to_load_results = ModelResults_GetFlowForConnection(m, conv_to_load_conn, t, results);
    EXPECT_TRUE(conv_to_load_results.has_value());
    EXPECT_EQ(conv_to_load_results.value().actual_W, 10);
    EXPECT_EQ(conv_to_load_results.value().requested_W, 10);
    EXPECT_EQ(conv_to_load_results.value().available_W, 250);

    conv_loss_to_mux_results =
        ModelResults_GetFlowForConnection(m, conv_loss_to_mux_conn, t, results);
    EXPECT_TRUE(conv_loss_to_mux_results.has_value());
    EXPECT_EQ(conv_loss_to_mux_results.value().actual_W, 30);
    EXPECT_EQ(conv_loss_to_mux_results.value().requested_W, 100);
    EXPECT_EQ(conv_loss_to_mux_results.value().available_W, 30);

    mux_to_load_results = ModelResults_GetFlowForConnection(m, mux_to_load_conn, t, results);
    EXPECT_TRUE(mux_to_load_results.has_value());
    EXPECT_EQ(mux_to_load_results.value().actual_W, 100);
    EXPECT_EQ(mux_to_load_results.value().requested_W, 100);
    EXPECT_EQ(mux_to_load_results.value().available_W, 1'030);

    src2_to_mux_results = ModelResults_GetFlowForConnection(m, src2_to_mux_conn, t, results);
    EXPECT_TRUE(src2_to_mux_results.has_value());
    EXPECT_EQ(src2_to_mux_results.value().actual_W, 70);
    EXPECT_EQ(src2_to_mux_results.value().requested_W, 70);
    EXPECT_EQ(src2_to_mux_results.value().available_W, 1'000);
}

TEST(Erin, Test16)
{
    Model m = {};
    m.random_function = []() { return 0.4; };
    m.final_time_s = 2.0;
    size_t src_id = Model_AddConstantSource(m, 100);
    size_t load_id = Model_AddConstantLoad(m, 50);
    size_t pass_id = Model_AddPassThrough(m);
    auto src_to_pass_conn = Model_AddConnection(m, src_id, 0, pass_id, 0);
    auto pass_to_load_conn = Model_AddConnection(m, pass_id, 0, load_id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 2);

    double t = 0.0;
    auto src_to_pass_results = ModelResults_GetFlowForConnection(m, src_to_pass_conn, t, results);
    EXPECT_TRUE(src_to_pass_results.has_value());
    EXPECT_EQ(src_to_pass_results.value().actual_W, 50);
    EXPECT_EQ(src_to_pass_results.value().requested_W, 50);
    EXPECT_EQ(src_to_pass_results.value().available_W, 100);

    auto pass_to_load_results = ModelResults_GetFlowForConnection(m, pass_to_load_conn, t, results);
    EXPECT_TRUE(pass_to_load_results.has_value());
    EXPECT_EQ(pass_to_load_results.value().actual_W, 50);
    EXPECT_EQ(pass_to_load_results.value().requested_W, 50);
    EXPECT_EQ(pass_to_load_results.value().available_W, 100);
}

TEST(Erin, Test17)
{
    std::vector<TimeState> a {{0.0, true, {}, {}}, {10.0, false, {1}, {}}, {100.0, true, {}, {}}};
    std::vector<TimeState> b {
        {0.0, true, {}, {}}, {40.0, false, {2}, {}}, {90.0, true, {}, {}}, {150.0, false, {2}, {}}};
    std::vector<TimeState> expected {{0.0, true, {}, {}},
                                     {10.0, false, {1}, {}},
                                     {40.0, false, {1, 2}, {}},
                                     {90.0, false, {1}, {}},
                                     {100.0, true, {}, {}},
                                     {150.0, false, {2}, {}}};
    std::vector<TimeState> actual = TimeState_Combine(a, b);
    EXPECT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], actual[i]);
    }
}

TEST(Erin, Test18)
{
    std::vector<TimeState> input {{0.0, true, {}, {}},
                                  {10.0, false, {1}, {}},
                                  {40.0, false, {1, 2}, {}},
                                  {90.0, false, {1}, {}},
                                  {100.0, true, {}, {}},
                                  {150.0, false, {2}, {}}};
    std::vector<TimeState> expected {
        {50.0, false, {1, 2}, {}},
        {90.0, false, {1}, {}},
        {100.0, true, {}, {}},
    };
    std::vector<TimeState> actual = TimeState_Clip(input, 50.0, 120.0, false);
    EXPECT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], actual[i]);
    }
    std::vector<TimeState> expected2 {
        {0.0, false, {1, 2}, {}},
        {40.0, false, {1}, {}},
        {50.0, true, {}, {}},
    };
    std::vector<TimeState> actual2 = TimeState_Clip(input, 50.0, 120.0, true);
    EXPECT_EQ(expected2.size(), actual2.size());
    for (size_t i = 0; i < expected2.size(); ++i)
    {
        EXPECT_EQ(expected2[i], actual2[i]);
    }
}

TEST(Erin, Test18a_TimeState_Combine_issue)
{
    std::vector<TimeState> a = {
        {.time = 43'800 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
        {.time = 43'896 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 87'696 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
    };
    std::vector<TimeState> b = {
        {.time = 2'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 3'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 5'520 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
        {.time = 6'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 8'520 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
        {.time = 9'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
    };
    std::vector<TimeState> expected = {
        {.time = 2'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 3'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 5'520 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
        {.time = 6'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 8'520 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
        {.time = 9'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 43'800 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
        {.time = 43'896 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 87'696 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
    };
    std::vector<TimeState> actual = TimeState_Combine(a, b);
    EXPECT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(actual[i], expected[i]);
    }
    actual = TimeState_Combine(b, a);
    EXPECT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(actual[i], expected[i]);
    }
}

TEST(Erin, Test18b_TimeState_Combine_issue)
{
    std::vector<TimeState> a = {
        {.time = 43'800 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
        {.time = 43'896 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 87'696 * 3600.0, .state = 0, .failureModeCauses = {1}, .fragilityModeCauses = {}},
    };
    std::vector<TimeState> b = {
        {.time = 2'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 3'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 5'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 6'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 8'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 9'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 11'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 12'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 14'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 15'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 17'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 18'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 20'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 21'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 23'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 24'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 26'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 27'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 29'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 30'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 32'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 33'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 35'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 36'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 38'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 39'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 41'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 42'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 44'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 45'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 47'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
    };
    std::vector<TimeState> expected = {
        {.time = 2'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 3'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 5'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 6'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 8'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 9'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 11'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 12'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 14'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 15'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 17'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 18'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 20'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 21'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 23'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 24'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 26'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 27'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 29'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 30'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 32'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 33'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 35'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 36'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 38'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 39'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 41'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 42'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 43'800 * 3600.0,
         .state = 0,
         .failureModeCauses = {1},
         .fragilityModeCauses = {}}, // *
        {.time = 43'896 * 3600.0,
         .state = 1,
         .failureModeCauses = {},
         .fragilityModeCauses = {}}, // *
        {.time = 44'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 45'000 * 3600.0, .state = 1, .failureModeCauses = {}, .fragilityModeCauses = {}},
        {.time = 47'520 * 3600.0, .state = 0, .failureModeCauses = {0}, .fragilityModeCauses = {}},
        {.time = 87'696 * 3600.0,
         .state = 0,
         .failureModeCauses = {0, 1},
         .fragilityModeCauses = {}}, // *
    };
    std::vector<TimeState> actual = TimeState_Combine(a, b);
    EXPECT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(actual[i], expected[i]);
    }
    actual = TimeState_Combine(b, a);
    EXPECT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(actual[i], expected[i]);
    }
}

TEST(Erin, Test19)
{
    std::vector<TimeState> A {
        {0.0, false, {}, {0}},
        {100.0, true, {}, {}},
    };
    std::vector<TimeState> B {
        {0.0, true, {}, {}},
        {120.0, false, {0}, {}},
        {180.0, true, {}, {}},
    };
    std::vector<TimeState> C {
        {0.0, true, {}, {}},
        {60.0, false, {1}, {}},
        {140.0, true, {}, {}},
    };
    std::vector<TimeState> expected {
        {0.0, false, {}, {0}},
        {60.0, false, {1}, {0}},
        {100.0, false, {1}, {}},
        {120.0, false, {0, 1}, {}},
        {140.0, false, {0}, {}},
        {180.0, true, {}, {}},
    };
    std::vector<TimeState> rel_sch;
    rel_sch = TimeState_Combine(rel_sch, A);
    rel_sch = TimeState_Combine(rel_sch, B);
    std::vector<TimeState> actual = TimeState_Combine(rel_sch, C);
    EXPECT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], actual[i]);
    }
}

TEST(Erin, Test20)
{
    std::vector<TimeState> input {
        {5.0, false, {}, {}},
        {7.0, true, {}, {}},
        {12.0, false, {}, {}},
        {14.0, true, {}, {}},
        {19.0, false, {}, {}},
        {21.0, true, {}, {}},
    };
    std::vector<TimeState> expected {
        {5.0, false, {}, {}},
        {7.0, true, {}, {}},
    };
    std::vector<TimeState> actual = TimeState_Clip(input, 0.0, 10.0, true);
    EXPECT_EQ(expected.size(), actual.size());
}

TEST(Erin, Test21)
{
    std::vector<TimeState> input {
        {0.0, true, {}, {}},
        {10.0, false, {1}, {}},
        {20.0, true, {}, {}},
        {100.0, false, {}, {1}},
        {180.0, true, {}, {}},
    };
    std::map<size_t, size_t> count_by_fail_mode_id;
    std::map<size_t, size_t> count_by_frag_mode_id;
    std::map<size_t, double> time_by_fail_mode_id;
    std::map<size_t, double> time_by_frag_mode_id;
    TimeState_CountAndTimeFailureEvents(input,
                                        200.0,
                                        count_by_fail_mode_id,
                                        count_by_frag_mode_id,
                                        time_by_fail_mode_id,
                                        time_by_frag_mode_id);
    std::map<size_t, size_t> expected_count_by_fail_mode_id {{1, 1}};
    std::map<size_t, size_t> expected_count_by_frag_mode_id {{1, 1}};
    std::map<size_t, double> expected_time_by_fail_mode_id {{1, 10.0}};
    std::map<size_t, double> expected_time_by_frag_mode_id {{1, 80.0}};
    EXPECT_EQ(expected_count_by_fail_mode_id.size(), count_by_fail_mode_id.size());
    EXPECT_EQ(expected_count_by_frag_mode_id.size(), count_by_frag_mode_id.size());
    EXPECT_EQ(expected_time_by_fail_mode_id.size(), time_by_fail_mode_id.size());
    EXPECT_EQ(expected_time_by_frag_mode_id.size(), time_by_frag_mode_id.size());
    for (auto const& p : expected_count_by_fail_mode_id)
    {
        EXPECT_TRUE(count_by_fail_mode_id.contains(p.first));
        EXPECT_EQ(p.second, count_by_fail_mode_id[p.first]);
    }
    for (auto const& p : expected_count_by_frag_mode_id)
    {
        EXPECT_TRUE(count_by_frag_mode_id.contains(p.first));
        EXPECT_EQ(p.second, count_by_frag_mode_id[p.first]);
    }
    for (auto const& p : expected_time_by_fail_mode_id)
    {
        EXPECT_TRUE(time_by_fail_mode_id.contains(p.first));
        EXPECT_EQ(p.second, time_by_fail_mode_id[p.first]);
    }
    for (auto const& p : expected_time_by_frag_mode_id)
    {
        EXPECT_TRUE(time_by_frag_mode_id.contains(p.first));
        EXPECT_EQ(p.second, time_by_frag_mode_id[p.first]);
    }
    count_by_fail_mode_id.clear();
    count_by_frag_mode_id.clear();
    time_by_fail_mode_id.clear();
    time_by_frag_mode_id.clear();
    input = {
        {10.0, false, {1}, {}},
        {20.0, true, {}, {}},
        {100.0, false, {}, {1}},
    };
    TimeState_CountAndTimeFailureEvents(input,
                                        180.0,
                                        count_by_fail_mode_id,
                                        count_by_frag_mode_id,
                                        time_by_fail_mode_id,
                                        time_by_frag_mode_id);
    EXPECT_EQ(expected_count_by_fail_mode_id.size(), count_by_fail_mode_id.size());
    EXPECT_EQ(expected_count_by_frag_mode_id.size(), count_by_frag_mode_id.size());
    EXPECT_EQ(expected_time_by_fail_mode_id.size(), time_by_fail_mode_id.size());
    EXPECT_EQ(expected_time_by_frag_mode_id.size(), time_by_frag_mode_id.size());
    for (auto const& p : expected_count_by_fail_mode_id)
    {
        EXPECT_TRUE(count_by_fail_mode_id.contains(p.first));
        EXPECT_EQ(p.second, count_by_fail_mode_id[p.first]);
    }
    for (auto const& p : expected_count_by_frag_mode_id)
    {
        EXPECT_TRUE(count_by_frag_mode_id.contains(p.first));
        EXPECT_EQ(p.second, count_by_frag_mode_id[p.first]);
    }
    for (auto const& p : expected_time_by_fail_mode_id)
    {
        EXPECT_TRUE(time_by_fail_mode_id.contains(p.first));
        EXPECT_EQ(p.second, time_by_fail_mode_id[p.first]);
    }
    for (auto const& p : expected_time_by_frag_mode_id)
    {
        EXPECT_TRUE(time_by_frag_mode_id.contains(p.first));
        EXPECT_EQ(p.second, time_by_frag_mode_id[p.first]);
    }
}

TEST(Erin, Test22)
{
    TabularFragilityCurve tfc {};
    tfc.VulnerabilityId = 0;
    tfc.Intensities = std::vector<double> {0.0, 1.0, 4.0, 6.0, 9.0, 10.0};
    tfc.FailureFractions = std::vector<double> {0.0, 0.3, 0.7, 0.8, 0.95, 1.0};
    double level = 7.0;
    double result = TabularFragilityCurve_GetFailureFraction(tfc, level);
    EXPECT_EQ(result, 0.85);
}

TEST(Erin, TestDoubleToString)
{
    double a = 1.5005;
    std::string expected_a_at_p0 = "2";
    std::string actual_a_at_p0 = erin::double_to_string(a, 0);
    EXPECT_EQ(expected_a_at_p0, actual_a_at_p0);
    std::string expected_a_at_p1 = "1.5";
    std::string actual_a_at_p1 = erin::double_to_string(a, 1);
    EXPECT_EQ(expected_a_at_p1, actual_a_at_p1);
    std::string expected_a_at_p2 = "1.5";
    std::string actual_a_at_p2 = erin::double_to_string(a, 2);
    EXPECT_EQ(expected_a_at_p2, actual_a_at_p2);
    std::string expected_a_at_p3 = "1.501";
    std::string actual_a_at_p3 = erin::double_to_string(a, 3);
    EXPECT_EQ(expected_a_at_p3, actual_a_at_p3);
    std::string expected_a_at_p4 = "1.5005";
    std::string actual_a_at_p4 = erin::double_to_string(a, 4);
    EXPECT_EQ(expected_a_at_p4, actual_a_at_p4);
    std::string expected_a_at_p5 = "1.5005";
    std::string actual_a_at_p5 = erin::double_to_string(a, 5);
    EXPECT_EQ(expected_a_at_p5, actual_a_at_p5);
    double b = 4.0;
    std::string expected_b_at_p0 = "4";
    std::string actual_b_at_p0 = erin::double_to_string(b, 0);
    EXPECT_EQ(expected_b_at_p0, actual_b_at_p0);
    std::string expected_b_at_p1 = "4";
    std::string actual_b_at_p1 = erin::double_to_string(b, 1);
    EXPECT_EQ(expected_b_at_p1, actual_b_at_p1);
    std::string expected_b_at_p2 = "4";
    std::string actual_b_at_p2 = erin::double_to_string(b, 2);
    EXPECT_EQ(expected_b_at_p2, actual_b_at_p2);
    double c = 1500.0;
    std::string expected_c_at_p0 = "1500";
    std::string actual_c_at_p0 = erin::double_to_string(c, 0);
    EXPECT_EQ(expected_c_at_p0, actual_c_at_p0);
    double d = 1.5009;
    std::string expected_d_at_p3 = "1.501";
    std::string actual_d_at_p3 = erin::double_to_string(d, 3);
    EXPECT_EQ(expected_d_at_p3, actual_d_at_p3);
    double e = 1.5006;
    std::string expected_e_at_p3 = "1.501";
    std::string actual_e_at_p3 = erin::double_to_string(e, 3);
    EXPECT_EQ(expected_e_at_p3, actual_e_at_p3);
    double f = 1.50051;
    std::string expected_f_at_p3 = "1.501";
    std::string actual_f_at_p3 = erin::double_to_string(f, 3);
    EXPECT_EQ(expected_f_at_p3, actual_f_at_p3);
    double g = static_cast<double>(std::numeric_limits<flow_t>::max()) / 5.0;
    EXPECT_EQ(3689348814741910500.0, g);
    g /= 1000.0;
    EXPECT_EQ(3689348814741910.5, g);
    std::string expected_g_at_p1 = "3689348814741910.5";
    std::string actual_g_at_p1 = erin::double_to_string(g, 1);
    EXPECT_EQ(expected_g_at_p1, actual_g_at_p1);
}

TEST(Erin, TestTimeConversion)
{
    double time_s = 8760.0 * 3600.0;
    double time_yr = time_in_seconds_to_desired_unit(time_s, TimeUnit::Year);
    EXPECT_NEAR(1.0, time_yr, 1e-6);
    double time_wk = time_in_seconds_to_desired_unit(time_s, TimeUnit::Week);
    EXPECT_NEAR(8760.0 / (24.0 * 7.0), time_wk, 1e-6);
    double time_day = time_in_seconds_to_desired_unit(time_s, TimeUnit::Day);
    EXPECT_NEAR(365.0, time_day, 1e-6);
    double time_hr = time_in_seconds_to_desired_unit(time_s, TimeUnit::Hour);
    EXPECT_NEAR(8760.0, time_hr, 1e-6);
    double time_min = time_in_seconds_to_desired_unit(time_s, TimeUnit::Minute);
    EXPECT_NEAR(8760.0 * 60.0, time_min, 1e-6);
    double output_time_s = time_in_seconds_to_desired_unit(time_s, TimeUnit::Second);
    EXPECT_NEAR(8760.0 * 60.0 * 60.0, output_time_s, 1e-6);
}

TEST(Erin, TestParseTagAndPort)
{
    std::string input = "electric_utility:OUT(0)";
    auto output = ParseTagAndPort(input, "");
    EXPECT_TRUE(output.has_value());
    EXPECT_EQ(output.value().Tag, "electric_utility");
    EXPECT_EQ(output.value().Port, 0);
    input = "bus:IN(1)";
    output = ParseTagAndPort(input, "");
    EXPECT_TRUE(output.has_value());
    EXPECT_EQ(output.value().Tag, "bus");
    EXPECT_EQ(output.value().Port, 1);
    input = "my_place:OUT(123)";
    output = ParseTagAndPort(input, "");
    EXPECT_TRUE(output.has_value());
    EXPECT_EQ(output.value().Tag, "my_place");
    EXPECT_EQ(output.value().Port, 123);
}

TEST(Erin, TestParsingComponentsInUse)
{
    std::vector<toml::value> conns {
        {"a:OUT(0)", "b:IN(0)", "electricity"},
        {"b:OUT(0)", "c:IN(0)", "electricity"},
        {"c:OUT(0)", "d:IN(0)", "electricity"},
    };
    std::unordered_map<std::string, toml::value> conn_table {
        {
            "connections",
            conns,
        },
    };
    toml::value example_input = std::unordered_map<std::string, toml::value> {
        {
            "network",
            conn_table,
        },
    };
    std::unordered_set<std::string> expected {"a", "b", "c", "d"};
    std::unordered_set<std::string> actual =
        erin::TOMLTable_parse_component_tags_in_use(example_input);
    EXPECT_EQ(expected.size(), actual.size());
    for (auto const& item : expected)
    {
        EXPECT_TRUE(actual.contains(item)) << "expected item '" << item << "' not present";
    }
}

TEST(Erin, TestApplyUniformTimeStep)
{
    // SIMULATION INFO and INITIALIZATION
    Model m = {};
    m.random_function = []() { return 0.4; };
    m.final_time_s = hours_as_seconds(24.0);

    // COMPONENTS
    std::vector<TimeAndAmount> ePV_avail {};
    ePV_avail.reserve(5);
    ePV_avail.push_back(TimeAndAmount {hours_as_seconds(0.0), kW_as_W(0.0)});
    ePV_avail.push_back(TimeAndAmount {hours_as_seconds(6.0), kW_as_W(1.0)});
    ePV_avail.push_back(TimeAndAmount {hours_as_seconds(9.0), kW_as_W(1.5)});
    ePV_avail.push_back(TimeAndAmount {hours_as_seconds(18.0), kW_as_W(1.0)});
    ePV_avail.push_back(TimeAndAmount {hours_as_seconds(21.0), kW_as_W(0.0)});

    // COMPONENTS
    auto e_pv = Model_AddScheduleBasedSource(m, ePV_avail);
    auto e_batt_id = Model_AddStore(m,
                                    static_cast<uint64_t>(kWh_as_J(2.0)),
                                    static_cast<uint64_t>(kW_as_W(0.5)),
                                    static_cast<uint64_t>(kW_as_W(1.0)),
                                    static_cast<uint64_t>(kWh_as_J(0.0)),
                                    static_cast<uint64_t>(kWh_as_J(1.0)));

    // LOADS
    std::vector<TimeAndAmount> e_load {};
    e_load.reserve(5);
    e_load.push_back(TimeAndAmount {hours_as_seconds(0.0), kW_as_W(0.1)});
    e_load.push_back(TimeAndAmount {hours_as_seconds(6.0), kW_as_W(1.5)});
    e_load.push_back(TimeAndAmount {hours_as_seconds(12.0), kW_as_W(0.5)});
    e_load.push_back(TimeAndAmount {hours_as_seconds(18.0), kW_as_W(1.0)});
    e_load.push_back(TimeAndAmount {hours_as_seconds(21.0), kW_as_W(0.1)});
    auto eLoadId = Model_AddScheduleBasedLoad(m, e_load);

    // NETWORK / CONNECTIONS
    Model_AddConnection(m, e_pv.id, 0, e_batt_id, 0);
    Model_AddConnection(m, e_batt_id, 0, eLoadId, 0);

    // SIMULATE
    auto results = Simulate(m, false);

    // NOTE: 1-h steps
    auto modified_results = ApplyUniformTimeStep(results, 1.0);

    EXPECT_EQ(modified_results.size(), 25) << "incorrect number of events";
    EXPECT_EQ(modified_results[8].time_s, hours_as_seconds(8.0)) << "incorrect time of event";
    EXPECT_EQ(modified_results[8].flows.size(), 3) << "incorrect number of flows";

    EXPECT_EQ(modified_results[8].flows[2].requested_W, kW_as_W(1.5))
        << "incorrect requested-flow value";
    EXPECT_EQ(modified_results[8].flows[2].actual_W, kW_as_W(1.0)) << "incorrect actual-flow value";
    EXPECT_EQ(modified_results[8].storage_amounts_J[0], kWh_as_J(0.0))
        << "incorrect storage amount";

    EXPECT_EQ(modified_results[14].storage_amounts_J[0], kWh_as_J(1.0))
        << "incorrect storage amount";
}
