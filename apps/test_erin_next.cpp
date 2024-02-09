#include "erin_next/erin_next.h"
#include <iomanip>
#include <chrono>

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
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToLoadConn = Model_AddConnection(m, ss, srcId, 0, loadId, 0);
	auto results = Simulate(m, ss, print);
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
	assert((srcToLoadResult.value().Actual == 10
		&& "actual value must equal 10"));
	assert((srcToLoadResult.value().Available == 100
		&& "available must equal 100"));
	assert((srcToLoadResult.value().Requested == 10
		&& "requested must equal 10"));
	PrintPass(print, "1");
}

static void
Test2(bool print) {
	PrintBanner(print, "2");
	Model m = {};
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto convId = Model_AddConstantEfficiencyConverter(m, ss, 1, 2);
	auto srcToConvConn = Model_AddConnection(m, ss, srcId, 0, convId.Id, 0);
	auto convToLoadConn = Model_AddConnection(m, ss, convId.Id, 0, loadId, 0);
	auto results = Simulate(m, ss, print);
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
	assert((srcToConvResults.value().Requested == 20
		&& "requested must equal 20"));
	assert((srcToConvResults.value().Actual == 20
		&& "actual value must equal 20"));
	assert((srcToConvResults.value().Available == 100
		&& "available must equal 100"));
	auto convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, 0.0, results);
	assert((convToLoadResults.has_value()
		&& "converter to load must have results"));
	assert((convToLoadResults.value().Requested == 10
		&& "requested must equal 10"));
	assert((convToLoadResults.value().Actual == 10
		&& "actual value must equal 10"));
	assert((convToLoadResults.value().Available == 50
		&& "available must equal 50"));
	auto convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value()
		&& "converter to waste must have results"));
	assert((convToWasteResults.value().Requested == 10
		&& "requested must equal 10"));
	assert((convToWasteResults.value().Actual == 10
		&& "actual value must equal 10"));
	assert((convToWasteResults.value().Available == 10
		&& "available must equal 10"));
	PrintPass(print, "2");
}

static void
Test3(bool print) {
	PrintBanner(print, "3");
	Model m = {};
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 100);
	auto load1Id = Model_AddConstantLoad(m, 10);
	auto load2Id = Model_AddConstantLoad(m, 2);
	auto convId = Model_AddConstantEfficiencyConverter(m, ss, 1, 2);
	auto srcToConvConn = Model_AddConnection(m, ss, srcId, 0, convId.Id, 0);
	auto convToLoad1Conn = Model_AddConnection(m, ss, convId.Id, 0, load1Id, 0);
	auto convToLoad2Conn = Model_AddConnection(m, ss, convId.Id, 1, load2Id, 0);
	auto results = Simulate(m, ss, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 4 && "size of flows must equal 4"));
	auto srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value()
		&& "source to converter must have results"));
	assert((srcToConvResults.value().Requested == 20
		&& "requested must equal 20"));
	assert((srcToConvResults.value().Actual == 20
		&& "actual value must equal 20"));
	assert((srcToConvResults.value().Available == 100
		&& "available must equal 100"));
	auto convToLoad1Results =
		ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
	assert((convToLoad1Results.has_value()
		&& "converter to load1 must have results"));
	assert((convToLoad1Results.value().Requested == 10
		&& "requested must equal 10"));
	assert((convToLoad1Results.value().Actual == 10
		&& "actual value must equal 10"));
	assert((convToLoad1Results.value().Available == 50
		&& "available must equal 50"));
	auto convToLoad2Results =
		ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
	assert((convToLoad2Results.has_value()
		&& "conv to load2 must have results"));
	assert((convToLoad2Results.value().Requested == 2
		&& "requested must equal 2"));
	assert((convToLoad2Results.value().Actual == 2
		&& "actual value must equal 2"));
	assert((convToLoad2Results.value().Available == 2
		&& "available must equal 50"));
	auto convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value()
		&& "conv to waste must have results"));
	assert((convToWasteResults.value().Requested == 8
		&& "requested must equal 8"));
	assert((convToWasteResults.value().Actual == 8
		&& "actual value must equal 8"));
	assert((convToWasteResults.value().Available == 8
		&& "available must equal 48"));
	PrintPass(print, "3");
}

static void
Test3A(bool print) {
	PrintBanner(print, "3a");
	Model m = {};
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 100);
	auto load1Id = Model_AddConstantLoad(m, 10);
	auto load2Id = Model_AddConstantLoad(m, 2);
	auto convId = Model_AddConstantEfficiencyConverter(m, ss, 1, 2);
	auto convToLoad2Conn = Model_AddConnection(m, ss, convId.Id, 1, load2Id, 0);
	auto convToLoad1Conn = Model_AddConnection(m, ss, convId.Id, 0, load1Id, 0);
	auto srcToConvConn = Model_AddConnection(m, ss, srcId, 0, convId.Id, 0);
	auto results = Simulate(m, ss, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 4 && "size of flows must equal 4"));
	auto srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value()
		&& "source to converter must have results"));
	assert((srcToConvResults.value().Requested == 20
		&& "requested must equal 20"));
	assert((srcToConvResults.value().Actual == 20
		&& "actual value must equal 20"));
	assert((srcToConvResults.value().Available == 100
		&& "available must equal 100"));
	auto convToLoad1Results =
		ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
	assert((convToLoad1Results.has_value()
		&& "converter to load1 must have results"));
	assert((convToLoad1Results.value().Requested == 10
		&& "requested must equal 10"));
	assert((convToLoad1Results.value().Actual == 10
		&& "actual value must equal 10"));
	assert((convToLoad1Results.value().Available == 50
		&& "available must equal 50"));
	auto convToLoad2Results =
		ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
	assert((convToLoad2Results.has_value()
		&& "conv to load2 must have results"));
	assert((convToLoad2Results.value().Requested == 2
		&& "requested must equal 2"));
	assert((convToLoad2Results.value().Actual == 2
		&& "actual value must equal 2"));
	assert((convToLoad2Results.value().Available == 2
		&& "available must equal 50"));
	auto convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value()
		&& "conv to waste must have results"));
	assert((convToWasteResults.value().Requested == 8
		&& "requested must equal 8"));
	assert((convToWasteResults.value().Actual == 8
		&& "actual value must equal 8"));
	assert((convToWasteResults.value().Available == 8
		&& "available must equal 48"));
	PrintPass(print, "3a");
}

static void
Test4(bool print) {
	PrintBanner(print, "4");
	std::vector<TimeAndAmount> timesAndLoads = {};
	timesAndLoads.push_back({ 0.0, 10 });
	timesAndLoads.push_back({ 3600.0, 200 });
	Model m = {};
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddScheduleBasedLoad(m, timesAndLoads);
	auto srcToLoadConn = Model_AddConnection(m, ss, srcId, 0, loadId, 0);
	auto results = Simulate(m, ss, print);
	assert((results.size() == 2 && "output must have a size of 2"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 1 && "size of flows[0] must equal 1"));
	auto srcToLoadResults_0 =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, 0.0, results);
	assert((srcToLoadResults_0.has_value()
		&& "source to load must have results at time=0.0"));
	assert((srcToLoadResults_0.value().Requested == 10
		&& "requested must equal 10"));
	assert((srcToLoadResults_0.value().Actual == 10
		&& "actual value must equal 10"));
	assert((srcToLoadResults_0.value().Available == 100
		&& "available must equal 100"));
	assert((results[1].Time == 3600.0
		&& "time must equal 3600.0"));
	assert((results[1].Flows.size() == 1
		&& "size of flows[1] must equal 1"));
	auto srcToLoadResults_3600 =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, 3600.0, results);
	assert((srcToLoadResults_3600.has_value()
		&& "source to load must have results at time=3600.0"));
	assert((srcToLoadResults_3600.value().Requested == 200
		&& "requested must equal 200"));
	assert((srcToLoadResults_3600.value().Actual == 100
		&& "actual value must equal 100"));
	assert((srcToLoadResults_3600.value().Available == 100
		&& "available must equal 100"));
	PrintPass(print, "4");
}

static void
Test5(bool print) {
	PrintBanner(print, "5");
	std::vector<TimeAndAmount> timesAndLoads = {};
	Model m = {};
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 100);
	auto load1Id = Model_AddConstantLoad(m, 10);
	auto load2Id = Model_AddConstantLoad(m, 7);
	auto load3Id = Model_AddConstantLoad(m, 5);
	auto conv1 = Model_AddConstantEfficiencyConverter(m, ss, 1, 4);
	auto conv2 = Model_AddConstantEfficiencyConverter(m, ss, 1, 4);
	auto conv3 = Model_AddConstantEfficiencyConverter(m, ss, 1, 4);
	auto srcToConv1Conn = Model_AddConnection(m, ss, srcId, 0, conv1.Id, 0);
	auto conv1ToLoad1Conn =
		Model_AddConnection(m, ss, conv1.Id, 0, load1Id, 0);
	auto conv1ToConv2Conn =
		Model_AddConnection(m, ss, conv1.Id, 1, conv2.Id, 0);
	auto conv2ToLoad2Conn =
		Model_AddConnection(m, ss, conv2.Id, 0, load2Id, 0);
	auto conv2ToConv3Conn =
		Model_AddConnection(m, ss, conv2.Id, 1, conv3.Id, 0);
	auto conv3ToLoad3Conn =
		Model_AddConnection(m, ss, conv3.Id, 0, load3Id, 0);
	auto results = Simulate(m, ss, print);
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
	assert((srcToConv1Results.value().Actual == 40
		&& "src to conv1 should flow 40"));
	assert((conv1ToLoad1Results.value().Actual == 10
		&& "conv1 to load1 should flow 10"));
	assert((conv1ToConv2Results.value().Actual == 28
		&& "conv1 to conv2 should flow 28"));
	assert((conv2ToLoad2Results.value().Actual == 7
		&& "conv1 to conv2 should flow 7"));
	assert((conv2ToConv3Results.value().Actual == 20
		&& "conv2 to conv3 should flow 21"));
	assert((conv3ToLoad3Results.value().Actual == 5
		&& "conv3 to load3 should flow 5"));
	PrintPass(print, "5");
}

