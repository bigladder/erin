#include "erin_next/erin_next.h"
#include "erin_next/erin_next_timestate.h"
#include <gtest/gtest.h>
#include <iomanip>
#include <limits>

using namespace erin;

static double
Round(double n, unsigned int places = 2)
{
    double mult = std::pow(10.0, (double)places);
    return std::round(n * mult) / mult;
}

TEST(Erin, Test1)
{
    Model m = {};
    auto srcId = Model_AddConstantSource(m, 100);
    auto loadId = Model_AddConstantLoad(m, 10);
    auto srcToLoadConn = Model_AddConnection(m, srcId, 0, loadId, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].Time, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].Flows.size(), 1) << "size of flows must equal 1";

    auto srcToLoadResult =
        ModelResults_GetFlowForConnection(m, srcToLoadConn, 0.0, results);
    EXPECT_TRUE(srcToLoadResult.has_value())
        << "connection result should have a value";
    EXPECT_EQ(srcToLoadResult.value().Actual_W, 10)
        << "actual value must equal 10";
    EXPECT_EQ(srcToLoadResult.value().Available_W, 100)
        << "available must equal 100";
    EXPECT_EQ(srcToLoadResult.value().Requested_W, 10)
        << "requested must equal 10";
}

TEST(Erin, Test2)
{
    Model m = {};
    auto srcId = Model_AddConstantSource(m, 100);
    auto loadId = Model_AddConstantLoad(m, 10);
    auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
    auto convToLoadConn = Model_AddConnection(m, convId.Id, 0, loadId, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].Time, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].Flows.size(), 3) << "size of flows must equal 3";

    auto srcToConvResults =
        ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
    EXPECT_TRUE(srcToConvResults.has_value())
        << "source to converter must have results";
    EXPECT_EQ(srcToConvResults.value().Requested_W, 20)
        << "requested must equal 20";
    EXPECT_EQ(srcToConvResults.value().Actual_W, 20)
        << "actual value must equal 20";
    EXPECT_EQ(srcToConvResults.value().Available_W, 100)
        << "available must equal 100";

    auto convToLoadResults =
        ModelResults_GetFlowForConnection(m, convToLoadConn, 0.0, results);
    EXPECT_TRUE(convToLoadResults.has_value())
        << "converter to load must have results";
    EXPECT_EQ(convToLoadResults.value().Requested_W, 10)
        << "requested must equal 10";
    EXPECT_EQ(convToLoadResults.value().Actual_W, 10)
        << "actual value must equal 10";
    EXPECT_EQ(convToLoadResults.value().Available_W, 50)
        << "available must equal 50";

    auto convToWasteResults = ModelResults_GetFlowForConnection(
        m, convId.WasteConnection, 0.0, results
    );
    EXPECT_TRUE(convToWasteResults.has_value())
        << "converter to waste must have results";
    EXPECT_EQ(convToWasteResults.value().Requested_W, 10)
        << "requested must equal 10";
    EXPECT_EQ(convToWasteResults.value().Actual_W, 10)
        << "actual value must equal 10";
    EXPECT_EQ(convToWasteResults.value().Available_W, 10)
        << "available must equal 10";
}

TEST(Erin, Test3)
{
    Model m = {};
    auto srcId = Model_AddConstantSource(m, 100);
    auto load1Id = Model_AddConstantLoad(m, 10);
    auto load2Id = Model_AddConstantLoad(m, 2);
    auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
    auto convToLoad1Conn = Model_AddConnection(m, convId.Id, 0, load1Id, 0);
    auto convToLoad2Conn = Model_AddConnection(m, convId.Id, 1, load2Id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].Time, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].Flows.size(), 4) << "size of flows must equal 4";

    auto srcToConvResults =
        ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
    EXPECT_TRUE(srcToConvResults.has_value())
        << "source to converter must have results";
    EXPECT_EQ(srcToConvResults.value().Requested_W, 20)
        << "requested must equal 20";
    EXPECT_EQ(srcToConvResults.value().Actual_W, 20)
        << "actual value must equal 20";
    EXPECT_EQ(srcToConvResults.value().Available_W, 100)
        << "available must equal 100";

    auto convToLoad1Results =
        ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
    EXPECT_TRUE(convToLoad1Results.has_value())
        << "converter to load1 must have results";
    EXPECT_EQ(convToLoad1Results.value().Requested_W, 10)
        << "requested must equal 10";
    EXPECT_EQ(convToLoad1Results.value().Actual_W, 10)
        << "actual value must equal 10";
    EXPECT_EQ(convToLoad1Results.value().Available_W, 50)
        << "available must equal 50";

    auto convToLoad2Results =
        ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
    EXPECT_TRUE(convToLoad2Results.has_value())
        << "conv to load2 must have results";
    EXPECT_EQ(convToLoad2Results.value().Requested_W, 2)
        << "requested must equal 2";
    EXPECT_EQ(convToLoad2Results.value().Actual_W, 2)
        << "actual value must equal 2";
    EXPECT_EQ(convToLoad2Results.value().Available_W, 10)
        << "available must equal 10";

    auto convToWasteResults = ModelResults_GetFlowForConnection(
        m, convId.WasteConnection, 0.0, results
    );
    EXPECT_TRUE(convToWasteResults.has_value())
        << "conv to waste must have results";
    EXPECT_EQ(convToWasteResults.value().Requested_W, 8)
        << "requested must equal 8";
    EXPECT_EQ(convToWasteResults.value().Actual_W, 8)
        << "actual value must equal 8";
    EXPECT_EQ(convToWasteResults.value().Available_W, 8)
        << "available must equal 8";
}

TEST(Erin, Test3A)
{
    Model m{};
    auto srcId = Model_AddConstantSource(m, 100);
    auto load1Id = Model_AddConstantLoad(m, 10);
    auto load2Id = Model_AddConstantLoad(m, 2);
    auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto convToLoad2Conn = Model_AddConnection(m, convId.Id, 1, load2Id, 0);
    auto convToLoad1Conn = Model_AddConnection(m, convId.Id, 0, load1Id, 0);
    auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].Time, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].Flows.size(), 4) << "size of flows must equal 4";

    auto srcToConvResults =
        ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
    EXPECT_TRUE(srcToConvResults.has_value())
        << "source to converter must have results";
    EXPECT_EQ(srcToConvResults.value().Requested_W, 20)
        << "requested must equal 20";
    EXPECT_EQ(srcToConvResults.value().Actual_W, 20)
        << "actual value must equal 20";
    EXPECT_EQ(srcToConvResults.value().Available_W, 100)
        << "available must equal 100";

    auto convToLoad1Results =
        ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
    EXPECT_TRUE(
        convToLoad1Results.has_value() && "converter to load1 must have results"
    );
    EXPECT_EQ(convToLoad1Results.value().Requested_W, 10)
        << "requested must equal 10";
    EXPECT_EQ(convToLoad1Results.value().Actual_W, 10)
        << "actual value must equal 10";
    EXPECT_EQ(convToLoad1Results.value().Available_W, 50)
        << "available must equal 50";

    auto convToLoad2Results =
        ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
    EXPECT_TRUE(convToLoad2Results.has_value())
        << "conv to load2 must have results";
    EXPECT_EQ(convToLoad2Results.value().Requested_W, 2)
        << "requested must equal 2";
    EXPECT_EQ(convToLoad2Results.value().Actual_W, 2)
        << "actual value must equal 2";
    EXPECT_EQ(convToLoad2Results.value().Available_W, 10)
        << "available must equal 10";

    auto convToWasteResults = ModelResults_GetFlowForConnection(
        m, convId.WasteConnection, 0.0, results
    );
    EXPECT_TRUE(convToWasteResults.has_value())
        << "conv to waste must have results";
    EXPECT_EQ(convToWasteResults.value().Requested_W, 8)
        << "requested must equal 8";
    EXPECT_EQ(convToWasteResults.value().Actual_W, 8)
        << "actual value must equal 8";
    EXPECT_EQ(convToWasteResults.value().Available_W, 8)
        << "available must equal 8";
}

TEST(Erin, Test4)
{
    std::vector<TimeAndAmount> timesAndLoads = {};
    timesAndLoads.push_back({0.0, 10});
    timesAndLoads.push_back({3600.0, 200});
    Model m = {};
    m.FinalTime = 3600.0;
    auto srcId = Model_AddConstantSource(m, 100);
    auto loadId = Model_AddScheduleBasedLoad(m, timesAndLoads);
    auto srcToLoadConn = Model_AddConnection(m, srcId, 0, loadId, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 2) << "output must have a size of 2";
    EXPECT_EQ(results[0].Time, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].Flows.size(), 1) << "size of flows[0] must equal 1";

    auto srcToLoadResults_0 =
        ModelResults_GetFlowForConnection(m, srcToLoadConn, 0.0, results);
    EXPECT_TRUE(srcToLoadResults_0.has_value())
        << "source to load must have results at time=0.0";
    EXPECT_EQ(srcToLoadResults_0.value().Requested_W, 10)
        << "requested must equal 10";
    EXPECT_EQ(srcToLoadResults_0.value().Actual_W, 10)
        << "actual value must equal 10";
    EXPECT_EQ(srcToLoadResults_0.value().Available_W, 100)
        << "available must equal 100";
    EXPECT_EQ(results[1].Time, 3600.0) << "time must equal 3600.0";
    EXPECT_EQ(results[1].Flows.size(), 1) << "size of flows[1] must equal 1";

    auto srcToLoadResults_3600 =
        ModelResults_GetFlowForConnection(m, srcToLoadConn, 3600.0, results);
    EXPECT_TRUE(srcToLoadResults_3600.has_value())
        << "source to load must have results at time=3600.0";
    EXPECT_EQ(srcToLoadResults_3600.value().Requested_W, 200)
        << "requested must equal 200";
    EXPECT_EQ(srcToLoadResults_3600.value().Actual_W, 100)
        << "actual value must equal 100";
    EXPECT_EQ(srcToLoadResults_3600.value().Available_W, 100)
        << "available must equal 100";
}

