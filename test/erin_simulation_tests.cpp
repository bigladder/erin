/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_reliability.h"
#include "erin_next/erin_next_simulation.h"
#include "erin_next/erin_next_timestate.h"
#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <optional>
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

std::vector<ScheduleBasedReliability>
RunApplyReliabilitiesAndFragilities(
    double scenarioOffset_s,
    double scenarioDuration_s,
    bool doRepair,
    double initialAge_s,
    std::unordered_map<size_t, std::vector<TimeState>> const& relSchByCompId
)
{
    // NOTE: our network is one source feeding electricity
    // into one load. That is, S -> L
    std::function<double()> randFn = [] { return 0.5; };
    std::vector<size_t> componentFailureModeComponentIds{};
    for (auto const& pair : relSchByCompId)
    {
        componentFailureModeComponentIds.push_back(pair.first);
    }
    std::vector<double> componentInitialAges_s = {initialAge_s, 0.0};
    std::vector<std::string> componentTags = {"S", "L"};
    std::vector<size_t> componentFragilityComponentIds = {0};
    std::vector<size_t> componentFragilityFragilityModeIds = {0};
    std::vector<size_t> fragilityModeFragilityCurveId = {0};
    DistributionSystem ds{};
    std::vector<std::optional<size_t>> fragilityModeRepairDistIds{};
    if (doRepair)
    {
        size_t repairId = ds.add_fixed("repair", 0.1 * scenarioDuration_s);
        fragilityModeRepairDistIds.push_back(repairId);
    }
    else
    {
        fragilityModeRepairDistIds.push_back({});
    }
    std::vector<std::string> fragilityModeTags = {"vulnerable_to_wind"};
    std::vector<size_t> fragilityCurveCurveIds = {0};
    std::vector<FragilityCurveType> fragilityCurveCurveTypes = {
        FragilityCurveType::Linear
    };
    std::vector<LinearFragilityCurve> linearFragilityCurves = {
        {.VulnerabilityId = 0, .LowerBound = 80.0, .UpperBound = 140.0}
    };
    std::vector<TabularFragilityCurve> tabularFragilityCurves = {};
    std::unordered_map<size_t, double> intensityIdToAmount = {
        {0, 160.0},
    };
    bool verbose = false;

    return ApplyReliabilitiesAndFragilities(
        randFn,
        componentFailureModeComponentIds,
        componentInitialAges_s,
        componentTags,
        componentFragilityComponentIds,
        componentFragilityFragilityModeIds,
        fragilityModeFragilityCurveId,
        fragilityModeRepairDistIds,
        fragilityModeTags,
        fragilityCurveCurveIds,
        fragilityCurveCurveTypes,
        linearFragilityCurves,
        tabularFragilityCurves,
        ds,
        scenarioOffset_s,
        scenarioOffset_s + scenarioDuration_s,
        intensityIdToAmount,
        relSchByCompId,
        verbose
    );
}

TEST(ErinSim, TestFragility_NoReliability_NoRepair_NoOffset_NoAge)
{
    double scenarioOffset_s = 0.0;
    double scenarioDuration_s = 1'000.0;
    bool doRepair = false;
    double initialAge_s = 0.0;
    std::unordered_map<size_t, std::vector<TimeState>> relSchByCompId{};
    std::vector<ScheduleBasedReliability> actual =
        RunApplyReliabilitiesAndFragilities(
            scenarioOffset_s,
            scenarioDuration_s,
            doRepair,
            initialAge_s,
            relSchByCompId
        );
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.ComponentId, 0);
        EXPECT_EQ(sbr.TimeStates.size(), 1);
        EXPECT_EQ(sbr.TimeStates[0].time, 0.0);
        EXPECT_EQ(sbr.TimeStates[0].state, false);
        EXPECT_EQ(sbr.TimeStates[0].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[0].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[0].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
    }
}

