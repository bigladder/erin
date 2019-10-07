/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */
#include <iostream>
#include "disco/disco.h"


int
main(int argc, char *argv[]) {
  if (argc != 3)
    // TODO: Add detailed error message
    return 1;
  auto input_toml = std::string{argv[1]};
  auto output_csv = std::string{argv[2]};

  auto m = DISCO::Main(input_toml, output_csv);
  std::cout << "Success!\n";
  return 0;
}
