#include "erin_next/erin_next.h"
#include "erin_next/erin_next_timestate.h"
#include <iomanip>
#include <chrono>
#include <limits>

using namespace erin_next;

static void
PrintBanner(bool doPrint, std::string name) {
	if (doPrint) {
		std::cout << "[Test " << std::right << std::setw(3)
			<< (name + ":") << std::endl;
	}
}

static void
PrintPass(bool doPrint, std::string name) {
	std::string preamble = doPrint
		? "  ... "
		: "[Test ";
	std::cout << preamble << std::right << std::setw(3)
		<< (name + "]") << " :: PASSED" << std::endl;
}

static double
Round(double n, unsigned int places=2) {
	double mult = std::pow(10.0, (double)places);
	return std::round(n * mult) / mult;
}

static void
Test1(bool print) {
	PrintBanner(print, "1");
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToLoadConn = Model_AddConnection(m, srcId, 0, loadId, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1
		&& "output must have a size of 1"));
	assert((results[0].Time == 0.0
		&& "time must equal 0.0"));
	assert((results[0].Flows.size() == 1
		&& "size of flows must equal 1"));
	auto srcToLoadResult =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, 0.0, results);
	assert((srcToLoadResult.has_value()
		&& "connection result should have a value"));
	assert((srcToLoadResult.value().Actual_W == 10
		&& "actual value must equal 10"));
	assert((srcToLoadResult.value().Available_W == 100
		&& "available must equal 100"));
	assert((srcToLoadResult.value().Requested_W == 10
		&& "requested must equal 10"));
	PrintPass(print, "1");
}

static void
Test2(bool print) {
	PrintBanner(print, "2");
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
	auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
	auto convToLoadConn = Model_AddConnection(m, convId.Id, 0, loadId, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1
		&& "output must have a size of 1"));
	assert((results[0].Time == 0.0
		&& "time must equal 0.0"));
	assert((results[0].Flows.size() == 3
		&& "size of flows must equal 3"));
	auto srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value()
		&& "source to converter must have results"));
	assert((srcToConvResults.value().Requested_W == 20
		&& "requested must equal 20"));
	assert((srcToConvResults.value().Actual_W == 20
		&& "actual value must equal 20"));
	assert((srcToConvResults.value().Available_W == 100
		&& "available must equal 100"));
	auto convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, 0.0, results);
	assert((convToLoadResults.has_value()
		&& "converter to load must have results"));
	assert((convToLoadResults.value().Requested_W == 10
		&& "requested must equal 10"));
	assert((convToLoadResults.value().Actual_W == 10
		&& "actual value must equal 10"));
	assert((convToLoadResults.value().Available_W == 50
		&& "available must equal 50"));
	auto convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value()
		&& "converter to waste must have results"));
	assert((convToWasteResults.value().Requested_W == 10
		&& "requested must equal 10"));
	assert((convToWasteResults.value().Actual_W == 10
		&& "actual value must equal 10"));
	assert((convToWasteResults.value().Available_W == 10
		&& "available must equal 10"));
	PrintPass(print, "2");
}

static void
Test3(bool print) {
	PrintBanner(print, "3");
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto load1Id = Model_AddConstantLoad(m, 10);
	auto load2Id = Model_AddConstantLoad(m, 2);
	auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
	auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
	auto convToLoad1Conn = Model_AddConnection(m, convId.Id, 0, load1Id, 0);
	auto convToLoad2Conn = Model_AddConnection(m, convId.Id, 1, load2Id, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 4 && "size of flows must equal 4"));
	auto srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value()
		&& "source to converter must have results"));
	assert((srcToConvResults.value().Requested_W == 20
		&& "requested must equal 20"));
	assert((srcToConvResults.value().Actual_W == 20
		&& "actual value must equal 20"));
	assert((srcToConvResults.value().Available_W == 100
		&& "available must equal 100"));
	auto convToLoad1Results =
		ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
	assert((convToLoad1Results.has_value()
		&& "converter to load1 must have results"));
	assert((convToLoad1Results.value().Requested_W == 10
		&& "requested must equal 10"));
	assert((convToLoad1Results.value().Actual_W == 10
		&& "actual value must equal 10"));
	assert((convToLoad1Results.value().Available_W == 50
		&& "available must equal 50"));
	auto convToLoad2Results =
		ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
	assert((convToLoad2Results.has_value()
		&& "conv to load2 must have results"));
	assert((convToLoad2Results.value().Requested_W == 2
		&& "requested must equal 2"));
	assert((convToLoad2Results.value().Actual_W == 2
		&& "actual value must equal 2"));
	assert((convToLoad2Results.value().Available_W == 10
		&& "available must equal 10"));
	auto convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value()
		&& "conv to waste must have results"));
	assert((convToWasteResults.value().Requested_W == 8
		&& "requested must equal 8"));
	assert((convToWasteResults.value().Actual_W == 8
		&& "actual value must equal 8"));
	assert((convToWasteResults.value().Available_W == 8
		&& "available must equal 8"));
	PrintPass(print, "3");
}

static void
Test3A(bool print) {
	PrintBanner(print, "3a");
	Model m{};
	auto srcId = Model_AddConstantSource(m, 100);
	auto load1Id = Model_AddConstantLoad(m, 10);
	auto load2Id = Model_AddConstantLoad(m, 2);
	auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
	auto convToLoad2Conn = Model_AddConnection(m, convId.Id, 1, load2Id, 0);
	auto convToLoad1Conn = Model_AddConnection(m, convId.Id, 0, load1Id, 0);
	auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 4 && "size of flows must equal 4"));
	auto srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value()
		&& "source to converter must have results"));
	assert((srcToConvResults.value().Requested_W == 20
		&& "requested must equal 20"));
	assert((srcToConvResults.value().Actual_W == 20
		&& "actual value must equal 20"));
	assert((srcToConvResults.value().Available_W == 100
		&& "available must equal 100"));
	auto convToLoad1Results =
		ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
	assert((convToLoad1Results.has_value()
		&& "converter to load1 must have results"));
	assert((convToLoad1Results.value().Requested_W == 10
		&& "requested must equal 10"));
	assert((convToLoad1Results.value().Actual_W == 10
		&& "actual value must equal 10"));
	assert((convToLoad1Results.value().Available_W == 50
		&& "available must equal 50"));
	auto convToLoad2Results =
		ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
	assert((convToLoad2Results.has_value()
		&& "conv to load2 must have results"));
	assert((convToLoad2Results.value().Requested_W == 2
		&& "requested must equal 2"));
	assert((convToLoad2Results.value().Actual_W == 2
		&& "actual value must equal 2"));
	assert((convToLoad2Results.value().Available_W == 10
		&& "available must equal 10"));
	auto convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value()
		&& "conv to waste must have results"));
	assert((convToWasteResults.value().Requested_W == 8
		&& "requested must equal 8"));
	assert((convToWasteResults.value().Actual_W == 8
		&& "actual value must equal 8"));
	assert((convToWasteResults.value().Available_W == 8
		&& "available must equal 8"));
	PrintPass(print, "3a");
}

