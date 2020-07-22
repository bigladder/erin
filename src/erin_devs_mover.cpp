
/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs/mover.h"
#include <sstream>
#include <stdexcept>

namespace erin::devs
{

  MoverData
  make_mover_data(FlowValueType COP)
  {
    if (COP <= 0.0) {
      std::ostringstream oss{};
      oss << "COP must be > 0.0 but is " << COP << ".\n";
      throw std::runtime_error(oss.str());
    }
    return MoverData{COP};
  }

  bool operator==(const MoverData& a, const MoverData& b)
  {
    return (a.COP == b.COP);
  }

  bool operator!=(const MoverData& a, const MoverData& b)
  {
    return !(a == b);
  }

  std::ostream& operator<<(std::ostream& os, const MoverData& a)
  {
    return os << "MoverData(COP = " << a.COP << ")";
  }

  MoverState make_mover_state()
  {
    return MoverState{};
  }

  bool operator==(const MoverState& a, const MoverState& b)
  {
    return (a.time == b.time) &&
           (a.inflow0_port == b.inflow0_port) &&
           (a.inflow1_port == b.inflow1_port) &&
           (a.outflow_port == b.outflow_port) &&
           (a.report_inflow0_request == b.report_inflow0_request) &&
           (a.report_inflow1_request == b.report_inflow1_request) &&
           (a.report_outflow_achieved == b.report_outflow_achieved);
  }

  bool operator!=(const MoverState& a, const MoverState& b)
  {
    return !(a == b);
  }

  std::ostream& operator<<(std::ostream& os, const MoverState& a)
  {
    return os << "MoverState("
              << "time = " << a.time << ", "
              << "inflow0_port = " << a.inflow0_port << ", "
              << "inflow1_port = " << a.inflow1_port << ", "
              << "outflow_port = " << a.outflow_port << ", "
              << "report_inflow0_request = " << a.report_inflow0_request << ", "
              << "report_inflow1_request = " << a.report_inflow1_request << ", "
              << "report_outflow_achieved = "
              << a.report_outflow_achieved << ")";
  }

  ////////////////////////////////////////////////////////////
  // output function
  std::vector<PortValue>
  mover_output_function(const MoverData& d, const MoverState& s)
  {
    std::vector<PortValue> ys{};
    mover_output_function_mutable(d, s, ys);
    return ys;
  }

  void
  mover_output_function_mutable(
      const MoverData& d,
      const MoverState& s,
      std::vector<PortValue>& ys)
  {
  }
}
