/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_TIMESTATE_H
#define ERIN_NEXT_TIMESTATE_H
#include <iostream>
#include <vector>
#include <set>

namespace erin_next
{

	struct TimeState
	{
		double time = 0.0;
		bool state = true;
		std::set<size_t> failureModeCauses;
		std::set<size_t> fragilityModeCauses;
	};

	bool
	operator==(const TimeState& a, const TimeState& b);

	bool
	operator!=(const TimeState& a, const TimeState& b);
	
	std::ostream&
	operator<<(std::ostream& os, const TimeState& ts);

	std::vector<TimeState>
	TimeState_Combine(
		std::vector<TimeState> const& a,
		std::vector<TimeState> const& b);

}

#endif