static void
Test4(bool print) {
	PrintBanner(print, "4");
	std::vector<TimeAndAmount> timesAndLoads = {};
	timesAndLoads.push_back({ 0.0, 10 });
	timesAndLoads.push_back({ 3600.0, 200 });
	Model m = {};
	m.FinalTime = 3600.0;
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddScheduleBasedLoad(m, timesAndLoads);
	auto srcToLoadConn = Model_AddConnection(m, srcId, 0, loadId, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 2 && "output must have a size of 2"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 1 && "size of flows[0] must equal 1"));
	auto srcToLoadResults_0 =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, 0.0, results);
	assert((srcToLoadResults_0.has_value()
		&& "source to load must have results at time=0.0"));
	assert((srcToLoadResults_0.value().Requested_W == 10
		&& "requested must equal 10"));
	assert((srcToLoadResults_0.value().Actual_W == 10
		&& "actual value must equal 10"));
	assert((srcToLoadResults_0.value().Available_W == 100
		&& "available must equal 100"));
	assert((results[1].Time == 3600.0
		&& "time must equal 3600.0"));
	assert((results[1].Flows.size() == 1
		&& "size of flows[1] must equal 1"));
	auto srcToLoadResults_3600 =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, 3600.0, results);
	assert((srcToLoadResults_3600.has_value()
		&& "source to load must have results at time=3600.0"));
	assert((srcToLoadResults_3600.value().Requested_W == 200
		&& "requested must equal 200"));
	assert((srcToLoadResults_3600.value().Actual_W == 100
		&& "actual value must equal 100"));
	assert((srcToLoadResults_3600.value().Available_W == 100
		&& "available must equal 100"));
	PrintPass(print, "4");
}

static void
Test5(bool print) {
	PrintBanner(print, "5");
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
	auto conv1ToLoad1Conn =
		Model_AddConnection(m, conv1.Id, 0, load1Id, 0);
	auto conv1ToConv2Conn =
		Model_AddConnection(m, conv1.Id, 1, conv2.Id, 0);
	auto conv2ToLoad2Conn =
		Model_AddConnection(m, conv2.Id, 0, load2Id, 0);
	auto conv2ToConv3Conn =
		Model_AddConnection(m, conv2.Id, 1, conv3.Id, 0);
	auto conv3ToLoad3Conn =
		Model_AddConnection(m, conv3.Id, 0, load3Id, 0);
	auto results = Simulate(m, print);
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
	assert((srcToConv1Results.value().Actual_W == 40
		&& "src to conv1 should flow 40"));
	assert((conv1ToLoad1Results.value().Actual_W == 10
		&& "conv1 to load1 should flow 10"));
	assert((conv1ToConv2Results.value().Actual_W == 28
		&& "conv1 to conv2 should flow 28"));
	assert((conv2ToLoad2Results.value().Actual_W == 7
		&& "conv1 to conv2 should flow 7"));
	assert((conv2ToConv3Results.value().Actual_W == 20
		&& "conv2 to conv3 should flow 21"));
	assert((conv3ToLoad3Results.value().Actual_W == 5
		&& "conv3 to load3 should flow 5"));
	PrintPass(print, "5");
}

static void
Test6(bool doPrint) {
	PrintBanner(doPrint, "6");
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
	auto results = Simulate(m, doPrint);
	auto src1ToMuxResults =
		ModelResults_GetFlowForConnection(m, src1ToMuxConn, 0.0, results);
	assert((src1ToMuxResults.value().Actual_W == 10
		&& "src1 -> mux expected actual flow of 10"));
	auto src2ToMuxResults =
		ModelResults_GetFlowForConnection(m, src2ToMuxConn, 0.0, results);
	assert((src2ToMuxResults.value().Actual_W == 50
		&& "src2 -> mux expected actual flow of 50"));
	auto muxToLoad1Results =
		ModelResults_GetFlowForConnection(m, muxToLoad1Conn, 0.0, results);
	assert((muxToLoad1Results.value().Actual_W == 10
		&& "mux -> load1 expected actual flow of 10"));
	auto muxToLoad2Results =
		ModelResults_GetFlowForConnection(m, muxToLoad2Conn, 0.0, results);
	assert((muxToLoad2Results.value().Actual_W == 50
		&& "mux -> load2 expected actual flow of 50"));
	PrintPass(doPrint, "6");
}

static void
Test7(bool doPrint) {
	PrintBanner(doPrint, "7");
	Model m = {};
	m.FinalTime = 10.0;
	auto srcId = Model_AddConstantSource(m, 0);
	auto storeId = Model_AddStore(m, 100, 10, 10, 0, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToStoreConn = Model_AddConnection(m, srcId, 0, storeId, 0);
	auto storeToLoadConn = Model_AddConnection(m, storeId, 0, loadId, 0);
	auto results = Simulate(m, doPrint);
	auto srcToStoreResults =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 0.0, results);
	assert(srcToStoreResults.value().Actual_W == 0
		&& "src to store should be providing 0");
	assert(srcToStoreResults.value().Requested_W == 10
		&& "src to store request is 10");
	assert(srcToStoreResults.value().Available_W == 0
		&& "src to store available is 0");
	auto storeToLoadResults =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 0.0, results);
	assert(storeToLoadResults.has_value()
		&& "should have results for store to load connection");
	assert(storeToLoadResults.value().Actual_W == 10
		&& "store to load should be providing 10");
	assert(storeToLoadResults.value().Requested_W == 10
		&& "store to load should be requesting 10");
	assert(storeToLoadResults.value().Available_W == 10
		&& "store to load available should be 10");
	assert(results.size() == 2
		&& "there should be two time events in results");
	assert(results[1].Time > 10.0 - 1e-6
		&& results[1].Time < 10.0 + 1e-6 && "time should be 10");
	auto srcToStoreResultsAt10 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 10.0, results);
	assert(srcToStoreResultsAt10.value().Actual_W == 0
		&& "src to store should be providing 0");
	assert(srcToStoreResultsAt10.value().Requested_W == 20
		&& "src to store request is 20");
	assert(srcToStoreResultsAt10.value().Available_W == 0
		&& "src to store available is 0");
	auto storeToLoadResultsAt10 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 10.0, results);
	assert(storeToLoadResultsAt10.has_value()
		&& "should have results for store to load connection");
	assert(storeToLoadResultsAt10.value().Actual_W == 0
		&& "store to load should be providing 0");
	assert(storeToLoadResultsAt10.value().Requested_W == 10
		&& "store to load should be requesting 10");
	assert(storeToLoadResultsAt10.value().Available_W == 0
		&& "store to load available should be 0");
	PrintPass(doPrint, "7");
}

