/* Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_csv.h"
#include <iostream>
#include <vector>
#include <fmt/core.h>

namespace erin
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
        while (std::getline(line_stream, cell, ','))
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

    // TODO: consider returning the ostream so we don't have to pass end_with_lf
    // e.g., std::cout << write_csv(std::cout, ...) << std::endl;
    // Not sure we need start either...
    void
    write_csv(
        std::ostream& os,
        const std::vector<std::string>& items,
        bool start,
        bool end_with_lf
    )
    {
        bool first{true};
        std::string delim = start ? "" : ",";
        for (const auto& item : items)
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

    // TODO: fix, this is slow! Almost 25% of benchmark occurs here...
    std::string
    DoubleToString(double value, unsigned int precision)
    {
        assert(precision <= 6);
        // NOTE: a small epsilon is added to the value to fix a bug where
        // for example, 1.505 at a fixed precision of 2 does not round to 1.51
        constexpr double eps = 1e-8;
        std::string proposed =
                fmt::format("{:.{}f}", value + eps, static_cast<int>(precision));
        int end_idx = static_cast<int>(proposed.size());
        bool has_decimal = false;
        for (char const& ch : proposed)
        {
            if (ch == '.')
            {
                has_decimal = true;
                break;
            }
        }
        if (has_decimal)
        {
            while (proposed[end_idx - 1] == '0' && end_idx > 0)
            {
                --end_idx;
            }
        }
        if (proposed[end_idx - 1] == '.')
        {
            --end_idx;
        }
        return proposed.substr(0, end_idx);
    }

} // namespace erin
