#include "erin_csv.h"
#include "gsl/span"
#include <fstream>
#include <iostream>
#include <stdexcept>

int
main(const int argc, const char* argv[])
{
  namespace csv = erin_csv;
  auto args = gsl::span<const char*>(argv, argc);
  if (argc != 2) {
    std::cout << "USAGE: " << argv[0] << " <csv_file>\n";
    std::cout << "- csv_file: path to a csv file\n";
    std::cout << "\nPrints out the csv file to the command line\n";
    return 1;
  }
  std::string csv_path{args[1]};
  std::ifstream ifs{csv_path};
  if (!ifs.is_open()) {
    std::ostringstream oss;
    oss << "input file stream on \"" << csv_path
        << "\" failed to open for reading\n";
    throw std::runtime_error(oss.str());
  }
  std::cout << "Contents of " << csv_path << ":\n";
  for (int row{0}; ifs.good(); ++row) {
    std::string delim{""};
    auto cells = csv::read_row(ifs);
    if (cells.size() == 0) {
      break;
    }
    std::cout << row << ": ";
    for (const auto& c : cells) {
      std::cout << delim << c;
      delim = ",";
    }
    std::cout << "\n";
  }
  ifs.close();
  return 0;
}
