cmake_minimum_required(VERSION 3.5)

project ( "adsw_unittest" )

## Compiler Options
add_compile_options(
    -fno-access-control
    -ftest-coverage
)

# IDL generation
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")
set(ROS2_GENEREATED_IDL
    ${GENERATED_DIR}/ros2/GidPubSubTypes.cxx
    ${GENERATED_DIR}/ros2/GidTypeObjectSupport.cxx
    ${GENERATED_DIR}/ros2/NodeEntitiesInfoPubSubTypes.cxx
    ${GENERATED_DIR}/ros2/NodeEntitiesInfoTypeObjectSupport.cxx
    ${GENERATED_DIR}/ros2/ParticipantEntitiesInfoPubSubTypes.cxx
    ${GENERATED_DIR}/ros2/ParticipantEntitiesInfoTypeObjectSupport.cxx
)

## Source
include_directories (
    ..
    ${CMAKE_SOURCE_DIR}/service
    ${GENERATED_DIR}
)

## Library
link_libraries ( gtest gmock fastcdr fastdds yaml-cpp )

## Target binary
add_executable ( ADSWServiceTest ServiceTest.cpp )
add_executable ( ADSWServiceRegistryTest ServiceRegistryTest.cpp )
add_executable ( ADSWServiceMonitorTest ServiceMonitorTest.cpp )
add_executable ( ADSWDDSTransmitterTest DDSTransmitterTest.cpp 
                                    ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx
                                    ${GENERATED_DIR}/lifecycle/LifecycleDiagReportPubSubTypes.cxx
                                    ${GENERATED_DIR}/lifecycle/LifecycleDiagReportTypeObjectSupport.cxx
)
add_executable ( ADSWROSDiscoveryInfoEndpointTest ROSDiscoveryInfoEndpointTest.cpp
                                                  ${ROS2_GENEREATED_IDL}
)

## Install
install ( TARGETS
          ADSWServiceTest
          ADSWServiceRegistryTest
          ADSWDDSTransmitterTest
          ADSWROSDiscoveryInfoEndpointTest
    RUNTIME DESTINATION bin/unittest/ros2/adsw_manager
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

## For test
add_executable ( ROSDiscoveryInfoSender test/ROSDiscoveryInfoSender.cpp
                                            ${ROS2_GENEREATED_IDL}
)
install ( TARGETS
          ROSDiscoveryInfoSender
    RUNTIME DESTINATION bin/unittest/ros2/adsw_manager/test
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )