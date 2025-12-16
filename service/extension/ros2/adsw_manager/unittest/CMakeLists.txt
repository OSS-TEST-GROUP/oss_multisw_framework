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
                mock/MockADSWManager.cpp
                ../service/ServiceRegistry.cpp )
add_executable ( ADSWTypeConvertTest TypeConvertTest.cpp )
                
add_executable ( ADSWServiceMonitorTest ServiceMonitorTest.cpp
                 mock/MockADSWManager.cpp
                 ../monitor/ServiceMonitor.cpp
                 ${GENERATED_DIR}/lifecycle/NotifyServiceStatePubSubTypes.cxx
                 ${GENERATED_DIR}/lifecycle/NotifyServiceStateTypeObjectSupport.cxx
                 ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                 ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx
                 ${GENERATED_DIR}/ros2/AdswServiceStatusPubSubTypes.cxx
                 ${GENERATED_DIR}/ros2/AdswServiceStatusTypeObjectSupport.cxx
)
add_executable ( ADSWROSServiceDiscoveryTest ROSServiceDiscoveryTest.cpp)
add_executable ( ADSWDDSTransmitterTest
    DDSTransmitterTest.cpp 
    ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx
    ${GENERATED_DIR}/lifecycle/LifecycleDiagReportPubSubTypes.cxx
    ${GENERATED_DIR}/lifecycle/LifecycleDiagReportTypeObjectSupport.cxx
)
add_executable ( ADSWROSDiscoveryInfoEndpointTest ROSDiscoveryInfoEndpointTest.cpp
    mock/MockADSWManager.cpp
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
add_executable( ADSWMessageQueueTest MessageQueueTest.cpp
                mock/MockADSWManager.cpp
                ../transmitter/MessageQueue.cpp)
add_executable( ADSWDDSMessageEndpointTest DDSMessageEndpointTest.cpp 
                ../transmitter/DDSMessageEndpoint.cpp)
add_executable( ADSWGetServiceStateEndPointTest GetServiceStateEndPointTest.cpp
                mock/MockADSWManager.cpp
                ../endpoint/GetServiceStateEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/GetServiceStateRequestPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/GetServiceStateRequestTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)
add_executable( ADSWNotifyServiceStateEndPointTest NotifyServiceStateEndPointTest.cpp
                mock/MockADSWManager.cpp
                ../endpoint/NotifyServiceStateEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/NotifyServiceStatePubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/NotifyServiceStateTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)
add_executable( ADSWStartServiceEndpointTest StartServiceEndpointTest.cpp
                mock/MockADSWManager.cpp
                ../endpoint/StartServiceEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/StartServiceRequestPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/StartServiceRequestTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)
add_executable( ADSWStopServiceEndpointTest StopServiceEndpointTest.cpp
                mock/MockADSWManager.cpp
                ../endpoint/StopServiceEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/StopServiceRequestPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/StopServiceRequestTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)
add_executable( ADSWUpdateServiceListEndpointTest UpdateServiceListEndpointTest.cpp
                mock/MockADSWManager.cpp
                ../endpoint/UpdateServiceListEndPoint.cpp
                ${GENERATED_DIR}/lifecycle/UpdateServiceListRequestPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/UpdateServiceListRequestTypeObjectSupport.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonPubSubTypes.cxx
                ${GENERATED_DIR}/lifecycle/LifecycleCommonTypeObjectSupport.cxx        
)


# Control directory tests
add_executable( ADSWContainerControllerTest ContinerControllerTest.cpp
                ../control/ContainerController.cpp
                ../service/Container.cpp
)

add_executable( ADSWROS2NodeControllerTest ROS2NodeControllerTest.cpp
                ../control/ROS2NodeController.cpp
)

add_executable( ADSWROS2LifecycleControllerTest ROS2LifecycleControllerTest.cpp
                ../control/ROS2LifecycleController.cpp
)

#add_executable( ADSWROSServiceControllerTest ROSServiceControllerTest.cpp
#                ../control/ROSServiceController.cpp
#                ../control/ContainerController.cpp
#                ../control/ROS2NodeController.cpp
#                ../service/Container.cpp
#)


gtest_discover_tests ( ADSWServiceTest )
gtest_discover_tests ( ADSWServiceRegistryTest )
gtest_discover_tests ( ADSWTypeConvertTest )
gtest_discover_tests ( ADSWServiceMonitorTest )
gtest_discover_tests ( ADSWROSServiceDiscoveryTest )
gtest_discover_tests ( ADSWDDSTransmitterTest )
gtest_discover_tests ( ADSWROSDiscoveryInfoEndpointTest )
gtest_discover_tests ( ADSWManager )
gtest_discover_tests ( ADSWMessageQueueTest )
gtest_discover_tests ( ADSWDDSMessageEndpointTest )
gtest_discover_tests ( ADSWGetServiceStateEndPointTest )
gtest_discover_tests ( ADSWNotifyServiceStateEndPointTest )
gtest_discover_tests ( ADSWStartServiceEndpointTest )   
gtest_discover_tests ( ADSWStopServiceEndpointTest )
gtest_discover_tests ( ADSWUpdateServiceListEndpointTest )
gtest_discover_tests ( ADSWContainerControllerTest )
gtest_discover_tests ( ADSWROS2NodeControllerTest )
gtest_discover_tests ( ADSWROS2LifecycleControllerTest )
#gtest_discover_tests ( ADSWROSServiceControllerTest )


## Install
install ( TARGETS
          ADSWServiceTest
          ADSWServiceRegistryTest
          ADSWTypeConvertTest
          ADSWServiceMonitorTest
          ADSWROSServiceDiscoveryTest
          ADSWDDSTransmitterTest
          ADSWROSDiscoveryInfoEndpointTest
          ADSWManager
          ADSWMessageQueueTest
          ADSWDDSMessageEndpointTest
          ADSWGetServiceStateEndPointTest
          ADSWNotifyServiceStateEndPointTest
          ADSWStartServiceEndpointTest
          ADSWStopServiceEndpointTest
          ADSWUpdateServiceListEndpointTest
          ADSWContainerControllerTest
          ADSWROS2NodeControllerTest
          ADSWROS2LifecycleControllerTest
#          ADSWROSServiceControllerTest
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
