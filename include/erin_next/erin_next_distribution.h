/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_DISTRIBUTION_H
#define ERIN_DISTRIBUTION_H
#include "erin_next/erin_next_valdata.h"
#include "erin_next/erin_next_result.h"
#include "../vendor/toml11/toml.hpp"
#include <chrono>
#include <exception>
#include <functional>
#include <random>
#include <sstream>
#include <string>
#include <stdint.h>
#include <optional>

namespace erin
{

	template<class T>
	std::function<T(void)>
	make_fixed(const T& value)
	{
		return [value]() -> T { return value; };
	};

	template<class T>
	std::function<T(void)>
	make_random_integer(
		const std::default_random_engine& generator,
		const T& lb,
		const T& ub
	)
	{
		if (lb >= ub)
		{
			std::ostringstream oss{};
			oss << "expected lower_bound < upper_bound but lower_bound = " << lb
				<< " and upper_bound = " << ub;
			throw std::invalid_argument(oss.str());
		}
		std::uniform_int_distribution<T> d{lb, ub};
		std::default_random_engine g = generator; // copy-assignment constructor
		return [d, g]() mutable -> T { return d(g); };
	}

	enum class DistType
	{
		Fixed = 0,
		Uniform,
		Normal,
		Weibull,
		QuantileTable // from times and variate: variate is from (0,1) time and
					  // variate must be always increasing
	};

	std::string
	dist_type_to_tag(DistType dist_type);

	std::optional<DistType>
	tag_to_dist_type(const std::string& tag);

	// SOA version of AOS, std::vector<Distribution>
	struct Dist
	{
		std::vector<std::string> tag{};
		std::vector<size_t> subtype_id{};
		std::vector<DistType> dist_type{};
	};

	struct Distribution
	{
		std::string Tag;
		size_t SubtypeIdx;
		DistType Type;
	};

	struct FixedDist
	{
		std::vector<double> value{};
	};

	struct UniformDist
	{
		std::vector<double> lower_bound{};
		std::vector<double> upper_bound{};
	};

	struct NormalDist
	{
		std::vector<double> average{};
		std::vector<double> stddev{};
	};

	struct QuantileTableDist
	{
		std::vector<double> variates{};
		std::vector<double> times{};
		std::vector<size_t> start_idx{};
		std::vector<size_t> end_idx{};
	};

	struct WeibullDist
	{
		std::vector<double> shape_params{}; // k
		std::vector<double> scale_params{}; // lambda
		std::vector<double> location_params{}; // gamma
	};

	class DistributionSystem
	{
	  public:
		DistributionSystem();

		size_t
		add_fixed(const std::string& tag, double value_in_seconds);

		size_t
		add_uniform(
			const std::string& tag,
			double lower_bound_s,
			double upper_bound_s
		);

		size_t
		add_normal(const std::string& tag, double mean_s, double stddev_s);

		size_t
		add_quantile_table(
			const std::string& tag,
			const std::vector<double>& xs,
			const std::vector<double>& dtimes_s
		);

		/*
		size_t add_pdf_table(
			const std::string& tag,
			const std::vector<double>& dtimes_s,
			const std::vector<double>& occurrences
			);
		*/

		size_t
		add_weibull(
			const std::string& tag,
			const double shape_parameter, // k
			const double scale_parameter, // lambda
			const double location_parameter = 0.0
		); // gamma

		[[nodiscard]] size_t
		lookup_dist_by_tag(std::string const& tag) const;

		std::optional<Distribution>
		get_dist_by_id(size_t id) const;

		double
		next_time_advance(size_t dist_id);

		double
		next_time_advance(size_t dist_id, double fraction) const;

		//[[nodiscard]] std::vector<double>
		//  sample_upto_including(const double max_time_s);
		void
		print_distributions() const;

	  private:
		Dist dist;
		FixedDist fixed_dist;
		UniformDist uniform_dist;
		NormalDist normal_dist;
		QuantileTableDist quantile_table_dist;
		WeibullDist weibull_dist;
		std::default_random_engine g;
		std::uniform_real_distribution<double> roll;
	};

	Result
	ParseDistributions(
		DistributionSystem& ds,
		toml::table const& table,
		DistributionValidationMap const& dvm
	);

} // namespace erin
#endif
