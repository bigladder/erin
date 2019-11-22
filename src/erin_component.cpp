/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/component.h"
#include <sstream>
#include <stdexcept>
#include <string>

namespace ERIN
{
  ComponentType
  tag_to_component_type(const std::string& tag)
  {
    if (tag == "load") {
      return ComponentType::Load;
    }
    else if (tag == "source") {
      return ComponentType::Source;
    }
    else if (tag == "converter") {
      return ComponentType::Converter;
    }
    else {
      std::ostringstream oss;
      oss << "Unhandled tag \"" << tag << "\"";
      throw std::invalid_argument(oss.str());
    }
  }

  std::string
  component_type_to_tag(ComponentType ct)
  {
    switch(ct) {
      case ComponentType::Load:
        return std::string{"load"};
      case ComponentType::Source:
        return std::string{"source"};
      case ComponentType::Converter:
        return std::string{"converter"};
      default:
        std::ostringstream oss;
        oss << "Unhandled ComponentType \"" << static_cast<int>(ct) << "\"";
        throw std::invalid_argument(oss.str());
    }
  }
}