TEST(Erin, Test5)
{
    std::vector<TimeAndAmount> timesAndLoads = {};
    Model m = {};
    auto srcId = Model_AddConstantSource(m, 100);
    auto load1Id = Model_AddConstantLoad(m, 10);
    auto load2Id = Model_AddConstantLoad(m, 7);
    auto load3Id = Model_AddConstantLoad(m, 5);
    auto conv1 = Model_AddConstantEfficiencyConverter(m, 1, 4);
    auto conv2 = Model_AddConstantEfficiencyConverter(m, 1, 4);
    auto conv3 = Model_AddConstantEfficiencyConverter(m, 1, 4);
    auto srcToConv1Conn = Model_AddConnection(m, srcId, 0, conv1.Id, 0);
    auto conv1ToLoad1Conn = Model_AddConnection(m, conv1.Id, 0, load1Id, 0);
    auto conv1ToConv2Conn = Model_AddConnection(m, conv1.Id, 1, conv2.Id, 0);
    auto conv2ToLoad2Conn = Model_AddConnection(m, conv2.Id, 0, load2Id, 0);
    auto conv2ToConv3Conn = Model_AddConnection(m, conv2.Id, 1, conv3.Id, 0);
    auto conv3ToLoad3Conn = Model_AddConnection(m, conv3.Id, 0, load3Id, 0);
    auto results = Simulate(m, false);
    auto srcToConv1Results =
        ModelResults_GetFlowForConnection(m, srcToConv1Conn, 0.0, results);
    auto conv1ToLoad1Results =
        ModelResults_GetFlowForConnection(m, conv1ToLoad1Conn, 0.0, results);
    auto conv1ToConv2Results =
        ModelResults_GetFlowForConnection(m, conv1ToConv2Conn, 0.0, results);
    auto conv2ToLoad2Results =
        ModelResults_GetFlowForConnection(m, conv2ToLoad2Conn, 0.0, results);
    auto conv2ToConv3Results =
        ModelResults_GetFlowForConnection(m, conv2ToConv3Conn, 0.0, results);
    auto conv3ToLoad3Results =
        ModelResults_GetFlowForConnection(m, conv3ToLoad3Conn, 0.0, results);
    EXPECT_EQ(srcToConv1Results.value().Actual_W, 40)
        << "src to conv1 should flow 40";
    EXPECT_EQ(conv1ToLoad1Results.value().Actual_W, 10)
        << "conv1 to load1 should flow 10";
    EXPECT_EQ(conv1ToConv2Results.value().Actual_W, 28)
        << "conv1 to conv2 should flow 28";
    EXPECT_EQ(conv2ToLoad2Results.value().Actual_W, 7)
        << "conv1 to conv2 should flow 7";
    EXPECT_EQ(conv2ToConv3Results.value().Actual_W, 20)
        << "conv2 to conv3 should flow 21";
    EXPECT_EQ(conv3ToLoad3Results.value().Actual_W, 5)
        << "conv3 to load3 should flow 5";
}

TEST(Erin, Test6)
{
    Model m = {};
    auto src1Id = Model_AddConstantSource(m, 10);
    auto src2Id = Model_AddConstantSource(m, 50);
    auto load1Id = Model_AddConstantLoad(m, 10);
    auto load2Id = Model_AddConstantLoad(m, 80);
    auto muxId = Model_AddMux(m, 2, 2);
    auto src1ToMuxConn = Model_AddConnection(m, src1Id, 0, muxId, 0);
    auto src2ToMuxConn = Model_AddConnection(m, src2Id, 0, muxId, 1);
    auto muxToLoad1Conn = Model_AddConnection(m, muxId, 0, load1Id, 0);
    auto muxToLoad2Conn = Model_AddConnection(m, muxId, 1, load2Id, 0);
    auto results = Simulate(m, false);
    auto src1ToMuxResults =
        ModelResults_GetFlowForConnection(m, src1ToMuxConn, 0.0, results);
    EXPECT_EQ(src1ToMuxResults.value().Actual_W, 10)
        << "src1 -> mux expected actual flow of 10";

    auto src2ToMuxResults =
        ModelResults_GetFlowForConnection(m, src2ToMuxConn, 0.0, results);
    EXPECT_EQ(src2ToMuxResults.value().Actual_W, 50)
        << "src2 -> mux expected actual flow of 50";

    auto muxToLoad1Results =
        ModelResults_GetFlowForConnection(m, muxToLoad1Conn, 0.0, results);
    EXPECT_EQ(muxToLoad1Results.value().Actual_W, 10)
        << "mux -> load1 expected actual flow of 10";

    auto muxToLoad2Results =
        ModelResults_GetFlowForConnection(m, muxToLoad2Conn, 0.0, results);
    EXPECT_EQ(muxToLoad2Results.value().Actual_W, 50)
        << "mux -> load2 expected actual flow of 50";
}

TEST(Erin, Test7)
{
    Model m = {};
    m.FinalTime = 10.0;
    auto srcId = Model_AddConstantSource(m, 0);
    auto storeId = Model_AddStore(m, 100, 10, 10, 0, 100);
    auto loadId = Model_AddConstantLoad(m, 10);
    auto srcToStoreConn = Model_AddConnection(m, srcId, 0, storeId, 0);
    auto storeToLoadConn = Model_AddConnection(m, storeId, 0, loadId, 0);
    auto results = Simulate(m, false);

    auto srcToStoreResults =
        ModelResults_GetFlowForConnection(m, srcToStoreConn, 0.0, results);
    EXPECT_EQ(srcToStoreResults.value().Actual_W, 0)
        << "src to store should be providing 0";
    EXPECT_EQ(srcToStoreResults.value().Requested_W, 10)
        << "src to store request is 10";
    EXPECT_EQ(srcToStoreResults.value().Available_W, 0)
        << "src to store available is 0";

    auto storeToLoadResults =
        ModelResults_GetFlowForConnection(m, storeToLoadConn, 0.0, results);
    EXPECT_TRUE(storeToLoadResults.has_value())
        << "should have results for store to load connection";
    EXPECT_EQ(storeToLoadResults.value().Actual_W, 10)
        << "store to load should be providing 10";
    EXPECT_EQ(storeToLoadResults.value().Requested_W, 10)
        << "store to load should be requesting 10";
    EXPECT_EQ(storeToLoadResults.value().Available_W, 10)
        << "store to load available should be 10";
    EXPECT_EQ(results.size(), 2)
        << "there should be two time events in results";
    EXPECT_TRUE(
        (results[1].Time > 10.0 - 1e-6) && (results[1].Time < 10.0 + 1e-6)
    ) << "time should be 10";

    auto srcToStoreResultsAt10 =
        ModelResults_GetFlowForConnection(m, srcToStoreConn, 10.0, results);
    EXPECT_EQ(srcToStoreResultsAt10.value().Actual_W, 0)
        << "src to store should be providing 0";
    EXPECT_EQ(srcToStoreResultsAt10.value().Requested_W, 20)
        << "src to store request is 20";
    EXPECT_EQ(srcToStoreResultsAt10.value().Available_W, 0)
        << "src to store available is 0";

    auto storeToLoadResultsAt10 =
        ModelResults_GetFlowForConnection(m, storeToLoadConn, 10.0, results);
    EXPECT_TRUE(storeToLoadResultsAt10.has_value())
        << "should have results for store to load connection";
    EXPECT_EQ(storeToLoadResultsAt10.value().Actual_W, 0)
        << "store to load should be providing 0";
    EXPECT_EQ(storeToLoadResultsAt10.value().Requested_W, 10)
        << "store to load should be requesting 10";
    EXPECT_EQ(storeToLoadResultsAt10.value().Available_W, 0)
        << "store to load available should be 0";
}

TEST(Erin, Test8)
{
    Model m = {};
    m.FinalTime = 20.0;
    auto srcId = Model_AddConstantSource(m, 5);
    auto storeId = Model_AddStore(m, 100, 10, 10, 0, 100);
    auto loadId = Model_AddConstantLoad(m, 10);
    auto srcToStoreConn = Model_AddConnection(m, srcId, 0, storeId, 0);
    auto storeToLoadConn = Model_AddConnection(m, storeId, 0, loadId, 0);
    auto results = Simulate(m, false);
    auto srcToStoreResults =
        ModelResults_GetFlowForConnection(m, srcToStoreConn, 0.0, results);
    EXPECT_EQ(srcToStoreResults.value().Actual_W, 5)
        << "src to store should be providing 5";
    EXPECT_EQ(srcToStoreResults.value().Requested_W, 10)
        << "src to store request is 10";
    EXPECT_EQ(srcToStoreResults.value().Available_W, 5)
        << "src to store available is 5";

    auto storeToLoadResults =
        ModelResults_GetFlowForConnection(m, storeToLoadConn, 0.0, results);
    EXPECT_TRUE(storeToLoadResults.has_value())
        << "should have results for store to load connection";
    EXPECT_EQ(storeToLoadResults.value().Actual_W, 10)
        << "store to load should be providing 10";
    EXPECT_EQ(storeToLoadResults.value().Requested_W, 10)
        << "store to load should be requesting 10";
    EXPECT_EQ(storeToLoadResults.value().Available_W, 15)
        << "store to load available should be 15";
    EXPECT_EQ(results.size(), 2)
        << "there should be two time events in results";
    EXPECT_TRUE(
        (results[1].Time > 20.0 - 1e-6) && (results[1].Time < 20.0 + 1e-6)
    ) << "time should be 20";

    auto srcToStoreResultsAt20 =
        ModelResults_GetFlowForConnection(m, srcToStoreConn, 20.0, results);
    EXPECT_EQ(srcToStoreResultsAt20.value().Actual_W, 5)
        << "src to store should be providing 5";
    EXPECT_EQ(srcToStoreResultsAt20.value().Requested_W, 20)
        << "src to store request is 20";
    EXPECT_EQ(srcToStoreResultsAt20.value().Available_W, 5)
        << "src to store available is 5";

    auto storeToLoadResultsAt20 =
        ModelResults_GetFlowForConnection(m, storeToLoadConn, 20.0, results);
    EXPECT_TRUE(storeToLoadResultsAt20.has_value())
        << "should have results for store to load connection";
    EXPECT_EQ(storeToLoadResultsAt20.value().Actual_W, 5)
        << "store to load should be providing 5";
    EXPECT_EQ(storeToLoadResultsAt20.value().Requested_W, 10)
        << "store to load should be requesting 10";
    EXPECT_EQ(storeToLoadResultsAt20.value().Available_W, 5)
        << "store to load available should be 5";
}

