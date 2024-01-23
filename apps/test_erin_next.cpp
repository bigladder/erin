#include "erin_next/erin_next.h"

using namespace erin_next;

static void
Example1(bool print) {
	if (print) {
		std::cout << "Example  1:" << std::endl;
	}
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto srcToLoadConn = Model_AddConnection(m, srcId, 0, loadId, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 1 && "size of flows must equal 1"));
	auto srcToLoadResult = ModelResults_GetFlowForConnection(m, srcToLoadConn, 0.0, results);
	assert((srcToLoadResult.has_value() && "connection result should have a value"));
	assert((srcToLoadResult.value().Actual == 10 && "actual value must equal 10"));
	assert((srcToLoadResult.value().Available == 100 && "available must equal 100"));
	assert((srcToLoadResult.value().Requested == 10 && "requested must equal 10"));
	std::cout << "[Example  1] :: PASSED" << std::endl;
}

static void
Example2(bool print) {
	if (print) {
		std::cout << "Example  2:" << std::endl;
	}
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddConstantLoad(m, 10);
	auto convId = Model_AddConstantEfficiencyConverter(m, 1, 2);
	auto srcToConvConn = Model_AddConnection(m, srcId, 0, convId.Id, 0);
	auto convToLoadConn = Model_AddConnection(m, convId.Id, 0, loadId, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 1 && "output must have a size of 1"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 3 && "size of flows must equal 3"));
	auto srcToConvResults = ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value() && "source to converter must have results"));
	assert((srcToConvResults.value().Requested == 20 && "requested must equal 20"));
	assert((srcToConvResults.value().Actual == 20 && "actual value must equal 20"));
	assert((srcToConvResults.value().Available == 100 && "available must equal 100"));
	auto convToLoadResults = ModelResults_GetFlowForConnection(m, convToLoadConn, 0.0, results);
	assert((convToLoadResults.has_value() && "converter to load must have results"));
	assert((convToLoadResults.value().Requested == 10 && "requested must equal 10"));
	assert((convToLoadResults.value().Actual == 10 && "actual value must equal 10"));
	assert((convToLoadResults.value().Available == 50 && "available must equal 50"));
	auto convToWasteResults = ModelResults_GetFlowForConnection(m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value() && "converter to waste must have results"));
	assert((convToWasteResults.value().Requested == 10 && "requested must equal 10"));
	assert((convToWasteResults.value().Actual == 10 && "actual value must equal 10"));
	assert((convToWasteResults.value().Available == 10 && "available must equal 10"));
	std::cout << "[Example  2] :: PASSED" << std::endl;
}

static void
Example3(bool print) {
	if (print) {
		std::cout << "Example  3:" << std::endl;
	}
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
	auto srcToConvResults = ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value() && "source to converter must have results"));
	assert((srcToConvResults.value().Requested == 20 && "requested must equal 20"));
	assert((srcToConvResults.value().Actual == 20 && "actual value must equal 20"));
	assert((srcToConvResults.value().Available == 100 && "available must equal 100"));
	auto convToLoad1Results = ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
	assert((convToLoad1Results.has_value() && "converter to load1 must have results"));
	assert((convToLoad1Results.value().Requested == 10 && "requested must equal 10"));
	assert((convToLoad1Results.value().Actual == 10 && "actual value must equal 10"));
	assert((convToLoad1Results.value().Available == 50 && "available must equal 50"));
	auto convToLoad2Results = ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
	assert((convToLoad2Results.has_value() && "conv to load2 must have results"));
	assert((convToLoad2Results.value().Requested == 2 && "requested must equal 2"));
	assert((convToLoad2Results.value().Actual == 2 && "actual value must equal 2"));
	assert((convToLoad2Results.value().Available == 2 && "available must equal 50"));
	auto convToWasteResults = ModelResults_GetFlowForConnection(m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value() && "conv to waste must have results"));
	assert((convToWasteResults.value().Requested == 8 && "requested must equal 8"));
	assert((convToWasteResults.value().Actual == 8 && "actual value must equal 8"));
	assert((convToWasteResults.value().Available == 8 && "available must equal 48"));
	std::cout << "[Example  3] :: PASSED" << std::endl;
}

static void
Example3A(bool print) {
	if (print) {
		std::cout << "Example 3A:" << std::endl;
	}
	Model m = {};
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
	auto srcToConvResults = ModelResults_GetFlowForConnection(m, srcToConvConn, 0.0, results);
	assert((srcToConvResults.has_value() && "source to converter must have results"));
	assert((srcToConvResults.value().Requested == 20 && "requested must equal 20"));
	assert((srcToConvResults.value().Actual == 20 && "actual value must equal 20"));
	assert((srcToConvResults.value().Available == 100 && "available must equal 100"));
	auto convToLoad1Results = ModelResults_GetFlowForConnection(m, convToLoad1Conn, 0.0, results);
	assert((convToLoad1Results.has_value() && "converter to load1 must have results"));
	assert((convToLoad1Results.value().Requested == 10 && "requested must equal 10"));
	assert((convToLoad1Results.value().Actual == 10 && "actual value must equal 10"));
	assert((convToLoad1Results.value().Available == 50 && "available must equal 50"));
	auto convToLoad2Results = ModelResults_GetFlowForConnection(m, convToLoad2Conn, 0.0, results);
	assert((convToLoad2Results.has_value() && "conv to load2 must have results"));
	assert((convToLoad2Results.value().Requested == 2 && "requested must equal 2"));
	assert((convToLoad2Results.value().Actual == 2 && "actual value must equal 2"));
	assert((convToLoad2Results.value().Available == 2 && "available must equal 50"));
	auto convToWasteResults = ModelResults_GetFlowForConnection(m, convId.WasteConnection, 0.0, results);
	assert((convToWasteResults.has_value() && "conv to waste must have results"));
	assert((convToWasteResults.value().Requested == 8 && "requested must equal 8"));
	assert((convToWasteResults.value().Actual == 8 && "actual value must equal 8"));
	assert((convToWasteResults.value().Available == 8 && "available must equal 48"));
	std::cout << "[Example 3A] :: PASSED" << std::endl;
}