static void
Test6(bool doPrint) {
	PrintBanner(doPrint, "6");
	Model m = {};
	SimulationState ss{};
	auto src1Id = Model_AddConstantSource(m, 10);
	auto src2Id = Model_AddConstantSource(m, 50);
	auto load1Id = Model_AddConstantLoad(m, 10);
	auto load2Id = Model_AddConstantLoad(m, 80);
	auto muxId = Model_AddMux(m, 2, 2);
	auto src1ToMuxConn = Model_AddConnection(m, ss, src1Id, 0, muxId, 0);
	auto src2ToMuxConn = Model_AddConnection(m, ss, src2Id, 0, muxId, 1);
	auto muxToLoad1Conn = Model_AddConnection(m, ss, muxId, 0, load1Id, 0);
	auto muxToLoad2Conn = Model_AddConnection(m, ss, muxId, 1, load2Id, 0);
	auto results = Simulate(m, ss, doPrint);
	auto src1ToMuxResults =
		ModelResults_GetFlowForConnection(m, src1ToMuxConn, 0.0, results);
	assert((src1ToMuxResults.value().Actual == 10
		&& "src1 -> mux expected actual flow of 10"));
	auto src2ToMuxResults =
		ModelResults_GetFlowForConnection(m, src2ToMuxConn, 0.0, results);
	assert((src2ToMuxResults.value().Actual == 50
		&& "src2 -> mux expected actual flow of 50"));
	auto muxToLoad1Results =
		ModelResults_GetFlowForConnection(m, muxToLoad1Conn, 0.0, results);
	assert((muxToLoad1Results.value().Actual == 10
		&& "mux -> load1 expected actual flow of 10"));
	auto muxToLoad2Results =
		ModelResults_GetFlowForConnection(m, muxToLoad2Conn, 0.0, results);
	assert((muxToLoad2Results.value().Actual == 50
		&& "mux -> load2 expected actual flow of 50"));
	PrintPass(doPrint, "6");
}

static void
Test7(bool doPrint) {
	PrintBanner(doPrint, "7");
	Model m = {};
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 0);
	auto storeId = Model_AddStore(m, 100, 10, 10, 0, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToStoreConn = Model_AddConnection(m, ss, srcId, 0, storeId, 0);
	auto storeToLoadConn = Model_AddConnection(m, ss, storeId, 0, loadId, 0);
	auto results = Simulate(m, ss, doPrint);
	auto srcToStoreResults =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 0.0, results);
	assert(srcToStoreResults.value().Actual == 0
		&& "src to store should be providing 0");
	assert(srcToStoreResults.value().Requested == 10
		&& "src to store request is 10");
	assert(srcToStoreResults.value().Available == 0
		&& "src to store available is 0");
	auto storeToLoadResults =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 0.0, results);
	assert(storeToLoadResults.has_value()
		&& "should have results for store to load connection");
	assert(storeToLoadResults.value().Actual == 10
		&& "store to load should be providing 10");
	assert(storeToLoadResults.value().Requested == 10
		&& "store to load should be requesting 10");
	assert(storeToLoadResults.value().Available == 10
		&& "store to load available should be 10");
	assert(results.size() == 2
		&& "there should be two time events in results");
	assert(results[1].Time > 10.0 - 1e-6
		&& results[1].Time < 10.0 + 1e-6 && "time should be 10");
	auto srcToStoreResultsAt10 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 10.0, results);
	assert(srcToStoreResultsAt10.value().Actual == 0
		&& "src to store should be providing 0");
	assert(srcToStoreResultsAt10.value().Requested == 20
		&& "src to store request is 20");
	assert(srcToStoreResultsAt10.value().Available == 0
		&& "src to store available is 0");
	auto storeToLoadResultsAt10 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 10.0, results);
	assert(storeToLoadResultsAt10.has_value()
		&& "should have results for store to load connection");
	assert(storeToLoadResultsAt10.value().Actual == 0
		&& "store to load should be providing 0");
	assert(storeToLoadResultsAt10.value().Requested == 10
		&& "store to load should be requesting 10");
	assert(storeToLoadResultsAt10.value().Available == 0
		&& "store to load available should be 0");
	PrintPass(doPrint, "7");
}

