/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#include "disco/disco.h"

namespace DISCO {

void
Model::addComponent(std::string const id, Component comp) {
    _comps.insert(std::pair<std::string,Component>(id, comp));
}

void
Model::connect(std::string const id1, int const port1, std::string const id2, int const port2) {
    // do nothing for now
}

}
