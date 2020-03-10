/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/mux.h"
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
  void
  check_num_flows(const std::string& tag, int n)
  {
    if ((n < minimum_number_of_ports) || (n > maximum_number_of_ports)) {
      std::ostringstream oss{};
      oss << tag << " must be >= " << minimum_number_of_ports
          << " and <= " << maximum_number_of_ports << "\n"
          << tag << " = " << n << "\n";
      throw std::invalid_argument(oss.str());
    }
  }

  MuxState
  make_mux_state(int num_inflows, int num_outflows)
  {
    check_num_flows("num_inflows", num_inflows);
    check_num_flows("num_outflows", num_outflows);
    return MuxState{
      0,
      num_inflows,
      num_outflows,
      std::vector<Port>(num_inflows),
      std::vector<Port>(num_outflows)};
  }

  RealTimeType
  mux_current_time(const MuxState& state)
  {
    return state.time;
  }

  RealTimeType
  mux_time_advance(const MuxState& /* state */)
  {
    return infinity;
  }

  MuxState
  mux_external_transition(
      const MuxState& state,
      RealTimeType dt,
      const std::vector<PortValue>& xs)
  {
    return MuxState{
      state.time + dt,
      state.num_inflows,
      state.num_outflows,
      state.inflow_ports,
      state.outflow_ports};
  }
}
