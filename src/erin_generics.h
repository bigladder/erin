/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_GENERICS_H
#define ERIN_GENERICS_H
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace erin_generics
{
  template <class TOut, class TResElem, class TStats>
  std::unordered_map<std::string, TOut>
  derive_statistic(
      const std::unordered_map<std::string, std::vector<TResElem>>& results,
      const std::vector<std::string>& keys,
      std::unordered_map<std::string, TStats>& statistics,
      const std::function<TStats(const std::vector<TResElem>&)>& calc_all_stats,
      const std::function<TOut(const TStats&)>& derive_stat)
  {
    std::unordered_map<std::string, TOut> out{};
    for (const auto k: keys) {
      auto stat_it = statistics.find(k);
      if (stat_it == statistics.end())
        statistics[k] = calc_all_stats(results.at(k));
      out[k] = derive_stat(statistics[k]);
    }
    return out;
  }

  template <class T>
  void
  print_unordered_map(const std::string& var, const std::unordered_map<std::string,T>& m)
  {
    std::cout << var << " = \n";
    for (const auto& e: m) {
      std::cout << "  " << e.first << ": " << e.second << "\n";
    }
  }
}
#endif // ERIN_GENERICS_H
