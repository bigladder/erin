// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <optional>
#include <unordered_map>

#include <gtest/gtest.h>

#include "erin/all.h"

using namespace erin;

std::unordered_map<size_t, std::vector<TimeState>>
run_create_failure_schedules(double initial_age_s, double scenario_offset_s)
{
    size_t comp_id = 0;
    DistributionSystem ds {};
    ReliabilityCoordinator rc {};
    size_t break_dist_id = ds.add_fixed("break", 10.0);
    size_t fix_dist_id = ds.add_fixed("fix", 2.0);
    size_t fm_id = rc.add_failure_mode("fm", break_dist_id, fix_dist_id);
    rc.link_component_with_failure_mode(comp_id, fm_id);
    std::vector<size_t> component_failure_mode_component_ids {};
    component_failure_mode_component_ids.push_back(comp_id);
    std::vector<size_t> component_failure_mode_failure_mode_ids {};
    component_failure_mode_failure_mode_ids.push_back(fm_id);
    std::vector<double> component_initial_ages_s {};
    component_initial_ages_s.push_back(initial_age_s);
    double scenario_duration_s = 144.0;
    return erin::create_failure_schedules(
        component_failure_mode_component_ids,
        component_failure_mode_failure_mode_ids,
        component_initial_ages_s,
        rc,
        []() { return 0.5; },
        ds,
        scenario_duration_s,
        scenario_offset_s);
}

TEST(ErinSim, TestCreateFailureSchedules)
{
    std::unordered_map<size_t, std::vector<TimeState>> actual =
        run_create_failure_schedules(0.0, 0.0);
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
        run_create_failure_schedules(0.0, 24.0);
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
        run_create_failure_schedules(24.0, 0.0);
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
        run_create_failure_schedules(12.0, 12.0);
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

std::vector<ScheduleBasedReliability> run_apply_reliabilities_and_fragilities(
    double scenario_offset_s,
    double scenario_duration_s,
    bool do_repair,
    double initial_age_s,
    std::unordered_map<size_t, std::vector<TimeState>> const& rel_sch_by_comp_id)
{
    // NOTE: our network is one source feeding electricity
    // into one load. That is, S -> L
    std::function<double()> rand_fn = [] { return 0.5; };
    std::vector<size_t> component_failure_mode_component_ids {};
    for (auto const& pair : rel_sch_by_comp_id)
    {
        component_failure_mode_component_ids.push_back(pair.first);
    }
    std::vector<double> component_initial_ages_s = {initial_age_s, 0.0};
    std::vector<std::string> component_tags = {"S", "L"};
    std::vector<size_t> component_fragility_component_ids = {0};
    std::vector<size_t> component_fragility_fragility_mode_ids = {0};
    std::vector<size_t> fragility_mode_fragility_curve_id = {0};
    DistributionSystem ds {};
    std::vector<std::optional<size_t>> fragility_mode_repair_dist_ids {};
    if (do_repair)
    {
        size_t repair_id = ds.add_fixed("repair", 0.1 * scenario_duration_s);
        fragility_mode_repair_dist_ids.push_back(repair_id);
    }
    else
    {
        fragility_mode_repair_dist_ids.push_back({});
    }
    std::vector<std::string> fragility_mode_tags = {"vulnerable_to_wind"};
    std::vector<size_t> fragility_curve_curve_ids = {0};
    std::vector<FragilityCurveType> fragility_curve_curve_types = {FragilityCurveType::linear};
    std::vector<LinearFragilityCurve> linear_fragility_curves = {
        {.vulnerability_id = 0, .lower_bound = 80.0, .upper_bound = 140.0}};
    std::vector<TabularFragilityCurve> tabular_fragility_curves = {};
    std::unordered_map<size_t, double> intensity_id_to_amount = {
        {0, 160.0},
    };
    bool verbose = false;
    Log log {};

    return apply_reliabilities_and_fragilities(rand_fn,
                                            component_failure_mode_component_ids,
                                            component_initial_ages_s,
                                            component_tags,
                                            component_fragility_component_ids,
                                            component_fragility_fragility_mode_ids,
                                            fragility_mode_fragility_curve_id,
                                            fragility_mode_repair_dist_ids,
                                            fragility_mode_tags,
                                            fragility_curve_curve_ids,
                                            fragility_curve_curve_types,
                                            linear_fragility_curves,
                                            tabular_fragility_curves,
                                            ds,
                                            scenario_offset_s,
                                            scenario_offset_s + scenario_duration_s,
                                            intensity_id_to_amount,
                                            rel_sch_by_comp_id,
                                            verbose,
                                            log);
}

TEST(ErinSim, TestFragility_NoReliability_NoRepair_NoOffset_NoAge)
{
    double scenario_offset_s = 0.0;
    double scenario_duration_s = 1'000.0;
    bool do_repair = false;
    double initial_age_s = 0.0;
    std::unordered_map<size_t, std::vector<TimeState>> rel_sch_by_comp_id {};
    std::vector<ScheduleBasedReliability> actual = run_apply_reliabilities_and_fragilities(
        scenario_offset_s, scenario_duration_s, do_repair, initial_age_s, rel_sch_by_comp_id);
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.component_id, 0);
        EXPECT_EQ(sbr.time_states.size(), 1);
        EXPECT_EQ(sbr.time_states[0].time, 0.0);
        EXPECT_EQ(sbr.time_states[0].state, false);
        EXPECT_EQ(sbr.time_states[0].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[0].fragility_mode_causes.size(), 1);
        for (size_t fmId : sbr.time_states[0].fragility_mode_causes)
        {
            EXPECT_EQ(fmId, 0);
        }
    }
}

