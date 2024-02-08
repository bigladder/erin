/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_COMPONENT_H
#define ERIN_NEXT_COMPONENT_H
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_simulation.h"
#include "../vendor/toml11/toml.hpp"

namespace erin_next
{
	bool
	ParseSingleComponent(
		Simulation& s,
		Model& m,
		toml::table const& table,
		std::string const& tag);

	bool
	ParseComponents(Simulation& s, Model& m, toml::table const& table);
}

#endif