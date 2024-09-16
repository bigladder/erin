/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_reliability.h"
#include "erin_next/erin_next_simulation.h"
#include "erin_next/erin_next_timestate.h"
#include <gtest/gtest.h>
#include <unordered_map>

using namespace erin;

std::unordered_map<size_t, std::vector<TimeState>>
RunCreateFailureSchedules(double initialAge_s, double scenarioOffset_s)
{
    size_t compId = 0;
    DistributionSystem ds{};
    ReliabilityCoordinator rc{};
    size_t breakDistId = ds.add_fixed("break", 10.0);
    size_t fixDistId = ds.add_fixed("fix", 2.0);
    size_t fmId = rc.add_failure_mode("fm", breakDistId, fixDistId);
    size_t compFmId = rc.link_component_with_failure_mode(compId, fmId);
    std::vector<size_t> componentFailureModeComponentIds{};
    componentFailureModeComponentIds.push_back(compId);
    std::vector<size_t> componentFailureModeFailureModeIds{};
    componentFailureModeFailureModeIds.push_back(fmId);
    std::vector<double> componentInitialAges_s{};
    componentInitialAges_s.push_back(initialAge_s);
    double scenarioDuration_s = 144.0;
    return erin::CreateFailureSchedules(
            componentFailureModeComponentIds,
            componentFailureModeFailureModeIds,
            componentInitialAges_s,
            rc,
            []() { return 0.5; },
            ds,
            scenarioDuration_s,
            scenarioOffset_s
        );
}

TEST(ErinSim, TestCreateFailureSchedules)
{
    std::unordered_map<size_t, std::vector<TimeState>> actual =
        RunCreateFailureSchedules(0.0, 0.0);
    EXPECT_EQ(actual.size(), 1);
    for (auto const& it : actual)
    {
        std::vector<TimeState> const& tss = it.second;
        EXPECT_EQ(tss.size(), 24);
        EXPECT_EQ(tss[0].time, 10.0) << tss[0];
        EXPECT_EQ(tss[0].state, false) << tss[0];
        EXPECT_EQ(tss[23].time, 144.0) << tss[23];
        EXPECT_EQ(tss[23].state, true) << tss[23];
    }
}

TEST(ErinSim, TestCreateFailureSchedulesWithOffset)
{
    std::unordered_map<size_t, std::vector<TimeState>> actual =
        RunCreateFailureSchedules(0.0, 24.0);
    EXPECT_EQ(actual.size(), 1);
    for (auto const& it : actual)
    {
        std::vector<TimeState> const& tss = it.second;
        EXPECT_EQ(tss.size(), 28);
        EXPECT_EQ(tss[0].time, 10.0) << tss[0];
        EXPECT_EQ(tss[0].state, false) << tss[0];
        EXPECT_EQ(tss[27].time, 168.0) << tss[27];
        EXPECT_EQ(tss[27].state, true) << tss[27];
    }
}

TEST(ErinSim, TestCreateFailureSchedulesWithInitialAge)
{
    std::unordered_map<size_t, std::vector<TimeState>> actual =
        RunCreateFailureSchedules(24.0, 0.0);
    EXPECT_EQ(actual.size(), 1);
    for (auto const& it : actual)
    {
        std::vector<TimeState> const& tss = it.second;
        EXPECT_EQ(tss.size(), 28);
        EXPECT_EQ(tss[0].time, 10.0) << tss[0];
        EXPECT_EQ(tss[0].state, false) << tss[0];
        EXPECT_EQ(tss[27].time, 168.0) << tss[27];
        EXPECT_EQ(tss[27].state, true) << tss[27];
    }
}

TEST(ErinSim, TestCreateFailureSchedulesWithInitialAgeAndOffset)
{
    std::unordered_map<size_t, std::vector<TimeState>> actual =
        RunCreateFailureSchedules(12.0, 12.0);
    EXPECT_EQ(actual.size(), 1);
    for (auto const& it : actual)
    {
        std::vector<TimeState> const& tss = it.second;
        EXPECT_EQ(tss.size(), 28);
        EXPECT_EQ(tss[0].time, 10.0) << tss[0];
        EXPECT_EQ(tss[0].state, false) << tss[0];
        EXPECT_EQ(tss[27].time, 168.0) << tss[27];
        EXPECT_EQ(tss[27].state, true) << tss[27];
    }
}
