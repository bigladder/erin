/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */
#include "erin_next/erin_next_distribution.h"
#include "erin_next/erin_next_utils.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_csv.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <random>
#include <iostream>
#include <unordered_map>

namespace erin
{
	// k = shape parameter, k > 0
	// a = scale parameter, a > 0, also called 'lambda'
	// b = location parameter, also called 'gamma'
	// 0 <= p < 1
	// reference: https://www.real-statistics.com/other-key-distributions/
	//    weibull-distribution/three-parameter-weibull-distribution/
	double
	weibull_quantile(
		const double& p,
		const double& k,
		const double& a,
		const double& b
	)
	{
		constexpr double highest_q{0.9999};
		double ans{0.0};
		if (p <= 0.0)
		{
			ans = b;
		}
		else
		{
			auto q{p};
			if (p >= 1.0)
			{
				q = highest_q;
			}
			ans = b + a * std::pow(-1.0 * std::log(1.0 - q), 1.0 / k);
		}
		return (ans < 0.0) ? 0.0 : ans;
	}

	double
	erfinv(double x)
	{
		/*
		 * From "A handy approximation for the error function and its inverse"
		 * by Sergei Winitzki February 6, 2008
		 * a = 8887/63473
		 * erfinv(x) ~= [
		 *    ((-2) / (pi * a))
		 *    - (ln(1 - x**2) / 2)
		 *    + sqrt(
		 *        ( (2 / (pi * a)) + (ln(1 - x**2) / 2) )**2
		 *        - ((1/a) * ln(1 - x**2)))
		 *    ] ** (1/2)
		 *  OR
		 *  A = C * (2.0 / pi)
		 *  B = ln(1 - x**2)
		 *  C = 1/a
		 *  D = B / 2
		 *  erfinv(x) ~= ((-A) + (-D) + sqrt((A + D)**2 - (C*B)))**0.5
		 *
		 *  domain is (-1, 1) but outliter values are allowed (they just get
		 *  cropped)
		 */
		constexpr double extent{3.0};
		constexpr double max_domain{1.0};
		if (x <= -max_domain)
		{
			return -extent;
		}
		if (x >= max_domain)
		{
			return extent;
		}
		constexpr double pi{3.1415'9265'3589'7932'3846'26433};
		constexpr double a{8'887.0 / 63'473.0};
		constexpr double C{1.0 / a};
		constexpr double two{2.0};
		constexpr double C_times_2{C * two};
		constexpr double A{C_times_2 / pi};
		double B = std::log(1.0 - (x * x));
		double D{B / two};
		double sum_A_D{A + D};
		double sum_A_D2{sum_A_D * sum_A_D};
		auto answer = std::sqrt((-A) + (-D) + std::sqrt(sum_A_D2 - (C * B)));
		if (x < 0.0)
		{
			answer = (-1.0) * answer;
		}
		if (answer > extent)
		{
			answer = extent;
		}
		if (answer < -extent)
		{
			answer = -extent;
		}
		return answer;
	}

	std::string
	dist_type_to_tag(DistType dist_type)
	{
		switch (dist_type)
		{
			case DistType::Fixed:
				return std::string{"fixed"};
			case DistType::Uniform:
				return std::string{"uniform"};
			case DistType::Normal:
				return std::string{"normal"};
			case DistType::Weibull:
				return std::string{"weibull"};
			case DistType::QuantileTable:
				return std::string{"table"};
		}
		std::ostringstream oss{};
		oss << "unhandled dist_type `" << static_cast<int>(dist_type) << "`";
		throw std::invalid_argument(oss.str());
	}

	std::optional<DistType>
	tag_to_dist_type(const std::string& tag)
	{
		if (tag == "fixed")
		{
			return DistType::Fixed;
		}
		if (tag == "uniform")
		{
			return DistType::Uniform;
		}
		if (tag == "normal")
		{
			return DistType::Normal;
		}
		if (tag == "weibull")
		{
			return DistType::Weibull;
		}
		if (tag == "quantile_table")
		{
			return DistType::QuantileTable;
		}
		return {};
	}

	DistributionSystem::DistributionSystem()
		: dist{}
		, fixed_dist{}
		, uniform_dist{}
		, normal_dist{}
		, quantile_table_dist{}
		, weibull_dist{}
		, g{}
		, roll{0.0, 1.0}
	{
	}

	size_t
	DistributionSystem::add_fixed(
		const std::string& tag,
		double value_in_seconds
	)
	{
		auto id{dist.tag.size()};
		auto subtype_id{fixed_dist.value.size()};
		fixed_dist.value.emplace_back(value_in_seconds);
		dist.tag.emplace_back(tag);
		dist.subtype_id.emplace_back(subtype_id);
		dist.dist_type.emplace_back(DistType::Fixed);
		return id;
	}

	size_t
	DistributionSystem::add_uniform(
		const std::string& tag,
		double lower_bound_s,
		double upper_bound_s
	)
	{
		if (lower_bound_s > upper_bound_s)
		{
			std::ostringstream oss{};
			oss << "lower_bound_s is greater than upper_bound_s\n"
				<< "lower_bound_s: " << lower_bound_s << "\n"
				<< "upper_bound_s: " << upper_bound_s << "\n";
			throw std::invalid_argument(oss.str());
		}
		auto id{dist.tag.size()};
		auto subtype_id{uniform_dist.lower_bound.size()};
		uniform_dist.lower_bound.emplace_back(lower_bound_s);
		uniform_dist.upper_bound.emplace_back(upper_bound_s);
		dist.tag.emplace_back(tag);
		dist.subtype_id.emplace_back(subtype_id);
		dist.dist_type.emplace_back(DistType::Uniform);
		return id;
	}

	size_t
	DistributionSystem::add_normal(
		const std::string& tag,
		double mean_s,
		double stddev_s
	)
	{
		auto id{dist.tag.size()};
		auto subtype_id{normal_dist.average.size()};
		normal_dist.average.emplace_back(mean_s);
		normal_dist.stddev.emplace_back(stddev_s);
		dist.tag.emplace_back(tag);
		dist.subtype_id.emplace_back(subtype_id);
		dist.dist_type.emplace_back(DistType::Normal);
		return id;
	}

	void
	ensure_sizes_equal(const std::string& tag, const size_t& a, const size_t& b)
	{
		if (a != b)
		{
			std::ostringstream oss{};
			oss << "tag `" << tag << "` not a valid tabular distribution.\n"
				<< "xs.size() (" << a << ") must equal (" << "dtimes_s.size() ("
				<< b << ")\n";
			throw std::invalid_argument(oss.str());
		}
	}

	void
	ensure_size_greater_than_or_equal_to(
		const std::string& tag,
		const size_t& a,
		const size_t& n
	)
	{
		if (a < n)
		{
			std::ostringstream oss{};
			oss << "tag `" << tag << "` not a valid tabular distribution.\n"
				<< "xs.size() (" << a << ") must be greater than 1\n";
			throw std::invalid_argument(oss.str());
		}
	}

	void
	ensure_always_increasing(
		const std::string& tag,
		const std::vector<double>& xs
	)
	{
		bool first{true};
		double last{0.0};
		for (size_t idx = 0; idx < xs.size(); ++idx)
		{
			const auto& x{xs[idx]};
			if (first)
			{
				first = false;
				last = x;
			}
			else
			{
				if (x <= last)
				{
					std::ostringstream oss{};
					oss << "tag `" << tag
						<< "` not a valid tabular distribution.\n"
						<< "values must be always increasing\n";
					throw std::invalid_argument(oss.str());
				}
			}
		}
	}

	void
	ensure_for_all(
		const std::string& tag,
		const std::vector<double>& xs,
		const std::function<bool(double)>& f
	)
	{
		for (const auto& x : xs)
		{
			if (!f(x))
			{
				std::ostringstream oss{};
				oss << "tag `" << tag << "` not valid.\n"
					<< "issue for x == " << x << "\n";
				throw std::invalid_argument(oss.str());
			}
		}
	}

	void
	ensure_equals(const std::string& tag, const double x, const double val)
	{
		if (x != val)
		{
			std::ostringstream oss{};
			oss << tag << ": expected x to be equal to " << val << "\n"
				<< ", but got x == " << x << "\n";
			throw std::invalid_argument(oss.str());
		}
	}

	void
	ensure_greater_than_or_equal_to(const double x, const double val)
	{
		if (x < val)
		{
			std::ostringstream oss{};
			oss << "expected x to be greater than or equal to " << val << "\n"
				<< ", but got x == " << x << "\n";
			throw std::invalid_argument(oss.str());
		}
	}

	void
	ensure_greater_than(const double x, const double val)
	{
		if (x <= val)
		{
			std::ostringstream oss{};
			oss << "expected x to be greater than " << val << "\n"
				<< ", but got x == " << x << "\n";
			throw std::invalid_argument(oss.str());
		}
	}

	void
	ensure_greater_than_zero(const double x)
	{
		ensure_greater_than(x, 0.0);
	}

	size_t
	DistributionSystem::add_quantile_table(
		const std::string& tag,
		const std::vector<double>& xs,
		const std::vector<double>& dtimes_s
	)
	{
		const size_t count{xs.size()};
		const size_t last_idx{(count == 0) ? 0 : (count - 1)};
		ensure_sizes_equal(tag, count, dtimes_s.size());
		ensure_size_greater_than_or_equal_to(tag, count, 2);
		ensure_always_increasing(tag, xs);
		ensure_always_increasing(tag, dtimes_s);
		ensure_equals(tag + "[0]", xs[0], 0.0);
		ensure_equals(
			tag + "[" + std::to_string(last_idx) + "]", xs[last_idx], 1.0
		);
		auto id{dist.tag.size()};
		auto subtype_id{quantile_table_dist.start_idx.size()};
		size_t start_idx{
			subtype_id == 0 ? 0
							: (quantile_table_dist.end_idx[subtype_id - 1] + 1)
		};
		size_t end_idx{start_idx + count - 1};
		quantile_table_dist.start_idx.emplace_back(start_idx);
		quantile_table_dist.end_idx.emplace_back(end_idx);
		for (size_t i{0}; i < count; ++i)
		{
			quantile_table_dist.variates.emplace_back(xs[i]);
			quantile_table_dist.times.emplace_back(dtimes_s[i]);
		}
		dist.tag.emplace_back(tag);
		dist.subtype_id.emplace_back(subtype_id);
		dist.dist_type.emplace_back(DistType::QuantileTable);
		return id;
	}

	/*
	size_type DistributionSystem::add_pdf_table(
		const std::string& tag,
		const std::vector<double>& dtimes_s,
		const std::vector<double>& occurrences)
	{
		const size_type count{dtimes_s.size()};
		const size_type last_idx{(count == 0) ? 0 : (count - 1)};
		ensure_sizes_equal(tag, count, occurrences.size());
		ensure_size_greater_than_or_equal_to(tag, count, 2);
		ensure_always_increasing(tag, dtimes_s);
		ensure_for_all(tag, occurrences, [](double x)->bool {
		  return x >= 0.0;
		});
		ensure_equals(tag + "[0]", occurrences[0], 0.0);
		const double total{std::accumulate(
		  occurrences.begin(), occurrences.end(), 0.0)};
		ensure_greater_than_zero(total);
		std::vector<double> dist_xs{};
		std::vector<double> dist_ys{};
		double running_sum{0.0};
		for (std::vector<double>::size_type idx{0}; idx < count; ++idx) {
		  const double& x = occurrences[idx];
		  running_sum += x;
		  dist_xs.emplace_back(dtimes_s[idx]);
		  dist_ys.emplace_back(running_sum / total);
		}
		return add_quantile_table(tag, dist_ys, dist_xs);
	}
	*/

	size_t
	DistributionSystem::add_weibull(
		const std::string& tag,
		const double shape_parameter, // k
		const double scale_parameter, // lambda
		const double location_parameter // gamma
	)
	{
		ensure_greater_than_zero(shape_parameter);
		ensure_greater_than_zero(scale_parameter);
		auto id{dist.tag.size()};
		auto subtype_id{weibull_dist.shape_params.size()};
		weibull_dist.shape_params.emplace_back(shape_parameter);
		weibull_dist.scale_params.emplace_back(scale_parameter);
		weibull_dist.location_params.emplace_back(location_parameter);
		dist.tag.emplace_back(tag);
		dist.subtype_id.emplace_back(subtype_id);
		dist.dist_type.emplace_back(DistType::Weibull);
		return id;
	}

	// TODO: update to return std::optional<size_t>
	size_t
	DistributionSystem::lookup_dist_by_tag(const std::string& tag) const
	{
		for (size_t i{0}; i < dist.tag.size(); ++i)
		{
			if (dist.tag[i] == tag)
			{
				return i;
			}
		}
		std::ostringstream oss{};
		oss << "tag `" << tag << "` not found in distribution list";
		throw std::invalid_argument(oss.str());
	}

	std::optional<Distribution>
	DistributionSystem::get_dist_by_id(size_t id) const
	{
		if (id >= dist.dist_type.size())
		{
			return {};
		}
		Distribution d = {};
		d.SubtypeIdx = dist.subtype_id[id];
		d.Tag = dist.tag[id];
		d.Type = dist.dist_type[id];
		return d;
	}

	double
	DistributionSystem::next_time_advance(size_t dist_id)
	{
		auto fraction = roll(g);
		return next_time_advance(dist_id, fraction);
	}

	double
	DistributionSystem::next_time_advance(size_t dist_id, double fraction) const
	{
		if (dist_id >= dist.tag.size())
		{
			std::ostringstream oss{};
			oss << "dist_id '" << dist_id << "' is out of range\n"
				<< "- id     : " << dist_id << "\n"
				<< "- max(id): " << (dist.tag.size() - 1) << "\n";
			throw std::out_of_range(oss.str());
		}
		const auto& subtype_id = dist.subtype_id.at(dist_id);
		const auto& dist_type = dist.dist_type.at(dist_id);
		double dt{0};
		switch (dist_type)
		{
			case DistType::Fixed:
			{
				dt = fixed_dist.value.at(subtype_id);
			}
			break;
			case DistType::Uniform:
			{
				auto lb = uniform_dist.lower_bound.at(subtype_id);
				auto ub = uniform_dist.upper_bound.at(subtype_id);
				auto delta = ub - lb;
				dt = static_cast<double>(
					fraction * static_cast<double>(delta)
					+ static_cast<uint32_t>(lb)
				);
			}
			break;
			case DistType::Normal:
			{
				constexpr double sqrt2{1.4142'1356'2373'0951};
				constexpr double twice{2.0};
				auto avg =
					static_cast<double>(normal_dist.average.at(subtype_id));
				auto sd =
					static_cast<double>(normal_dist.stddev.at(subtype_id));
				dt = static_cast<double>(std::round(
					avg + sd * sqrt2 * erfinv(twice * fraction - 1.0)
				));
			}
			break;
			case DistType::QuantileTable:
			{
				const auto& start_idx =
					quantile_table_dist.start_idx[subtype_id];
				const auto& end_idx = quantile_table_dist.end_idx[subtype_id];
				if (fraction >= 1.0)
				{
					dt = static_cast<double>(
						std::round(quantile_table_dist.times[end_idx])
					);
				}
				else
				{
					for (size_t idx{start_idx}; idx < end_idx; ++idx)
					{
						const auto& v0 = quantile_table_dist.variates[idx];
						const auto& v1 = quantile_table_dist.variates[idx + 1];
						if ((fraction >= v0) && (fraction < v1))
						{
							if (fraction == v0)
							{
								dt = static_cast<double>(
									std::round(quantile_table_dist.times[idx])
								);
								break;
							}
							else
							{
								const auto df{fraction - v0};
								const auto dv{v1 - v0};
								const auto time0{quantile_table_dist.times[idx]
								};
								const auto time1{
									quantile_table_dist.times[idx + 1]
								};
								const auto dtimes{time1 - time0};
								dt = static_cast<double>(
									std::round(time0 + (df / dv) * dtimes)
								);
							}
						}
					}
				}
			}
			break;
			case DistType::Weibull:
			{
				const auto& k = weibull_dist.shape_params[subtype_id];
				const auto& a = weibull_dist.scale_params[subtype_id];
				const auto& b = weibull_dist.location_params[subtype_id];
				dt = static_cast<double>(
					std::round(weibull_quantile(fraction, k, a, b))
				);
			}
			break;
			default:
			{
				WriteErrorMessage(
					"distribution", "unhandled cumulative density function"
				);
				std::exit(1);
			}
		}
		if (dt < 0)
		{
			dt = 0;
		}
		return dt;
	}

	void
	DistributionSystem::print_distributions() const
	{
		for (size_t i = 0; i < dist.dist_type.size(); ++i)
		{
			std::cout << i << ": " << dist_type_to_tag(dist.dist_type[i])
					  << " -- " << dist.tag[i] << std::endl;
		}
	}

	// TODO: change to get rid of DistributionSystem object; pass Simulation
	// instead
	Result
	ParseDistributions(
		DistributionSystem& ds,
		toml::table const& table,
		DistributionValidationMap const& dvm
	)
	{
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			std::string distTag = it->first;
			std::string fullTableName = "dist." + distTag;
			if (it->second.is_table())
			{
				toml::table distTable = it->second.as_table();
				if (!distTable.contains("type"))
				{
					WriteErrorMessage(
						fullTableName, "missing required field 'type'"
					);
					return Result::Failure;
				}
				std::string distTypeTag = distTable.at("type").as_string();
				std::optional<DistType> maybeDistType =
					tag_to_dist_type(distTypeTag);
				if (!maybeDistType.has_value())
				{
					WriteErrorMessage(
						fullTableName,
						"unhandled distribution type '" + distTypeTag + "'"
					);
					return Result::Failure;
				}
				DistType distType = maybeDistType.value();
				std::vector<std::string> errors;
				std::vector<std::string> warnings;
				std::unordered_map<std::string, InputValue> inputs;
				switch (distType)
				{
					case DistType::Fixed:
					{
						inputs = TOMLTable_ParseWithValidation(
							distTable,
							dvm.Fixed,
							fullTableName,
							errors,
							warnings
						);
					}
					break;
					case DistType::Normal:
					{
						inputs = TOMLTable_ParseWithValidation(
							distTable,
							dvm.Normal,
							fullTableName,
							errors,
							warnings
						);
					}
					break;
					case DistType::QuantileTable:
					{
						if (distTable.contains("csv_file"))
						{
							inputs = TOMLTable_ParseWithValidation(
								distTable,
								dvm.QuantileTableFromFile,
								fullTableName,
								errors,
								warnings
							);
						}
						else
						{
							inputs = TOMLTable_ParseWithValidation(
								distTable,
								dvm.QuantileTableExplicit,
								fullTableName,
								errors,
								warnings
							);
						}
					}
					break;
					case DistType::Uniform:
					{
						inputs = TOMLTable_ParseWithValidation(
							distTable,
							dvm.Uniform,
							fullTableName,
							errors,
							warnings
						);
					}
					break;
					case DistType::Weibull:
					{
						inputs = TOMLTable_ParseWithValidation(
							distTable,
							dvm.Weibull,
							fullTableName,
							errors,
							warnings
						);
					}
					break;
					default:
					{
						WriteErrorMessage(fullTableName, "unhandled dist type");
						return Result::Failure;
					}
					break;
				}
				if (errors.size() > 0)
				{
					for (std::string const& err : errors)
					{
						WriteErrorMessage("", err);
					}
					return Result::Failure;
				}
				for (std::string const& w : warnings)
				{
					WriteErrorMessage("", w);
				}
				// TODO: pull default time from SimulationInfo
				TimeUnit timeUnit = TimeUnit::Second;
				if (inputs.contains("time_unit"))
				{
					std::string timeUnitStr =
						std::get<std::string>(inputs.at("time_unit").Value);
					std::optional<TimeUnit> maybeTimeUnit =
						TagToTimeUnit(timeUnitStr);
					if (!maybeTimeUnit.has_value())
					{
						WriteErrorMessage(
							fullTableName,
							"unhandled time unit '" + timeUnitStr + "'"
						);
						return Result::Failure;
					}
					timeUnit = maybeTimeUnit.value();
				}
				switch (distType)
				{
					case (DistType::Fixed):
					{
						double value =
							std::get<double>(inputs.at("value").Value);
						ds.add_fixed(distTag, Time_ToSeconds(value, timeUnit));
					}
					break;
					case DistType::Normal:
					{
						double mean = std::get<double>(inputs.at("mean").Value);
						double sd = std::get<double>(
							inputs.at("standard_deviation").Value
						);
						ds.add_normal(
							distTag,
							Time_ToSeconds(mean, timeUnit),
							Time_ToSeconds(sd, timeUnit)
						);
					}
					break;
					case DistType::QuantileTable:
					{
						std::vector<double> xs;
						std::vector<double> times_s;
						if (inputs.contains("variate_time_pairs"))
						{
							std::vector<std::vector<double>> vt_pairs =
								std::get<std::vector<std::vector<double>>>(
									inputs.at("variate_time_pairs").Value
								);
							xs.reserve(vt_pairs.size());
							times_s.reserve(vt_pairs.size());
							for (std::vector<double> const& vt : vt_pairs)
							{
								xs.push_back(vt[0]);
								times_s.push_back(
									Time_ToSeconds(vt[1], timeUnit)
								);
							}
						}
						else if (inputs.contains("csv_file"))
						{
							// TODO: move csv_file read into separate function
							// std::optional<std::vector<std::array<double,2>>>
							// ReadCsvToArrayOfTwoTuples(std::string csvFile);
							std::string csvFileName = std::get<std::string>(
								inputs.at("csv_file").Value
							);
							std::ifstream inputDataFile;
							inputDataFile.open(csvFileName);
							if (!inputDataFile.good())
							{
								WriteErrorMessage(
									fullTableName,
									"unable to load input csv file '"
										+ csvFileName + "'"
								);
								return {};
							}
							auto header = read_row(inputDataFile);
							if (header.size() == 2)
							{
								std::string const& timeUnitStr = header[1];
								std::optional<TimeUnit> maybeTimeUnit =
									TagToTimeUnit(timeUnitStr);
								if (!maybeTimeUnit.has_value())
								{
									WriteErrorMessage(
										fullTableName,
										"unhandled time unit: " + timeUnitStr
									);
									return {};
								}
								TimeUnit timeUnitForRead =
									maybeTimeUnit.value();
								int rowIdx = 1;
								while (inputDataFile.is_open()
									   && inputDataFile.good())
								{
									std::vector<std::string> pair =
										read_row(inputDataFile);
									if (pair.size() == 0)
									{
										break;
									}
									++rowIdx;
									if (pair.size() != 2)
									{
										WriteErrorMessage(
											fullTableName,
											"csv file '" + csvFileName
												+ "'"
												  " row: "
												+ std::to_string(rowIdx)
												+ "; must have 2 columns; "
												  "found: "
												+ std::to_string(pair.size())
										);
										return Result::Failure;
									}
									xs.push_back(std::stod(pair[0]));
									times_s.push_back(Time_ToSeconds(
										std::stod(pair[1]), timeUnitForRead
									));
								}
								inputDataFile.close();
							}
							else
							{
								WriteErrorMessage(
									fullTableName,
									"csv file '" + csvFileName
										+ "'"
										  " -- header must have 2 columns: "
										  "variate "
										  "and time unit"
								);
								return Result::Failure;
							}
						}
						else
						{
							WriteErrorMessage(
								fullTableName,
								"need one of 'variate_time_pairs' or 'csv_file'"
							);
							return Result::Failure;
						}
						ds.add_quantile_table(distTag, xs, times_s);
					}
					break;
					case DistType::Uniform:
					{
						double lower_bound_s = Time_ToSeconds(
							std::get<double>(inputs.at("lower_bound").Value),
							timeUnit
						);
						double upper_bound_s = Time_ToSeconds(
							std::get<double>(inputs.at("upper_bound").Value),
							timeUnit
						);
						ds.add_uniform(distTag, lower_bound_s, upper_bound_s);
					}
					break;
					case DistType::Weibull:
					{
						double shape =
							std::get<double>(inputs.at("shape").Value);
						double scale =
							std::get<double>(inputs.at("scale").Value);
						double location = 0.0;
						if (inputs.contains("location"))
						{
							location =
								std::get<double>(inputs.at("location").Value);
						}
						ds.add_weibull(
							distTag,
							shape,
							Time_ToSeconds(scale, timeUnit),
							Time_ToSeconds(location, timeUnit)
						);
					}
					break;
					default:
					{
						WriteErrorMessage(
							"distribution",
							"unhandled distribution type: " + distTypeTag
						);
						std::exit(1);
					}
					break;
				}
			}
		}
		return Result::Success;
	}

} // namespace erin