TEST(ErinSim, TestFragility_NoReliability_Repair_NoOffset_NoAge)
{
    double scenarioOffset_s = 0.0;
    double scenarioDuration_s = 1'000.0;
    bool doRepair = true;
    double initialAge_s = 0.0;
    std::unordered_map<size_t, std::vector<TimeState>> relSchByCompId{};
    std::vector<ScheduleBasedReliability> actual =
        RunApplyReliabilitiesAndFragilities(
            scenarioOffset_s,
            scenarioDuration_s,
            doRepair,
            initialAge_s,
            relSchByCompId
        );
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.ComponentId, 0);
        EXPECT_EQ(sbr.TimeStates.size(), 2);
        EXPECT_EQ(sbr.TimeStates[0].time, 0.0);
        EXPECT_EQ(sbr.TimeStates[0].state, false);
        EXPECT_EQ(sbr.TimeStates[0].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[0].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[0].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
        EXPECT_DOUBLE_EQ(sbr.TimeStates[1].time, 100.0);
        EXPECT_EQ(sbr.TimeStates[1].state, true);
        EXPECT_EQ(sbr.TimeStates[1].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[1].fragilityModeCauses.size(), 0);
    }
}

TEST(ErinSim, TestFragility_NoReliability_NoRepair_Offset_NoAge)
{
    double scenarioOffset_s = 250.0;
    double scenarioDuration_s = 1'000.0;
    bool doRepair = false;
    double initialAge_s = 0.0;
    std::unordered_map<size_t, std::vector<TimeState>> relSchByCompId{};
    std::vector<ScheduleBasedReliability> actual =
        RunApplyReliabilitiesAndFragilities(
            scenarioOffset_s,
            scenarioDuration_s,
            doRepair,
            initialAge_s,
            relSchByCompId
        );
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.ComponentId, 0);
        EXPECT_EQ(sbr.TimeStates.size(), 1);
        EXPECT_EQ(sbr.TimeStates[0].time, 0.0);
        EXPECT_EQ(sbr.TimeStates[0].state, false);
        EXPECT_EQ(sbr.TimeStates[0].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[0].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[0].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
    }
}

TEST(ErinSim, TestFragility_NoReliability_NoRepair_NoOffset_Age)
{
    double scenarioOffset_s = 0.0;
    double scenarioDuration_s = 1'000.0;
    double initialAge_s = 2'000;
    bool doRepair = false;
    std::unordered_map<size_t, std::vector<TimeState>> relSchByCompId{};
    std::vector<ScheduleBasedReliability> actual =
        RunApplyReliabilitiesAndFragilities(
            scenarioOffset_s,
            scenarioDuration_s,
            doRepair,
            initialAge_s,
            relSchByCompId
        );
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.ComponentId, 0);
        EXPECT_EQ(sbr.TimeStates.size(), 1);
        EXPECT_EQ(sbr.TimeStates[0].time, 0.0);
        EXPECT_EQ(sbr.TimeStates[0].state, false);
        EXPECT_EQ(sbr.TimeStates[0].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[0].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[0].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
    }
}

TEST(ErinSim, TestFragility_NoReliability_NoRepair_Offset_Age)
{
    double scenarioOffset_s = 250.0;
    double scenarioDuration_s = 1'000.0;
    double initialAge_s = 2'000;
    bool doRepair = false;
    std::unordered_map<size_t, std::vector<TimeState>> relSchByCompId{};
    std::vector<ScheduleBasedReliability> actual =
        RunApplyReliabilitiesAndFragilities(
            scenarioOffset_s,
            scenarioDuration_s,
            doRepair,
            initialAge_s,
            relSchByCompId
        );
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.ComponentId, 0);
        EXPECT_EQ(sbr.TimeStates.size(), 1);
        EXPECT_EQ(sbr.TimeStates[0].time, 0.0);
        EXPECT_EQ(sbr.TimeStates[0].state, false);
        EXPECT_EQ(sbr.TimeStates[0].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[0].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[0].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
    }
}

TEST(TimeState, TestTimeStateCombine)
{
    std::vector<TimeState> A = {
        {
            .time = 10.0,
            .state = false,
            .failureModeCauses = {0},
            .fragilityModeCauses = {},
        },
        {
            .time = 20.0,
            .state = true,
            .failureModeCauses = {},
            .fragilityModeCauses = {},
        },
    };
    std::vector<TimeState> B = {
        {
            .time = 0.0,
            .state = false,
            .failureModeCauses = {},
            .fragilityModeCauses = {0},
        },
    };
    std::vector<TimeState> expected = {
        {
            .time = 0.0,
            .state = false,
            .failureModeCauses = {},
            .fragilityModeCauses = {0},
        },
        {
            .time = 10.0,
            .state = false,
            .failureModeCauses = {0},
            .fragilityModeCauses = {0},
        },
        {
            .time = 20.0,
            .state = false,
            .failureModeCauses = {},
            .fragilityModeCauses = {0},
        },
    };
    std::vector<TimeState> actual = TimeState_Combine(A, B);
    EXPECT_EQ(actual.size(), expected.size());
}

