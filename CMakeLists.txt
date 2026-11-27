cmake_minimum_required(VERSION 3.31)

# before project(): the oldest macOS the binaries run on (Qt 6.12)
include(csCMakeSetup)
cs_macos_deployment_target(14.4)

project(csSkiTrackerSol VERSION 0.1.1 LANGUAGES CXX)

# add the csCMake system
include(csCMake)

cs_initialize_project(
  NAMESPACE csSkiTracker
  CXX_STANDARD 17
  # add the default configurations supported by this project
  DEFAULT_CONFIG Debug
  DEFAULT_CONFIG Release
  DEFAULT_CONFIG_MAPPED RelWithDebInfo
  MAPPED_TO "RelWithDebInfo,Release,"
  # and the custom configurations supported by this project,
  # specifiying on which base configuration they are based as second value,
  # e.g. CUSTOM_CONFIG Paranoid Debug
  CUSTOM_CONFIG Paranoid
  BASED_ON "Debug,Release,"
  POSTFIX "dp"
)

# add libs, executables, etc
add_subdirectory(csSkiTracker)
