/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */

#ifndef ERIN_PORT_H
#define ERIN_PORT_H
#include <string>

namespace erin::port
{
  const std::string outflow{"outflow"}; 
  const std::string inflow{"inflow"};

  enum class Type
  {
    Inflow = 0,
    Outflow
  };

  Type tag_to_type(const std::string& tag);
  std::string type_to_tag(Type t);
}

#endif // ERIN_PORT_H
