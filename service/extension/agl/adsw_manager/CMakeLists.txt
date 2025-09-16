cmake_minimum_required(VERSION 3.5)
project(agl_adsw_manager)

find_package(fastcdr REQUIRED)
find_package(fastdds REQUIRED)

set(MESSAGE_TRANSMITTER
	message_transmitter/MessageTransmitter.cpp
	message_transmitter/DDSManager.cpp
	message_transmitter/AppStateHandler.cpp
	message_transmitter/StartAppHandler.cpp
	message_transmitter/StopAppHandler.cpp
	message_transmitter/NotifyAppStateWriter.cpp
)

set(SERVICE_CONTROLLER
	service_controller/ServiceController.cpp
	service_controller/AppController.cpp
)

set(SERVICE_MONITOR
    service_monitor/ServiceMonitor.cpp
    service_monitor/AppStateReader.cpp
)

set(SERVICE_REGISTRY
    service_registry/ServiceRegistry.cpp
)

set(GENERATED_IDL_DIR ${CMAKE_INSTALL_PREFIX}/generated/idl)

file(GLOB GENERATED_LIFECYCLE_FILES "${GENERATED_IDL_DIR}/lifecycle/*.cxx")
file(GLOB GENERATED_AGL_FILES "${GENERATED_IDL_DIR}/agl/*.cxx")

add_library(agl_adsw_core STATIC
			ADSWManager.cpp
			${MESSAGE_TRANSMITTER}
			${SERVICE_CONTROLLER}
			${SERVICE_MONITOR}
			${SERVICE_REGISTRY}
			${GENERATED_LIFECYCLE_FILES}
			${GENERATED_AGL_FILES}
)

target_include_directories(agl_adsw_core PUBLIC
							.
							service_registry
							message_transmitter
							service_controller
							${GENERATED_IDL_DIR})

target_link_libraries(agl_adsw_core PUBLIC
						fastcdr
						fastdds)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE agl_adsw_core)

option(BUILD_UNITTEST "Build unit tests" OFF)
if (BUILD_UNITTEST)
  add_subdirectory(unittest)
endif()

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

# install(DIRECTORY
# 	${CMAKE_SOURCE_DIR}/idl/dds/common
# 	${CMAKE_SOURCE_DIR}/idl/lifecycle
#	${CMAKE_SOURCE_DIR}/idl/agl
# 	DESTINATION bin/idl
# )
