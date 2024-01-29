/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_NEXT_RELIABILITY_H
#define ERIN_NEXT_RELIABILITY_H
#include "erin_next/distribution.h"
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace erin_next
{
	// Data Structs
	struct TimeState
	{
		double time{0};
		bool state{true};
	};

	bool operator==(const TimeState& a, const TimeState& b);
	bool operator!=(const TimeState& a, const TimeState& b);
	std::ostream& operator<<(std::ostream& os, const TimeState& ts);

	struct FailureMode
	{
		std::vector<std::string> tag{};
		std::vector<size_t> failure_dist{};
		std::vector<size_t> repair_dist{};
	};

	struct FailureMode_Component_Link {
		std::vector<size_t> failure_mode_id{};
		std::vector<size_t> component_id{};
	};

	struct Component_meta
	{
		std::vector<std::string> tag{};
	};

	// Main Class to do Reliability Schedule Creation
	class ReliabilityCoordinator
	{
		public:
		ReliabilityCoordinator();

		size_t
		add_failure_mode(
			const std::string& tag,
			const size_t& failure_dist_id,
			const size_t& repair_dist_id
			);

		void
		link_component_with_failure_mode(
			const size_t& comp_id,
			const size_t& fm_id);

		size_t
		register_component(const std::string& tag);

		std::unordered_map<size_t, std::vector<TimeState>>
		calc_reliability_schedule(
			const std::function<double()>& rand_fn,
			const DistributionSystem& cds,
			double final_time) const;

		std::unordered_map<std::string, std::vector<TimeState>>
		calc_reliability_schedule_by_component_tag(
			const std::function<double()>& rand_fn,
			const DistributionSystem& cds,
			double final_time) const;

		private:
		FailureMode fms;
		FailureMode_Component_Link fm_comp_links;
		Component_meta comp_meta;

		void
		calc_next_events(
			const std::function<double()>& rand_fn,
			const DistributionSystem& cds,
			std::unordered_map<size_t, double>& comp_id_to_dt,
			bool is_failure) const;

		size_t
		update_schedule(
			std::unordered_map<size_t, double>& comp_id_to_time,
			std::unordered_map<size_t, double>& comp_id_to_dt,
			std::unordered_map<size_t, std::vector<TimeState>>&
				comp_id_to_reliability_schedule,
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
