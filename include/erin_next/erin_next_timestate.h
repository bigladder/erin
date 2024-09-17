/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
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
        std::set<size_t> failureModeCauses;
        std::set<size_t> fragilityModeCauses;
    };

    std::ostream&
    operator<<(std::ostream& os, TimeState const& ts);

    bool
    operator==(TimeState const& a, TimeState const& b);

    bool
    operator!=(TimeState const& a, TimeState const& b);

    std::vector<TimeState>
    TimeState_Combine(
        std::vector<TimeState> const& a,
        std::vector<TimeState> const& b
    );

    std::vector<TimeState>
    TimeState_Clip(
        std::vector<TimeState> const& input,
        double startTime_s,
        double endTime_s,
        bool rezeroTime
    );

    std::vector<TimeState>
    TimeState_Translate(std::vector<TimeState> const& input, double dt_s);

    TimeState
    TimeState_Copy(TimeState const& ts);

    double
    TimeState_CalcAvailability_s(
        std::vector<TimeState> const& tss,
        double endTime_s
    );

    TimeState
    TimeState_GetActiveTimeState(
        std::vector<TimeState> const& tss,
        double time_s
    );

    void
    TimeState_CountAndTimeFailureEvents(
        std::vector<TimeState> const& tss,
        double finalTime_s,
        std::map<size_t, size_t>& eventCountsByFailureModeId,
        std::map<size_t, size_t>& eventCountsByFragilityModeId,
        std::map<size_t, double>& timeByFailureModeId_s,
        std::map<size_t, double>& timeByFragilityModeId_s
    );

    void
    TimeState_Print(std::vector<TimeState> const& tss);

} // namespace erin

#endif