static void
Test8(bool doPrint) {
	PrintBanner(doPrint, "8");
	Model m = {};
	m.FinalTime = 20.0;
	auto srcId = Model_AddConstantSource(m, 5);
	auto storeId = Model_AddStore(m, 100, 10, 10, 0, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToStoreConn = Model_AddConnection(m, srcId, 0, storeId, 0);
	auto storeToLoadConn = Model_AddConnection(m, storeId, 0, loadId, 0);
	auto results = Simulate(m, doPrint);
	auto srcToStoreResults =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 0.0, results);
	assert(srcToStoreResults.value().Actual_W == 5
		&& "src to store should be providing 5");
	assert(srcToStoreResults.value().Requested_W == 10
		&& "src to store request is 10");
	assert(srcToStoreResults.value().Available_W == 5
		&& "src to store available is 5");
	auto storeToLoadResults =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 0.0, results);
	assert(storeToLoadResults.has_value()
		&& "should have results for store to load connection");
	assert(storeToLoadResults.value().Actual_W == 10
		&& "store to load should be providing 10");
	assert(storeToLoadResults.value().Requested_W == 10
		&& "store to load should be requesting 10");
	assert(storeToLoadResults.value().Available_W == 15
		&& "store to load available should be 15");
	assert(results.size() == 2
		&& "there should be two time events in results");
	assert(results[1].Time > 20.0 - 1e-6
		&& results[1].Time < 20.0 + 1e-6 && "time should be 20");
	auto srcToStoreResultsAt20 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 20.0, results);
	assert(srcToStoreResultsAt20.value().Actual_W == 5
		&& "src to store should be providing 5");
	assert(srcToStoreResultsAt20.value().Requested_W == 20
		&& "src to store request is 20");
	assert(srcToStoreResultsAt20.value().Available_W == 5
		&& "src to store available is 5");
	auto storeToLoadResultsAt20 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 20.0, results);
	assert(storeToLoadResultsAt20.has_value()
		&& "should have results for store to load connection");
	assert(storeToLoadResultsAt20.value().Actual_W == 5
		&& "store to load should be providing 5");
	assert(storeToLoadResultsAt20.value().Requested_W == 10
		&& "store to load should be requesting 10");
	assert(storeToLoadResultsAt20.value().Available_W == 5
		&& "store to load available should be 5");
	PrintPass(doPrint, "8");
}

static void
Test9(bool doPrint) {
	PrintBanner(doPrint, "9");
	std::vector<TimeAndAmount> timesAndLoads = {};
	timesAndLoads.push_back({  0.0, 20 });
	timesAndLoads.push_back({  5.0,  5 });
	timesAndLoads.push_back({ 10.0, 15 });
	Model m = {};
	m.FinalTime = 25.0;
	auto srcId = Model_AddConstantSource(m, 10);
	auto storeId = Model_AddStore(m, 100, 10, 10, 80, 100);
	auto loadId = Model_AddScheduleBasedLoad(m, timesAndLoads);
	auto srcToStoreConn = Model_AddConnection(m, srcId, 0, storeId, 0);
	auto storeToLoadConn = Model_AddConnection(m, storeId, 0, loadId, 0);
	auto results = Simulate(m, doPrint);
	assert(results.size() == 5 && "expected 5 time steps");
	assert(Round(results[0].Time) == 0.0 && "expect first time is 0.0");
	assert(Round(results[1].Time) == 2.0 && "expect second time is 2.0");
	assert(Round(results[2].Time) == 5.0 && "expect third time is 5.0");
	assert(Round(results[3].Time) == 10.0 && "expect fourth time is 10.0");
	assert(Round(results[4].Time) == 25.0 && "expect fifth time is 25.0");
	auto srcToStoreResultsAt0 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 0.0, results);
	auto storeToLoadResultsAt0 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 0.0, results);
	auto storeAmount0 = ModelResults_GetStoreState(m, storeId, 0.0, results);
	assert(srcToStoreResultsAt0.value().Actual_W == 10);
	assert(srcToStoreResultsAt0.value().Requested_W == 20);
	assert(srcToStoreResultsAt0.value().Available_W == 10);
	assert(storeToLoadResultsAt0.value().Actual_W == 20);
	assert(storeToLoadResultsAt0.value().Requested_W == 20);
	assert(storeToLoadResultsAt0.value().Available_W == 20);
	assert(storeAmount0.value() == 100);
	auto srcToStoreResultsAt2 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 2.0, results);
	auto storeToLoadResultsAt2 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 2.0, results);
	auto storeAmount2 =
		ModelResults_GetStoreState(m, storeId, 2.0, results);
	assert(srcToStoreResultsAt2.value().Actual_W == 10);
	assert(srcToStoreResultsAt2.value().Requested_W == 30);
	assert(srcToStoreResultsAt2.value().Available_W == 10);
	assert(storeToLoadResultsAt2.value().Actual_W == 20);
	assert(storeToLoadResultsAt2.value().Requested_W == 20);
	assert(storeToLoadResultsAt2.value().Available_W == 20);
	assert(storeAmount2.value() == 80);
	auto srcToStoreResultsAt5 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 5.0, results);
	auto storeToLoadResultsAt5 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 5.0, results);
	auto storeAmount5 = ModelResults_GetStoreState(m, storeId, 5.0, results);
	assert(srcToStoreResultsAt5.value().Actual_W == 10);
	assert(srcToStoreResultsAt5.value().Requested_W == 15);
	assert(srcToStoreResultsAt5.value().Available_W == 10);
	assert(storeToLoadResultsAt5.value().Actual_W == 5);
	assert(storeToLoadResultsAt5.value().Requested_W == 5);
	assert(storeToLoadResultsAt5.value().Available_W == 20);
	assert(storeAmount5.value() == 50);
	auto srcToStoreResultsAt10 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 10.0, results);
	auto storeToLoadResultsAt10 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 10.0, results);
	auto storeAmount10 =
		ModelResults_GetStoreState(m, storeId, 10.0, results);
	assert(srcToStoreResultsAt10.value().Actual_W == 10);
	assert(srcToStoreResultsAt10.value().Requested_W == 25);
	assert(srcToStoreResultsAt10.value().Available_W == 10);
	assert(storeToLoadResultsAt10.value().Actual_W == 15);
	assert(storeToLoadResultsAt10.value().Requested_W == 15);
	assert(storeToLoadResultsAt10.value().Available_W == 20);
	assert(storeAmount10.value() == 75);
	auto srcToStoreResultsAt25 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 25.0, results);
	auto storeToLoadResultsAt25 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 25.0, results);
	auto storeAmount25 =
		ModelResults_GetStoreState(m, storeId, 25.0, results);
	assert(srcToStoreResultsAt25.value().Actual_W == 10);
	assert(srcToStoreResultsAt25.value().Requested_W == 25);
	assert(srcToStoreResultsAt25.value().Available_W == 10);
	assert(storeToLoadResultsAt25.value().Actual_W == 10);
	assert(storeToLoadResultsAt25.value().Requested_W == 15);
	assert(storeToLoadResultsAt25.value().Available_W == 10);
	assert(storeAmount25.value() == 0);
	PrintPass(doPrint, "9");
}

