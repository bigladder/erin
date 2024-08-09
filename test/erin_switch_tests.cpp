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

TEST(Switch, TestModelIntegration)
{
    Model m = {};
    auto switchId = Model_AddSwitch(m, 0, "ATS");
}
