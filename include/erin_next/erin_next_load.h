/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_LOAD_H
#define ERIN_NEXT_LOAD_H
#include "erin_next/erin_next_time_and_amount.h"
#include "erin_next/erin_next_units.h"
#include "../vendor/toml11/toml.hpp"
#include <string>
#include <vector>
#include <optional>
#include <ostream>

namespace erin_next
{
	struct Load {
		std::string Tag;
		TimeUnit TimeUnit;
		std::string RateUnit;
		std::vector<TimeAndAmount> TimeAndLoads;
	};

	std::optional<Load>
	ParseSingleLoad(toml::table const& table, std::string const& tag);

	std::optional<std::vector<Load>>
	ParseLoads(toml::table const& table);

	std::ostream&
	operator<<(std::ostream& os, Load const& load);
}

#endif // ERIN_NEXT_LOAD_H