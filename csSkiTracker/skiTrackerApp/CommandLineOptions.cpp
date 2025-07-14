#include "CommandLineOptions.h"
#include <tclap/CmdLine.h>

CommandLineOptions::CommandLineOptions(int argc, char *argv[]) {
  try {
    TCLAP::CmdLine cmd("", ' ');

    TCLAP::UnlabeledValueArg<std::string> filename("filePath", "filename to read. Note, a file with the same name and extension ", false, "", "string");
    cmd.add(filename);
    cmd.parse(argc, argv);

    this->filename = filename.getValue();

  }
  catch (TCLAP::ArgException &e)  // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    std::exit(-1);
  }
}

CommandLineOptions::~CommandLineOptions() {

}
