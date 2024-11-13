// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
// reference: https://stackoverflow.com/a/1120224
#ifndef ERIN_CSV_H
#define ERIN_CSV_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace erin
{

std::vector<std::string> read_row(std::istream& stream);

void stream_out(std::ostream& stream, const std::vector<std::string>& xs);

void write_csv(std::ostream& os,
               const std::vector<std::string>& items,
               bool start = true,
               bool end_with_lf = true);

} // namespace erin

#endif