TEST(Erin, Test9)
{
    std::vector<TimeAndAmount> timesAndLoads = {};
    timesAndLoads.push_back({0.0, 20});
    timesAndLoads.push_back({5.0, 5});
    timesAndLoads.push_back({10.0, 15});
    Model m = {};
    m.FinalTime = 25.0;
    auto srcId = Model_AddConstantSource(m, 10);
    auto storeId = Model_AddStore(m, 100, 10, 10, 80, 100);
    auto loadId = Model_AddScheduleBasedLoad(m, timesAndLoads);
    auto srcToStoreConn = Model_AddConnection(m, srcId, 0, storeId, 0);
    auto storeToLoadConn = Model_AddConnection(m, storeId, 0, loadId, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 5) << "expected 5 time steps";
    EXPECT_EQ(Round(results[0].Time), 0.0) << "expect first time is 0.0";
    EXPECT_EQ(Round(results[1].Time), 2.0) << "expect second time is 2.0";
    EXPECT_EQ(Round(results[2].Time), 5.0) << "expect third time is 5.0";
    EXPECT_EQ(Round(results[3].Time), 10.0) << "expect fourth time is 10.0";
    EXPECT_EQ(Round(results[4].Time), 25.0) << "expect fifth time is 25.0";

    auto srcToStoreResultsAt0 =
        ModelResults_GetFlowForConnection(m, srcToStoreConn, 0.0, results);
    auto storeToLoadResultsAt0 =
        ModelResults_GetFlowForConnection(m, storeToLoadConn, 0.0, results);
    auto storeAmount0 = ModelResults_GetStoreState(m, storeId, 0.0, results);
    EXPECT_EQ(srcToStoreResultsAt0.value().Actual_W, 10);
    EXPECT_EQ(srcToStoreResultsAt0.value().Requested_W, 20);
    EXPECT_EQ(srcToStoreResultsAt0.value().Available_W, 10);
    EXPECT_EQ(storeToLoadResultsAt0.value().Actual_W, 20);
    EXPECT_EQ(storeToLoadResultsAt0.value().Requested_W, 20);
    EXPECT_EQ(storeToLoadResultsAt0.value().Available_W, 20);
    EXPECT_EQ(storeAmount0.value(), 100);

    auto srcToStoreResultsAt2 =
        ModelResults_GetFlowForConnection(m, srcToStoreConn, 2.0, results);
    auto storeToLoadResultsAt2 =
        ModelResults_GetFlowForConnection(m, storeToLoadConn, 2.0, results);
    auto storeAmount2 = ModelResults_GetStoreState(m, storeId, 2.0, results);
    EXPECT_EQ(srcToStoreResultsAt2.value().Actual_W, 10);
    EXPECT_EQ(srcToStoreResultsAt2.value().Requested_W, 30);
    EXPECT_EQ(srcToStoreResultsAt2.value().Available_W, 10);
    EXPECT_EQ(storeToLoadResultsAt2.value().Actual_W, 20);
    EXPECT_EQ(storeToLoadResultsAt2.value().Requested_W, 20);
    EXPECT_EQ(storeToLoadResultsAt2.value().Available_W, 20);
    EXPECT_EQ(storeAmount2.value(), 80);

    auto srcToStoreResultsAt5 =
        ModelResults_GetFlowForConnection(m, srcToStoreConn, 5.0, results);
    auto storeToLoadResultsAt5 =
        ModelResults_GetFlowForConnection(m, storeToLoadConn, 5.0, results);
    auto storeAmount5 = ModelResults_GetStoreState(m, storeId, 5.0, results);
    EXPECT_EQ(srcToStoreResultsAt5.value().Actual_W, 10);
    EXPECT_EQ(srcToStoreResultsAt5.value().Requested_W, 15);
    EXPECT_EQ(srcToStoreResultsAt5.value().Available_W, 10);
    EXPECT_EQ(storeToLoadResultsAt5.value().Actual_W, 5);
    EXPECT_EQ(storeToLoadResultsAt5.value().Requested_W, 5);
    EXPECT_EQ(storeToLoadResultsAt5.value().Available_W, 20);
    EXPECT_EQ(storeAmount5.value(), 50);

    auto srcToStoreResultsAt10 =
        ModelResults_GetFlowForConnection(m, srcToStoreConn, 10.0, results);
    auto storeToLoadResultsAt10 =
        ModelResults_GetFlowForConnection(m, storeToLoadConn, 10.0, results);
    auto storeAmount10 = ModelResults_GetStoreState(m, storeId, 10.0, results);
    EXPECT_EQ(srcToStoreResultsAt10.value().Actual_W, 10);
    EXPECT_EQ(srcToStoreResultsAt10.value().Requested_W, 25);
    EXPECT_EQ(srcToStoreResultsAt10.value().Available_W, 10);
    EXPECT_EQ(storeToLoadResultsAt10.value().Actual_W, 15);
    EXPECT_EQ(storeToLoadResultsAt10.value().Requested_W, 15);
    EXPECT_EQ(storeToLoadResultsAt10.value().Available_W, 20);
    EXPECT_EQ(storeAmount10.value(), 75);

    auto srcToStoreResultsAt25 =
        ModelResults_GetFlowForConnection(m, srcToStoreConn, 25.0, results);
    auto storeToLoadResultsAt25 =
        ModelResults_GetFlowForConnection(m, storeToLoadConn, 25.0, results);
    auto storeAmount25 = ModelResults_GetStoreState(m, storeId, 25.0, results);
    EXPECT_EQ(srcToStoreResultsAt25.value().Actual_W, 10);
    EXPECT_EQ(srcToStoreResultsAt25.value().Requested_W, 25);
    EXPECT_EQ(srcToStoreResultsAt25.value().Available_W, 10);
    EXPECT_EQ(storeToLoadResultsAt25.value().Actual_W, 10);
    EXPECT_EQ(storeToLoadResultsAt25.value().Requested_W, 15);
    EXPECT_EQ(storeToLoadResultsAt25.value().Available_W, 10);
    EXPECT_EQ(storeAmount25.value(), 0);
}

