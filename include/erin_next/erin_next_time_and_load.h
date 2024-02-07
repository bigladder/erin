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
		double Time;
		uint32_t Load;

		TimeAndLoad(double t, uint32_t load) :Time{ t }, Load{ load } {}
	};

	std::ostream&
	operator<<(std::ostream& os, TimeAndLoad const& timeAndLoad);
}

#endif // ERIN_NEXT_TIME_AND_LOAD_H