TEST(ErinSim, TestFragility_NoReliability_Repair_NoOffset_NoAge)
{
    double scenario_offset_s = 0.0;
    double scenario_duration_s = 1'000.0;
    bool do_repair = true;
    double initial_age_s = 0.0;
    std::unordered_map<size_t, std::vector<TimeState>> rel_sch_by_comp_id {};
    std::vector<ScheduleBasedReliability> actual = run_apply_reliabilities_and_fragilities(
        scenario_offset_s, scenario_duration_s, do_repair, initial_age_s, rel_sch_by_comp_id);
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.component_id, 0);
        EXPECT_EQ(sbr.time_states.size(), 2);
        EXPECT_EQ(sbr.time_states[0].time, 0.0);
        EXPECT_EQ(sbr.time_states[0].state, false);
        EXPECT_EQ(sbr.time_states[0].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[0].fragility_mode_causes.size(), 1);
        for (size_t fmId : sbr.time_states[0].fragility_mode_causes)
        {
            EXPECT_EQ(fmId, 0);
        }
        EXPECT_DOUBLE_EQ(sbr.time_states[1].time, 100.0);
        EXPECT_EQ(sbr.time_states[1].state, true);
        EXPECT_EQ(sbr.time_states[1].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[1].fragility_mode_causes.size(), 0);
    }
}

TEST(ErinSim, TestFragility_NoReliability_NoRepair_Offset_NoAge)
{
    double scenario_offset_s = 250.0;
    double scenario_duration_s = 1'000.0;
    bool do_repair = false;
    double initial_age_s = 0.0;
    std::unordered_map<size_t, std::vector<TimeState>> rel_sch_by_comp_id {};
    std::vector<ScheduleBasedReliability> actual = run_apply_reliabilities_and_fragilities(
        scenario_offset_s, scenario_duration_s, do_repair, initial_age_s, rel_sch_by_comp_id);
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.component_id, 0);
        EXPECT_EQ(sbr.time_states.size(), 1);
        EXPECT_EQ(sbr.time_states[0].time, 0.0);
        EXPECT_EQ(sbr.time_states[0].state, false);
        EXPECT_EQ(sbr.time_states[0].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[0].fragility_mode_causes.size(), 1);
        for (size_t fmId : sbr.time_states[0].fragility_mode_causes)
        {
            EXPECT_EQ(fmId, 0);
        }
    }
}

