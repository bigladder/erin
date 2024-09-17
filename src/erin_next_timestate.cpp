/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_timestate.h"
#include <assert.h>

namespace erin
{

    std::ostream&
    operator<<(std::ostream& os, const TimeState& ts)
    {
        os << "TimeState(" << "time=" << ts.time << ", " << "state=" << ts.state
           << ", failureModeCauses={";
        bool first = true;
        for (auto const& x : ts.failureModeCauses)
        {
            os << (first ? "" : ",") << x;
            first = false;
        }
        os << "}, fragilityModeCauses={";
        first = true;
        for (auto const& x : ts.fragilityModeCauses)
        {
            os << (first ? "" : ",") << x;
            first = false;
        }
        os << "})";
        return os;
    }

    bool
    operator==(TimeState const& a, TimeState const& b)
    {
        bool result = true;
        result = result && a.time == b.time;
        result = result && a.state == b.state;
        result =
            result && a.failureModeCauses.size() == b.failureModeCauses.size();
        result = result
            && a.fragilityModeCauses.size() == b.fragilityModeCauses.size();
        if (result)
        {
            for (auto const& aFm : a.failureModeCauses)
            {
                result = result && b.failureModeCauses.contains(aFm);
                if (!result)
                {
                    break;
                }
            }
        }
        if (result)
        {
            for (auto const& aFm : a.fragilityModeCauses)
            {
                result = result && b.fragilityModeCauses.contains(aFm);
                if (!result)
                {
                    break;
                }
            }
        }
        return result;
    }

    bool
    operator!=(TimeState const& a, TimeState const& b)
    {
        return !(a == b);
    }

    std::vector<TimeState>
    TimeState_Combine(
        std::vector<TimeState> const& a,
        std::vector<TimeState> const& b
    )
    {
        std::vector<TimeState> result;
        if (a.size() == 0 && b.size() > 0)
        {
            result.reserve(b.size());
            for (size_t i = 0; i < b.size(); ++i)
            {
                result.push_back(TimeState_Copy(b.at(i)));
            }
            return result;
        }
        else if (a.size() > 0 && b.size() == 0)
        {
            result.reserve(a.size());
            for (size_t i = 0; i < a.size(); ++i)
            {
                result.push_back(TimeState_Copy(a.at(i)));
            }
            return result;
        }
        else if (a.size() == 0 && b.size() == 0)
        {
            return result;
        }
        size_t aIdx = 0;
        size_t bIdx = 0;
        double time = 0.0;
        bool state = true;
        size_t iter = 0;
        while (true)
        {
            TimeState const& nextA = a.at(aIdx);
            TimeState const& nextB = b.at(bIdx);
            std::set<size_t> failureModes{};
            std::set<size_t> fragilityModes{};
            if (time >= nextA.time && time >= nextB.time)
            {
                state = nextA.state && nextB.state;
            }
            else if (time >= nextA.time)
            {
                state = nextA.state;
            }
            else if (time >= nextB.time)
            {
                state = nextB.state;
            }
            else
            {
                if (nextA.time < nextB.time)
                {
                    time = nextA.time;
                    state = nextA.state;
                }
                else if (nextB.time < nextA.time)
                {
                    time = nextB.time;
                    state = nextB.state;
                }
                else
                {
                    time = nextA.time;
                    state = nextA.state && nextB.state;
                }
            }
            if (time >= nextA.time && !nextA.state)
            {
                for (auto const& fmA : nextA.failureModeCauses)
                {
                    failureModes.insert(fmA);
                }
                for (auto const& fmA : nextA.fragilityModeCauses)
                {
                    fragilityModes.insert(fmA);
                }
            }
            if (time >= nextB.time && !nextB.state)
            {
                for (auto const& fmB : nextB.failureModeCauses)
                {
                    failureModes.insert(fmB);
                }
                for (auto const& fmB : nextB.fragilityModeCauses)
                {
                    fragilityModes.insert(fmB);
                }
            }
            result.push_back({
                .time=time,
                .state=state,
                .failureModeCauses=std::move(failureModes),
                .fragilityModeCauses=std::move(fragilityModes),
            });
            // increment to the lowest time (ta or tb) ahead of t
            bool aCanInc = (aIdx + 1) < a.size();
            bool bCanInc = (bIdx + 1) < b.size();
            size_t nextAIdx = aIdx;
            size_t nextBIdx = bIdx;
            double nextATime = a.at(aIdx).time;
            double nextBTime = b.at(bIdx).time;
            if (a.at(aIdx).time <= time && aCanInc)
            {
                nextATime = a.at(aIdx + 1).time;
                nextAIdx = aIdx + 1;
            }
            if (b.at(bIdx).time <= time && bCanInc)
            {
                nextBTime = b.at(bIdx + 1).time;
                nextBIdx = bIdx + 1;
            }
            if (nextATime > time && nextBTime > time)
            {
                if (nextATime < nextBTime)
                {
                    aIdx = nextAIdx;
                    time = nextATime;
                }
                else if (nextBTime < nextATime)
                {
                    bIdx = nextBIdx;
                    time = nextBTime;
                }
                else
                {
                    aIdx = nextAIdx;
                    bIdx = nextBIdx;
                    time = nextATime;
                }
            }
            else if (nextATime > time)
            {
                aIdx = nextAIdx;
                time = nextATime;
            }
            else if (nextBTime > time)
            {
                bIdx = nextBIdx;
                time = nextBTime;
            }
            else
            {
                break;
            }
            ++iter;
        }
        return result;
    }