static void
Example4(bool print) {
	if (print) {
		std::cout << "Example  4:" << std::endl;
	}
	std::vector<TimeAndLoad> timesAndLoads = {};
	timesAndLoads.push_back({ 0.0, 10 });
	timesAndLoads.push_back({ 3600.0, 200 });
	Model m = {};
	auto srcId = Model_AddConstantSource(m, 100);
	auto loadId = Model_AddScheduleBasedLoad(m, timesAndLoads);
	auto srcToLoadConn = Model_AddConnection(m, srcId, 0, loadId, 0);
	auto results = Simulate(m, print);
	assert((results.size() == 2 && "output must have a size of 2"));
	assert((results[0].Time == 0.0 && "time must equal 0.0"));
	assert((results[0].Flows.size() == 1 && "size of flows[0] must equal 1"));
	auto srcToLoadResults_0 = ModelResults_GetFlowForConnection(m, srcToLoadConn, 0.0, results);
	assert((srcToLoadResults_0.has_value() && "source to load must have results at time=0.0"));
	assert((srcToLoadResults_0.value().Requested == 10 && "requested must equal 10"));
	assert((srcToLoadResults_0.value().Actual == 10 && "actual value must equal 10"));
	assert((srcToLoadResults_0.value().Available == 100 && "available must equal 100"));
	assert((results[1].Time == 3600.0 && "time must equal 3600.0"));
	assert((results[1].Flows.size() == 1 && "size of flows[1] must equal 1"));
	auto srcToLoadResults_3600 = ModelResults_GetFlowForConnection(m, srcToLoadConn, 3600.0, results);
	assert((srcToLoadResults_3600.has_value() && "source to load must have results at time=3600.0"));
	assert((srcToLoadResults_3600.value().Requested == 200 && "requested must equal 200"));
	assert((srcToLoadResults_3600.value().Actual == 100 && "actual value must equal 100"));
	assert((srcToLoadResults_3600.value().Available == 100 && "available must equal 100"));
	std::cout << "[Example  4] :: PASSED" << std::endl;
}

static void
Example5(bool print) {
	if (print) {
		std::cout << "Example  5:" << std::endl;
	}
	std::vector<TimeAndLoad> timesAndLoads = {};
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
	auto results = Simulate(m, print);
	auto srcToConv1Results = ModelResults_GetFlowForConnection(m, srcToConv1Conn, 0.0, results);
	auto conv1ToLoad1Results = ModelResults_GetFlowForConnection(m, conv1ToLoad1Conn, 0.0, results);
	auto conv1ToConv2Results = ModelResults_GetFlowForConnection(m, conv1ToConv2Conn, 0.0, results);
	auto conv2ToLoad2Results = ModelResults_GetFlowForConnection(m, conv2ToLoad2Conn, 0.0, results);
	auto conv2ToConv3Results = ModelResults_GetFlowForConnection(m, conv2ToConv3Conn, 0.0, results);
	auto conv3ToLoad3Results = ModelResults_GetFlowForConnection(m, conv3ToLoad3Conn, 0.0, results);
	assert((srcToConv1Results.value().Actual == 40 && "src to conv1 should flow 40"));
	assert((conv1ToLoad1Results.value().Actual == 10 && "conv1 to load1 should flow 10"));
	assert((conv1ToConv2Results.value().Actual == 28 && "conv1 to conv2 should flow 28"));
	assert((conv2ToLoad2Results.value().Actual == 7 && "conv1 to conv2 should flow 7"));
	assert((conv2ToConv3Results.value().Actual == 20 && "conv2 to conv3 should flow 21"));
	assert((conv3ToLoad3Results.value().Actual == 5 && "conv3 to load3 should flow 5"));
	std::cout << "[Example  5] :: PASSED" << std::endl;
}

static void
Example6(bool doPrint) {
	if (doPrint) {
		std::cout << "[Example  6:" << std::endl;
	}
	// Example with a Mux
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
	auto src1ToMuxResults = ModelResults_GetFlowForConnection(m, src1ToMuxConn, 0.0, results);
	assert((src1ToMuxResults.value().Actual == 10 && "src1 -> mux expected actual flow of 10"));
	auto src2ToMuxResults = ModelResults_GetFlowForConnection(m, src2ToMuxConn, 0.0, results);
	assert((src2ToMuxResults.value().Actual == 50 && "src2 -> mux expected actual flow of 50"));
	auto muxToLoad1Results = ModelResults_GetFlowForConnection(m, muxToLoad1Conn, 0.0, results);
	assert((muxToLoad1Results.value().Actual == 10 && "mux -> load1 expected actual flow of 10"));
	auto muxToLoad2Results = ModelResults_GetFlowForConnection(m, muxToLoad2Conn, 0.0, results);
	assert((muxToLoad2Results.value().Actual == 50 && "mux -> load2 expected actual flow of 50"));
	std::cout << (doPrint ? "           ]" : "[ Example  6]") << " :: PASSED" << std::endl;
}

int
main(int argc, char** argv) {
	Example1(false);
	Example2(false);
	Example3(false);
	Example3A(false);
	Example4(false);
	Example5(false);
	Example6(true);
	return EXIT_SUCCESS;
}
