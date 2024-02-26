/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_timestate.h"

namespace erin_next
{

	bool
		operator==(const TimeState& a, const TimeState& b)
	{
		return (a.time == b.time)
			&& (a.state == b.state);
	}

	bool
		operator!=(const TimeState& a, const TimeState& b)
	{
		return !(a == b);
	}

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

	std::vector<TimeState>
	TimeState_Combine(
		std::vector<TimeState> const& a,
		std::vector<TimeState> const& b)
	{
		std::vector<TimeState> result;
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

}