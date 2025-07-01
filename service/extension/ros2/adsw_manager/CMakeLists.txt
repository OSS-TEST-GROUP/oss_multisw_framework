cmake_minimum_required(VERSION 3.5)

project ( "adsw_manager" )

## Unittest
if (BUILD_UNITTEST)
    add_subdirectory ( unittest )
endif()


set ( MISC_SRC
    logger/Logger.cpp
)

set ( TRANSMIT_SRC
    transmitter/DDSTransmitter.cpp
    transmitter/DDSMessageEndpoint.cpp
    endpoint/EndPointFactory.cpp
    endpoint/ROSDiscoveryInfoEndpoint.cpp
)

set ( SERVICE_SRC
    service/ServiceRegistry.cpp
    service/Service.cpp
)

set ( MONITOR_SRC
    monitor/ServiceMonitor.cpp
    monitor/ROSServiceDiscovery.cpp
    monitor/UpdateServiceInfo.cpp
)

set ( CONTROL_SRC
    control/ContainerController.cpp
    control/ROS2NodeController.cpp
    control/ROSServiceController.cpp
    control/ServiceController.cpp
)

# IDL generation
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")

set(GENERATED_DDS_FILES
                        ${GENERATED_DIR}/ros2/GidPubSubTypes.cxx
                        ${GENERATED_DIR}/ros2/GidTypeObjectSupport.cxx
                        ${GENERATED_DIR}/ros2/NodeEntitiesInfoPubSubTypes.cxx
                        ${GENERATED_DIR}/ros2/NodeEntitiesInfoTypeObjectSupport.cxx
                        ${GENERATED_DIR}/ros2/ParticipantEntitiesInfoPubSubTypes.cxx
                        ${GENERATED_DIR}/ros2/ParticipantEntitiesInfoTypeObjectSupport.cxx
)


## Source
set ( MAIN_SRC main.cc
               ADSWManager.cpp
               ${MONITOR_SRC} ${SERVICE_SRC} ${TRANSMIT_SRC} ${CONTROL_SRC} ${MISC_SRC}
)

include_directories (
    .
    ${CMAKE_SOURCE_DIR}/service
)
include_directories ( ${GENERATED_DIR} )

## Library
#link_libraries ( pthread )
#link_directories ( ${CMAKE_INSTALL_PREFIX} )
link_libraries ( pthread fastcdr fastdds )

## Target binary
#add_library ( abc_static STATIC or SHARED or OBJECT $<TARGET_OBJECTS:abcd_obj> )
#set_target_properties ( abc_static PROPERTIES OUTPUT_NAME abc )
#target_link_libraries ( abc_static ${} )

#add_executable ( ${PROJECT_NAME} ${MAIN_SRC} $<TARGET_OBJECTS:abc_obj> )
add_executable ( ${PROJECT_NAME} ${MAIN_SRC} ${GENERATED_DDS_FILES})

## Install
install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
