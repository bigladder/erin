/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next.h"
#include <gtest/gtest.h>

using namespace ::erin;

TEST(Switch, TestGetSwitchState)
{
    SimulationState ss = {};
    ss.SwitchStates.push_back(SwitchState::Primary);
    size_t idx = 0;
    SwitchState actualSwitchState = SimulationState_GetSwitchState(ss, idx);
    SwitchState expectedSwitchState = SwitchState::Primary;
    EXPECT_EQ(expectedSwitchState, actualSwitchState);
}

TEST(Switch, TestSetSwitchState)
{
    SimulationState ss = {};
    ss.SwitchStates.push_back(SwitchState::Primary);
    size_t idx = 0;
    SimulationState_SetSwitchState(ss, idx, SwitchState::Secondary);
    SwitchState actualSwitchState = SimulationState_GetSwitchState(ss, idx);
    SwitchState expectedSwitchState = SwitchState::Secondary;
    EXPECT_EQ(expectedSwitchState, actualSwitchState);
}

TEST(Switch, TestAddingSwitchToModel)
{
    Model m = {};
    EXPECT_EQ(m.ComponentMap.CompType.size(), 0);
    EXPECT_EQ(m.Switches.size(), 0);
    auto switchIdx = Model_AddSwitch(m, 0, "ATS");
    EXPECT_EQ(m.ComponentMap.CompType.size(), 1);
    EXPECT_EQ(m.Switches.size(), 1);
    EXPECT_EQ(switchIdx, 0);
}

TEST(Switch, TestSimulateSwitchNoLogic)
{
    Model m = {};
    auto src0 = Model_AddConstantSource(m, 100, 0, "src0");
    auto src1 = Model_AddConstantSource(m, 250, 0, "src1");
    auto switchIdx = Model_AddSwitch(m, 0, "ATS");
    auto load = Model_AddConstantLoad(m, 200);
    auto src0ToSwitch = Model_AddConnection(m, src0, 0, switchIdx, 0, true);
    auto src1ToSwitch = Model_AddConnection(m, src1, 0, switchIdx, 1, true);
    auto switchToLoad = Model_AddConnection(m, switchIdx, 0, load, 0, true);
    auto results = Simulate(m, false, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].Time, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].Flows.size(), 3) << "size of flows must equal 3";
    auto switchToLoadResults =
        ModelResults_GetFlowForConnection(m, switchToLoad, 0.0, results);
    EXPECT_TRUE(switchToLoadResults.has_value());
    EXPECT_EQ(switchToLoadResults.value().Requested_W, 200);
    EXPECT_EQ(switchToLoadResults.value().Available_W, 100);
    EXPECT_EQ(switchToLoadResults.value().Actual_W, 100);
    auto src0ToPrimaryResults =
        ModelResults_GetFlowForConnection(m, src0ToSwitch, 0.0, results);
    EXPECT_TRUE(src0ToPrimaryResults.has_value());
    EXPECT_EQ(src0ToPrimaryResults.value().Requested_W, 200);
    EXPECT_EQ(src0ToPrimaryResults.value().Available_W, 100);
    EXPECT_EQ(src0ToPrimaryResults.value().Actual_W, 100);
    auto src1ToSecondaryResults =
        ModelResults_GetFlowForConnection(m, src1ToSwitch, 0.0, results);
    EXPECT_TRUE(src1ToSecondaryResults.has_value());
    EXPECT_EQ(src1ToSecondaryResults.value().Requested_W, 0);
    EXPECT_EQ(src1ToSecondaryResults.value().Available_W, 250);
    EXPECT_EQ(src1ToSecondaryResults.value().Actual_W, 0);
}

TEST(Switch, TestSimulateSwitchWithLogic)
{
    Model m = {};
    auto src0 = Model_AddConstantSource(m, 100, 0, "src0");
    auto src1 = Model_AddConstantSource(m, 250, 0, "src1");
    auto switchIdx = Model_AddSwitch(m, 0, "ATS");
    auto load = Model_AddConstantLoad(m, 200);
    auto src0ToSwitch = Model_AddConnection(m, src0, 0, switchIdx, 0, true);
    auto src1ToSwitch = Model_AddConnection(m, src1, 0, switchIdx, 1, true);
    auto switchToLoad = Model_AddConnection(m, switchIdx, 0, load, 0, true);
    auto results = Simulate(m, false, true);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].Time, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].Flows.size(), 3) << "size of flows must equal 3";
    auto switchToLoadResults =
        ModelResults_GetFlowForConnection(m, switchToLoad, 0.0, results);
    EXPECT_TRUE(switchToLoadResults.has_value());
    EXPECT_EQ(switchToLoadResults.value().Requested_W, 200);
    EXPECT_EQ(switchToLoadResults.value().Available_W, 250);
    EXPECT_EQ(switchToLoadResults.value().Actual_W, 200);
    auto src0ToPrimaryResults =
        ModelResults_GetFlowForConnection(m, src0ToSwitch, 0.0, results);
    EXPECT_TRUE(src0ToPrimaryResults.has_value());
    EXPECT_EQ(src0ToPrimaryResults.value().Requested_W, 0);
    EXPECT_EQ(src0ToPrimaryResults.value().Available_W, 100);
    EXPECT_EQ(src0ToPrimaryResults.value().Actual_W, 0);
    auto src1ToSecondaryResults =
        ModelResults_GetFlowForConnection(m, src1ToSwitch, 0.0, results);
    EXPECT_TRUE(src1ToSecondaryResults.has_value());
    EXPECT_EQ(src1ToSecondaryResults.value().Requested_W, 200);
    EXPECT_EQ(src1ToSecondaryResults.value().Available_W, 250);
    EXPECT_EQ(src1ToSecondaryResults.value().Actual_W, 200);
}
