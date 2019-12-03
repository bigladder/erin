/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/port.h"
#include <sstream>
#include <stdexcept>

namespace erin::port
{
  Type
  tag_to_type(const std::string& tag)
  {
    if (tag == inflow) {
      return Type::Inflow;
    }
    if (tag == outflow) {
      return Type::Outflow;
    }
    std::ostringstream oss;
    oss << "unhandled tag \"" << tag << "\"";
    throw std::invalid_argument(oss.str());
  }

  std::string
  type_to_tag(Type t)
  {
    switch (t) {
      case Type::Inflow:
        return inflow;
      case Type::Outflow:
        return outflow;
    }
    std::ostringstream oss;
    oss << "unhandled port type \"" << static_cast<int>(t) << "\"";
    throw std::invalid_argument(oss.str());
  }
}
