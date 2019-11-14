/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
// reference: https://stackoverflow.com/a/1120224

#ifndef ERIN_CSV_H
#define ERIN_CSV_H
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

namespace erin_csv
{
  std::vector<std::string>
  read_row(std::istream& stream)
  {
    std::vector<std::string> data;
    std::string line;
    std::getline(stream, line);
    if (line.empty()) {
      return data;
    }
    std::stringstream line_stream(line);
    std::string cell;
    while(std::getline(line_stream, cell, ',')) {
      data.push_back(cell);
    }
    if (!line_stream && cell.empty()) {
      data.push_back("");
    }
    return data;
  }

  void
  stream_out(std::ostream& stream, const std::vector<std::string>& xs)
  {
    for (std::vector<std::string>::size_type i{0}; i < xs.size(); ++i) {
      if (i == 0) {
        stream << "[";
      }
      else {
        stream << ", ";
      }
      stream << xs[i];
    }
    stream << "]";
  }
}
#endif // ERIN_CSV_H
