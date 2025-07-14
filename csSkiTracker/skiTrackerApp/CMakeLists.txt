project(skiTrackerApp LANGUAGES CXX)

cs_add_executable(skiTrackerApp)
cs_add_executable(csSkiTracker::skiTrackerApp ALIAS skiTrackerApp)

cs_find_package(skiTrackerApp PUBLIC Eigen3 REQUIRED CONFIG)
cs_find_package(skiTrackerApp PUBLIC Qt6 COMPONENTS Core Widgets  REQUIRED CONFIG)
cs_find_package(skiTrackerApp PUBLIC csVisOpenGL REQUIRED COMPONENTS csVisOpenGL CONFIG)
if(WIN32)
  # TODO(sc) add tclap in windows
  cs_find_package(skiTrackerApp PUBLIC tclap REQUIRED CONFIG)
endif()
cs_find_package(skiTrackerApp PUBLIC csNelson REQUIRED COMPONENTS csNelson CONFIG)
cs_find_package(skiTrackerApp PUBLIC csCamera REQUIRED COMPONENTS csCamera CONFIG)
cs_find_package(skiTrackerApp PUBLIC csLie REQUIRED COMPONENTS csLie CONFIG)


cs_target_sources(skiTrackerApp
	PRIVATE
  Camera3dPointReprojectionError.cpp
  Camera3dPointReprojectionError.h
  CameraCalibSection.cpp
  CameraCalibSection.h
  CameraNode.cpp
  CameraNode.h
  CameraNode.hpp
  CameraPosePointNodeBase.cpp
  CameraPosePointNodeBase.h
  CommandLineOptions.cpp
  CommandLineOptions.h
  main.cpp
  PointNode.cpp
  PointNode.h
  PoseNode.cpp
  PoseNode.h
  ProblemDescription.cpp
  ProblemDescription.h
  ProjectionMatrixEstimate.cpp
  ProjectionMatrixEstimate.h
  SkierModel.cpp
  SkierModel.h
  Triangulate.cpp
  Triangulate.h
  Visualizer.cpp
  Visualizer.h
  Window.cpp
  Window.h
  WindowPlots.cpp
  Window.ui
    )

	set_target_properties(skiTrackerApp PROPERTIES AUTOMOC ON)
set_target_properties(skiTrackerApp PROPERTIES AUTOUIC ON)
set_target_properties(skiTrackerApp PROPERTIES AUTORCC ON)

cs_target_link_libraries(skiTrackerApp
	PUBLIC
		csSkiTracker::dataReader
		Eigen3::Eigen
		Qt6::Core
		Qt6::Widgets
	csVisOpenGL::csVisOpenGL
    csCamera::csCamera
    csLie::csLie
    csNelson::csNelson)

cs_install_target(skiTrackerApp skiTrackerApp)

cs_finalize_target(skiTrackerApp)
