#pragma once
#include <string>

struct CommandLineOptions {
  std::string filename;
  int startFrame1, startFrame2;
  std::string fixedPointsString;
  bool smoke;

  CommandLineOptions(int argc, char *argv[]);
  virtual ~CommandLineOptions();
};
