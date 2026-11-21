project(csSkiTracker LANGUAGES CXX)

# prepare custom project asserts, note: use standard find_package
find_package(csCore REQUIRED COMPONENTS assert CONFIG)

# create a project assert include
list(APPEND CMAKE_MODULE_PATH "${csCore_BASE_DIR}/cmake/csCore/assert/")
include(csAssert)

# set defaults for Paranoid additional configuration
cssetassertdefaultonforconfig(${PROJECT_NAME} ASSERT RELEASE Paranoid ON)
cssetassertdefaultonforconfig(${PROJECT_NAME} ASSERT RELWITHDEBINFO Paranoid ON)
cssetassertdefaultonforconfig(${PROJECT_NAME} ASSERT DEBUG Paranoid ON)
cssetassertdefaultonforconfig(${PROJECT_NAME} ASSERT PARANOID Paranoid ON)
cssetassertdefaultonforconfig(${PROJECT_NAME} ASSERT SAFE_MATH Paranoid ON)

cs_assert_generate_project(csSkiTracker csSkiTracker) #generate target csSkiTracker::csSkiTrackerAssert
list(POP_BACK CMAKE_MODULE_PATH)

cs_add_library(csSkiTracker INTERFACE)
cs_add_library(csSkiTracker::csSkiTracker ALIAS csSkiTracker)

cs_target_link_libraries(csSkiTracker INTERFACE csSkiTracker::cscmake_base_include)

cs_install_target(csSkiTracker COMPONENT csSkiTracker DEFAULT)

# add libs, executables, etc
add_subdirectory(dataReader)
add_subdirectory(skiTrackerApp)
# add_subdirectory(...)