static void
Test8(bool doPrint) {
	PrintBanner(doPrint, "8");
	Model m = {};
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 5);
	auto storeId = Model_AddStore(m, 100, 10, 10, 0, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToStoreConn = Model_AddConnection(m, ss, srcId, 0, storeId, 0);
	auto storeToLoadConn = Model_AddConnection(m, ss, storeId, 0, loadId, 0);
	auto results = Simulate(m, ss, doPrint);
	auto srcToStoreResults =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 0.0, results);
	assert(srcToStoreResults.value().Actual == 5
		&& "src to store should be providing 5");
	assert(srcToStoreResults.value().Requested == 10
		&& "src to store request is 10");
	assert(srcToStoreResults.value().Available == 5
		&& "src to store available is 5");
	auto storeToLoadResults =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 0.0, results);
	assert(storeToLoadResults.has_value()
		&& "should have results for store to load connection");
	assert(storeToLoadResults.value().Actual == 10
		&& "store to load should be providing 10");
	assert(storeToLoadResults.value().Requested == 10
		&& "store to load should be requesting 10");
	assert(storeToLoadResults.value().Available == 15
		&& "store to load available should be 15");
	assert(results.size() == 2
		&& "there should be two time events in results");
	assert(results[1].Time > 20.0 - 1e-6
		&& results[1].Time < 20.0 + 1e-6 && "time should be 20");
	auto srcToStoreResultsAt20 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 20.0, results);
	assert(srcToStoreResultsAt20.value().Actual == 5
		&& "src to store should be providing 5");
	assert(srcToStoreResultsAt20.value().Requested == 20
		&& "src to store request is 20");
	assert(srcToStoreResultsAt20.value().Available == 5
		&& "src to store available is 5");
	auto storeToLoadResultsAt20 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 20.0, results);
	assert(storeToLoadResultsAt20.has_value()
		&& "should have results for store to load connection");
	assert(storeToLoadResultsAt20.value().Actual == 5
		&& "store to load should be providing 5");
	assert(storeToLoadResultsAt20.value().Requested == 10
		&& "store to load should be requesting 10");
	assert(storeToLoadResultsAt20.value().Available == 5
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
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 10);
	auto storeId = Model_AddStore(m, 100, 10, 10, 80, 100);
	auto loadId = Model_AddScheduleBasedLoad(m, timesAndLoads);
	auto srcToStoreConn = Model_AddConnection(m, ss, srcId, 0, storeId, 0);
	auto storeToLoadConn = Model_AddConnection(m, ss, storeId, 0, loadId, 0);
	auto results = Simulate(m, ss, doPrint);
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
	assert(srcToStoreResultsAt0.value().Actual == 10);
	assert(srcToStoreResultsAt0.value().Requested == 20);
	assert(srcToStoreResultsAt0.value().Available == 10);
	assert(storeToLoadResultsAt0.value().Actual == 20);
	assert(storeToLoadResultsAt0.value().Requested == 20);
	assert(storeToLoadResultsAt0.value().Available == 20);
	assert(storeAmount0.value() == 100);
	auto srcToStoreResultsAt2 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 2.0, results);
	auto storeToLoadResultsAt2 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 2.0, results);
	auto storeAmount2 =
		ModelResults_GetStoreState(m, storeId, 2.0, results);
	assert(srcToStoreResultsAt2.value().Actual == 10);
	assert(srcToStoreResultsAt2.value().Requested == 30);
	assert(srcToStoreResultsAt2.value().Available == 10);
	assert(storeToLoadResultsAt2.value().Actual == 20);
	assert(storeToLoadResultsAt2.value().Requested == 20);
	assert(storeToLoadResultsAt2.value().Available == 20);
	assert(storeAmount2.value() == 80);
	auto srcToStoreResultsAt5 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 5.0, results);
	auto storeToLoadResultsAt5 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 5.0, results);
	auto storeAmount5 = ModelResults_GetStoreState(m, storeId, 5.0, results);
	assert(srcToStoreResultsAt5.value().Actual == 10);
	assert(srcToStoreResultsAt5.value().Requested == 15);
	assert(srcToStoreResultsAt5.value().Available == 10);
	assert(storeToLoadResultsAt5.value().Actual == 5);
	assert(storeToLoadResultsAt5.value().Requested == 5);
	assert(storeToLoadResultsAt5.value().Available == 20);
	assert(storeAmount5.value() == 50);
	auto srcToStoreResultsAt10 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 10.0, results);
	auto storeToLoadResultsAt10 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 10.0, results);
	auto storeAmount10 =
		ModelResults_GetStoreState(m, storeId, 10.0, results);
	assert(srcToStoreResultsAt10.value().Actual == 10);
	assert(srcToStoreResultsAt10.value().Requested == 25);
	assert(srcToStoreResultsAt10.value().Available == 10);
	assert(storeToLoadResultsAt10.value().Actual == 15);
	assert(storeToLoadResultsAt10.value().Requested == 15);
	assert(storeToLoadResultsAt10.value().Available == 20);
	assert(storeAmount10.value() == 75);
	auto srcToStoreResultsAt25 =
		ModelResults_GetFlowForConnection(m, srcToStoreConn, 25.0, results);
	auto storeToLoadResultsAt25 =
		ModelResults_GetFlowForConnection(m, storeToLoadConn, 25.0, results);
	auto storeAmount25 =
		ModelResults_GetStoreState(m, storeId, 25.0, results);
	assert(srcToStoreResultsAt25.value().Actual == 10);
	assert(srcToStoreResultsAt25.value().Requested == 25);
	assert(srcToStoreResultsAt25.value().Available == 10);
	assert(storeToLoadResultsAt25.value().Actual == 10);
	assert(storeToLoadResultsAt25.value().Requested == 15);
	assert(storeToLoadResultsAt25.value().Available == 10);
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
	SimulationState ss{};
	auto src1Id = Model_AddConstantSource(m, 20);
	auto src2Id = Model_AddConstantSource(m, 5);
	auto storeId = Model_AddStore(m, 100, 10, 10, 80, 100);
	auto muxId = Model_AddMux(m, 2, 2);
	auto conv = Model_AddConstantEfficiencyConverter(m, ss, 1, 2);
	auto load1Id = Model_AddConstantLoad(m, 20);
	auto load2Id = Model_AddScheduleBasedLoad(m, timesAndLoads);
	auto load3Id = Model_AddConstantLoad(m, 5);
	auto src1ToMux0Port0Conn = Model_AddConnection(m, ss, src1Id, 0, muxId, 0);
	auto src2ToStoreConn = Model_AddConnection(m, ss, src2Id, 0, storeId, 0);
	auto storeToMux0Port1Conn = Model_AddConnection(m, ss, storeId, 0, muxId, 1);
	auto mux0Port0ToLoad1Conn = Model_AddConnection(m, ss, muxId, 0, load1Id, 0);
	auto mux0Port1ToConvConn = Model_AddConnection(m, ss, muxId, 1, conv.Id, 0);
	auto convToLoad2Conn = Model_AddConnection(m, ss, conv.Id, 0, load2Id, 0);
	auto convToLoad3Conn = Model_AddConnection(m, ss, conv.Id, 1, load3Id, 0);
	auto results = Simulate(m, ss, doPrint);
	assert(results.size() == 5 && "expect 5 events");
	// time = 0.0
	double t = 0.0;
	size_t resultsIdx = 0;
	assert(results[resultsIdx].Time == t);
	auto convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 3);
	assert(convToWasteResults.value().Requested == 3);
	assert(convToWasteResults.value().Available == 3);
	auto src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, 0.0, results);
	assert(src1ToMuxResults.value().Actual == 20);
	assert(src1ToMuxResults.value().Available == 20);
	assert(src1ToMuxResults.value().Requested == 45);
	auto src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, 0.0, results);
	assert(src2ToStoreResults.value().Actual == 5);
	assert(src2ToStoreResults.value().Available == 5);
	assert(src2ToStoreResults.value().Requested == 15);
	auto storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, 0.0, results);
	assert(storeToMuxResults.value().Actual == 15);
	assert(storeToMuxResults.value().Available == 15);
	assert(storeToMuxResults.value().Requested == 15);
	auto muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, 0.0, results);
	assert(muxToLoad1Results.value().Actual == 20);
	assert(muxToLoad1Results.value().Available == 20);
	assert(muxToLoad1Results.value().Requested == 20);
	auto muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, 0.0, results);
	assert(muxToConvResults.value().Actual == 15);
	assert(muxToConvResults.value().Available == 15);
	assert(muxToConvResults.value().Requested == 40);
	auto convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, 0.0, results);
	assert(convToLoad2Results.value().Actual == 7);
	assert(convToLoad2Results.value().Available == 7);
	assert(convToLoad2Results.value().Requested == 20);
	auto convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, 0.0, results);
	assert(convToLoad3Results.value().Actual == 5);
	assert(convToLoad3Results.value().Available == 5);
	assert(convToLoad3Results.value().Requested == 5);
	auto storeAmount = ModelResults_GetStoreState(m, storeId, 0.0, results);
	assert(storeAmount.value() == 100);
	// time = 2.0
	t = 2.0;
	resultsIdx = 1;
	assert(results[resultsIdx].Time == t);;
	convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 3);
	assert(convToWasteResults.value().Requested == 3);
	assert(convToWasteResults.value().Available == 3);
	src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, t, results);
	assert(src1ToMuxResults.value().Actual == 20);
	assert(src1ToMuxResults.value().Available == 20);
	assert(src1ToMuxResults.value().Requested == 45);
	src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, t, results);
	assert(src2ToStoreResults.value().Actual == 5);
	assert(src2ToStoreResults.value().Available == 5);
	assert(src2ToStoreResults.value().Requested == 25);
	storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, t, results);
	assert(storeToMuxResults.value().Actual == 15);
	assert(storeToMuxResults.value().Available == 15);
	assert(storeToMuxResults.value().Requested == 15);
	muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, t, results);
	assert(muxToLoad1Results.value().Actual == 20);
	assert(muxToLoad1Results.value().Available == 20);
	assert(muxToLoad1Results.value().Requested == 20);
	muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, t, results);
	assert(muxToConvResults.value().Actual == 15);
	assert(muxToConvResults.value().Available == 15);
	assert(muxToConvResults.value().Requested == 40);
	convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, t, results);
	assert(convToLoad2Results.value().Actual == 7);
	assert(convToLoad2Results.value().Available == 7);
	assert(convToLoad2Results.value().Requested == 20);
	convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, t, results);
	assert(convToLoad3Results.value().Actual == 5);
	assert(convToLoad3Results.value().Available == 5);
	assert(convToLoad3Results.value().Requested == 5);
	storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
	assert(storeAmount.value() == 80);
	// time = 5.0
	t = 5.0;
	resultsIdx = 2;
	assert(results[resultsIdx].Time == t);
	convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 0);
	assert(convToWasteResults.value().Requested == 0);
	assert(convToWasteResults.value().Available == 0);
	src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, t, results);
	assert(src1ToMuxResults.value().Actual == 20);
	assert(src1ToMuxResults.value().Available == 20);
	assert(src1ToMuxResults.value().Requested == 20);
	src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, t, results);
	assert(src2ToStoreResults.value().Actual == 5);
	assert(src2ToStoreResults.value().Available == 5);
	assert(src2ToStoreResults.value().Requested == 20);
	storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, t, results);
	assert(storeToMuxResults.value().Actual == 10);
	assert(storeToMuxResults.value().Available == 15);
	assert(storeToMuxResults.value().Requested == 10);
	muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, t, results);
	assert(muxToLoad1Results.value().Actual == 20);
	assert(muxToLoad1Results.value().Available == 25);
	assert(muxToLoad1Results.value().Requested == 20);
	muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, t, results);
	assert(muxToConvResults.value().Actual == 10);
	assert(muxToConvResults.value().Available == 10);
	assert(muxToConvResults.value().Requested == 10);
	convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, t, results);
	assert(convToLoad2Results.value().Actual == 5);
	assert(convToLoad2Results.value().Available == 5);
	assert(convToLoad2Results.value().Requested == 5);
	convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, t, results);
	assert(convToLoad3Results.value().Actual == 5);
	assert(convToLoad3Results.value().Available == 5);
	assert(convToLoad3Results.value().Requested == 5);
	storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
	assert(storeAmount.value() == 50);
	// time = 10.0
	t = 10.0;
	resultsIdx = 3;
	assert(results[resultsIdx].Time == t);
	convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 3);
	assert(convToWasteResults.value().Requested == 3);
	assert(convToWasteResults.value().Available == 3);
	src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, t, results);
	assert(src1ToMuxResults.value().Actual == 20);
	assert(src1ToMuxResults.value().Available == 20);
	assert(src1ToMuxResults.value().Requested == 35);
	src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, t, results);
	assert(src2ToStoreResults.value().Actual == 5);
	assert(src2ToStoreResults.value().Available == 5);
	assert(src2ToStoreResults.value().Requested == 25);
	storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, t, results);
	assert(storeToMuxResults.value().Actual == 15);
	assert(storeToMuxResults.value().Available == 15);
	assert(storeToMuxResults.value().Requested == 15);
	muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, t, results);
	assert(muxToLoad1Results.value().Actual == 20);
	assert(muxToLoad1Results.value().Available == 20);
	assert(muxToLoad1Results.value().Requested == 20);
	muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, t, results);
	assert(muxToConvResults.value().Actual == 15);
	assert(muxToConvResults.value().Available == 15);
	assert(muxToConvResults.value().Requested == 30);
	convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, t, results);
	assert(convToLoad2Results.value().Actual == 7);
	assert(convToLoad2Results.value().Available == 7);
	assert(convToLoad2Results.value().Requested == 15);
	convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, t, results);
	assert(convToLoad3Results.value().Actual == 5);
	assert(convToLoad3Results.value().Available == 5);
	assert(convToLoad3Results.value().Requested == 5);
	storeAmount = ModelResults_GetStoreState(m, storeId, t, results);
	assert(storeAmount.value() == 25);
	// time = 12.5
	t = 12.5;
	resultsIdx = 4;
	assert(results[resultsIdx].Time == t);
	convToWasteResults = ModelResults_GetFlowForConnection(
		m, conv.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 0);
	assert(convToWasteResults.value().Requested == 0);
	assert(convToWasteResults.value().Available == 0);
	src1ToMuxResults = ModelResults_GetFlowForConnection(
		m, src1ToMux0Port0Conn, t, results);
	assert(src1ToMuxResults.value().Actual == 20);
	assert(src1ToMuxResults.value().Available == 20);
	assert(src1ToMuxResults.value().Requested == 45);
	src2ToStoreResults = ModelResults_GetFlowForConnection(
		m, src2ToStoreConn, t, results);
	assert(src2ToStoreResults.value().Actual == 5);
	assert(src2ToStoreResults.value().Available == 5);
	assert(src2ToStoreResults.value().Requested == 15);
	storeToMuxResults = ModelResults_GetFlowForConnection(
		m, storeToMux0Port1Conn, t, results);
	assert(storeToMuxResults.value().Actual == 5);
	assert(storeToMuxResults.value().Available == 5);
	assert(storeToMuxResults.value().Requested == 5);
	muxToLoad1Results = ModelResults_GetFlowForConnection(
		m, mux0Port0ToLoad1Conn, t, results);
	assert(muxToLoad1Results.value().Actual == 20);
	assert(muxToLoad1Results.value().Available == 20);
	assert(muxToLoad1Results.value().Requested == 20);
	muxToConvResults = ModelResults_GetFlowForConnection(
		m, mux0Port1ToConvConn, t, results);
	assert(muxToConvResults.value().Actual == 5);
	assert(muxToConvResults.value().Available == 5);
	assert(muxToConvResults.value().Requested == 30);
	convToLoad2Results = ModelResults_GetFlowForConnection(
		m, convToLoad2Conn, t, results);
	assert(convToLoad2Results.value().Actual == 2);
	assert(convToLoad2Results.value().Available == 2);
	assert(convToLoad2Results.value().Requested == 15);
	convToLoad3Results = ModelResults_GetFlowForConnection(
		m, convToLoad3Conn, t, results);
	assert(convToLoad3Results.value().Actual == 3);
	assert(convToLoad3Results.value().Available == 3);
	assert(convToLoad3Results.value().Requested == 5);
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
	SimulationState ss{};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto convId = Model_AddConstantEfficiencyConverter(m, ss, 1, 2);
	auto srcToConvConn = Model_AddConnection(m, ss, srcId, 0, convId.Id, 0);
	auto convToLoadConn = Model_AddConnection(m, ss, convId.Id, 0, loadId, 0);
	auto fixedDistId = Model_AddFixedReliabilityDistribution(m, 10.0);
	Model_AddFailureModeToComponent(m, convId.Id, fixedDistId, fixedDistId);
	auto results = Simulate(m, ss, doPrint);
	assert(results.size() == 6
		&& "Expect 6 times: 0.0, 10.0, 20.0, 30.0, 40.0, 50.0");
	double t = 0.0;
	auto srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual == 20
		&& "src -> conv actual should be 20");
	assert(srcToConvResults.value().Requested == 20
		&& "src -> conv requested should be 20");
	assert(srcToConvResults.value().Available == 100
		&& "src -> conv available should be 100");
	auto convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual == 10
		&& "conv -> load actual should be 10");
	assert(convToLoadResults.value().Requested == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available == 50
		&& "conv -> load available should be 50");
	auto convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 10
		&& "conv -> waste actual should be 10");
	assert(convToWasteResults.value().Requested == 10
		&& "conv -> waste requested should be 10");
	assert(convToWasteResults.value().Available == 10
		&& "conv -> waste available should be 10");
	// time = 10.0, failed
	t = 10.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual == 0
		&& "src -> conv actual should be 0");
	assert(srcToConvResults.value().Requested == 0
		&& "src -> conv requested should be 0");
	assert(srcToConvResults.value().Available == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual == 0
		&& "conv -> load actual should be 0");
	assert(convToLoadResults.value().Requested == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available == 0
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 0
		&& "conv -> waste actual should be 0");
	assert(convToWasteResults.value().Requested == 0
		&& "conv -> waste requested should be 0");
	assert(convToWasteResults.value().Available == 0
		&& "conv -> waste available should be 0");
	// time = 20.0, fixed/restored
	t = 20.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual == 20
		&& "src -> conv actual should be 20");
	assert(srcToConvResults.value().Requested == 20
		&& "src -> conv requested should be 20");
	assert(srcToConvResults.value().Available == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual == 10
		&& "conv -> load actual should be 10");
	assert(convToLoadResults.value().Requested == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available == 50
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 10
		&& "conv -> waste actual should be 10");
	assert(convToWasteResults.value().Requested == 10
		&& "conv -> waste requested should be 10");
	assert(convToWasteResults.value().Available == 10
		&& "conv -> waste available should be 10");
	// time = 30.0, failed
	t = 30.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual == 0
		&& "src -> conv actual should be 0");
	assert(srcToConvResults.value().Requested == 0
		&& "src -> conv requested should be 0");
	assert(srcToConvResults.value().Available == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual == 0
		&& "conv -> load actual should be 0");
	assert(convToLoadResults.value().Requested == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available == 0
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 0
		&& "conv -> waste actual should be 0");
	assert(convToWasteResults.value().Requested == 0
		&& "conv -> waste requested should be 0");
	assert(convToWasteResults.value().Available == 0
		&& "conv -> waste available should be 0");
	// time = 40.0, fixed/restored
	t = 40.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual == 20
		&& "src -> conv actual should be 20");
	assert(srcToConvResults.value().Requested == 20
		&& "src -> conv requested should be 20");
	assert(srcToConvResults.value().Available == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual == 10
		&& "conv -> load actual should be 10");
	assert(convToLoadResults.value().Requested == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available == 50
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 10
		&& "conv -> waste actual should be 10");
	assert(convToWasteResults.value().Requested == 10
		&& "conv -> waste requested should be 10");
	assert(convToWasteResults.value().Available == 10
		&& "conv -> waste available should be 10");
	// time = 50.0, failed
	t = 50.0;
	srcToConvResults =
		ModelResults_GetFlowForConnection(m, srcToConvConn, t, results);
	assert(srcToConvResults.value().Actual == 0
		&& "src -> conv actual should be 0");
	assert(srcToConvResults.value().Requested == 0
		&& "src -> conv requested should be 0");
	assert(srcToConvResults.value().Available == 100
		&& "src -> conv available should be 100");
	convToLoadResults =
		ModelResults_GetFlowForConnection(m, convToLoadConn, t, results);
	assert(convToLoadResults.value().Actual == 0
		&& "conv -> load actual should be 0");
	assert(convToLoadResults.value().Requested == 10
		&& "conv -> load requested should be 10");
	assert(convToLoadResults.value().Available == 0
		&& "conv -> load available should be 0");
	convToWasteResults =
		ModelResults_GetFlowForConnection(
			m, convId.WasteConnection, t, results);
	assert(convToWasteResults.value().Actual == 0
		&& "conv -> waste actual should be 0");
	assert(convToWasteResults.value().Requested == 0
		&& "conv -> waste requested should be 0");
	assert(convToWasteResults.value().Available == 0
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
	m.FinalTime = 50.0;
	std::vector<TimeAndAmount> sourceAvailability{};
	sourceAvailability.reserve(5);
	sourceAvailability.emplace_back(0, 10);
	sourceAvailability.emplace_back(10, 8);
	sourceAvailability.emplace_back(20, 12);
	SimulationState ss{};
	auto srcId = Model_AddScheduleBasedSource(m, ss, sourceAvailability);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToLoadConn = Model_AddConnection(m, ss, srcId.Id, 0, loadId, 0);
	auto results = Simulate(m, ss, doPrint);
	assert(results.size() == 3 && "should have 3 time results");
	assert(results[0].Time == 0.0);
	assert(results[1].Time == 10.0);
	assert(results[2].Time == 20.0);
	double t = 0.0;
	auto srcToLoadResults =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, t, results);
	assert(srcToLoadResults.value().Actual == 10);
	assert(srcToLoadResults.value().Available == 10);
	assert(srcToLoadResults.value().Requested == 10);
	auto srcToWasteResults =
		ModelResults_GetFlowForConnection(m, srcId.WasteConnection, t, results);
	assert(srcToWasteResults.value().Actual == 0);
	assert(srcToWasteResults.value().Available == 0);
	assert(srcToWasteResults.value().Requested == 0);
	t = 10.0;
	srcToLoadResults =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, t, results);
	assert(srcToLoadResults.value().Actual == 8);
	assert(srcToLoadResults.value().Available == 8);
	assert(srcToLoadResults.value().Requested == 10);
	srcToWasteResults =
		ModelResults_GetFlowForConnection(m, srcId.WasteConnection, t, results);
	assert(srcToWasteResults.value().Actual == 0);
	assert(srcToWasteResults.value().Available == 0);
	assert(srcToWasteResults.value().Requested == 0);
	t = 20.0;
	srcToLoadResults =
		ModelResults_GetFlowForConnection(m, srcToLoadConn, t, results);
	assert(srcToLoadResults.value().Actual == 10);
	assert(srcToLoadResults.value().Available == 12);
	assert(srcToLoadResults.value().Requested == 10);
	srcToWasteResults =
		ModelResults_GetFlowForConnection(m, srcId.WasteConnection, t, results);
	assert(srcToWasteResults.value().Actual == 2);
	assert(srcToWasteResults.value().Available == 2);
	assert(srcToWasteResults.value().Requested == 2);
	PrintPass(doPrint, "12");
}

