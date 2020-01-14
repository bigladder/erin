/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_PORT_H
#define ERIN_PORT_H
#include <string>

namespace erin::port
{
  const std::string outflow{"outflow"}; 
  const std::string inflow{"inflow"};
  const std::string lossflow{"lossflow"};

  enum class Type
  {
    Inflow = 0,
    Outflow,
    Lossflow
  };

  Type tag_to_type(const std::string& tag);
  std::string type_to_tag(Type t);
}

#endif // ERIN_PORT_H
