/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_TIME_AND_AMOUNT_H
#define ERIN_NEXT_TIME_AND_AMOUNT_H
#include <stdint.h>
#include <ostream>

namespace erin_next
{
	struct TimeAndAmount
	{
		double Time = 0.0;
		uint32_t Amount = 0;
	};

	std::ostream&
	operator<<(std::ostream& os, TimeAndAmount const& timeAndAmount);
}

#endif