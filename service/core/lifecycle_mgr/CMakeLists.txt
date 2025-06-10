cmake_minimum_required(VERSION 3.5)

project ( "lifecycle_mgr" )

## Unittest
#if (BUILD_UNITTEST)
#   add_subdirectory ( unittest )
#endif()

## Source
set ( ENDPOINT_SRC
#    endpoint/LifecycleDiagReportEndPoint.cpp
)


set ( POLICY_SRC
    policy/PolicyManager.cpp
)

set ( SERIVCE_SRC
    service/MWService.cpp
    service/MWServiceManager.cpp
    service/Service.cpp
    service/ServiceRegistry.cpp
)

set ( STATE_SRC
    state/StateManager.cpp
    state/StateMonitor.cpp
)

set ( TRANSMITTER_SRC
    transmitter/DDSMessageEndPoint.cpp
    transmitter/DDSTransmitter.cpp
    transmitter/MessageEndPoint.cpp
)

# IDL generation
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")
set(GENERATED_DDS_FILES ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx
                        ${GENERATED_DIR}/lifecycle/LifecycleDiagReportPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/LifecycleDiagReportTypeObjectSupport.cxx
                        ${GENERATED_DIR}/lifecycle/SetServiceModeRequestPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/SetServiceModeRequestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/lifecycle/UpdateServiceListRequestPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/UpdateServiceListRequestTypeObjectSupport.cxx
)
set ( MAIN_SRC main.cc
               LifecycleManager.cpp
               ${ENDPOINT_SRC}
               ${POLICY_SRC}
               ${SERIVCE_SRC}
               ${STATE_SRC}
               ${TRANSMITTER_SRC}
)

## Library
include_directories ( . )
include_directories ( ${GENERATED_DIR} )
link_libraries ( pthread fastcdr fastdds )
#link_directories ( ${CMAKE_INSTALL_PREFIX} )

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