TEST(ErinSim, TestFragility_NoReliability_NoRepair_NoOffset_Age)
{
    double scenario_offset_s = 0.0;
    double scenario_duration_s = 1'000.0;
    double initial_age_s = 2'000;
    bool do_repair = false;
    std::unordered_map<size_t, std::vector<TimeState>> rel_sch_by_comp_id {};
    std::vector<ScheduleBasedReliability> actual = run_apply_reliabilities_and_fragilities(
        scenario_offset_s, scenario_duration_s, do_repair, initial_age_s, rel_sch_by_comp_id);
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.component_id, 0);
        EXPECT_EQ(sbr.time_states.size(), 1);
        EXPECT_EQ(sbr.time_states[0].time, 0.0);
        EXPECT_EQ(sbr.time_states[0].state, false);
        EXPECT_EQ(sbr.time_states[0].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[0].fragility_mode_causes.size(), 1);
        for (size_t fmId : sbr.time_states[0].fragility_mode_causes)
        {
            EXPECT_EQ(fmId, 0);
        }
    }
}

TEST(ErinSim, TestFragility_NoReliability_NoRepair_Offset_Age)
{
    double scenario_offset_s = 250.0;
    double scenario_duration_s = 1'000.0;
    double initial_age_s = 2'000;
    bool do_repair = false;
    std::unordered_map<size_t, std::vector<TimeState>> rel_sch_by_comp_id {};
    std::vector<ScheduleBasedReliability> actual = run_apply_reliabilities_and_fragilities(
        scenario_offset_s, scenario_duration_s, do_repair, initial_age_s, rel_sch_by_comp_id);
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.component_id, 0);
        EXPECT_EQ(sbr.time_states.size(), 1);
        EXPECT_EQ(sbr.time_states[0].time, 0.0);
        EXPECT_EQ(sbr.time_states[0].state, false);
        EXPECT_EQ(sbr.time_states[0].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[0].fragility_mode_causes.size(), 1);
        for (size_t fmId : sbr.time_states[0].fragility_mode_causes)
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
            .failure_mode_causes = {0},
            .fragility_mode_causes = {},
        },
        {
            .time = 20.0,
            .state = true,
            .failure_mode_causes = {},
            .fragility_mode_causes = {},
        },
    };
    std::vector<TimeState> B = {
        {
            .time = 0.0,
            .state = false,
            .failure_mode_causes = {},
            .fragility_mode_causes = {0},
        },
    };
    std::vector<TimeState> expected = {
        {
            .time = 0.0,
            .state = false,
            .failure_mode_causes = {},
            .fragility_mode_causes = {0},
        },
        {
            .time = 10.0,
            .state = false,
            .failure_mode_causes = {0},
            .fragility_mode_causes = {0},
        },
        {
            .time = 20.0,
            .state = false,
            .failure_mode_causes = {},
            .fragility_mode_causes = {0},
        },
    };
    std::vector<TimeState> actual = combine(A, B);
    EXPECT_EQ(actual.size(), expected.size());
}

