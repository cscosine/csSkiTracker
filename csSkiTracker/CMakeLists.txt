project(csSkiTracker LANGUAGES CXX)

# prepare custom project asserts, note: use standard find_package
find_package(csCore REQUIRED COMPONENTS assert CONFIG)

# create a project assert include
list(APPEND CMAKE_MODULE_PATH "${csCore_BASE_DIR}/cmake/csCore/assert/")
include(csAssert)

# set defaults for Paranoid additional configuration
csSetAssertDefaultOnForConfig(${PROJECT_NAME} ASSERT RELEASE Paranoid ON)
csSetAssertDefaultOnForConfig(${PROJECT_NAME} ASSERT RELWITHDEBINFO Paranoid ON)
csSetAssertDefaultOnForConfig(${PROJECT_NAME} ASSERT DEBUG Paranoid ON)
csSetAssertDefaultOnForConfig(${PROJECT_NAME} ASSERT PARANOID Paranoid ON)
csSetAssertDefaultOnForConfig(${PROJECT_NAME} ASSERT SAFE_MATH Paranoid ON)

csAssertGenerateProject(csSkiTracker csSkiTracker) #generate target csSkiTracker::csSkiTrackerAssert
list(POP_BACK CMAKE_MODULE_PATH)

cs_add_library(csSkiTracker INTERFACE)
cs_add_library(csSkiTracker::csSkiTracker ALIAS csSkiTracker)

cs_target_sources(csSkiTracker
	PRIVATE_INSTALL
	PRIVATE
)

cs_target_link_libraries(csSkiTracker
	INTERFACE
		csSkiTrackerLib::csSkiTrackerLib_csCMakeBaseInclude
		csSkiTrackerLib::csSkiTrackerLib_csCMakeCompileOptionsDefault
)

cs_install_target(csSkiTracker csSkiTracker)

cs_finalize_target(csSkiTracker)

# add libs, executables, etc
add_subdirectory(dataReader)
add_subdirectory(skiTrackerApp)
# add_subdirectory(...)