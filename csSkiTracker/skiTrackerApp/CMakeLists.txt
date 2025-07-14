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
	Visualizer.h
	Window.h
	Window.cpp
  WindowPlots.cpp
	Visualizer.cpp
	main.cpp
	Window.ui
  CommandLineOptions.h
  CommandLineOptions.cpp
  ProjectionMatrixEstimate.h
  ProjectionMatrixEstimate.cpp
  ProblemDescription.h
  ProblemDescription.cpp
  Triangulate.h
  Triangulate.cpp
#--------------------
  PoseNode.h
  PoseNode.cpp
  Camera3dPointReprojectionError.h
  Camera3dPointReprojectionError.cpp
  CameraNode.h
  CameraNode.hpp
  CameraNode.cpp
  CameraCalibSection.h
  CameraCalibSection.cpp
  CameraPoseNodeBase.h
  CameraPoseNodeBase.cpp
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