TEST(ErinSim, TestFragility_Reliability_NoRepair_NoOffset_NoAge)
{
    double scenario_offset_s = 0.0;
    double scenario_duration_s = 1'000.0;
    double initial_age_s = 0.0;
    bool do_repair = false;
    std::unordered_map<size_t, std::vector<TimeState>> rel_sch_by_comp_id {};
    std::vector<TimeState> rel_sch {};
    rel_sch.push_back({
        .time = 10.0,
        .state = false,
        .failure_mode_causes = {0},
        .fragility_mode_causes = {},
    });
    rel_sch.push_back({
        .time = 20.0,
        .state = true,
        .failure_mode_causes = {},
        .fragility_mode_causes = {},
    });
    rel_sch_by_comp_id[0] = std::move(rel_sch);
    std::vector<ScheduleBasedReliability> actual = run_apply_reliabilities_and_fragilities(
        scenario_offset_s, scenario_duration_s, do_repair, initial_age_s, rel_sch_by_comp_id);
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.component_id, 0);
        EXPECT_EQ(sbr.time_states.size(), 3);
        // 1st
        EXPECT_EQ(sbr.time_states[0].time, 0.0);
        EXPECT_EQ(sbr.time_states[0].state, false);
        EXPECT_EQ(sbr.time_states[0].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[0].fragility_mode_causes.size(), 1);
        for (size_t fm_id : sbr.time_states[0].fragility_mode_causes)
        {
            EXPECT_EQ(fm_id, 0);
        }
        // 2nd
        EXPECT_EQ(sbr.time_states[1].time, 10.0);
        EXPECT_EQ(sbr.time_states[1].state, false);
        EXPECT_EQ(sbr.time_states[1].failure_mode_causes.size(), 1);
        for (size_t fm_id : sbr.time_states[1].failure_mode_causes)
        {
            EXPECT_EQ(fm_id, 0);
        }
        EXPECT_EQ(sbr.time_states[1].fragility_mode_causes.size(), 1);
        for (size_t fm_id : sbr.time_states[1].fragility_mode_causes)
        {
            EXPECT_EQ(fm_id, 0);
        }
        // 3rd
        EXPECT_EQ(sbr.time_states[2].time, 20.0);
        EXPECT_EQ(sbr.time_states[2].state, false);
        EXPECT_EQ(sbr.time_states[2].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[2].fragility_mode_causes.size(), 1);
        for (size_t fm_id : sbr.time_states[2].fragility_mode_causes)
        {
            EXPECT_EQ(fm_id, 0);
        }
    }
}

TEST(ErinSim, TestFragility_Reliability_Repair_Offset_Age)
{
    double scenario_offset_s = 250.0;
    double scenario_duration_s = 1'000.0;
    double initial_age_s = 250.0;
    bool do_repair = false;
    std::unordered_map<size_t, std::vector<TimeState>> rel_sch_by_comp_id {};
    std::vector<TimeState> rel_sch {};
    rel_sch.push_back({
        .time = 510.0,
        .state = false,
        .failure_mode_causes = {0},
        .fragility_mode_causes = {},
    });
    rel_sch.push_back({
        .time = 520.0,
        .state = true,
        .failure_mode_causes = {},
        .fragility_mode_causes = {},
    });
    rel_sch_by_comp_id[0] = std::move(rel_sch);
    std::vector<ScheduleBasedReliability> actual = run_apply_reliabilities_and_fragilities(
        scenario_offset_s, scenario_duration_s, do_repair, initial_age_s, rel_sch_by_comp_id);
    EXPECT_EQ(actual.size(), 1);
    for (ScheduleBasedReliability const& sbr : actual)
    {
        EXPECT_EQ(sbr.component_id, 0);
        EXPECT_EQ(sbr.time_states.size(), 3);
        // 1st
        EXPECT_EQ(sbr.time_states[0].time, 0.0);
        EXPECT_EQ(sbr.time_states[0].state, false);
        EXPECT_EQ(sbr.time_states[0].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[0].fragility_mode_causes.size(), 1);
        for (size_t fm_id : sbr.time_states[0].fragility_mode_causes)
        {
            EXPECT_EQ(fm_id, 0);
        }
        // 2nd
        EXPECT_EQ(sbr.time_states[1].time, 10.0);
        EXPECT_EQ(sbr.time_states[1].state, false);
        EXPECT_EQ(sbr.time_states[1].failure_mode_causes.size(), 1);
        for (size_t fmId : sbr.time_states[1].failure_mode_causes)
        {
            EXPECT_EQ(fmId, 0);
        }
        EXPECT_EQ(sbr.time_states[1].fragility_mode_causes.size(), 1);
        for (size_t fm_id : sbr.time_states[1].fragility_mode_causes)
        {
            EXPECT_EQ(fm_id, 0);
        }
        // 3rd
        EXPECT_EQ(sbr.time_states[2].time, 20.0);
        EXPECT_EQ(sbr.time_states[2].state, false);
        EXPECT_EQ(sbr.time_states[2].failure_mode_causes.size(), 0);
        EXPECT_EQ(sbr.time_states[2].fragility_mode_causes.size(), 1);
        for (size_t fm_id : sbr.time_states[2].fragility_mode_causes)
        {
            EXPECT_EQ(fm_id, 0);
        }
    }
}
