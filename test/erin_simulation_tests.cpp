/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_reliability.h"
#include "erin_next/erin_next_simulation.h"
#include <gtest/gtest.h>

using namespace erin;

TEST(ErinSim, TestCreateFailureSchedules)
{
    size_t compId = 0;
    double initialAge_s = 0.0;
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
    double scenarioOffset_s = 0.0;
    std::unordered_map<size_t, std::vector<TimeState>> actual =
        erin::CreateFailureSchedules(
            componentFailureModeComponentIds,
            componentFailureModeFailureModeIds,
            componentInitialAges_s,
            rc,
            []() { return 0.5; },
            ds,
            scenarioDuration_s,
            scenarioOffset_s
        );
    EXPECT_EQ(actual.size(), 1);
}
