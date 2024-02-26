/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_timestate.h"
#include <assert.h>

namespace erin_next
{

	std::ostream&
		operator<<(std::ostream& os, const TimeState& ts)
	{
		os << "TimeState("
			<< "time=" << ts.time << ", "
			<< "state=" << ts.state << ", failureModeCauses={";
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
		result = result
			&& a.failureModeCauses.size() == b.failureModeCauses.size();
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
		std::vector<TimeState> const& b)
	{
		std::vector<TimeState> result;
		if (a.size() == 0 && b.size() > 0)
		{
			result.reserve(b.size());
			for (size_t i = 0; i < b.size(); ++i)
			{
				result.push_back(std::move(TimeState_Copy(b[i])));
			}
			return result;
		}
		else if (a.size() > 0 && b.size() == 0)
		{
			result.reserve(a.size());
			for (size_t i = 0; i < a.size(); ++i)
			{
				result.push_back(std::move(TimeState_Copy(a[i])));
			}
			return result;
		}
		else if (a.size() == 0 && b.size() == 0)
		{
			return result;
		}
		size_t aIdx = 0;
		size_t bIdx = 0;
		TimeState next{0.0, true};
		bool aState = true;
		bool bState = true;
		while (aIdx < a.size() && bIdx < b.size())
		{
			TimeState const& nextA = a[aIdx];
			TimeState const& nextB = b[bIdx];
			if (next.time == nextA.time)
			{
				aState = nextA.state;
			}
			if (next.time == nextB.time)
			{
				bState = nextB.state;
			}
			if (next.time == nextA.time || next.time == nextB.time)
			{
				next.state = aState && bState;
				std::set<size_t> failureModes;
				std::set<size_t> fragilityModes;
				if (!aState)
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
				if (!bState)
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
				next.failureModeCauses = std::move(failureModes);
				next.fragilityModeCauses = std::move(fragilityModes);
			}
			result.push_back(next);
			if ((aIdx + 1) < a.size() && (bIdx + 1) < b.size())
			{
				if (a[aIdx + 1].time < b[bIdx + 1].time)
				{
					next = a[aIdx + 1];
					++aIdx;
				}
				else if (b[bIdx + 1].time < a[aIdx + 1].time)
				{
					next = b[bIdx + 1];
					++bIdx;
				}
				else
				{
					next = a[aIdx + 1];
					++aIdx;
					++bIdx;
				}
			}
			else if ((aIdx + 1) < a.size())
			{
				next = a[aIdx + 1];
				++aIdx;
			}
			else if ((bIdx + 1) < b.size())
			{
				next = b[bIdx + 1];
				++bIdx;
			}
			else
			{
				break;
			}
		}
		return result;
	}

	std::vector<TimeState>
	TimeState_Clip(
		std::vector<TimeState> const& input,
		double startTime_s,
		double endTime_s,
		bool rezeroTime)
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

	TimeState
	TimeState_Copy(TimeState const& ts)
	{
		TimeState result;
		result.time = ts.time;
		result.state = ts.state;
		std::set<size_t> failureModes;
		std::set<size_t> fragilityModes;
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
		double endTime_s)
	{
		double result = endTime_s;
		for (size_t i = 0; i < sch.size(); ++i)
		{
			double dt = (i + 1) < sch.size()
				? sch[i + 1].time - sch[i].time
				: endTime_s - sch[i].time;
			if (!sch[i].state)
			{
				result -= dt;
			}
		}
		return result;
	}

}