TEST(ErinSim, TestFragility_Reliability_NoRepair_NoOffset_NoAge)
{
    double scenarioOffset_s = 0.0;
    double scenarioDuration_s = 1'000.0;
    double initialAge_s = 0.0;
    bool doRepair = false;
    std::unordered_map<size_t, std::vector<TimeState>> relSchByCompId{};
    std::vector<TimeState> relSch{};
    relSch.push_back({
        .time = 10.0,
        .state = false,
        .failureModeCauses = {0},
        .fragilityModeCauses = {},
    });
    relSch.push_back({
        .time = 20.0,
        .state = true,
        .failureModeCauses = {},
        .fragilityModeCauses = {},
    });
    relSchByCompId[0] = std::move(relSch);
    std::vector<ScheduleBasedReliability> actual =
        RunApplyReliabilitiesAndFragilities(
            scenarioOffset_s,
            scenarioDuration_s,
            doRepair,
            initialAge_s,
            relSchByCompId
        );
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.ComponentId, 0);
        EXPECT_EQ(sbr.TimeStates.size(), 3);
        // 1st
        EXPECT_EQ(sbr.TimeStates[0].time, 0.0);
        EXPECT_EQ(sbr.TimeStates[0].state, false);
        EXPECT_EQ(sbr.TimeStates[0].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[0].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[0].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
        // 2nd
        EXPECT_EQ(sbr.TimeStates[1].time, 10.0);
        EXPECT_EQ(sbr.TimeStates[1].state, false);
        EXPECT_EQ(sbr.TimeStates[1].failureModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[1].failureModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
        EXPECT_EQ(sbr.TimeStates[1].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[1].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
        // 3rd
        EXPECT_EQ(sbr.TimeStates[2].time, 20.0);
        EXPECT_EQ(sbr.TimeStates[2].state, false);
        EXPECT_EQ(sbr.TimeStates[2].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[2].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[2].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
    }
}

TEST(ErinSim, TestFragility_Reliability_Repair_Offset_Age)
{
    double scenarioOffset_s = 250.0;
    double scenarioDuration_s = 1'000.0;
    double initialAge_s = 250.0;
    bool doRepair = false;
    std::unordered_map<size_t, std::vector<TimeState>> relSchByCompId{};
    std::vector<TimeState> relSch{};
    relSch.push_back({
        .time = 510.0,
        .state = false,
        .failureModeCauses = {0},
        .fragilityModeCauses = {},
    });
    relSch.push_back({
        .time = 520.0,
        .state = true,
        .failureModeCauses = {},
        .fragilityModeCauses = {},
    });
    relSchByCompId[0] = std::move(relSch);
    std::vector<ScheduleBasedReliability> actual =
        RunApplyReliabilitiesAndFragilities(
            scenarioOffset_s,
            scenarioDuration_s,
            doRepair,
            initialAge_s,
            relSchByCompId
        );
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.ComponentId, 0);
        EXPECT_EQ(sbr.TimeStates.size(), 3);
        // 1st
        EXPECT_EQ(sbr.TimeStates[0].time, 0.0);
        EXPECT_EQ(sbr.TimeStates[0].state, false);
        EXPECT_EQ(sbr.TimeStates[0].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[0].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[0].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
        // 2nd
        EXPECT_EQ(sbr.TimeStates[1].time, 10.0);
        EXPECT_EQ(sbr.TimeStates[1].state, false);
        EXPECT_EQ(sbr.TimeStates[1].failureModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[1].failureModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
        EXPECT_EQ(sbr.TimeStates[1].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[1].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
        // 3rd
        EXPECT_EQ(sbr.TimeStates[2].time, 20.0);
        EXPECT_EQ(sbr.TimeStates[2].state, false);
        EXPECT_EQ(sbr.TimeStates[2].failureModeCauses.size(), 0);
        EXPECT_EQ(sbr.TimeStates[2].fragilityModeCauses.size(), 1);
        for (size_t fmId : sbr.TimeStates[2].fragilityModeCauses)
        {
            EXPECT_EQ(fmId, 0);
        }
    }
}
