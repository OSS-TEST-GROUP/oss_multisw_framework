cmake_minimum_required(VERSION 3.5)

project ( "adsw_manager_general" )


set ( MISC_SRC
    logger/Logger.cpp
)

set ( TRANSMIT_SRC
    transmitter/DDSTransmitter.cpp
    transmitter/DDSMessageEndpoint.cpp
    transmitter/MessageQueue.cpp
    endpoint/EndPointFactory.cpp
    endpoint/GetServiceStateEndPoint.cpp
    endpoint/NotifyServiceStateEndPoint.cpp
    endpoint/StartServiceEndPoint.cpp
    endpoint/StopServiceEndPoint.cpp
    endpoint/UpdateServiceListEndPoint.cpp
    endpoint/AdswServiceStatusEndPoint.cpp
    # F013: container actuator surface
    endpoint/NotifyContainerStateEndPoint.cpp
    endpoint/StartContainerEndPoint.cpp
    endpoint/StopContainerEndPoint.cpp
    # F014 follow-up: recovery policy actuator
    endpoint/SetRecoveryPolicyEndPoint.cpp
)

set ( SERVICE_SRC
    service/ServiceRegistry.cpp
    service/Service.cpp
    service/Container.cpp
)

set ( MONITOR_SRC
    monitor/ServiceMonitor.cpp
    monitor/ContainerHealthMonitor.cpp
    monitor/UpdateServiceInfo.cpp
)

set ( CONTROL_SRC
    control/ContainerController.cpp
    control/ContainerServiceController.cpp
)

# IDL generation
#set(GENERATED_ROS2_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl/ros2/idl/dds")
#set(GENERATED_LIFECYCLE_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl/lifecycle/idl/dds")
set(GENERATED_STATUS_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")
set(GENERATED_LIFECYCLE_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")

# Neutral status type (AdswServiceStatus) used by AdswServiceStatusEndPoint.
# ros2 node-discovery types (Gid / NodeEntitiesInfo / ParticipantEntitiesInfo)
# are intentionally dropped from the general variant.
set(GENERATED_DDS_STATUS_FILES
                        ${GENERATED_STATUS_DIR}/ros2/AdswServiceStatusPubSubTypes.cxx
                        ${GENERATED_STATUS_DIR}/ros2/AdswServiceStatusTypeObjectSupport.cxx
)
set(GENERATED_DDS_LIFECYCLE_FILES
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/StartServiceRequestPubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/StartServiceRequestTypeObjectSupport.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/StopServiceRequestPubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/StopServiceRequestTypeObjectSupport.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/NotifyServiceStatePubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/NotifyServiceStateTypeObjectSupport.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/GetServiceStateRequestPubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/GetServiceStateRequestTypeObjectSupport.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/UpdateServiceListRequestPubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/UpdateServiceListRequestTypeObjectSupport.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx
                        # F013
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/NotifyContainerStatePubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/NotifyContainerStateTypeObjectSupport.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/StartContainerRequestPubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/StartContainerRequestTypeObjectSupport.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/StopContainerRequestPubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/StopContainerRequestTypeObjectSupport.cxx
                        # F014 follow-up
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/SetRecoveryPolicyRequestPubSubTypes.cxx
                        ${GENERATED_LIFECYCLE_DIR}/lifecycle/SetRecoveryPolicyRequestTypeObjectSupport.cxx
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
include_directories ( ${GENERATED_STATUS_DIR} ${GENERATED_LIFECYCLE_DIR} )

## Library
#link_libraries ( pthread )
#link_directories ( ${CMAKE_INSTALL_PREFIX} )
link_libraries ( pthread fastcdr fastdds yaml-cpp )

## Target binary
#add_library ( abc_static STATIC or SHARED or OBJECT $<TARGET_OBJECTS:abcd_obj> )
#set_target_properties ( abc_static PROPERTIES OUTPUT_NAME abc )
#target_link_libraries ( abc_static ${} )

#add_executable ( ${PROJECT_NAME} ${MAIN_SRC} $<TARGET_OBJECTS:abc_obj> )
add_executable ( ${PROJECT_NAME} ${MAIN_SRC} ${GENERATED_DDS_STATUS_FILES} ${GENERATED_DDS_LIFECYCLE_FILES})

## Install
install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# config
install(
    DIRECTORY
        config
    DESTINATION
        bin
    FILES_MATCHING PATTERN "*.yaml"
)


# idl
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/idl/dds/common
    ${CMAKE_SOURCE_DIR}/idl/dds/lifecycle
    DESTINATION bin/idl
)
