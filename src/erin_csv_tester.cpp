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
    std::cout << "USAGE: " << args[0] << " <csv_file>\n"
              << "- csv_file: path to a csv file\n"
              << "\nPrints out the csv file to the command line\n"
              << std::flush;
    return 1;
  }
  std::string csv_path{args[1]};
  try {
    std::ifstream ifs{csv_path};
    if (!ifs.is_open()) {
      std::ostringstream oss{};
      oss << "input file stream on \"" << csv_path
          << "\" failed to open for reading\n";
      std::cerr << oss.str() << "\n";
      return 1;
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
  }
  catch (const std::exception& e) {
    std::cout << "Unhandled exception: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
