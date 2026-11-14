project(skiTrackerApp LANGUAGES CXX)

cs_add_executable(skiTrackerApp)
cs_add_executable(csSkiTracker::skiTrackerApp ALIAS skiTrackerApp)

cs_find_package(skiTrackerApp PUBLIC Eigen3 REQUIRED CONFIG)
cs_find_package(skiTrackerApp PUBLIC Qt6 COMPONENTS Core Widgets  REQUIRED CONFIG)
cs_find_package(skiTrackerApp PUBLIC csVisOpenGL REQUIRED COMPONENTS csVisOpenGL renderers painters CONFIG)
cs_find_package(skiTrackerApp PUBLIC tclap REQUIRED CONFIG)
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
	csVisOpenGL::painters
	csVisOpenGL::renderers
    csCamera::csCamera
    csLie::csLie
    csNelson::csNelson
    tclap::tclap
    )

cs_install_target(skiTrackerApp COMPONENT skiTrackerApp DEFAULT)

# deployment: a folder that runs on another machine, made by the install (Release), with the Qt plugins it needs and
# the sample dataset (data/), on which the smoke run starts the application
cs_deployment(app TARGETS skiTrackerApp)
cs_deployment(
  app
  FILES "${CMAKE_CURRENT_LIST_DIR}/../../datasets/frame_data_bolshunov.std"
  DESTINATION data
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  cs_deployment(
    app
    PLUGINS Qt6::QXcbIntegrationPlugin Qt6::QWaylandIntegrationPlugin Qt6::QOffscreenIntegrationPlugin
    DESTINATION plugins/platforms
  )
  cs_deployment(
    app
    PLUGINS Qt6::QXcbGlxIntegrationPlugin Qt6::QXcbEglIntegrationPlugin
    DESTINATION plugins/xcbglintegrations
  )
  cs_deployment(app PLUGINS Qt6::QWaylandXdgShellIntegrationPlugin DESTINATION plugins/wayland-shell-integration)
  cs_deployment(
    app
    PLUGINS Qt6::QWaylandEglClientBufferPlugin
    DESTINATION plugins/wayland-graphics-integration-client
  )
  cs_deployment(app PLUGINS Qt6::QWaylandBradientDecorationPlugin DESTINATION plugins/wayland-decoration-client)
elseif(WIN32)
  # Windows: next to the executable (bin/platforms), where Qt looks for them (the layout of windeployqt)
  cs_deployment(
    app
    PLUGINS Qt6::QWindowsIntegrationPlugin Qt6::QOffscreenIntegrationPlugin
    DESTINATION bin/platforms
  )
  # the software OpenGL of Qt (loaded at run time), for the machines without GPU (QT_OPENGL=software)
  if(EXISTS "${QT6_INSTALL_PREFIX}/bin/opengl32sw.dll")
    cs_deployment(app FILES "${QT6_INSTALL_PREFIX}/bin/opengl32sw.dll" DESTINATION bin)
  else()
    message(STATUS "opengl32sw.dll not in ${QT6_INSTALL_PREFIX}/bin: not deployed")
  endif()
endif()
