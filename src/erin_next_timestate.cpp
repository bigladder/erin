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
		return os << "TimeState("
			<< "time=" << ts.time << ", "
			<< "state=" << ts.state << ")";
	}

}