TEST(Erin, Test10)
{
    std::vector<TimeAndAmount> timesAndLoads = {};
    timesAndLoads.push_back({0.0, 20});
    timesAndLoads.push_back({5.0, 5});
    timesAndLoads.push_back({10.0, 15});
    Model m = {};
    m.FinalTime = 12.5;
    auto src1Id = Model_AddConstantSource(m, 20);
    auto src2Id = Model_AddConstantSource(m, 5);
    auto storeId = Model_AddStore(m, 100, 10, 10, 80, 100);
    auto muxId = Model_AddMux(m, 2, 2);
    auto conv = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto load1Id = Model_AddConstantLoad(m, 20);
    auto load2Id = Model_AddScheduleBasedLoad(m, timesAndLoads);
    auto load3Id = Model_AddConstantLoad(m, 5);
    auto src1ToMux0Port0Conn = Model_AddConnection(m, src1Id, 0, muxId, 0);
    auto src2ToStoreConn = Model_AddConnection(m, src2Id, 0, storeId, 0);
    auto storeToMux0Port1Conn = Model_AddConnection(m, storeId, 0, muxId, 1);
    auto mux0Port0ToLoad1Conn = Model_AddConnection(m, muxId, 0, load1Id, 0);
    auto mux0Port1ToConvConn = Model_AddConnection(m, muxId, 1, conv.Id, 0);
    auto convToLoad2Conn = Model_AddConnection(m, conv.Id, 0, load2Id, 0);
    auto convToLoad3Conn = Model_AddConnection(m, conv.Id, 1, load3Id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 5) << "expect 5 events";

    // time = 0.0
    double t = 0.0;
    size_t resultsIdx = 0;
    EXPECT_EQ(results[resultsIdx].Time, t);
    auto convToWasteResults =
        ModelResults_GetFlowForConnection(m, conv.WasteConnection, t, results);
    EXPECT_EQ(convToWasteResults.value().Actual_W, 3);
    EXPECT_EQ(convToWasteResults.value().Requested_W, 3);
    EXPECT_EQ(convToWasteResults.value().Available_W, 3);

    auto src1ToMuxResults =
        ModelResults_GetFlowForConnection(m, src1ToMux0Port0Conn, 0.0, results);
    EXPECT_EQ(src1ToMuxResults.value().Actual_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Available_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Requested_W, 60);

    auto src2ToStoreResults =
        ModelResults_GetFlowForConnection(m, src2ToStoreConn, 0.0, results);
    EXPECT_EQ(src2ToStoreResults.value().Actual_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Available_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Requested_W, 40);

    auto storeToMuxResults = ModelResults_GetFlowForConnection(
        m, storeToMux0Port1Conn, 0.0, results
    );
    EXPECT_EQ(storeToMuxResults.value().Actual_W, 15);
    EXPECT_EQ(storeToMuxResults.value().Available_W, 15);
    EXPECT_EQ(storeToMuxResults.value().Requested_W, 40);

    auto muxToLoad1Results = ModelResults_GetFlowForConnection(
        m, mux0Port0ToLoad1Conn, 0.0, results
    );
    EXPECT_EQ(muxToLoad1Results.value().Actual_W, 20);
    EXPECT_EQ(muxToLoad1Results.value().Available_W, 20);
    EXPECT_EQ(muxToLoad1Results.value().Requested_W, 20);

    auto muxToConvResults =
        ModelResults_GetFlowForConnection(m, mux0Port1ToConvConn, 0.0, results);
    EXPECT_EQ(muxToConvResults.value().Actual_W, 15);
    EXPECT_EQ(muxToConvResults.value().Available_W, 15);
    EXPECT_EQ(muxToConvResults.value().Requested_W, 40);

    auto convToLoad2Results =
        ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
    EXPECT_EQ(convToLoad2Results.value().Actual_W, 7);
    EXPECT_EQ(convToLoad2Results.value().Available_W, 7);
    EXPECT_EQ(convToLoad2Results.value().Requested_W, 20);

    auto convToLoad3Results =
        ModelResults_GetFlowForConnection(m, convToLoad3Conn, 0.0, results);
    EXPECT_EQ(convToLoad3Results.value().Actual_W, 5);
    EXPECT_EQ(convToLoad3Results.value().Available_W, 8);
    EXPECT_EQ(convToLoad3Results.value().Requested_W, 5);

    auto storeAmount = ModelResults_GetStoreState(m, storeId, 0.0, results);
    EXPECT_EQ(storeAmount.value(), 100);

    // time = 2.0
    t = 2.0;
    resultsIdx = 1;
    EXPECT_EQ(results[resultsIdx].Time, t);

    convToWasteResults =
        ModelResults_GetFlowForConnection(m, conv.WasteConnection, t, results);
    EXPECT_EQ(convToWasteResults.value().Actual_W, 3);
    EXPECT_EQ(convToWasteResults.value().Requested_W, 3);
    EXPECT_EQ(convToWasteResults.value().Available_W, 3);

    src1ToMuxResults =
        ModelResults_GetFlowForConnection(m, src1ToMux0Port0Conn, t, results);
    EXPECT_EQ(src1ToMuxResults.value().Actual_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Available_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Requested_W, 60);

    src2ToStoreResults =
        ModelResults_GetFlowForConnection(m, src2ToStoreConn, t, results);
    EXPECT_EQ(src2ToStoreResults.value().Actual_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Available_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Requested_W, 50);

    storeToMuxResults =
        ModelResults_GetFlowForConnection(m, storeToMux0Port1Conn, t, results);
    EXPECT_EQ(storeToMuxResults.value().Actual_W, 15);
    EXPECT_EQ(storeToMuxResults.value().Available_W, 15);
    EXPECT_EQ(storeToMuxResults.value().Requested_W, 40);

    muxToLoad1Results =
        ModelResults_GetFlowForConnection(m, mux0Port0ToLoad1Conn, t, results);
    EXPECT_EQ(muxToLoad1Results.value().Actual_W, 20);
    EXPECT_EQ(muxToLoad1Results.value().Available_W, 20);
    EXPECT_EQ(muxToLoad1Results.value().Requested_W, 20);

    muxToConvResults =
        ModelResults_GetFlowForConnection(m, mux0Port1ToConvConn, t, results);
    EXPECT_EQ(muxToConvResults.value().Actual_W, 15);
    EXPECT_EQ(muxToConvResults.value().Available_W, 15);
    EXPECT_EQ(muxToConvResults.value().Requested_W, 40);

    convToLoad2Results =
        ModelResults_GetFlowForConnection(m, convToLoad2Conn, t, results);
    EXPECT_EQ(convToLoad2Results.value().Actual_W, 7);
    EXPECT_EQ(convToLoad2Results.value().Available_W, 7);
    EXPECT_EQ(convToLoad2Results.value().Requested_W, 20);

    convToLoad3Results =
        ModelResults_GetFlowForConnection(m, convToLoad3Conn, t, results);
    EXPECT_EQ(convToLoad3Results.value().Actual_W, 5);
    EXPECT_EQ(convToLoad3Results.value().Available_W, 8);
    EXPECT_EQ(convToLoad3Results.value().Requested_W, 5);

    storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
    EXPECT_EQ(storeAmount.value(), 80);

    // time = 5.0
    t = 5.0;
    resultsIdx = 2;
    EXPECT_EQ(results[resultsIdx].Time, t);

    convToWasteResults =
        ModelResults_GetFlowForConnection(m, conv.WasteConnection, t, results);
    EXPECT_EQ(convToWasteResults.value().Actual_W, 0);
    EXPECT_EQ(convToWasteResults.value().Requested_W, 0);
    EXPECT_EQ(convToWasteResults.value().Available_W, 0);

    src1ToMuxResults =
        ModelResults_GetFlowForConnection(m, src1ToMux0Port0Conn, t, results);
    EXPECT_EQ(src1ToMuxResults.value().Actual_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Available_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Requested_W, 30);

    src2ToStoreResults =
        ModelResults_GetFlowForConnection(m, src2ToStoreConn, t, results);
    EXPECT_EQ(src2ToStoreResults.value().Actual_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Available_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Requested_W, 20);

    storeToMuxResults =
        ModelResults_GetFlowForConnection(m, storeToMux0Port1Conn, t, results);
    EXPECT_EQ(storeToMuxResults.value().Actual_W, 10);
    EXPECT_EQ(storeToMuxResults.value().Available_W, 15);
    EXPECT_EQ(storeToMuxResults.value().Requested_W, 10);

    muxToLoad1Results =
        ModelResults_GetFlowForConnection(m, mux0Port0ToLoad1Conn, t, results);
    EXPECT_EQ(muxToLoad1Results.value().Actual_W, 20);
    EXPECT_EQ(muxToLoad1Results.value().Available_W, 25);
    EXPECT_EQ(muxToLoad1Results.value().Requested_W, 20);

    muxToConvResults =
        ModelResults_GetFlowForConnection(m, mux0Port1ToConvConn, t, results);
    EXPECT_EQ(muxToConvResults.value().Actual_W, 10);
    EXPECT_EQ(muxToConvResults.value().Available_W, 10);
    EXPECT_EQ(muxToConvResults.value().Requested_W, 10);

    convToLoad2Results =
        ModelResults_GetFlowForConnection(m, convToLoad2Conn, t, results);
    EXPECT_EQ(convToLoad2Results.value().Actual_W, 5);
    EXPECT_EQ(convToLoad2Results.value().Available_W, 5);
    EXPECT_EQ(convToLoad2Results.value().Requested_W, 5);

    convToLoad3Results =
        ModelResults_GetFlowForConnection(m, convToLoad3Conn, t, results);
    EXPECT_EQ(convToLoad3Results.value().Actual_W, 5);
    EXPECT_EQ(convToLoad3Results.value().Available_W, 5);
    EXPECT_EQ(convToLoad3Results.value().Requested_W, 5);

    storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
    EXPECT_EQ(storeAmount.value(), 50);

    // time = 10.0
    t = 10.0;
    resultsIdx = 3;
    EXPECT_EQ(results[resultsIdx].Time, t);

    convToWasteResults =
        ModelResults_GetFlowForConnection(m, conv.WasteConnection, t, results);
    EXPECT_EQ(convToWasteResults.value().Actual_W, 3);
    EXPECT_EQ(convToWasteResults.value().Requested_W, 3);
    EXPECT_EQ(convToWasteResults.value().Available_W, 3);

    src1ToMuxResults =
        ModelResults_GetFlowForConnection(m, src1ToMux0Port0Conn, t, results);
    EXPECT_EQ(src1ToMuxResults.value().Actual_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Available_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Requested_W, 50);

    src2ToStoreResults =
        ModelResults_GetFlowForConnection(m, src2ToStoreConn, t, results);
    EXPECT_EQ(src2ToStoreResults.value().Actual_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Available_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Requested_W, 40);

    storeToMuxResults =
        ModelResults_GetFlowForConnection(m, storeToMux0Port1Conn, t, results);
    EXPECT_EQ(storeToMuxResults.value().Actual_W, 15);
    EXPECT_EQ(storeToMuxResults.value().Available_W, 15);
    EXPECT_EQ(storeToMuxResults.value().Requested_W, 30);

    muxToLoad1Results =
        ModelResults_GetFlowForConnection(m, mux0Port0ToLoad1Conn, t, results);
    EXPECT_EQ(muxToLoad1Results.value().Actual_W, 20);
    EXPECT_EQ(muxToLoad1Results.value().Available_W, 20);
    EXPECT_EQ(muxToLoad1Results.value().Requested_W, 20);

    muxToConvResults =
        ModelResults_GetFlowForConnection(m, mux0Port1ToConvConn, t, results);
    EXPECT_EQ(muxToConvResults.value().Actual_W, 15);
    EXPECT_EQ(muxToConvResults.value().Available_W, 15);
    EXPECT_EQ(muxToConvResults.value().Requested_W, 30);

    convToLoad2Results =
        ModelResults_GetFlowForConnection(m, convToLoad2Conn, t, results);
    EXPECT_EQ(convToLoad2Results.value().Actual_W, 7);
    EXPECT_EQ(convToLoad2Results.value().Available_W, 7);
    EXPECT_EQ(convToLoad2Results.value().Requested_W, 15);

    convToLoad3Results =
        ModelResults_GetFlowForConnection(m, convToLoad3Conn, t, results);
    EXPECT_EQ(convToLoad3Results.value().Actual_W, 5);
    EXPECT_EQ(convToLoad3Results.value().Available_W, 8);
    EXPECT_EQ(convToLoad3Results.value().Requested_W, 5);

    storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
    EXPECT_EQ(storeAmount.value(), 25);

    // time = 12.5
    t = 12.5;
    resultsIdx = 4;
    EXPECT_EQ(results[resultsIdx].Time, t);

    convToWasteResults =
        ModelResults_GetFlowForConnection(m, conv.WasteConnection, t, results);
    EXPECT_EQ(convToWasteResults.value().Actual_W, 0);
    EXPECT_EQ(convToWasteResults.value().Requested_W, 0);
    EXPECT_EQ(convToWasteResults.value().Available_W, 0);

    src1ToMuxResults =
        ModelResults_GetFlowForConnection(m, src1ToMux0Port0Conn, t, results);
    EXPECT_EQ(src1ToMuxResults.value().Actual_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Available_W, 20);
    EXPECT_EQ(src1ToMuxResults.value().Requested_W, 50);

    src2ToStoreResults =
        ModelResults_GetFlowForConnection(m, src2ToStoreConn, t, results);
    EXPECT_EQ(src2ToStoreResults.value().Actual_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Available_W, 5);
    EXPECT_EQ(src2ToStoreResults.value().Requested_W, 40);

    storeToMuxResults =
        ModelResults_GetFlowForConnection(m, storeToMux0Port1Conn, t, results);
    EXPECT_EQ(storeToMuxResults.value().Actual_W, 5);
    EXPECT_EQ(storeToMuxResults.value().Available_W, 5);
    EXPECT_EQ(storeToMuxResults.value().Requested_W, 30);

    muxToLoad1Results =
        ModelResults_GetFlowForConnection(m, mux0Port0ToLoad1Conn, t, results);
    EXPECT_EQ(muxToLoad1Results.value().Actual_W, 20);
    EXPECT_EQ(muxToLoad1Results.value().Available_W, 20);
    EXPECT_EQ(muxToLoad1Results.value().Requested_W, 20);

    muxToConvResults =
        ModelResults_GetFlowForConnection(m, mux0Port1ToConvConn, t, results);
    EXPECT_EQ(muxToConvResults.value().Actual_W, 5);
    EXPECT_EQ(muxToConvResults.value().Available_W, 5);
    EXPECT_EQ(muxToConvResults.value().Requested_W, 30);

    convToLoad2Results =
        ModelResults_GetFlowForConnection(m, convToLoad2Conn, t, results);
    EXPECT_EQ(convToLoad2Results.value().Actual_W, 2);
    EXPECT_EQ(convToLoad2Results.value().Available_W, 2);
    EXPECT_EQ(convToLoad2Results.value().Requested_W, 15);

    convToLoad3Results =
        ModelResults_GetFlowForConnection(m, convToLoad3Conn, t, results);
    EXPECT_EQ(convToLoad3Results.value().Actual_W, 3);
    EXPECT_EQ(convToLoad3Results.value().Available_W, 3);
    EXPECT_EQ(convToLoad3Results.value().Requested_W, 5);

    storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
    EXPECT_EQ(storeAmount.value(), 0);
}

