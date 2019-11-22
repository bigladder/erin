/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_COMPONENT_H
#define ERIN_COMPONENT_H
#include <string>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // ComponentType
  enum class ComponentType
  {
    Load = 0,
    Source,
    Converter
  };

  ComponentType tag_to_component_type(const std::string& tag);

  std::string component_type_to_tag(ComponentType ct);
}

#endif // ERIN_COMPONENT_H
