/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_TIME_AND_LOAD_H
#define ERIN_NEXT_TIME_AND_LOAD_H
#include <stdint.h>
#include <ostream>

namespace erin_next
{
	struct TimeAndLoad
	{
		double Time = 0.0;
		uint32_t Load = 0;
	};

	std::ostream&
	operator<<(std::ostream& os, TimeAndLoad const& timeAndLoad);
}

#endif // ERIN_NEXT_TIME_AND_LOAD_H