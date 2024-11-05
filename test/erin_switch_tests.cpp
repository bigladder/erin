// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <gtest/gtest.h>

#include "erin/all.h"

using namespace ::erin;

TEST(Switch, TestGetSwitchState)
{
    SimulationState ss = {};
    ss.SwitchStates.push_back(SwitchState::primary);
    size_t idx = 0;
    SwitchState actual_switch_state = SimulationState_GetSwitchState(ss, idx);
    SwitchState expected_switch_state = SwitchState::primary;
    EXPECT_EQ(expected_switch_state, actual_switch_state);
}

TEST(Switch, TestSetSwitchState)
{
    SimulationState ss = {};
    ss.SwitchStates.push_back(SwitchState::primary);
    size_t idx = 0;
    SimulationState_SetSwitchState(ss, idx, SwitchState::secondary);
    SwitchState actual_switch_state = SimulationState_GetSwitchState(ss, idx);
    SwitchState expected_switch_state = SwitchState::secondary;
    EXPECT_EQ(expected_switch_state, actual_switch_state);
}

TEST(Switch, TestAddingSwitchToModel)
{
    Model m = {};
    EXPECT_EQ(m.component.component_type.size(), 0);
    EXPECT_EQ(m.transfer_switch.size(), 0);
    auto switch_idx = Model_AddSwitch(m, 0, "ATS");
    EXPECT_EQ(m.component.component_type.size(), 1);
    EXPECT_EQ(m.transfer_switch.size(), 1);
    EXPECT_EQ(switch_idx, 0);
}

TEST(Switch, TestSimulateSwitchNoLogic)
{
    Model m = {};
    auto src0 = Model_AddConstantSource(m, 100, 0, "src0");
    auto src1 = Model_AddConstantSource(m, 250, 0, "src1");
    auto switch_idx = Model_AddSwitch(m, 0, "ATS");
    auto load = Model_AddConstantLoad(m, 200);
    auto src0_to_switch = Model_AddConnection(m, src0, 0, switch_idx, 0, true);
    auto src1_to_switch = Model_AddConnection(m, src1, 0, switch_idx, 1, true);
    auto switch_to_load = Model_AddConnection(m, switch_idx, 0, load, 0, true);
    auto results = Simulate(m, false, false);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].time_s, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].flows.size(), 3) << "size of flows must equal 3";
    auto switch_to_load_results =
        ModelResults_GetFlowForConnection(m, switch_to_load, 0.0, results);
    EXPECT_TRUE(switch_to_load_results.has_value());
    EXPECT_EQ(switch_to_load_results.value().requested_W, 200);
    EXPECT_EQ(switch_to_load_results.value().available_W, 100);
    EXPECT_EQ(switch_to_load_results.value().actual_W, 100);
    auto src0_to_primary_results =
        ModelResults_GetFlowForConnection(m, src0_to_switch, 0.0, results);
    EXPECT_TRUE(src0_to_primary_results.has_value());
    EXPECT_EQ(src0_to_primary_results.value().requested_W, 200);
    EXPECT_EQ(src0_to_primary_results.value().available_W, 100);
    EXPECT_EQ(src0_to_primary_results.value().actual_W, 100);
    auto src1_to_secondary_results =
        ModelResults_GetFlowForConnection(m, src1_to_switch, 0.0, results);
    EXPECT_TRUE(src1_to_secondary_results.has_value());
    EXPECT_EQ(src1_to_secondary_results.value().requested_W, 0);
    EXPECT_EQ(src1_to_secondary_results.value().available_W, 250);
    EXPECT_EQ(src1_to_secondary_results.value().actual_W, 0);
}

TEST(Switch, TestSimulateSwitchWithLogic)
{
    Model m = {};
    auto src0 = Model_AddConstantSource(m, 100, 0, "src0");
    auto src1 = Model_AddConstantSource(m, 250, 0, "src1");
    auto switch_idx = Model_AddSwitch(m, 0, "ATS");
    auto load = Model_AddConstantLoad(m, 200);
    auto src0_to_switch = Model_AddConnection(m, src0, 0, switch_idx, 0, true);
    auto src1_to_switch = Model_AddConnection(m, src1, 0, switch_idx, 1, true);
    auto switch_to_load = Model_AddConnection(m, switch_idx, 0, load, 0, true);
    auto results = Simulate(m, false, true);
    EXPECT_EQ(results.size(), 1) << "output must have a size of 1";
    EXPECT_EQ(results[0].time_s, 0.0) << "time must equal 0.0";
    EXPECT_EQ(results[0].flows.size(), 3) << "size of flows must equal 3";
    auto switch_to_load_results =
        ModelResults_GetFlowForConnection(m, switch_to_load, 0.0, results);
    EXPECT_TRUE(switch_to_load_results.has_value());
    EXPECT_EQ(switch_to_load_results.value().requested_W, 200);
    EXPECT_EQ(switch_to_load_results.value().available_W, 250);
    EXPECT_EQ(switch_to_load_results.value().actual_W, 200);
    auto src0_to_primary_results =
        ModelResults_GetFlowForConnection(m, src0_to_switch, 0.0, results);
    EXPECT_TRUE(src0_to_primary_results.has_value());
    EXPECT_EQ(src0_to_primary_results.value().requested_W, 0);
    EXPECT_EQ(src0_to_primary_results.value().available_W, 100);
    EXPECT_EQ(src0_to_primary_results.value().actual_W, 0);
    auto src1_to_secondary_results =
        ModelResults_GetFlowForConnection(m, src1_to_switch, 0.0, results);
    EXPECT_TRUE(src1_to_secondary_results.has_value());
    EXPECT_EQ(src1_to_secondary_results.value().requested_W, 200);
    EXPECT_EQ(src1_to_secondary_results.value().available_W, 250);
    EXPECT_EQ(src1_to_secondary_results.value().actual_W, 200);
}
