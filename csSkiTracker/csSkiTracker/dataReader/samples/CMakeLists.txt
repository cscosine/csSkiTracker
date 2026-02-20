project(dataReaderSamples LANGUAGES CXX)

#---------------------------------------------------------------
# prepare a set of samples dependent on the above library
set(SAMPLES_LIST
  dataReader.cpp
)

foreach(s ${SAMPLES_LIST})
	get_filename_component(sn "${s}" NAME_WE)
	cs_add_executable(dataReaderSamples${sn})
	cs_target_sources(dataReaderSamples${sn} PRIVATE ${s})
	cs_target_link_libraries(dataReaderSamples${sn} PRIVATE csSkiTracker::dataReader)
	cs_install_target(dataReaderSamples${sn} dataReaderSamples${sn})
	cs_finalize_target(dataReaderSamples${sn})
endforeach()
