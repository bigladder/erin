/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/mux.h"
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
  MuxState
  make_mux_state(int num_inflows, int num_outflows)
  {
    if (num_inflows < 1) {
      std::ostringstream oss{};
      oss << "num_inflows must be > 0\n"
          << "num_inflows = " << num_inflows << "\n";
      throw std::invalid_argument(oss.str());
    }
    if (num_outflows < 1) {
      std::ostringstream oss{};
      oss << "num_outflows must be > 0\n"
          << "num_outflows = " << num_outflows << "\n";
      throw std::invalid_argument(oss.str());
    }
    return MuxState{
      num_inflows,
      num_outflows,
      std::vector<Port>(num_inflows),
      std::vector<Port>(num_outflows)};
  }
}
