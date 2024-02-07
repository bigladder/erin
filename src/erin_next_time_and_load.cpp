/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_time_and_load.h"

namespace erin_next
{
	std::ostream&
	operator<<(std::ostream& os, TimeAndLoad const& timeAndLoad)
	{
		os << "TimeAndLoad{"
			<< "Time=" << timeAndLoad.Time << "; "
			<< "Load=" << timeAndLoad.Load << "}";
		return os;
	}
}