/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/storage.h"
#include <sstream>
#include <stdexcept>
#include <string>

namespace erin::devs
{
  StorageState
  storage_make_state(
      FlowValueType capacity,
      double soc)
  {
    if ((soc > 1.0) || (soc < 0.0)) {
      std::ostringstream oss{};
      oss << "soc must be >= 0.0 and <= 1.0\n"
          << "soc = " << soc << "\n";
      throw std::invalid_argument(oss.str());
    }
    if (capacity < 0.0) {
      std::ostringstream oss{};
      oss << "capacity must be >= 0.0\n"
          << "capacity = " << capacity << "\n";
      throw std::invalid_argument(oss.str());
    }
    return StorageState{
      capacity,
      soc,
      Port{0, 0.0, 0.0},
      Port{0, 0.0, 0.0},
      false,
      false};
  }
}
