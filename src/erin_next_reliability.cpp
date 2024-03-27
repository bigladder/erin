/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_reliability.h"
#include <iostream>
#include <utility>
#include <stdexcept>

namespace erin
{

ReliabilityCoordinator::ReliabilityCoordinator() : fms {}, fm_comp_links {} {}

size_t ReliabilityCoordinator::add_failure_mode(const std::string& tag,
                                                const size_t& failure_dist_id,
                                                const size_t& repair_dist_id)
{
    auto id {fms.tag.size()};
    fms.tag.emplace_back(tag);
    fms.failure_dist.emplace_back(failure_dist_id);
    fms.repair_dist.emplace_back(repair_dist_id);
    return id;
}

size_t ReliabilityCoordinator::link_component_with_failure_mode(const size_t& component_id,
                                                                const size_t& failure_mode_id)
{
    size_t idx = fm_comp_links.component_id.size();
    fm_comp_links.component_id.emplace_back(component_id);
    fm_comp_links.failure_mode_id.emplace_back(failure_mode_id);
    fm_comp_links.schedules.emplace_back(std::vector<TimeState> {});
    return idx;
}

double ReliabilityCoordinator::calc_next_event(size_t link_id,
                                               double dt_fm,
                                               const std::function<double()>& rand_fn,
                                               const DistributionSystem& cds,
                                               bool is_failure) const
{
    const auto& fm_id = fm_comp_links.failure_mode_id.at(link_id);
    size_t dist_id = is_failure ? fms.failure_dist.at(fm_id) : fms.repair_dist.at(fm_id);
    auto dt = cds.next_time_advance(dist_id, rand_fn());
    if ((dt_fm == -1.0) || (dt >= 0.0 && dt < dt_fm))
    {
        dt_fm = dt;
    }
    return dt_fm;
}

bool ReliabilityCoordinator::update_single_schedule(double& time,
                                                    double& dt,
                                                    std::vector<TimeState>& schedule,
                                                    double final_time,
                                                    bool next_state) const
{
    bool is_finished {false};
    if (time > final_time)
    {
        is_finished = true;
        return is_finished;
    }
    time += dt;
    dt = -1.0; // TODO: rename as infinity
    schedule.push_back(TimeState {time, next_state, {}, {}});
    is_finished = time >= final_time;
    return is_finished;
}

std::vector<TimeState>
ReliabilityCoordinator::make_schedule_for_link(size_t link_id,
                                               const std::function<double()>& rand_fn,
                                               const DistributionSystem& cds,
                                               double final_time)
{
    double time = 0.0;
    double dt = -1.0;
    std::vector<TimeState> reliability_schedule;
    while (true)
    {
        dt = calc_next_event(link_id, dt, rand_fn, cds, true);
        bool gteq_final_time =
            update_single_schedule(time, dt, reliability_schedule, final_time, false);
        if (gteq_final_time)
        {
            break;
        }
        dt = calc_next_event(link_id, dt, rand_fn, cds, false);
        gteq_final_time = update_single_schedule(time, dt, reliability_schedule, final_time, true);
        if (gteq_final_time)
        {
            break;
        }
    }
    return reliability_schedule;
}

std::vector<TimeState>
clip_schedule_to(std::vector<TimeState>& schedule, double start_time, double end_time)
{
    std::vector<TimeState> new_schedule {};
    bool state {true};
    for (const auto& ts : schedule)
    {
        if (ts.time < start_time)
        {
            state = ts.state;
            continue;
        }
        else if (ts.time == start_time)
        {
            new_schedule.emplace_back(TimeState {0.0, ts.state, {}, {}});
        }
        else if ((ts.time > start_time) && (ts.time <= end_time))
        {
            if (new_schedule.size() == 0)
            {
                new_schedule.emplace_back(TimeState {0.0, state, {}, {}});
            }
            new_schedule.emplace_back(TimeState {ts.time - start_time, ts.state, {}, {}});
        }
        else if (ts.time > end_time)
        {
            break;
        }
    }
    return new_schedule;
}

bool schedule_state_at_time(const std::vector<TimeState>& schedule, double time, bool initial_value)
{
    bool flag {initial_value};
    if (schedule.size() > 0)
    {
        for (const auto& ts : schedule)
        {
            if (time >= ts.time)
            {
                flag = ts.state;
            }
            if (time < ts.time)
            {
                break;
            }
        }
    }
    return flag;
}

} // namespace erin
