/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_NEXT_RELIABILITY_H
#define ERIN_NEXT_RELIABILITY_H
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_timestate.h"
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace erin_next
{

	// TODO: move to separate file
	struct FailureMode
	{
		std::vector<std::string> tag{};
		std::vector<size_t> failure_dist{};
		std::vector<size_t> repair_dist{};
	};

	// TODO: move to separate file
	struct FailureMode_Component_Link
	{
		std::vector<size_t> failure_mode_id{};
		std::vector<size_t> component_id{};
		std::vector<std::vector<TimeState>> schedules{};
	};

	class ReliabilityCoordinator
	{
	  public:
		ReliabilityCoordinator();

		size_t
		add_failure_mode(
			std::string const& tag,
			size_t const& failure_dist_id,
			size_t const& repair_dist_id
		);

		size_t
		link_component_with_failure_mode(
			size_t const& comp_id,
			size_t const& fm_id
		);

		std::vector<TimeState>
		make_schedule_for_link(
			size_t linkId,
			const std::function<double()>& rand_fn,
			const DistributionSystem& cds,
			double final_time
		);

	  private:
		FailureMode fms;
		FailureMode_Component_Link fm_comp_links;

		double
		calc_next_event(
			size_t link_id,
			double dt_fm,
			const std::function<double()>& rand_fn,
			const DistributionSystem& cds,
			bool is_failure
		) const;

		bool
		update_single_schedule(
			double& time,
			double& dt,
			std::vector<TimeState>& schedule,
			double final_time,
			bool next_state
		) const;
	};

	// Helper Functions -- move to separate file: reliability_schedule.h?
	std::vector<TimeState>
	clip_schedule_to(
		std::vector<TimeState>& schedule,
		double start_time,
		double end_time
	);

	bool
	schedule_state_at_time(
		const std::vector<TimeState>& schedule,
		double time,
		bool initial_value = true
	);
}

#endif
