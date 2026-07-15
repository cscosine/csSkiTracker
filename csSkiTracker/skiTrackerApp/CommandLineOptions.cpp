#include "CommandLineOptions.h"
#include <tclap/CmdLine.h>

CommandLineOptions::CommandLineOptions(int argc, char* argv[]) {
  try {
    TCLAP::CmdLine cmd("", ' ');

    TCLAP::UnlabeledValueArg<std::string> filename("filePath", "filename to read. Note, a file with the same name and extension ",
                                                   false, "", "string");

    TCLAP::ValueArg<int> startFrame1("", "s1", "start frame for camera 1 (fix)", false, 0, "number");
    TCLAP::ValueArg<int> startFrame2("", "s2", "start frame for camera 1 (fix)", false, 0, "number");
    TCLAP::ValueArg<std::string> fixedPointsString("f", "fixedPoints", "Comma separated fixed points", false, "",
                                                   "3 int comma separated");

    cmd.add(filename);
    cmd.add(startFrame1);
    cmd.add(startFrame2);
    cmd.add(fixedPointsString);
    cmd.parse(argc, argv);

    this->filename = filename.getValue();
    this->startFrame1 = startFrame1.getValue();
    this->startFrame2 = startFrame2.getValue();
    this->fixedPointsString = fixedPointsString.getValue();

  } catch (TCLAP::ArgException& e) // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    std::exit(-1);
  }
}

CommandLineOptions::~CommandLineOptions() {}
