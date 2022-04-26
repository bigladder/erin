/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
// reference: https://stackoverflow.com/a/1120224

#ifndef ERIN_CSV_H
#define ERIN_CSV_H
#include <iostream>
#include <functional>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

namespace erin_csv
{
  std::vector<std::string> read_row(std::istream& stream);

  void stream_out(std::ostream& stream, const std::vector<std::string>& xs);

  void write_csv(
      std::ostream& os,
      const std::vector<std::string>& items,
      bool start=true,
      bool end_with_lf=true);

  template <class T>
  void
  write_csv_with_tranform(
      std::ostream& os,
      const std::vector<T>& items,
      const std::function<std::string(const T&)>& f,
      bool start=true,
      bool end_with_lf=true)
  {
    std::vector<std::string> out(items.size());
    std::transform(items.begin(), items.end(), out.begin(), f);
    write_csv(os, out, start, end_with_lf);
  }
}
#endif // ERIN_CSV_H
