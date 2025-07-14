#pragma once
#include <string>

struct CommandLineOptions {
  std::string filename;

  CommandLineOptions(int argc, char* argv[]);
  virtual ~CommandLineOptions();
};