/*
void
Test13(bool doPrint)
{
	PrintBanner(doPrint, "13");
	// Add a schedule-based source (availability, uncontrolled source)
	// NOTE: it would be good to have a waste connection so that the component
	// always "spills" (ullage) when not all available is used.
	Model m = {};
	m.RandFn = []() { return 0.4; };
	m.FinalTime = 50.0;
	std::vector<TimeAndAmount> sourceAvailability{};
	sourceAvailability.reserve(5);
	sourceAvailability.emplace_back(0, 10);
	sourceAvailability.emplace_back(10, 8);
	sourceAvailability.emplace_back(20, 6);
	sourceAvailability.emplace_back(30, 12);
	sourceAvailability.emplace_back(40, 16);
	std::vector<TimeAndAmount> loadRequest{};
	loadRequest.reserve(5);
	loadRequest.emplace_back(0, 20);
	loadRequest.emplace_back(10, 18);
	loadRequest.emplace_back(20, 16);
	loadRequest.emplace_back(30, 12);
	loadRequest.emplace_back(40, 8);
	loadRequest.emplace_back(40, 4);
	SimulationState ss{};
	auto src1Id = Model_AddScheduleBasedSource(m, sourceAvailability);
	auto src2Id = Model_AddConstantSource(m, 10);
	auto loadId = Model_AddScheduleBasedLoad(m, loadRequest);
	auto storeId = Model_AddStore(m, 50, 5, 10, 15, 40);
	auto mux1Id = Model_AddMux(m, 2, 2);
	auto mux2Id = Model_AddMux(m, 2, 1);
	auto src1ToMux1Conn = Model_AddConnection(m, ss, src1Id, 0, mux1Id, 0);
	auto src2ToMux1Conn = Model_AddConnection(m, ss, src2Id, 0, mux1Id, 1);
	// HERE: finish wiring this up and implementing
	auto convToLoadConn = Model_AddConnection(m, ss, convId.Id, 0, loadId, 0);
	auto fixedDistId = Model_AddFixedReliabilityDistribution(m, 10.0);
	Model_AddFailureModeToComponent(m, convId.Id, fixedDistId, fixedDistId);
	auto results = Simulate(m, ss, doPrint);
	PrintPass(doPrint, "13");
}

void
Test13(bool doPrint)
{
	PrintBanner(doPrint, "14");
	// 13 with reliability
	PrintPass(doPrint, "14");
}
*/

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
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << "Duration " << ((double)duration.count() / 1000.0) << " ms" << std::endl;
	return EXIT_SUCCESS;
}