TEST(Erin, Test11)
{
    // create a model of src->conv->load and place a reliability dist on conv
    // ensure the component goes down and comes back up (i.e., is repaired)
    Model m = {};
    m.RandFn = []() { return 0.4; };
    m.FinalTime = 50.0;
    auto srcId = Model_AddConstantSource(m, 100);
    auto loadId = Model_AddConstantLoad(m, 10);
    auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
    auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
    auto convToLoadConn = Model_AddConnection(m, convId.Id, 0, loadId, 0);
    auto fixedDistId = Model_AddFixedReliabilityDistribution(m, 10.0);
    Model_AddFailureModeToComponent(m, convId.Id, fixedDistId, fixedDistId);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 6)
        << "Expect 6 times: 0.0, 10.0, 20.0, 30.0, 40.0, 50.0";

    double t = 0.0;
    auto srcToConvResults =
        ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
    EXPECT_EQ(srcToConvResults.value().Actual_W, 20)
        << "src -> conv actual should be 20";
    EXPECT_EQ(srcToConvResults.value().Requested_W, 20)
        << "src -> conv requested should be 20";
    EXPECT_EQ(srcToConvResults.value().Available_W, 100)
        << "src -> conv available should be 100";

    auto convToLoadResults =
        ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
    EXPECT_EQ(convToLoadResults.value().Actual_W, 10)
        << "conv -> load actual should be 10";
    EXPECT_EQ(convToLoadResults.value().Requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(convToLoadResults.value().Available_W, 50)
        << "conv -> load available should be 50";

    auto convToWasteResults = ModelResults_GetFlowForConnection(
        m, convId.WasteConnection, t, results
    );
    EXPECT_EQ(convToWasteResults.value().Actual_W, 10)
        << "conv -> waste actual should be 10";
    EXPECT_EQ(convToWasteResults.value().Requested_W, 10)
        << "conv -> waste requested should be 10";
    EXPECT_EQ(convToWasteResults.value().Available_W, 10)
        << "conv -> waste available should be 10";

    // time = 10.0, failed
    t = 10.0;
    srcToConvResults =
        ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
    EXPECT_EQ(srcToConvResults.value().Actual_W, 0)
        << "src -> conv actual should be 0";
    EXPECT_EQ(srcToConvResults.value().Requested_W, 0)
        << "src -> conv requested should be 0";
    EXPECT_EQ(srcToConvResults.value().Available_W, 100)
        << "src -> conv available should be 100";

    convToLoadResults =
        ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
    EXPECT_EQ(convToLoadResults.value().Actual_W, 0)
        << "conv -> load actual should be 0";
    EXPECT_EQ(convToLoadResults.value().Requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(convToLoadResults.value().Available_W, 0)
        << "conv -> load available should be 0";

    convToWasteResults = ModelResults_GetFlowForConnection(
        m, convId.WasteConnection, t, results
    );
    EXPECT_EQ(convToWasteResults.value().Actual_W, 0)
        << "conv -> waste actual should be 0";
    EXPECT_EQ(convToWasteResults.value().Requested_W, 0)
        << "conv -> waste requested should be 0";
    EXPECT_EQ(convToWasteResults.value().Available_W, 0)
        << "conv -> waste available should be 0";

    // time = 20.0, fixed/restored
    t = 20.0;
    srcToConvResults =
        ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
    EXPECT_EQ(srcToConvResults.value().Actual_W, 20)
        << "src -> conv actual should be 20";
    EXPECT_EQ(srcToConvResults.value().Requested_W, 20)
        << "src -> conv requested should be 20";
    EXPECT_EQ(srcToConvResults.value().Available_W, 100)
        << "src -> conv available should be 100";

    convToLoadResults =
        ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
    EXPECT_EQ(convToLoadResults.value().Actual_W, 10)
        << "conv -> load actual should be 10";
    EXPECT_EQ(convToLoadResults.value().Requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(convToLoadResults.value().Available_W, 50)
        << "conv -> load available should be 0";

    convToWasteResults = ModelResults_GetFlowForConnection(
        m, convId.WasteConnection, t, results
    );
    EXPECT_EQ(convToWasteResults.value().Actual_W, 10)
        << "conv -> waste actual should be 10";
    EXPECT_EQ(convToWasteResults.value().Requested_W, 10)
        << "conv -> waste requested should be 10";
    EXPECT_EQ(convToWasteResults.value().Available_W, 10)
        << "conv -> waste available should be 10";

    // time = 30.0, failed
    t = 30.0;
    srcToConvResults =
        ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
    EXPECT_EQ(srcToConvResults.value().Actual_W, 0)
        << "src -> conv actual should be 0";
    EXPECT_EQ(srcToConvResults.value().Requested_W, 0)
        << "src -> conv requested should be 0";
    EXPECT_EQ(srcToConvResults.value().Available_W, 100)
        << "src -> conv available should be 100";

    convToLoadResults =
        ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
    EXPECT_EQ(convToLoadResults.value().Actual_W, 0)
        << "conv -> load actual should be 0";
    EXPECT_EQ(convToLoadResults.value().Requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(convToLoadResults.value().Available_W, 0)
        << "conv -> load available should be 0";

    convToWasteResults = ModelResults_GetFlowForConnection(
        m, convId.WasteConnection, t, results
    );
    EXPECT_EQ(convToWasteResults.value().Actual_W, 0)
        << "conv -> waste actual should be 0";
    EXPECT_EQ(convToWasteResults.value().Requested_W, 0)
        << "conv -> waste requested should be 0";
    EXPECT_EQ(convToWasteResults.value().Available_W, 0)
        << "conv -> waste available should be 0";

    // time = 40.0, fixed/restored
    t = 40.0;
    srcToConvResults =
        ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
    EXPECT_EQ(srcToConvResults.value().Actual_W, 20)
        << "src -> conv actual should be 20";
    EXPECT_EQ(srcToConvResults.value().Requested_W, 20)
        << "src -> conv requested should be 20";
    EXPECT_EQ(srcToConvResults.value().Available_W, 100)
        << "src -> conv available should be 100";

    convToLoadResults =
        ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
    EXPECT_EQ(convToLoadResults.value().Actual_W, 10)
        << "conv -> load actual should be 10";
    EXPECT_EQ(convToLoadResults.value().Requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(convToLoadResults.value().Available_W, 50)
        << "conv -> load available should be 0";

    convToWasteResults = ModelResults_GetFlowForConnection(
        m, convId.WasteConnection, t, results
    );
    EXPECT_EQ(convToWasteResults.value().Actual_W, 10)
        << "conv -> waste actual should be 10";
    EXPECT_EQ(convToWasteResults.value().Requested_W, 10)
        << "conv -> waste requested should be 10";
    EXPECT_EQ(convToWasteResults.value().Available_W, 10)
        << "conv -> waste available should be 10";

    // time = 50.0, failed
    t = 50.0;
    srcToConvResults =
        ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
    EXPECT_EQ(srcToConvResults.value().Actual_W, 0)
        << "src -> conv actual should be 0";
    EXPECT_EQ(srcToConvResults.value().Requested_W, 0)
        << "src -> conv requested should be 0";
    EXPECT_EQ(srcToConvResults.value().Available_W, 100)
        << "src -> conv available should be 100";

    convToLoadResults =
        ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
    EXPECT_EQ(convToLoadResults.value().Actual_W, 0)
        << "conv -> load actual should be 0";
    EXPECT_EQ(convToLoadResults.value().Requested_W, 10)
        << "conv -> load requested should be 10";
    EXPECT_EQ(convToLoadResults.value().Available_W, 0)
        << "conv -> load available should be 0";

    convToWasteResults = ModelResults_GetFlowForConnection(
        m, convId.WasteConnection, t, results
    );
    EXPECT_EQ(convToWasteResults.value().Actual_W, 0)
        << "conv -> waste actual should be 0";
    EXPECT_EQ(convToWasteResults.value().Requested_W, 0)
        << "conv -> waste requested should be 0";
    EXPECT_EQ(convToWasteResults.value().Available_W, 0)
        << "conv -> waste available should be 0";
}

TEST(Erin, Test12)
{
    // Add a schedule-based source (availability, uncontrolled source)
    // NOTE: it would be good to have a waste connection so that the component
    // always "spills" (ullage) when not all available is used.
    Model m = {};
    m.RandFn = []() { return 0.4; };
    m.FinalTime = 20.0;
    std::vector<TimeAndAmount> sourceAvailability{};
    sourceAvailability.reserve(5);
    sourceAvailability.push_back(TimeAndAmount{0, 10});
    sourceAvailability.push_back(TimeAndAmount{10, 8});
    sourceAvailability.push_back(TimeAndAmount{20, 12});
    auto srcId = Model_AddScheduleBasedSource(m, sourceAvailability);
    auto loadId = Model_AddConstantLoad(m, 10);
    auto srcToLoadConn = Model_AddConnection(m, srcId.Id, 0, loadId, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 3) << "should have 3 time results";
    EXPECT_EQ(results[0].Time, 0.0);
    EXPECT_EQ(results[1].Time, 10.0);
    EXPECT_EQ(results[2].Time, 20.0);
    double t = 0.0;
    auto srcToLoadResults =
        ModelResults_GetFlowForConnection(m, srcToLoadConn, t, results);
    EXPECT_EQ(srcToLoadResults.value().Actual_W, 10);
    EXPECT_EQ(srcToLoadResults.value().Available_W, 10);
    EXPECT_EQ(srcToLoadResults.value().Requested_W, 10);
    auto srcToWasteResults =
        ModelResults_GetFlowForConnection(m, srcId.WasteConnection, t, results);
    EXPECT_EQ(srcToWasteResults.value().Actual_W, 0);
    EXPECT_EQ(srcToWasteResults.value().Available_W, 0);
    EXPECT_EQ(srcToWasteResults.value().Requested_W, 0);
    t = 10.0;
    srcToLoadResults =
        ModelResults_GetFlowForConnection(m, srcToLoadConn, t, results);
    EXPECT_EQ(srcToLoadResults.value().Actual_W, 8);
    EXPECT_EQ(srcToLoadResults.value().Available_W, 8);
    EXPECT_EQ(srcToLoadResults.value().Requested_W, 10);
    srcToWasteResults =
        ModelResults_GetFlowForConnection(m, srcId.WasteConnection, t, results);
    EXPECT_EQ(srcToWasteResults.value().Actual_W, 0);
    EXPECT_EQ(srcToWasteResults.value().Available_W, 0);
    EXPECT_EQ(srcToWasteResults.value().Requested_W, 0);
    t = 20.0;
    srcToLoadResults =
        ModelResults_GetFlowForConnection(m, srcToLoadConn, t, results);
    EXPECT_EQ(srcToLoadResults.value().Actual_W, 10);
    EXPECT_EQ(srcToLoadResults.value().Available_W, 12);
    EXPECT_EQ(srcToLoadResults.value().Requested_W, 10);
    srcToWasteResults =
        ModelResults_GetFlowForConnection(m, srcId.WasteConnection, t, results);
    EXPECT_EQ(srcToWasteResults.value().Actual_W, 2);
    EXPECT_EQ(srcToWasteResults.value().Available_W, 2);
    EXPECT_EQ(srcToWasteResults.value().Requested_W, 2);
}

TEST(Erin, Test13)
{
    auto kW_as_W = [](double p_kW) -> uint32_t
    { return static_cast<uint32_t>(std::round(p_kW * 1000.0)); };
    auto hours_as_seconds = [](double h) -> double { return h * 3600.0; };
    auto kWh_as_J = [](double kWh) -> double { return kWh * 3'600'000.0; };
    // SIMULATION INFO and INITIALIZATION
    Model m = {};
    m.RandFn = []() { return 0.4; };
    m.FinalTime = hours_as_seconds(48.0);
    // LOADS
    std::vector<TimeAndAmount> elecLoad{};
    elecLoad.reserve(49);
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(0.0), kW_as_W(187.47)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(1.0), kW_as_W(146.271)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(2.0), kW_as_W(137.308)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(3.0), kW_as_W(170.276)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(4.0), kW_as_W(139.068)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(5.0), kW_as_W(171.944)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(6.0), kW_as_W(140.051)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(7.0), kW_as_W(173.406)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(8.0), kW_as_W(127.54)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(9.0), kW_as_W(135.751)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(10.0), kW_as_W(95.195)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(11.0), kW_as_W(107.644)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(12.0), kW_as_W(81.227)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(13.0), kW_as_W(98.928)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(14.0), kW_as_W(80.134)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(15.0), kW_as_W(97.222)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(16.0), kW_as_W(81.049)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(17.0), kW_as_W(114.29)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(18.0), kW_as_W(102.652)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(19.0), kW_as_W(125.672)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(20.0), kW_as_W(105.254)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(21.0), kW_as_W(125.047)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(22.0), kW_as_W(104.824)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(23.0), kW_as_W(126.488)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(24.0), kW_as_W(107.094)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(25.0), kW_as_W(135.559)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(26.0), kW_as_W(115.588)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(27.0), kW_as_W(137.494)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(28.0), kW_as_W(115.386)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(29.0), kW_as_W(133.837)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(30.0), kW_as_W(113.812)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(31.0), kW_as_W(343.795)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(32.0), kW_as_W(284.121)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(33.0), kW_as_W(295.434)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(34.0), kW_as_W(264.364)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(35.0), kW_as_W(247.33)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(36.0), kW_as_W(235.89)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(37.0), kW_as_W(233.43)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(38.0), kW_as_W(220.77)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(39.0), kW_as_W(213.825)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(40.0), kW_as_W(210.726)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(41.0), kW_as_W(223.706)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(42.0), kW_as_W(219.193)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(43.0), kW_as_W(186.31)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(44.0), kW_as_W(185.658)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(45.0), kW_as_W(173.137)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(46.0), kW_as_W(172.236)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(47.0), kW_as_W(47.676)});
    elecLoad.push_back(TimeAndAmount{hours_as_seconds(48.0), kW_as_W(48.952)});
    std::vector<TimeAndAmount> heatLoad{};
    heatLoad.reserve(49);
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(0.0), kW_as_W(29.60017807)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(1.0), kW_as_W(16.70505099)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(2.0), kW_as_W(16.99812206)}
    );
    heatLoad.push_back(TimeAndAmount{hours_as_seconds(3.0), kW_as_W(23.4456856)}
    );
    heatLoad.push_back(TimeAndAmount{hours_as_seconds(4.0), kW_as_W(17.5842642)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(5.0), kW_as_W(23.73875667)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(6.0), kW_as_W(17.87733527)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(7.0), kW_as_W(24.03182774)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(8.0), kW_as_W(17.87733527)}
    );
    heatLoad.push_back(TimeAndAmount{hours_as_seconds(9.0), kW_as_W(23.4456856)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(10.0), kW_as_W(16.41197992)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(11.0), kW_as_W(18.75654848)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(12.0), kW_as_W(14.36048243)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(13.0), kW_as_W(16.11890885)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(14.0), kW_as_W(10.55055852)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(15.0), kW_as_W(13.77434029)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(16.0), kW_as_W(9.37827424)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(17.0), kW_as_W(13.18819815)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(18.0), kW_as_W(9.37827424)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(19.0), kW_as_W(13.48126922)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(20.0), kW_as_W(9.67134531)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(21.0), kW_as_W(12.30898494)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(22.0), kW_as_W(10.55055852)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(23.0), kW_as_W(13.48126922)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(24.0), kW_as_W(9.67134531)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(25.0), kW_as_W(13.48126922)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(26.0), kW_as_W(12.30898494)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(27.0), kW_as_W(14.06741136)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(28.0), kW_as_W(12.30898494)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(29.0), kW_as_W(13.48126922)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(30.0), kW_as_W(10.84362959)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(31.0), kW_as_W(4.10299498)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(32.0), kW_as_W(45.71908692)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(33.0), kW_as_W(38.97845231)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(34.0), kW_as_W(33.11703091)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(35.0), kW_as_W(26.96253844)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(36.0), kW_as_W(24.32489881)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(37.0), kW_as_W(22.85954346)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(38.0), kW_as_W(26.66946737)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(39.0), kW_as_W(29.89324914)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(40.0), kW_as_W(26.66946737)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(41.0), kW_as_W(24.32489881)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(42.0), kW_as_W(27.25560951)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(43.0), kW_as_W(26.66946737)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(44.0), kW_as_W(22.85954346)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(45.0), kW_as_W(21.10111704)}
    );
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(46.0), kW_as_W(18.46347741)}
    );
    heatLoad.push_back(TimeAndAmount{hours_as_seconds(47.0), kW_as_W(0.0)});
    heatLoad.push_back(
        TimeAndAmount{hours_as_seconds(48.0), kW_as_W(3.22378177)}
    );
    std::vector<TimeAndAmount> pvAvail{};
    pvAvail.reserve(49);
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(0.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(1.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(2.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(3.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(4.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(5.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(6.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(7.0), kW_as_W(14.36)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(8.0), kW_as_W(671.759)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(9.0), kW_as_W(1265.933)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(10.0), kW_as_W(1583.21)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(11.0), kW_as_W(1833.686)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(12.0), kW_as_W(1922.872)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(13.0), kW_as_W(1749.437)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(14.0), kW_as_W(994.715)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(15.0), kW_as_W(468.411)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(16.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(17.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(18.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(19.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(20.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(21.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(22.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(23.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(24.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(25.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(26.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(27.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(28.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(29.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(30.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(31.0), kW_as_W(10.591)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(32.0), kW_as_W(693.539)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(33.0), kW_as_W(1191.017)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(34.0), kW_as_W(1584.868)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(35.0), kW_as_W(1820.692)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(36.0), kW_as_W(1952.869)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(37.0), kW_as_W(1799.1)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(38.0), kW_as_W(1067.225)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(39.0), kW_as_W(396.023)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(40.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(41.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(42.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(43.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(44.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(45.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(46.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(47.0), kW_as_W(0.0)});
    pvAvail.push_back(TimeAndAmount{hours_as_seconds(48.0), kW_as_W(0.0)});
    // COMPONENTS
    auto pvArrayId = Model_AddScheduleBasedSource(m, pvAvail);
    auto elecUtilId = Model_AddConstantSource(m, kW_as_W(10.0));
    auto batteryId = Model_AddStore(
        m,
        kWh_as_J(100.0),
        kW_as_W(10.0),
        kW_as_W(1'000.0),
        kWh_as_J(80.0),
        kWh_as_J(100.0)
    );
    auto elecSourceMuxId = Model_AddMux(m, 2, 1);
    auto elecSupplyMuxId = Model_AddMux(m, 2, 2);
    auto ngUtilId =
        Model_AddConstantSource(m, std::numeric_limits<uint32_t>::max());
    auto ngSourceMuxId = Model_AddMux(m, 1, 2);
    auto ngToElecConvId = Model_AddConstantEfficiencyConverter(m, 42, 100);
    auto elecHeatPumpConvId = Model_AddConstantEfficiencyConverter(m, 35, 10);
    auto ngHeaterConvId = Model_AddConstantEfficiencyConverter(m, 98, 100);
    auto heatingSupplyMuxId = Model_AddMux(m, 3, 1);
    auto elecLoadId = Model_AddScheduleBasedLoad(m, elecLoad);
    auto heatLoadId = Model_AddScheduleBasedLoad(m, heatLoad);
    // NETWORK / CONNECTIONS
    // - electricity
    auto pvToEmuxConn =
        Model_AddConnection(m, pvArrayId.Id, 0, elecSourceMuxId, 0);
    auto eutilToEmuxConn =
        Model_AddConnection(m, elecUtilId, 0, elecSourceMuxId, 1);
    auto emuxToBatteryConn =
        Model_AddConnection(m, elecSourceMuxId, 0, batteryId, 0);
    auto batteryToEsupplyConn =
        Model_AddConnection(m, batteryId, 0, elecSupplyMuxId, 0);
    auto ngGenToEsupplyConn =
        Model_AddConnection(m, ngToElecConvId.Id, 0, elecSupplyMuxId, 1);
    auto esupplyToLoadConn =
        Model_AddConnection(m, elecSupplyMuxId, 0, elecLoadId, 0);
    auto esupplyToHeatPumpConn =
        Model_AddConnection(m, elecSupplyMuxId, 1, elecHeatPumpConvId.Id, 0);
    // - natural gas
    auto ngGridToNgMuxConn =
        Model_AddConnection(m, ngUtilId, 0, ngSourceMuxId, 0);
    auto ngMuxToNgGenConn =
        Model_AddConnection(m, ngSourceMuxId, 0, ngToElecConvId.Id, 0);
    auto ngMuxToNgHeaterConn =
        Model_AddConnection(m, ngSourceMuxId, 1, ngHeaterConvId.Id, 0);
    // - heating
    auto ngGenLossToHeatMuxConn =
        Model_AddConnection(m, ngToElecConvId.Id, 1, heatingSupplyMuxId, 0);
    auto ngHeaterToHeatMuxConn =
        Model_AddConnection(m, ngHeaterConvId.Id, 0, heatingSupplyMuxId, 1);
    auto heatPumpToHeatMuxConn =
        Model_AddConnection(m, elecHeatPumpConvId.Id, 0, heatingSupplyMuxId, 2);
    auto heatMuxToLoadConn =
        Model_AddConnection(m, heatingSupplyMuxId, 0, heatLoadId, 0);
    // SIMULATE
    auto results = Simulate(m, false);
}

TEST(Erin, Test14)
{
    Model m = {};
    m.RandFn = []() { return 0.4; };
    m.FinalTime = 4.0;
    std::vector<TimeAndAmount> availablePower{
        {0.0, 50},
        {2.0, 10},
    };
    auto src01Id = Model_AddConstantSource(m, 50);
    auto src02Id = Model_AddScheduleBasedSource(m, availablePower);
    auto muxId = Model_AddMux(m, 2, 1);
    auto loadId = Model_AddConstantLoad(m, 100);
    auto src1ToMuxConn = Model_AddConnection(m, src01Id, 0, muxId, 0);
    auto src2ToMuxConn = Model_AddConnection(m, src02Id.Id, 0, muxId, 1);
    auto muxToLoadConn = Model_AddConnection(m, muxId, 0, loadId, 0);
    auto results = Simulate(m, false);
    // TODO: add tests/checks
}

TEST(Erin, Test15)
{
    Model m = {};
    m.RandFn = []() { return 0.4; };
    m.FinalTime = 2.0;
    std::vector<TimeAndAmount> loadOne{
        {0.0, 50},
        {2.0, 10},
    };
    auto src01Id = Model_AddConstantSource(m, 1'000);
    auto src02Id = Model_AddConstantSource(m, 1'000);
    auto convId = Model_AddConstantEfficiencyConverter(m, 1, 4);
    auto muxId = Model_AddMux(m, 2, 1);
    auto load01Id = Model_AddScheduleBasedLoad(m, loadOne);
    auto load02Id = Model_AddConstantLoad(m, 100);
    auto src1ToConvConn = Model_AddConnection(m, src01Id, 0, convId.Id, 0);
    auto convToLoadConn = Model_AddConnection(m, convId.Id, 0, load01Id, 0);
    auto convLossToMuxConn = Model_AddConnection(m, convId.Id, 1, muxId, 0);
    auto src2ToMuxConn = Model_AddConnection(m, src02Id, 0, muxId, 1);
    auto muxToLoadConn = Model_AddConnection(m, muxId, 0, load02Id, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 2);

    double t = 0.0;
    auto src1ToConvResults =
        ModelResults_GetFlowForConnection(m, src1ToConvConn, t, results);
    EXPECT_TRUE(src1ToConvResults.has_value());
    EXPECT_EQ(src1ToConvResults.value().Actual_W, 200);
    EXPECT_EQ(src1ToConvResults.value().Requested_W, 200);
    EXPECT_EQ(src1ToConvResults.value().Available_W, 1'000);

    auto convToLoadResults =
        ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
    EXPECT_TRUE(convToLoadResults.has_value());
    EXPECT_EQ(convToLoadResults.value().Actual_W, 50);
    EXPECT_EQ(convToLoadResults.value().Requested_W, 50);
    EXPECT_EQ(convToLoadResults.value().Available_W, 250);

    auto convLossToMuxResults =
        ModelResults_GetFlowForConnection(m, convLossToMuxConn, t, results);
    EXPECT_TRUE(convLossToMuxResults.has_value());
    EXPECT_EQ(convLossToMuxResults.value().Actual_W, 100);
    EXPECT_EQ(convLossToMuxResults.value().Requested_W, 100);
    EXPECT_EQ(convLossToMuxResults.value().Available_W, 150);

    auto src2ToMuxResults =
        ModelResults_GetFlowForConnection(m, src2ToMuxConn, t, results);
    EXPECT_TRUE(src2ToMuxResults.has_value());
    EXPECT_EQ(src2ToMuxResults.value().Actual_W, 0);
    EXPECT_EQ(src2ToMuxResults.value().Requested_W, 0);
    EXPECT_EQ(src2ToMuxResults.value().Available_W, 1'000);

    auto muxToLoadResults =
        ModelResults_GetFlowForConnection(m, muxToLoadConn, t, results);
    EXPECT_TRUE(muxToLoadResults.has_value());
    EXPECT_EQ(muxToLoadResults.value().Actual_W, 100);
    EXPECT_EQ(muxToLoadResults.value().Requested_W, 100);
    EXPECT_EQ(muxToLoadResults.value().Available_W, 1'150);

    t = 2.0;
    src1ToConvResults =
        ModelResults_GetFlowForConnection(m, src1ToConvConn, t, results);
    EXPECT_TRUE(src1ToConvResults.has_value());
    EXPECT_EQ(src1ToConvResults.value().Actual_W, 40);
    EXPECT_EQ(src1ToConvResults.value().Requested_W, 40);
    EXPECT_EQ(src1ToConvResults.value().Available_W, 1'000);

    convToLoadResults =
        ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
    EXPECT_TRUE(convToLoadResults.has_value());
    EXPECT_EQ(convToLoadResults.value().Actual_W, 10);
    EXPECT_EQ(convToLoadResults.value().Requested_W, 10);
    EXPECT_EQ(convToLoadResults.value().Available_W, 250);

    convLossToMuxResults =
        ModelResults_GetFlowForConnection(m, convLossToMuxConn, t, results);
    EXPECT_TRUE(convLossToMuxResults.has_value());
    EXPECT_EQ(convLossToMuxResults.value().Actual_W, 30);
    EXPECT_EQ(convLossToMuxResults.value().Requested_W, 100);
    EXPECT_EQ(convLossToMuxResults.value().Available_W, 30);

    muxToLoadResults =
        ModelResults_GetFlowForConnection(m, muxToLoadConn, t, results);
    EXPECT_TRUE(muxToLoadResults.has_value());
    EXPECT_EQ(muxToLoadResults.value().Actual_W, 100);
    EXPECT_EQ(muxToLoadResults.value().Requested_W, 100);
    EXPECT_EQ(muxToLoadResults.value().Available_W, 1'030);

    src2ToMuxResults =
        ModelResults_GetFlowForConnection(m, src2ToMuxConn, t, results);
    EXPECT_TRUE(src2ToMuxResults.has_value());
    EXPECT_EQ(src2ToMuxResults.value().Actual_W, 70);
    EXPECT_EQ(src2ToMuxResults.value().Requested_W, 70);
    EXPECT_EQ(src2ToMuxResults.value().Available_W, 1'000);
}

TEST(Erin, Test16)
{
    Model m = {};
    m.RandFn = []() { return 0.4; };
    m.FinalTime = 2.0;
    size_t srcId = Model_AddConstantSource(m, 100);
    size_t loadId = Model_AddConstantLoad(m, 50);
    size_t passId = Model_AddPassThrough(m);
    auto srcToPassConn = Model_AddConnection(m, srcId, 0, passId, 0);
    auto passToLoadConn = Model_AddConnection(m, passId, 0, loadId, 0);
    auto results = Simulate(m, false);
    EXPECT_EQ(results.size(), 2);

    double t = 0.0;
    auto srcToPassResults =
        ModelResults_GetFlowForConnection(m, srcToPassConn, t, results);
    EXPECT_TRUE(srcToPassResults.has_value());
    EXPECT_EQ(srcToPassResults.value().Actual_W, 50);
    EXPECT_EQ(srcToPassResults.value().Requested_W, 50);
    EXPECT_EQ(srcToPassResults.value().Available_W, 100);

    auto passToLoadResults =
        ModelResults_GetFlowForConnection(m, passToLoadConn, t, results);
    EXPECT_TRUE(passToLoadResults.has_value());
    EXPECT_EQ(passToLoadResults.value().Actual_W, 50);
    EXPECT_EQ(passToLoadResults.value().Requested_W, 50);
    EXPECT_EQ(passToLoadResults.value().Available_W, 100);
}

TEST(Erin, Test17)
{
    std::vector<TimeState> a{{0.0, true}, {10.0, false, {1}}, {100.0, true}};
    std::vector<TimeState> b{
        {0.0, true}, {40.0, false, {2}}, {90.0, true}, {150.0, false, {2}}
    };
    std::vector<TimeState> expected{
        {0.0, true},
        {10.0, false, {1}},
        {40.0, false, {1, 2}},
        {90.0, false, {1}},
        {100.0, true},
        {150.0, false, {2}}
    };
    std::vector<TimeState> actual = TimeState_Combine(a, b);
    EXPECT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], actual[i]);
    }
}

TEST(Erin, Test18)
{
    std::vector<TimeState> input{
        {0.0, true},
        {10.0, false, {1}},
        {40.0, false, {1, 2}},
        {90.0, false, {1}},
        {100.0, true},
        {150.0, false, {2}}
    };
    std::vector<TimeState> expected{
        {50.0, false, {1, 2}},
        {90.0, false, {1}},
        {100.0, true},
    };
    std::vector<TimeState> actual = TimeState_Clip(input, 50.0, 120.0, false);
    EXPECT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], actual[i]);
    }
    std::vector<TimeState> expected2{
        {0.0, false, {1, 2}},
        {40.0, false, {1}},
        {50.0, true},
    };
    std::vector<TimeState> actual2 = TimeState_Clip(input, 50.0, 120.0, true);
    EXPECT_EQ(expected2.size(), actual2.size());
    for (size_t i = 0; i < expected2.size(); ++i)
    {
        EXPECT_EQ(expected2[i], actual2[i]);
    }
}

TEST(Erin, Test19)
{
    std::vector<TimeState> A{
        {0.0, false, {}, {0}},
        {100.0, true},
    };
    std::vector<TimeState> B{
        {0.0, true},
        {120.0, false, {0}},
        {180.0, true},
    };
    std::vector<TimeState> C{
        {0.0, true},
        {60.0, false, {1}},
        {140.0, true},
    };
    std::vector<TimeState> expected{
        {0.0, false, {}, {0}},
        {60.0, false, {1}, {0}},
        {100.0, false, {1}},
        {120.0, false, {0, 1}},
        {140.0, false, {0}},
        {180.0, true},
    };
    std::vector<TimeState> relSch;
    relSch = TimeState_Combine(relSch, A);
    relSch = TimeState_Combine(relSch, B);
    std::vector<TimeState> actual = TimeState_Combine(relSch, C);
    EXPECT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], actual[i]);
    }
}

TEST(Erin, Test20)
{
    std::vector<TimeState> input{
        {5.0, false},
        {7.0, true},
        {12.0, false},
        {14.0, true},
        {19.0, false},
        {21.0, true},
    };
    std::vector<TimeState> expected{
        {5.0, false},
        {7.0, true},
    };
    std::vector<TimeState> actual = TimeState_Clip(input, 0.0, 10.0, true);
    EXPECT_EQ(expected.size(), actual.size());
}

TEST(Erin, Test21)
{
    std::vector<TimeState> input{
        {0.0, true},
        {10.0, false, {1}},
        {20.0, true},
        {100.0, false, {}, {1}},
        {180.0, true},
    };
    std::map<size_t, size_t> countByFailModeId;
    std::map<size_t, size_t> countByFragModeId;
    std::map<size_t, double> timeByFailModeId;
    std::map<size_t, double> timeByFragModeId;
    TimeState_CountAndTimeFailureEvents(
        input,
        200.0,
        countByFailModeId,
        countByFragModeId,
        timeByFailModeId,
        timeByFragModeId
    );
    std::map<size_t, size_t> expectedCountByFailModeId{{1, 1}};
    std::map<size_t, size_t> expectedCountByFragModeId{{1, 1}};
    std::map<size_t, double> expectedTimeByFailModeId{{1, 10.0}};
    std::map<size_t, double> expectedTimeByFragModeId{{1, 80.0}};
    EXPECT_EQ(expectedCountByFailModeId.size(), countByFailModeId.size());
    EXPECT_EQ(expectedCountByFragModeId.size(), countByFragModeId.size());
    EXPECT_EQ(expectedTimeByFailModeId.size(), timeByFailModeId.size());
    EXPECT_EQ(expectedTimeByFragModeId.size(), timeByFragModeId.size());
    for (auto const& p : expectedCountByFailModeId)
    {
        EXPECT_TRUE(countByFailModeId.contains(p.first));
        EXPECT_EQ(p.second, countByFailModeId[p.first]);
    }
    for (auto const& p : expectedCountByFragModeId)
    {
        EXPECT_TRUE(countByFragModeId.contains(p.first));
        EXPECT_EQ(p.second, countByFragModeId[p.first]);
    }
    for (auto const& p : expectedTimeByFailModeId)
    {
        EXPECT_TRUE(timeByFailModeId.contains(p.first));
        EXPECT_EQ(p.second, timeByFailModeId[p.first]);
    }
    for (auto const& p : expectedTimeByFragModeId)
    {
        EXPECT_TRUE(timeByFragModeId.contains(p.first));
        EXPECT_EQ(p.second, timeByFragModeId[p.first]);
    }
    countByFailModeId.clear();
    countByFragModeId.clear();
    timeByFailModeId.clear();
    timeByFragModeId.clear();
    input = {
        {10.0, false, {1}},
        {20.0, true},
        {100.0, false, {}, {1}},
    };
    TimeState_CountAndTimeFailureEvents(
        input,
        180.0,
        countByFailModeId,
        countByFragModeId,
        timeByFailModeId,
        timeByFragModeId
    );
    EXPECT_EQ(expectedCountByFailModeId.size(), countByFailModeId.size());
    EXPECT_EQ(expectedCountByFragModeId.size(), countByFragModeId.size());
    EXPECT_EQ(expectedTimeByFailModeId.size(), timeByFailModeId.size());
    EXPECT_EQ(expectedTimeByFragModeId.size(), timeByFragModeId.size());
    for (auto const& p : expectedCountByFailModeId)
    {
        EXPECT_TRUE(countByFailModeId.contains(p.first));
        EXPECT_EQ(p.second, countByFailModeId[p.first]);
    }
    for (auto const& p : expectedCountByFragModeId)
    {
        EXPECT_TRUE(countByFragModeId.contains(p.first));
        EXPECT_EQ(p.second, countByFragModeId[p.first]);
    }
    for (auto const& p : expectedTimeByFailModeId)
    {
        EXPECT_TRUE(timeByFailModeId.contains(p.first));
        EXPECT_EQ(p.second, timeByFailModeId[p.first]);
    }
    for (auto const& p : expectedTimeByFragModeId)
    {
        EXPECT_TRUE(timeByFragModeId.contains(p.first));
        EXPECT_EQ(p.second, timeByFragModeId[p.first]);
    }
}

TEST(Erin, Test22)
{
    TabularFragilityCurve tfc{};
    tfc.VulnerabilityId = 0;
    tfc.Intensities = std::vector<double>{0.0, 1.0, 4.0, 6.0, 9.0, 10.0};
    tfc.FailureFractions = std::vector<double>{0.0, 0.3, 0.7, 0.8, 0.95, 1.0};
    double level = 7.0;
    double result = TabularFragilityCurve_GetFailureFraction(tfc, level);
    EXPECT_EQ(result, 0.85);
}
