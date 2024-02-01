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

	struct FailureMode
	{
		std::vector<std::string> tag{};
		std::vector<size_t> failure_dist{};
		std::vector<size_t> repair_dist{};
	};

	struct FailureMode_Component_Link {
		std::vector<size_t> failure_mode_id{};
		std::vector<size_t> component_id{};
		std::vector<std::vector<TimeState>> schedules{};
	};

	// Main Class to do Reliability Schedule Creation
	class ReliabilityCoordinator
	{
		public:
		ReliabilityCoordinator();

		size_t
		add_failure_mode(
			std::string const& tag,
			size_t const& failure_dist_id,
			size_t const& repair_dist_id);

		size_t
		link_component_with_failure_mode(
			size_t const& comp_id,
			size_t const& fm_id);

		std::vector<TimeState>
		make_schedule_for_link(
			size_t linkId,
			const std::function<double()>& rand_fn,
			const DistributionSystem& cds,
			double final_time);

		private:
		FailureMode fms;
		FailureMode_Component_Link fm_comp_links;

		double
		calc_next_event(
			size_t link_id,
			double dt_fm,
			const std::function<double()>& rand_fn,
			const DistributionSystem& cds,
			bool is_failure) const;

		bool
		update_single_schedule(
			double& time,
			double& dt,
			std::vector<TimeState>& schedule,
			double final_time,
			bool next_state) const;
	};

	template <class T>
	std::unordered_map<T, std::vector<TimeState>>
	clip_schedule_to(
		const std::unordered_map<T, std::vector<TimeState>>& schedule,
		double start_time,
		double end_time)
	{
		std::unordered_map<T, std::vector<TimeState>> new_sch{};
		for (const auto& item : schedule)
		{
			std::vector<TimeState> tss{};
			bool state{true};
			for (const auto& ts : item.second)
			{
				if (ts.time < start_time)
				{
					state = ts.state;
					continue;
				}
				else if (ts.time == start_time)
				{
					tss.emplace_back(TimeState{0, ts.state});
				}
				else if ((ts.time > start_time) && (ts.time <= end_time))
				{
					if (tss.size() == 0)
					{
						tss.emplace_back(TimeState{0, state});
					}
					tss.emplace_back(TimeState{ts.time - start_time, ts.state});
				}
				else if (ts.time > end_time)
				{
					break;
				}
			}
			new_sch[item.first] = std::move(tss);
		}
		return new_sch;
	}

	bool schedule_state_at_time(
		const std::vector<TimeState>& schedule,
		double time,
		bool initial_value = true);
}

#endif // ERIN_NEXT_RELIABILITY_H