    std::vector<TimeState>
    TimeState_Clip(
        std::vector<TimeState> const& input,
        double startTime_s,
        double endTime_s,
        bool rezeroTime
    )
    {
        assert(startTime_s <= endTime_s);
        std::vector<TimeState> result;
        size_t firstIdx = 0;
        bool foundFirst = false;
        for (size_t i = 0; i < input.size(); ++i)
        {
            TimeState const& ts = input[i];
            if (ts.time < startTime_s)
            {
                foundFirst = true;
                firstIdx = i;
            }
            else if (ts.time > endTime_s)
            {
                break;
            }
            else
            {
                if (result.size() == 0 && ts.time > startTime_s && foundFirst)
                {
                    TimeState firstTs = TimeState_Copy(input[firstIdx]);
                    firstTs.time = rezeroTime ? 0.0 : startTime_s;
                    result.push_back(std::move(firstTs));
                }
                TimeState newTs = TimeState_Copy(input[i]);
                if (rezeroTime)
                {
                    newTs.time = newTs.time - startTime_s;
                }
                result.push_back(std::move(newTs));
            }
        }
        return result;
    }

    std::vector<TimeState>
    TimeState_Translate(std::vector<TimeState> const& input, double dt_s)
    {
        if (dt_s == 0.0)
        {
            return input;
        }
        std::vector<TimeState> result;
        result.reserve(input.size());
        for (auto const& ts : input)
        {
            TimeState newTs = TimeState_Copy(ts);
            newTs.time -= dt_s;
            result.push_back(std::move(newTs));
        }
        return result;
    }

    TimeState
    TimeState_Copy(TimeState const& ts)
    {
        TimeState result;
        result.time = ts.time;
        result.state = ts.state;
        for (auto x : ts.failureModeCauses)
        {
            result.failureModeCauses.insert(x);
        }
        for (auto x : ts.fragilityModeCauses)
        {
            result.fragilityModeCauses.insert(x);
        }
        return result;
    }

    double
    TimeState_CalcAvailability_s(
        std::vector<TimeState> const& sch,
        double endTime_s
    )
    {
        double result = endTime_s;
        for (size_t i = 0; i < sch.size(); ++i)
        {
            double dt = (i + 1) < sch.size() ? sch[i + 1].time - sch[i].time
                                             : endTime_s - sch[i].time;
            if (!sch[i].state)
            {
                result -= dt;
            }
        }
        return result;
    }

    TimeState
    TimeState_GetActiveTimeState(
        std::vector<TimeState> const& tss,
        double time_s
    )
    {
        TimeState result{time_s, true, {}, {}};
        for (auto const& ts : tss)
        {
            if (ts.time < time_s)
            {
                result = ts;
            }
            else if (ts.time == time_s)
            {
                return ts;
            }
            else
            {
                break;
            }
        }
        return result;
    }

    void
    TimeState_CountAndTimeFailureEvents(
        std::vector<TimeState> const& tss,
        double finalTime_s,
        std::map<size_t, size_t>& eventCountsByFailureModeId,
        std::map<size_t, size_t>& eventCountsByFragilityModeId,
        std::map<size_t, double>& timeByFailureModeId_s,
        std::map<size_t, double>& timeByFragilityModeId_s
    )
    {
        for (size_t i = 0; i < tss.size(); ++i)
        {
            TimeState const& ts = tss[i];
            if (i == 0 && !ts.state)
            {
                // count initial failures
                for (auto failModeId : ts.failureModeCauses)
                {
                    if (eventCountsByFailureModeId.contains(failModeId))
                    {
                        eventCountsByFailureModeId[failModeId] += 1;
                    }
                    else
                    {
                        eventCountsByFailureModeId[failModeId] = 1;
                    }
                }
                for (auto fragModeId : ts.fragilityModeCauses)
                {
                    if (eventCountsByFragilityModeId.contains(fragModeId))
                    {
                        eventCountsByFragilityModeId[fragModeId] += 1;
                    }
                    else
                    {
                        eventCountsByFragilityModeId[fragModeId] = 1;
                    }
                }
            }
            TimeState const nextTs = (i + 1) < tss.size()
                ? tss[i + 1]
                : TimeState{finalTime_s, ts.state, {}, {}};
            double dt = nextTs.time - ts.time;
            if (dt <= 0.0)
            {
                break;
            }
            if (!ts.state)
            {
                for (auto failModeId : ts.failureModeCauses)
                {
                    if (timeByFailureModeId_s.contains(failModeId))
                    {
                        timeByFailureModeId_s[failModeId] += dt;
                    }
                    else
                    {
                        timeByFailureModeId_s[failModeId] = dt;
                    }
                }
                for (auto fragModeId : ts.fragilityModeCauses)
                {
                    if (timeByFragilityModeId_s.contains(fragModeId))
                    {
                        timeByFragilityModeId_s[fragModeId] += dt;
                    }
                    else
                    {
                        timeByFragilityModeId_s[fragModeId] = dt;
                    }
                }
            }
            if (ts.state && !nextTs.state)
            {
                for (auto failModeId : nextTs.failureModeCauses)
                {
                    if (eventCountsByFailureModeId.contains(failModeId))
                    {
                        eventCountsByFailureModeId[failModeId] += 1;
                    }
                    else
                    {
                        eventCountsByFailureModeId[failModeId] = 1;
                    }
                }
                for (auto fragModeId : nextTs.fragilityModeCauses)
                {
                    if (eventCountsByFragilityModeId.contains(fragModeId))
                    {
                        eventCountsByFragilityModeId[fragModeId] += 1;
                    }
                    else
                    {
                        eventCountsByFragilityModeId[fragModeId] = 1;
                    }
                }
            }
        }
    }

    void
    TimeState_Print(std::vector<TimeState> const& tss)
    {
        for (TimeState const& ts : tss)
        {
            std::cout << "- " << ts << std::endl;
        }
    }

} // namespace erin
