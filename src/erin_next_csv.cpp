/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_csv.h"
#include <iostream>
#include <vector>

namespace erin_next
{
	std::vector<std::string>
	read_row(std::istream& stream)
	{
		std::vector<std::string> data{};
		std::string line{};
		std::getline(stream, line);
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
		if (line.empty())
		{
			return data;
		}
		std::stringstream line_stream(line);
		std::string cell;
		while(std::getline(line_stream, cell, ','))
		{
			data.emplace_back(cell);
		}
		if (!line_stream && cell.empty())
		{
			data.emplace_back("");
		}
		return data;
	}

	void
	stream_out(std::ostream& stream, const std::vector<std::string>& xs)
	{
		for (size_t i = 0; i < xs.size(); ++i)
		{
			if (i == 0)
			{
				stream << "[";
			}
			else
			{
				stream << ", ";
			}
			stream << xs[i];
		}
		stream << "]";
	}

	void
	write_csv(
		std::ostream& os,
		const std::vector<std::string>& items,
		bool start,
		bool end_with_lf)
	{
		bool first{true};
		std::string delim = start ? "" : ",";
		for (const auto& item: items)
		{
			os << delim << item;
			if (first)
			{
				delim = ",";
				first = false;
			}
		}
		if (end_with_lf)
		{
			os << std::endl;
		}
		return;
	}
}