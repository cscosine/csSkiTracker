#include "csSkiTracker/dataReader/DataReader.h"
#include <iostream>

int main(int argc, char* argv[]) {

  if (argc != 2) {
    std::cerr << "specify input file as argument" << std::endl;
    return -1;
  }

  csSkiTracker::dataReader::DataReader reader;
  auto d = reader.readFromFile(argv[1]);
  if (d == nullptr) {
    std::cerr << "Error " << reader.error().toStdString() << std::endl;
  }
  else {
    std::cout << "Read " << d->frames.size() << " frames" << std::endl;
  }

  return 0;
}