static void
Test10(bool doPrint) {
	PrintBanner(doPrint, "10");
	std::vector<TimeAndAmount> timesAndLoads = {};
	timesAndLoads.push_back({ 0.0, 20 });
	timesAndLoads.push_back({ 5.0,  5 });
	timesAndLoads.push_back({ 10.0, 15 });
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
	auto results = Simulate(m, doPrint);
	assert(results.size() == 5 && "expect 5 events");
	// time = 0.0
	double t = 0.0;
	size_t resultsIdx = 0;
	assert(results[resultsIdx].Time == t);
	auto convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 3);
	assert(convToWasteResults.value().Requested_W == 3);
	assert(convToWasteResults.value().Available_W == 3);
	auto src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, 0.0, results);
	assert(src1ToMuxResults.value().Actual_W == 20);
	assert(src1ToMuxResults.value().Available_W == 20);
	assert(src1ToMuxResults.value().Requested_W == 60);
	auto src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, 0.0, results);
	assert(src2ToStoreResults.value().Actual_W == 5);
	assert(src2ToStoreResults.value().Available_W == 5);
	assert(src2ToStoreResults.value().Requested_W == 40);
	auto storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, 0.0, results);
	assert(storeToMuxResults.value().Actual_W == 15);
	assert(storeToMuxResults.value().Available_W == 15);
	assert(storeToMuxResults.value().Requested_W == 40);
	auto muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, 0.0, results);
	assert(muxToLoad1Results.value().Actual_W == 20);
	assert(muxToLoad1Results.value().Available_W == 20);
	assert(muxToLoad1Results.value().Requested_W == 20);
	auto muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, 0.0, results);
	assert(muxToConvResults.value().Actual_W == 15);
	assert(muxToConvResults.value().Available_W == 15);
	assert(muxToConvResults.value().Requested_W == 40);
	auto convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, 0.0, results);
	assert(convToLoad2Results.value().Actual_W == 7);
	assert(convToLoad2Results.value().Available_W == 7);
	assert(convToLoad2Results.value().Requested_W == 20);
	auto convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, 0.0, results);
	assert(convToLoad3Results.value().Actual_W == 5);
	assert(convToLoad3Results.value().Available_W == 8);
	assert(convToLoad3Results.value().Requested_W == 5);
	auto storeAmount = ModelResults_GetStoreState(m, storeId, 0.0, results);
	assert(storeAmount.value() == 100);
	// time = 2.0
	t = 2.0;
	resultsIdx = 1;
	assert(results[resultsIdx].Time == t);;
	convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 3);
	assert(convToWasteResults.value().Requested_W == 3);
	assert(convToWasteResults.value().Available_W == 3);
	src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, t, results);
	assert(src1ToMuxResults.value().Actual_W == 20);
	assert(src1ToMuxResults.value().Available_W == 20);
	assert(src1ToMuxResults.value().Requested_W == 60);
	src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, t, results);
	assert(src2ToStoreResults.value().Actual_W == 5);
	assert(src2ToStoreResults.value().Available_W == 5);
	assert(src2ToStoreResults.value().Requested_W == 50);
	storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, t, results);
	assert(storeToMuxResults.value().Actual_W == 15);
	assert(storeToMuxResults.value().Available_W == 15);
	assert(storeToMuxResults.value().Requested_W == 40);
	muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, t, results);
	assert(muxToLoad1Results.value().Actual_W == 20);
	assert(muxToLoad1Results.value().Available_W == 20);
	assert(muxToLoad1Results.value().Requested_W == 20);
	muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, t, results);
	assert(muxToConvResults.value().Actual_W == 15);
	assert(muxToConvResults.value().Available_W == 15);
	assert(muxToConvResults.value().Requested_W == 40);
	convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, t, results);
	assert(convToLoad2Results.value().Actual_W == 7);
	assert(convToLoad2Results.value().Available_W == 7);
	assert(convToLoad2Results.value().Requested_W == 20);
	convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, t, results);
	assert(convToLoad3Results.value().Actual_W == 5);
	assert(convToLoad3Results.value().Available_W == 8);
	assert(convToLoad3Results.value().Requested_W == 5);
	storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
	assert(storeAmount.value() == 80);
	// time = 5.0
	t = 5.0;
	resultsIdx = 2;
	assert(results[resultsIdx].Time == t);
	convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 0);
	assert(convToWasteResults.value().Requested_W == 0);
	assert(convToWasteResults.value().Available_W == 0);
	src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, t, results);
	assert(src1ToMuxResults.value().Actual_W == 20);
	assert(src1ToMuxResults.value().Available_W == 20);
	assert(src1ToMuxResults.value().Requested_W == 30);
	src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, t, results);
	assert(src2ToStoreResults.value().Actual_W == 5);
	assert(src2ToStoreResults.value().Available_W == 5);
	assert(src2ToStoreResults.value().Requested_W == 20);
	storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, t, results);
	assert(storeToMuxResults.value().Actual_W == 10);
	assert(storeToMuxResults.value().Available_W == 15);
	assert(storeToMuxResults.value().Requested_W == 10);
	muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, t, results);
	assert(muxToLoad1Results.value().Actual_W == 20);
	assert(muxToLoad1Results.value().Available_W == 25);
	assert(muxToLoad1Results.value().Requested_W == 20);
	muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, t, results);
	assert(muxToConvResults.value().Actual_W == 10);
	assert(muxToConvResults.value().Available_W == 10);
	assert(muxToConvResults.value().Requested_W == 10);
	convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, t, results);
	assert(convToLoad2Results.value().Actual_W == 5);
	assert(convToLoad2Results.value().Available_W == 5);
	assert(convToLoad2Results.value().Requested_W == 5);
	convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, t, results);
	assert(convToLoad3Results.value().Actual_W == 5);
	assert(convToLoad3Results.value().Available_W == 5);
	assert(convToLoad3Results.value().Requested_W == 5);
	storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
	assert(storeAmount.value() == 50);
	// time = 10.0
	t = 10.0;
	resultsIdx = 3;
	assert(results[resultsIdx].Time == t);
	convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 3);
	assert(convToWasteResults.value().Requested_W == 3);
	assert(convToWasteResults.value().Available_W == 3);
	src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, t, results);
	assert(src1ToMuxResults.value().Actual_W == 20);
	assert(src1ToMuxResults.value().Available_W == 20);
	assert(src1ToMuxResults.value().Requested_W == 50);
	src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, t, results);
	assert(src2ToStoreResults.value().Actual_W == 5);
	assert(src2ToStoreResults.value().Available_W == 5);
	assert(src2ToStoreResults.value().Requested_W == 40);
	storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, t, results);
	assert(storeToMuxResults.value().Actual_W == 15);
	assert(storeToMuxResults.value().Available_W == 15);
	assert(storeToMuxResults.value().Requested_W == 30);
	muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, t, results);
	assert(muxToLoad1Results.value().Actual_W == 20);
	assert(muxToLoad1Results.value().Available_W == 20);
	assert(muxToLoad1Results.value().Requested_W == 20);
	muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, t, results);
	assert(muxToConvResults.value().Actual_W == 15);
	assert(muxToConvResults.value().Available_W == 15);
	assert(muxToConvResults.value().Requested_W == 30);
	convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, t, results);
	assert(convToLoad2Results.value().Actual_W == 7);
	assert(convToLoad2Results.value().Available_W == 7);
	assert(convToLoad2Results.value().Requested_W == 15);
	convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, t, results);
	assert(convToLoad3Results.value().Actual_W == 5);
	assert(convToLoad3Results.value().Available_W == 8);
	assert(convToLoad3Results.value().Requested_W == 5);
	storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
	assert(storeAmount.value() == 25);
	// time = 12.5
	t = 12.5;
	resultsIdx = 4;
	assert(results[resultsIdx].Time == t);
	convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 0);
	assert(convToWasteResults.value().Requested_W == 0);
	assert(convToWasteResults.value().Available_W == 0);
	src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, t, results);
	assert(src1ToMuxResults.value().Actual_W == 20);
	assert(src1ToMuxResults.value().Available_W == 20);
	assert(src1ToMuxResults.value().Requested_W == 50);
	src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, t, results);
	assert(src2ToStoreResults.value().Actual_W == 5);
	assert(src2ToStoreResults.value().Available_W == 5);
	assert(src2ToStoreResults.value().Requested_W == 40);
	storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, t, results);
	assert(storeToMuxResults.value().Actual_W == 5);
	assert(storeToMuxResults.value().Available_W == 5);
	assert(storeToMuxResults.value().Requested_W == 30);
	muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, t, results);
	assert(muxToLoad1Results.value().Actual_W == 20);
	assert(muxToLoad1Results.value().Available_W == 20);
	assert(muxToLoad1Results.value().Requested_W == 20);
	muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, t, results);
	assert(muxToConvResults.value().Actual_W == 5);
	assert(muxToConvResults.value().Available_W == 5);
	assert(muxToConvResults.value().Requested_W == 30);
	convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, t, results);
	assert(convToLoad2Results.value().Actual_W == 2);
	assert(convToLoad2Results.value().Available_W == 2);
	assert(convToLoad2Results.value().Requested_W == 15);
	convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, t, results);
	assert(convToLoad3Results.value().Actual_W == 3);
	assert(convToLoad3Results.value().Available_W == 3);
	assert(convToLoad3Results.value().Requested_W == 5);
	storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
	assert(storeAmount.value() == 0);
	PrintPass(doPrint, "10");
}

