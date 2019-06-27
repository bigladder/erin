/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#ifndef DISCO_DISCO_H
#define DISCO_DISCO_H
#include <string>
#include <map>
#include "../../vendor/bdevs/include/adevs.h"
#include "../../vendor/bdevs/include/adevs_time.h"

namespace DISCO {
  class Component {
    public:
  };

  class Source : public Component {
    public:
      static const int OUT = 1;
  };

  class Sink : public Component {
    public:
      static const int IN = 1;
  };

  class Results {
    public:
  };

  class Model {
    public:
      void addComponent(std::string const id, Component const comp);
      void connect(std::string const id1, int const port1, std::string const id2, int const port2);
      Results simulate();

    private:
      std::map<std::string, Component> _comps;
  };

}

#endif // DISCO_DISCO_H
