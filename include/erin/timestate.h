// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_TIMESTATE_H
#define ERIN_TIMESTATE_H
#include <iostream>
#include <vector>
#include <set>
#include <map>

namespace erin
{

struct TimeState
{
    double time = 0.0;
    bool state = true;
    std::set<size_t> failure_mode_causes;
    std::set<size_t> fragility_mode_causes;
};

std::ostream& operator<<(std::ostream& os, TimeState const& ts);

bool operator==(TimeState const& a, TimeState const& b);

bool operator!=(TimeState const& a, TimeState const& b);

std::vector<TimeState> combine(std::vector<TimeState> const& a, std::vector<TimeState> const& b);

std::vector<TimeState>
clip(std::vector<TimeState> const& input, double start_time_s, double end_time_s, bool rezero_time);

std::vector<TimeState> translate(std::vector<TimeState> const& input, double dt_s);

TimeState copy(TimeState const& ts);

double calculate_availability_s(std::vector<TimeState> const& tss, double endTime_s);

TimeState get_active_time_state(std::vector<TimeState> const& tss, double time_s);

void count_and_time_failure_events(std::vector<TimeState> const& tss,
                                   double final_time_s,
                                   std::map<size_t, size_t>& event_cunts_by_failure_mode_id,
                                   std::map<size_t, size_t>& event_counts_by_fragility_mode_id,
                                   std::map<size_t, double>& time_by_failure_mode_id_s,
                                   std::map<size_t, double>& time_by_fragility_mode_id_s);

void print(std::vector<TimeState> const& tss);

std::string to_string(TimeState const& ts);

} // namespace erin

#endif