void
Test11(bool doPrint)
{
	PrintBanner(doPrint, "11");
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
	auto results = Simulate(m, doPrint);
	assert(results.size() == 6
		&& "Expect 6 times: 0.0, 10.0, 20.0, 30.0, 40.0, 50.0");
	double t = 0.0;
	auto srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual_W == 20
		&& "src -> conv actual should be 20");
	assert(srcToConvResults.value().Requested_W == 20
		&& "src -> conv requested should be 20");
	assert(srcToConvResults.value().Available_W == 100
		&& "src -> conv available should be 100");
	auto convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual_W == 10
		&& "conv -> load actual should be 10");
	assert(convToLoadResults.value().Requested_W == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available_W == 50
		&& "conv -> load available should be 50");
	auto convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 10
		&& "conv -> waste actual should be 10");
	assert(convToWasteResults.value().Requested_W == 10
		&& "conv -> waste requested should be 10");
	assert(convToWasteResults.value().Available_W == 10
		&& "conv -> waste available should be 10");
	// time = 10.0, failed
	t = 10.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual_W == 0
		&& "src -> conv actual should be 0");
	assert(srcToConvResults.value().Requested_W == 0
		&& "src -> conv requested should be 0");
	assert(srcToConvResults.value().Available_W == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual_W == 0
		&& "conv -> load actual should be 0");
	assert(convToLoadResults.value().Requested_W == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available_W == 0
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 0
		&& "conv -> waste actual should be 0");
	assert(convToWasteResults.value().Requested_W == 0
		&& "conv -> waste requested should be 0");
	assert(convToWasteResults.value().Available_W == 0
		&& "conv -> waste available should be 0");
	// time = 20.0, fixed/restored
	t = 20.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual_W == 20
		&& "src -> conv actual should be 20");
	assert(srcToConvResults.value().Requested_W == 20
		&& "src -> conv requested should be 20");
	assert(srcToConvResults.value().Available_W == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual_W == 10
		&& "conv -> load actual should be 10");
	assert(convToLoadResults.value().Requested_W == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available_W == 50
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 10
		&& "conv -> waste actual should be 10");
	assert(convToWasteResults.value().Requested_W == 10
		&& "conv -> waste requested should be 10");
	assert(convToWasteResults.value().Available_W == 10
		&& "conv -> waste available should be 10");
	// time = 30.0, failed
	t = 30.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual_W == 0
		&& "src -> conv actual should be 0");
	assert(srcToConvResults.value().Requested_W == 0
		&& "src -> conv requested should be 0");
	assert(srcToConvResults.value().Available_W == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual_W == 0
		&& "conv -> load actual should be 0");
	assert(convToLoadResults.value().Requested_W == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available_W == 0
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 0
		&& "conv -> waste actual should be 0");
	assert(convToWasteResults.value().Requested_W == 0
		&& "conv -> waste requested should be 0");
	assert(convToWasteResults.value().Available_W == 0
		&& "conv -> waste available should be 0");
	// time = 40.0, fixed/restored
	t = 40.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual_W == 20
		&& "src -> conv actual should be 20");
	assert(srcToConvResults.value().Requested_W == 20
		&& "src -> conv requested should be 20");
	assert(srcToConvResults.value().Available_W == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual_W == 10
		&& "conv -> load actual should be 10");
	assert(convToLoadResults.value().Requested_W == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available_W == 50
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 10
		&& "conv -> waste actual should be 10");
	assert(convToWasteResults.value().Requested_W == 10
		&& "conv -> waste requested should be 10");
	assert(convToWasteResults.value().Available_W == 10
		&& "conv -> waste available should be 10");
	// time = 50.0, failed
	t = 50.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual_W == 0
		&& "src -> conv actual should be 0");
	assert(srcToConvResults.value().Requested_W == 0
		&& "src -> conv requested should be 0");
	assert(srcToConvResults.value().Available_W == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual_W == 0
		&& "conv -> load actual should be 0");
	assert(convToLoadResults.value().Requested_W == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available_W == 0
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual_W == 0
		&& "conv -> waste actual should be 0");
	assert(convToWasteResults.value().Requested_W == 0
		&& "conv -> waste requested should be 0");
	assert(convToWasteResults.value().Available_W == 0
		&& "conv -> waste available should be 0");
	PrintPass(doPrint, "11");
}

void
Test12(bool doPrint)
{
	PrintBanner(doPrint, "12");
	// Add a schedule-based source (availability, uncontrolled source)
	// NOTE: it would be good to have a waste connection so that the component
	// always "spills" (ullage) when not all available is used.
	Model m = {};
	m.RandFn = []() { return 0.4; };
	m.FinalTime = 20.0;
	std::vector<TimeAndAmount> sourceAvailability{};
	sourceAvailability.reserve(5);
	sourceAvailability.emplace_back(0, 10);
	sourceAvailability.emplace_back(10, 8);
	sourceAvailability.emplace_back(20, 12);
	auto srcId = Model_AddScheduleBasedSource(m, sourceAvailability);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToLoadConn = Model_AddConnection(m, srcId.Id, 0, loadId, 0);
	auto results = Simulate(m, doPrint);
	assert(results.size() == 3 && "should have 3 time results");
	assert(results[0].Time == 0.0);
	assert(results[1].Time == 10.0);
	assert(results[2].Time == 20.0);
	double t = 0.0;
	auto srcToLoadResults =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, t, results);
	assert(srcToLoadResults.value().Actual_W == 10);
	assert(srcToLoadResults.value().Available_W == 10);
	assert(srcToLoadResults.value().Requested_W == 10);
	auto srcToWasteResults =
		ModelResults_GetFlowForConnection(m, srcId.WasteConnection, t, results);
	assert(srcToWasteResults.value().Actual_W == 0);
	assert(srcToWasteResults.value().Available_W == 0);
	assert(srcToWasteResults.value().Requested_W == 0);
	t = 10.0;
	srcToLoadResults =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, t, results);
	assert(srcToLoadResults.value().Actual_W == 8);
	assert(srcToLoadResults.value().Available_W == 8);
	assert(srcToLoadResults.value().Requested_W == 10);
	srcToWasteResults =
		ModelResults_GetFlowForConnection(m, srcId.WasteConnection, t, results);
	assert(srcToWasteResults.value().Actual_W == 0);
	assert(srcToWasteResults.value().Available_W == 0);
	assert(srcToWasteResults.value().Requested_W == 0);
	t = 20.0;
	srcToLoadResults =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, t, results);
	assert(srcToLoadResults.value().Actual_W == 10);
	assert(srcToLoadResults.value().Available_W == 12);
	assert(srcToLoadResults.value().Requested_W == 10);
	srcToWasteResults =
		ModelResults_GetFlowForConnection(m, srcId.WasteConnection, t, results);
	assert(srcToWasteResults.value().Actual_W == 2);
	assert(srcToWasteResults.value().Available_W == 2);
	assert(srcToWasteResults.value().Requested_W == 2);
	PrintPass(doPrint, "12");
}

