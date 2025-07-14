project(dataReader LANGUAGES CXX)

cs_add_library(dataReader STATIC)
cs_add_library(csSkiTracker::dataReader ALIAS dataReader)

cs_find_package(dataReader PUBLIC Eigen3 REQUIRED CONFIG)
cs_find_package(dataReader PUBLIC Qt6 COMPONENTS Core REQUIRED CONFIG)


cs_target_sources(dataReader
	PRIVATE_INSTALL
    DataDescription.h
		DataReader.h
	PRIVATE
    DataDescription.cpp
		DataReader.cpp
    )

cs_target_link_libraries(dataReader
	PUBLIC
		csSkiTracker::csSkiTracker
		Eigen3::Eigen
		Qt6::Core
)

add_subdirectory(samples)

cs_install_target(dataReader dataReader)

cs_finalize_target(dataReader)
