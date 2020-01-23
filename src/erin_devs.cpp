/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#include "erin/devs.h"
#include <sstream>
#include <stdexcept>

namespace erin::devs
{
  Port::Port():
    Port(0.0, 0.0)
  {}

  Port::Port(double r, double a):
    requested{r},
    achieved{a}
  {
  }

  Port
  Port::with_requested(double new_requested) const
  {
    // when we set a new request, we assume achieved is met until we hear
    // otherwise
    return Port{new_requested, new_requested};
  }

  Port
  Port::with_achieved(double new_achieved) const
  {
    // when we set an achieved flow, we do not touch the request; we are still
    // requesting what we request regardless of what is achieved.
    // if achieved is more than requested, that is an error.
    if (new_achieved > requested) {
      std::ostringstream oss;
      oss << "achieved more than requested error!\n"
          << "new_achieved: " << new_achieved << "\n"
          << "requested   : " << requested << "\n"
          << "achieved    : " << achieved << "\n";
      throw std::invalid_argument(oss.str());
    }
    return Port{requested, new_achieved};
  }
}
