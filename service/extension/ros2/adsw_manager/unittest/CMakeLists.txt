cmake_minimum_required(VERSION 3.5)

project ( "adsw_unittest" )

## Compiler Options
add_compile_options(
    -fno-access-control
    -ftest-coverage
)

# Enable UNITTEST_MOCK for unit test builds
add_compile_definitions(UNITTEST_MOCK)

# IDL generation
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")
set(ROS2_GENERATED_IDL
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

enable_testing()
include (GoogleTest)

## Target binary
add_executable ( ADSWServiceTest ServiceTest.cpp )
add_executable ( ADSWServiceRegistryTest ServiceRegistryTest.cpp
                ../service/ServiceRegistry.cpp )
                
add_executable ( ADSWServiceMonitorTest ServiceMonitorTest.cpp)
add_executable ( ADSWDDSTransmitterTest
    DDSTransmitterTest.cpp 
    ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx
    ${GENERATED_DIR}/lifecycle/LifecycleDiagReportPubSubTypes.cxx
    ${GENERATED_DIR}/lifecycle/LifecycleDiagReportTypeObjectSupport.cxx
)
add_executable ( ADSWROSDiscoveryInfoEndpointTest ROSDiscoveryInfoEndpointTest.cpp
    ../endpoint/ROSDiscoveryInfoEndpoint.cpp
    ../monitor/ROSServiceDiscovery.cpp
    ${GENERATED_DIR}/ros2/GidPubSubTypes.cxx
    ${GENERATED_DIR}/ros2/GidTypeObjectSupport.cxx
    ${GENERATED_DIR}/ros2/NodeEntitiesInfoPubSubTypes.cxx
    ${GENERATED_DIR}/ros2/NodeEntitiesInfoTypeObjectSupport.cxx
    ${GENERATED_DIR}/ros2/ParticipantEntitiesInfoPubSubTypes.cxx
    ${GENERATED_DIR}/ros2/ParticipantEntitiesInfoTypeObjectSupport.cxx
)

add_executable( ADSWManager ADSWManagerTest.cpp ../ADSWManager.cpp)
add_executable( ADSWMessageQueueTest MessageQueueTest.cpp ../transmitter/MessageQueue.cpp)
add_executable( GetServiceStateEndPointTest GetServiceStateEndPointTest.cpp
                ../endpoint/GetServiceStateEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/GetServiceStateRequestPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/GetServiceStateRequestTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)
add_executable( NotifyServiceStateEndPointTest NotifyServiceStateEndPointTest.cpp
                ../endpoint/NotifyServiceStateEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/NotifyServiceStatePubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/NotifyServiceStateTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)
add_executable( StartServiceEndpointTest StartServiceEndpointTest.cpp
                ../endpoint/StartServiceEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/StartServiceRequestPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/StartServiceRequestTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)
add_executable( StopServiceEndpointTest StopServiceEndpointTest.cpp
                ../endpoint/StopServiceEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/StopServiceRequestPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/StopServiceRequestTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)
add_executable( UpdateServiceListEndpointTest UpdateServiceListEndpointTest.cpp
                ../endpoint/UpdateServiceListEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/UpdateServiceListRequestPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/UpdateServiceListRequestTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)


gtest_discover_tests ( ADSWServiceTest )
gtest_discover_tests ( ADSWServiceRegistryTest )
gtest_discover_tests ( ADSWServiceMonitorTest )
gtest_discover_tests ( ADSWDDSTransmitterTest )
gtest_discover_tests ( ADSWROSDiscoveryInfoEndpointTest )
gtest_discover_tests ( ADSWMessageQueueTest )
gtest_discover_tests ( GetServiceStateEndPointTest )
gtest_discover_tests ( NotifyServiceStateEndPointTest )
gtest_discover_tests ( StartServiceEndpointTest )   
gtest_discover_tests ( StopServiceEndpointTest )
gtest_discover_tests ( UpdateServiceListEndpointTest )
    

## Install
install ( TARGETS
          ADSWServiceTest
          ADSWServiceRegistryTest
          ADSWDDSTransmitterTest
          ADSWROSDiscoveryInfoEndpointTest
          ADSWManager
          ADSWMessageQueueTest
          GetServiceStateEndPointTest
            NotifyServiceStateEndPointTest
            StartServiceEndpointTest
            StopServiceEndpointTest
            UpdateServiceListEndpointTest
    RUNTIME DESTINATION bin/unittest/adsw_manager
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

## For test
add_executable ( ROSDiscoveryInfoSender test/ROSDiscoveryInfoSender.cpp
                                            ${ROS2_GENERATED_IDL}
)
install ( TARGETS
          ROSDiscoveryInfoSender
    RUNTIME DESTINATION bin/unittest/ros2/adsw_manager/test
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