void
Test13(bool doPrint)
{
	auto kW_as_W = [](double p_kW) -> uint32_t {
		return static_cast<uint32_t>(std::round(p_kW * 1000.0));
	};
	auto hours_as_seconds = [](double h) -> double {
		return h * 3600.0;
	};
	auto kWh_as_J = [](double kWh) -> double {
		return kWh * 3'600'000.0;
	};
	PrintBanner(doPrint, "13");
	// SIMULATION INFO and INITIALIZATION
	Model m = {};
	m.RandFn = []() { return 0.4; };
	m.FinalTime = hours_as_seconds(48.0);
	// LOADS
	std::vector<TimeAndAmount> elecLoad{};
	elecLoad.reserve(49);
	elecLoad.emplace_back(hours_as_seconds(0.0), kW_as_W(187.47));
	elecLoad.emplace_back(hours_as_seconds(1.0), kW_as_W(146.271));
	elecLoad.emplace_back(hours_as_seconds(2.0), kW_as_W(137.308));
	elecLoad.emplace_back(hours_as_seconds(3.0), kW_as_W(170.276));
	elecLoad.emplace_back(hours_as_seconds(4.0), kW_as_W(139.068));
	elecLoad.emplace_back(hours_as_seconds(5.0), kW_as_W(171.944));
	elecLoad.emplace_back(hours_as_seconds(6.0), kW_as_W(140.051));
	elecLoad.emplace_back(hours_as_seconds(7.0), kW_as_W(173.406));
	elecLoad.emplace_back(hours_as_seconds(8.0), kW_as_W(127.54));
	elecLoad.emplace_back(hours_as_seconds(9.0), kW_as_W(135.751));
	elecLoad.emplace_back(hours_as_seconds(10.0), kW_as_W(95.195));
	elecLoad.emplace_back(hours_as_seconds(11.0), kW_as_W(107.644));
	elecLoad.emplace_back(hours_as_seconds(12.0), kW_as_W(81.227));
	elecLoad.emplace_back(hours_as_seconds(13.0), kW_as_W(98.928));
	elecLoad.emplace_back(hours_as_seconds(14.0), kW_as_W(80.134));
	elecLoad.emplace_back(hours_as_seconds(15.0), kW_as_W(97.222));
	elecLoad.emplace_back(hours_as_seconds(16.0), kW_as_W(81.049));
	elecLoad.emplace_back(hours_as_seconds(17.0), kW_as_W(114.29));
	elecLoad.emplace_back(hours_as_seconds(18.0), kW_as_W(102.652));
	elecLoad.emplace_back(hours_as_seconds(19.0), kW_as_W(125.672));
	elecLoad.emplace_back(hours_as_seconds(20.0), kW_as_W(105.254));
	elecLoad.emplace_back(hours_as_seconds(21.0), kW_as_W(125.047));
	elecLoad.emplace_back(hours_as_seconds(22.0), kW_as_W(104.824));
	elecLoad.emplace_back(hours_as_seconds(23.0), kW_as_W(126.488));
	elecLoad.emplace_back(hours_as_seconds(24.0), kW_as_W(107.094));
	elecLoad.emplace_back(hours_as_seconds(25.0), kW_as_W(135.559));
	elecLoad.emplace_back(hours_as_seconds(26.0), kW_as_W(115.588));
	elecLoad.emplace_back(hours_as_seconds(27.0), kW_as_W(137.494));
	elecLoad.emplace_back(hours_as_seconds(28.0), kW_as_W(115.386));
	elecLoad.emplace_back(hours_as_seconds(29.0), kW_as_W(133.837));
	elecLoad.emplace_back(hours_as_seconds(30.0), kW_as_W(113.812));
	elecLoad.emplace_back(hours_as_seconds(31.0), kW_as_W(343.795));
	elecLoad.emplace_back(hours_as_seconds(32.0), kW_as_W(284.121));
	elecLoad.emplace_back(hours_as_seconds(33.0), kW_as_W(295.434));
	elecLoad.emplace_back(hours_as_seconds(34.0), kW_as_W(264.364));
	elecLoad.emplace_back(hours_as_seconds(35.0), kW_as_W(247.33));
	elecLoad.emplace_back(hours_as_seconds(36.0), kW_as_W(235.89));
	elecLoad.emplace_back(hours_as_seconds(37.0), kW_as_W(233.43));
	elecLoad.emplace_back(hours_as_seconds(38.0), kW_as_W(220.77));
	elecLoad.emplace_back(hours_as_seconds(39.0), kW_as_W(213.825));
	elecLoad.emplace_back(hours_as_seconds(40.0), kW_as_W(210.726));
	elecLoad.emplace_back(hours_as_seconds(41.0), kW_as_W(223.706));
	elecLoad.emplace_back(hours_as_seconds(42.0), kW_as_W(219.193));
	elecLoad.emplace_back(hours_as_seconds(43.0), kW_as_W(186.31));
	elecLoad.emplace_back(hours_as_seconds(44.0), kW_as_W(185.658));
	elecLoad.emplace_back(hours_as_seconds(45.0), kW_as_W(173.137));
	elecLoad.emplace_back(hours_as_seconds(46.0), kW_as_W(172.236));
	elecLoad.emplace_back(hours_as_seconds(47.0), kW_as_W(47.676));
	elecLoad.emplace_back(hours_as_seconds(48.0), kW_as_W(48.952));
	std::vector<TimeAndAmount> heatLoad{};
	heatLoad.reserve(49);
	heatLoad.emplace_back(hours_as_seconds(0.0), kW_as_W(29.60017807));
	heatLoad.emplace_back(hours_as_seconds(1.0), kW_as_W(16.70505099));
	heatLoad.emplace_back(hours_as_seconds(2.0), kW_as_W(16.99812206));
	heatLoad.emplace_back(hours_as_seconds(3.0), kW_as_W(23.4456856));
	heatLoad.emplace_back(hours_as_seconds(4.0), kW_as_W(17.5842642));
	heatLoad.emplace_back(hours_as_seconds(5.0), kW_as_W(23.73875667));
	heatLoad.emplace_back(hours_as_seconds(6.0), kW_as_W(17.87733527));
	heatLoad.emplace_back(hours_as_seconds(7.0), kW_as_W(24.03182774));
	heatLoad.emplace_back(hours_as_seconds(8.0), kW_as_W(17.87733527));
	heatLoad.emplace_back(hours_as_seconds(9.0), kW_as_W(23.4456856));
	heatLoad.emplace_back(hours_as_seconds(10.0), kW_as_W(16.41197992));
	heatLoad.emplace_back(hours_as_seconds(11.0), kW_as_W(18.75654848));
	heatLoad.emplace_back(hours_as_seconds(12.0), kW_as_W(14.36048243));
	heatLoad.emplace_back(hours_as_seconds(13.0), kW_as_W(16.11890885));
	heatLoad.emplace_back(hours_as_seconds(14.0), kW_as_W(10.55055852));
	heatLoad.emplace_back(hours_as_seconds(15.0), kW_as_W(13.77434029));
	heatLoad.emplace_back(hours_as_seconds(16.0), kW_as_W(9.37827424));
	heatLoad.emplace_back(hours_as_seconds(17.0), kW_as_W(13.18819815));
	heatLoad.emplace_back(hours_as_seconds(18.0), kW_as_W(9.37827424));
	heatLoad.emplace_back(hours_as_seconds(19.0), kW_as_W(13.48126922));
	heatLoad.emplace_back(hours_as_seconds(20.0), kW_as_W(9.67134531));
	heatLoad.emplace_back(hours_as_seconds(21.0), kW_as_W(12.30898494));
	heatLoad.emplace_back(hours_as_seconds(22.0), kW_as_W(10.55055852));
	heatLoad.emplace_back(hours_as_seconds(23.0), kW_as_W(13.48126922));
	heatLoad.emplace_back(hours_as_seconds(24.0), kW_as_W(9.67134531));
	heatLoad.emplace_back(hours_as_seconds(25.0), kW_as_W(13.48126922));
	heatLoad.emplace_back(hours_as_seconds(26.0), kW_as_W(12.30898494));
	heatLoad.emplace_back(hours_as_seconds(27.0), kW_as_W(14.06741136));
	heatLoad.emplace_back(hours_as_seconds(28.0), kW_as_W(12.30898494));
	heatLoad.emplace_back(hours_as_seconds(29.0), kW_as_W(13.48126922));
	heatLoad.emplace_back(hours_as_seconds(30.0), kW_as_W(10.84362959));
	heatLoad.emplace_back(hours_as_seconds(31.0), kW_as_W(4.10299498));
	heatLoad.emplace_back(hours_as_seconds(32.0), kW_as_W(45.71908692));
	heatLoad.emplace_back(hours_as_seconds(33.0), kW_as_W(38.97845231));
	heatLoad.emplace_back(hours_as_seconds(34.0), kW_as_W(33.11703091));
	heatLoad.emplace_back(hours_as_seconds(35.0), kW_as_W(26.96253844));
	heatLoad.emplace_back(hours_as_seconds(36.0), kW_as_W(24.32489881));
	heatLoad.emplace_back(hours_as_seconds(37.0), kW_as_W(22.85954346));
	heatLoad.emplace_back(hours_as_seconds(38.0), kW_as_W(26.66946737));
	heatLoad.emplace_back(hours_as_seconds(39.0), kW_as_W(29.89324914));
	heatLoad.emplace_back(hours_as_seconds(40.0), kW_as_W(26.66946737));
	heatLoad.emplace_back(hours_as_seconds(41.0), kW_as_W(24.32489881));
	heatLoad.emplace_back(hours_as_seconds(42.0), kW_as_W(27.25560951));
	heatLoad.emplace_back(hours_as_seconds(43.0), kW_as_W(26.66946737));
	heatLoad.emplace_back(hours_as_seconds(44.0), kW_as_W(22.85954346));
	heatLoad.emplace_back(hours_as_seconds(45.0), kW_as_W(21.10111704));
	heatLoad.emplace_back(hours_as_seconds(46.0), kW_as_W(18.46347741));
	heatLoad.emplace_back(hours_as_seconds(47.0), kW_as_W(0.0));
	heatLoad.emplace_back(hours_as_seconds(48.0), kW_as_W(3.22378177));
	std::vector<TimeAndAmount> pvAvail{};
	pvAvail.reserve(49);
	pvAvail.emplace_back(hours_as_seconds(0.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(1.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(2.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(3.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(4.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(5.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(6.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(7.0), kW_as_W(14.36));
	pvAvail.emplace_back(hours_as_seconds(8.0), kW_as_W(671.759));
	pvAvail.emplace_back(hours_as_seconds(9.0), kW_as_W(1265.933));
	pvAvail.emplace_back(hours_as_seconds(10.0), kW_as_W(1583.21));
	pvAvail.emplace_back(hours_as_seconds(11.0), kW_as_W(1833.686));
	pvAvail.emplace_back(hours_as_seconds(12.0), kW_as_W(1922.872));
	pvAvail.emplace_back(hours_as_seconds(13.0), kW_as_W(1749.437));
	pvAvail.emplace_back(hours_as_seconds(14.0), kW_as_W(994.715));
	pvAvail.emplace_back(hours_as_seconds(15.0), kW_as_W(468.411));
	pvAvail.emplace_back(hours_as_seconds(16.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(17.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(18.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(19.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(20.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(21.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(22.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(23.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(24.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(25.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(26.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(27.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(28.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(29.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(30.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(31.0), kW_as_W(10.591));
	pvAvail.emplace_back(hours_as_seconds(32.0), kW_as_W(693.539));
	pvAvail.emplace_back(hours_as_seconds(33.0), kW_as_W(1191.017));
	pvAvail.emplace_back(hours_as_seconds(34.0), kW_as_W(1584.868));
	pvAvail.emplace_back(hours_as_seconds(35.0), kW_as_W(1820.692));
	pvAvail.emplace_back(hours_as_seconds(36.0), kW_as_W(1952.869));
	pvAvail.emplace_back(hours_as_seconds(37.0), kW_as_W(1799.1));
	pvAvail.emplace_back(hours_as_seconds(38.0), kW_as_W(1067.225));
	pvAvail.emplace_back(hours_as_seconds(39.0), kW_as_W(396.023));
	pvAvail.emplace_back(hours_as_seconds(40.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(41.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(42.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(43.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(44.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(45.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(46.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(47.0), kW_as_W(0.0));
	pvAvail.emplace_back(hours_as_seconds(48.0), kW_as_W(0.0));
	// COMPONENTS
	auto pvArrayId = Model_AddScheduleBasedSource(m, pvAvail);
	auto elecUtilId = Model_AddConstantSource(
		m, kW_as_W(10.0));
	auto batteryId = Model_AddStore(
		m,
		kWh_as_J(100.0),
		kW_as_W(10.0),
		kW_as_W(1'000.0),
		kWh_as_J(80.0),
		kWh_as_J(100.0));
	auto elecSourceMuxId = Model_AddMux(m, 2, 1);
	auto elecSupplyMuxId = Model_AddMux(m, 2, 2);
	auto ngUtilId = Model_AddConstantSource(
		m, std::numeric_limits<uint32_t>::max());
	auto ngSourceMuxId = Model_AddMux(m, 1, 2);
	auto ngToElecConvId = Model_AddConstantEfficiencyConverter(
		m, 42, 100);
	auto elecHeatPumpConvId = Model_AddConstantEfficiencyConverter(
		m, 35, 10);
	auto ngHeaterConvId = Model_AddConstantEfficiencyConverter(
		m, 98, 100);
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
	auto results = Simulate(m, doPrint);
	PrintPass(doPrint, "13");
}

void
Test14(bool doPrint)
{
	PrintBanner(doPrint, "14");
	Model m = {};
	m.RandFn = []() { return 0.4; };
	m.FinalTime = 4.0;
	std::vector<TimeAndAmount> availablePower{
		{ 0.0, 50 },
		{ 2.0, 10 },
	};
	auto src01Id = Model_AddConstantSource(m, 50);
	auto src02Id = Model_AddScheduleBasedSource(m, availablePower);
	auto muxId = Model_AddMux(m, 2, 1);
	auto loadId = Model_AddConstantLoad(m, 100);
	auto src1ToMuxConn = Model_AddConnection(m, src01Id, 0, muxId, 0);
	auto src2ToMuxConn = Model_AddConnection(m, src02Id.Id, 0, muxId, 1);
	auto muxToLoadConn = Model_AddConnection(m, muxId, 0, loadId, 0);
	auto results = Simulate(m, doPrint);
	// TODO: add tests/checks
	PrintPass(doPrint, "14");
}

void
Test15(bool doPrint)
{
	PrintBanner(doPrint, "15");
	Model m = {};
	m.RandFn = []() { return 0.4; };
	m.FinalTime = 2.0;
	std::vector<TimeAndAmount> loadOne{
		{ 0.0, 50 },
		{ 2.0, 10 },
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
	auto results = Simulate(m, doPrint);
	assert(results.size() == 2);
	double t = 0.0;
	auto src1ToConvResults =
		ModelResults_GetFlowForConnection(m, src1ToConvConn, t, results);
	assert(src1ToConvResults.has_value());
	assert(src1ToConvResults.value().Actual_W == 200);
	assert(src1ToConvResults.value().Requested_W == 200);
	assert(src1ToConvResults.value().Available_W == 1'000);
	auto convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.has_value());
	assert(convToLoadResults.value().Actual_W == 50);
	assert(convToLoadResults.value().Requested_W == 50);
	assert(convToLoadResults.value().Available_W == 250);
	auto convLossToMuxResults =
		ModelResults_GetFlowForConnection(m, convLossToMuxConn, t, results);
	assert(convLossToMuxResults.has_value());
	assert(convLossToMuxResults.value().Actual_W == 100);
	assert(convLossToMuxResults.value().Requested_W == 100);
	assert(convLossToMuxResults.value().Available_W == 150);
	auto src2ToMuxResults =
		ModelResults_GetFlowForConnection(m, src2ToMuxConn, t, results);
	assert(src2ToMuxResults.has_value());
	assert(src2ToMuxResults.value().Actual_W == 0);
	assert(src2ToMuxResults.value().Requested_W == 0);
	assert(src2ToMuxResults.value().Available_W == 1'000);
	auto muxToLoadResults =
		ModelResults_GetFlowForConnection(m, muxToLoadConn, t, results);
	assert(muxToLoadResults.has_value());
	assert(muxToLoadResults.value().Actual_W == 100);
	assert(muxToLoadResults.value().Requested_W == 100);
	assert(muxToLoadResults.value().Available_W == 1'150);
	t = 2.0;
	src1ToConvResults =
		ModelResults_GetFlowForConnection(m, src1ToConvConn, t, results);
	assert(src1ToConvResults.has_value());
	assert(src1ToConvResults.value().Actual_W == 40);
	assert(src1ToConvResults.value().Requested_W == 40);
	assert(src1ToConvResults.value().Available_W == 1'000);
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.has_value());
	assert(convToLoadResults.value().Actual_W == 10);
	assert(convToLoadResults.value().Requested_W == 10);
	assert(convToLoadResults.value().Available_W == 250);
	convLossToMuxResults =
		ModelResults_GetFlowForConnection(m, convLossToMuxConn, t, results);
	assert(convLossToMuxResults.has_value());
	assert(convLossToMuxResults.value().Actual_W == 30);
	assert(convLossToMuxResults.value().Requested_W == 100);
	assert(convLossToMuxResults.value().Available_W == 30);
	muxToLoadResults =
		ModelResults_GetFlowForConnection(m, muxToLoadConn, t, results);
	assert(muxToLoadResults.has_value());
	assert(muxToLoadResults.value().Actual_W == 100);
	assert(muxToLoadResults.value().Requested_W == 100);
	assert(muxToLoadResults.value().Available_W == 1'030);
	src2ToMuxResults =
		ModelResults_GetFlowForConnection(m, src2ToMuxConn, t, results);
	assert(src2ToMuxResults.has_value());
	assert(src2ToMuxResults.value().Actual_W == 70);
	assert(src2ToMuxResults.value().Requested_W == 70);
	assert(src2ToMuxResults.value().Available_W == 1'000);
	PrintPass(doPrint, "15");
}

void
Test16(bool doPrint)
{
	PrintBanner(doPrint, "16");
	Model m = {};
	m.RandFn = []() { return 0.4; };
	m.FinalTime = 2.0;
	size_t srcId = Model_AddConstantSource(m, 100);
	size_t loadId = Model_AddConstantLoad(m, 50);
	size_t passId = Model_AddPassThrough(m);
	auto srcToPassConn = Model_AddConnection(m, srcId, 0, passId, 0);
	auto passToLoadConn = Model_AddConnection(m, passId, 0, loadId, 0);
	auto results = Simulate(m, doPrint);
	assert(results.size() == 2);
	double t = 0.0;
	auto srcToPassResults =
		ModelResults_GetFlowForConnection(m, srcToPassConn, t, results);
	assert(srcToPassResults.has_value());
	assert(srcToPassResults.value().Actual_W == 50);
	assert(srcToPassResults.value().Requested_W == 50);
	assert(srcToPassResults.value().Available_W == 100);
	auto passToLoadResults =
		ModelResults_GetFlowForConnection(m, passToLoadConn, t, results);
	assert(passToLoadResults.has_value());
	assert(passToLoadResults.value().Actual_W == 50);
	assert(passToLoadResults.value().Requested_W == 50);
	assert(passToLoadResults.value().Available_W == 100);
	PrintPass(doPrint, "16");
}

void
Test17(bool doPrint)
{
	PrintBanner(doPrint, "17");
	std::vector<TimeState> a{
		{0.0, true}, {10.0, false, {1}}, {100.0, true}};
	std::vector<TimeState> b{
		{0.0, true}, {40.0, false, {2}}, {90.0, true}, {150.0, false, {2}}};
	std::vector<TimeState> expected{
		{0.0, true},
		{10.0, false, {1}},
		{40.0, false, {1, 2}},
		{90.0, false, {1}},
		{100.0, true},
		{150.0, false, {2}}};
	std::vector<TimeState> actual = TimeState_Combine(a, b);
	assert(expected.size() == actual.size());
	for (size_t i = 0; i < expected.size(); ++i)
	{
		assert(expected[i].time == actual[i].time);
		assert(expected[i].state == actual[i].state);
		assert(expected[i].failureModeCauses.size()
			== actual[i].failureModeCauses.size());
		for (auto const& fmId : expected[i].failureModeCauses)
		{
			assert(actual[i].failureModeCauses.contains(fmId));
		}
	}
	PrintPass(doPrint, "17");
}

int
main(int argc, char** argv) {
	auto start = std::chrono::high_resolution_clock::now();
	Test1(false);
	Test2(false);
	Test3(false);
	Test3A(false);
	Test4(false);
	Test5(false);
	Test6(false);
	Test7(false);
	Test8(false);
	Test9(false);
	Test10(false);
	Test11(false);
	Test12(false);
	Test13(false);
	Test14(false);
	Test15(false);
	Test16(false);
	Test17(false);
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration =
		std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << "Duration "
		<< ((double)duration.count() / 1000.0) << " ms" << std::endl;
	return EXIT_SUCCESS;
}
