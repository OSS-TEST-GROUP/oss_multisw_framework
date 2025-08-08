cmake_minimum_required(VERSION 3.5)
project(agl_adsw_manager)

# Unittest
if (BUILD_UNITTEST)
	add_subdirectory(unittest)
endif()

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
	service_controller/ContainerController.cpp
	service_controller/AppController.cpp
)

set(SERVICE_MONITOR
    service_monitor/ServiceMonitor.cpp
    service_monitor/AppStateReader.cpp
)

set(SERVICE_REGISTRY
    service_registry/ServiceRegistry.cpp
)

set(MAIN main.cpp 
         ADSWManager.cpp 
	 	 ${MESSAGE_TRANSMITTER} 
	 	 ${SERVICE_CONTROLLER} 
         ${SERVICE_MONITOR}
         ${SERVICE_REGISTRY} 
)

set(GENERATED_AGL_DIR ${CMAKE_INSTALL_PREFIX}/generated/idl)

include_directories(. service_registry message_transmitter service_controller ${GENERATED_AGL_DIR})
link_libraries(fastcdr fastdds)

file(GLOB GENERATED_AGL_FILES "${GENERATED_AGL_DIR}/agl/*.cxx")

add_executable(${PROJECT_NAME} ${MAIN} ${GENERATED_AGL_FILES})

# install(TARGET ${PROJECT_NAME}
# 	RUNTIME DESTINATION bin
# 	LIBRARY DESTINATION lib
# 	ARCHIVE DESTINATION lib
# )

# install(DIRECTORY
# 	${CMAKE_SOURCE_DIR}/idl/dds/common
# 	${CMAKE_SOURCE_DIR}/idl/lifecycle
#	${CMAKE_SOURCE_DIR}/idl/agl
# 	DESTINATION bin/idl
# )