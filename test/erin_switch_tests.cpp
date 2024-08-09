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

TEST(Switch, TestRunningSwitchBackward)
{
    Model m = {};
    auto src0 = Model_AddConstantSource(m, 100, 0, "src0");
    auto src1 = Model_AddConstantSource(m, 50, 0, "src1");
    auto switchIdx = Model_AddSwitch(m, 0, "ATS");
    auto load = Model_AddConstantLoad(m, 200);
}
