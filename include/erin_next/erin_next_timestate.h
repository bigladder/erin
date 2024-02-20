/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_TIMESTATE_H
#define ERIN_NEXT_TIMESTATE_H
#include <iostream>

namespace erin_next
{

	struct TimeState
	{
		double time = 0.0;
		bool state = true;
		// TODO: add container to track failure modes and fragility modes
		// that caused the failure when state=false. Either a
		// std::vector<std::string> with tags for failure modes/fragility modes
		// OR a std::set<size_t> failureModeCauses{};
		// AND std::set<size_t> fragilityModeCauses{};
	};

	bool operator==(const TimeState& a, const TimeState& b);
	bool operator!=(const TimeState& a, const TimeState& b);
	std::ostream& operator<<(std::ostream& os, const TimeState& ts);

}

#endif