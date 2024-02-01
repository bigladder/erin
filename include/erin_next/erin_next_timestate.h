/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_TIMESTATE_H
#define ERIN_NEXT_TIMESTATE_H
#include <iostream>

namespace erin_next
{

	struct TimeState
	{
		double time{ 0 };
		bool state{ true };
	};

	bool operator==(const TimeState& a, const TimeState& b);
	bool operator!=(const TimeState& a, const TimeState& b);
	std::ostream& operator<<(std::ostream& os, const TimeState& ts);

}

#endif // ERIN_NEXT_TIMESTATE_H