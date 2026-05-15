# CMake configuration for lifecycle_mgr unit tests using GoogleTest

cmake_minimum_required(VERSION 3.5)

project(lifecycle_mgr_unittest)

## Compiler Options
add_compile_options(
    -fno-access-control
    -ftest-coverage
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
                        ${GENERATED_DIR}/lifecycle/StartServiceRequestPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/StartServiceRequestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/lifecycle/StopServiceRequestPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/StopServiceRequestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/lifecycle/GetServiceStateRequestPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/GetServiceStateRequestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/lifecycle/NotifyServiceStatePubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/NotifyServiceStateTypeObjectSupport.cxx
)

## Source
include_directories (
    ..
    ${GENERATED_DIR}
)



## Library
link_libraries ( gtest gmock fastcdr fastdds yaml-cpp )


enable_testing()
include (GoogleTest)

# Add test executable
add_executable ( LifecycleManagerTest LifecycleManagerTest.cpp ../manager/LifecycleManager.cpp )
add_executable ( DDSTransmitterTest DDSTransmitterTest.cpp ../transmitter/DDSTransmitter.cpp )
add_executable ( DDSMessageEndPointTest DDSMessageEndPointTest.cpp ../transmitter/DDSMessageEndPoint.cpp )
add_executable ( DDSMessageEndPointReadTest DDSMessageEndPointReadTest.cpp 
                ../transmitter/DDSMessageEndPoint.cpp
                ../transmitter/MessageEndPoint.cpp
                ${GENERATED_DDS_FILES}
                )
add_executable ( EndPointFactoryTest EndPointFactoryTest.cpp 
                ../endpoint/EndPointFactory.cpp
                ../endpoint/GetServiceStateEndPoint.cpp
                ../endpoint/LifecycleDiagReportEndPoint.cpp
                ../endpoint/NotifyServiceStateEndPoint.cpp
                ../endpoint/SetServiceModeEndPoint.cpp
                ../endpoint/StartServiceEndPoint.cpp
                ../endpoint/StopServiceEndPoint.cpp
                ../endpoint/UpdateServiceListEndPoint.cpp
                ${GENERATED_DDS_FILES}
                )
add_executable ( MWServiceManagerTest MWServiceManagerTest.cpp ../service/MWServiceManager.cpp)
add_executable ( LFPolicyManagerTest PolicyManagerTest.cpp 
                ../policy/PolicyManager.cpp 
                ../policy/PolicyBase.cpp
                ../policy/PolicyAlways.cpp
                ../policy/PolicyOnFailure.cpp
                ../policy/PolicyManual.cpp
                ../policy/PolicyOnSuccess.cpp
                ../policy/PolicyOnNotResponding.cpp
                )
add_executable ( ServiceRegistryTest ServiceRegistryTest.cpp ../service/ServiceRegistry.cpp)
add_executable ( ServiceTest ServiceTest.cpp ../service/Service.cpp )
add_executable ( StateManagerTest StateManagerTest.cpp ../state/StateManager.cpp )
add_executable ( StateMonitorTest StateMonitorTest.cpp ../state/StateMonitor.cpp)
add_executable ( ArgumentConfigTest ArgumentConfigTest.cpp ../service/ArgumentConfig.cpp )
add_executable ( MWServiceTest MWServiceTest.cpp ../service/MWService.cpp ../service/ArgumentConfig.cpp )
add_executable ( HelperTest HelperTest.cpp )

# Integration test for concrete endpoints with actual DDS read
add_executable ( ConcreteEndpointIntegrationTest
                ConcreteEndpointIntegrationTest.cpp
                ../endpoint/StartServiceEndPoint.cpp
                ../endpoint/StopServiceEndPoint.cpp
                ../endpoint/GetServiceStateEndPoint.cpp
                ../endpoint/NotifyServiceStateEndPoint.cpp
                ../endpoint/UpdateServiceListEndPoint.cpp
                ../endpoint/SetServiceModeEndPoint.cpp
                ../endpoint/LifecycleDiagReportEndPoint.cpp
                ../transmitter/DDSMessageEndPoint.cpp
                ../transmitter/MessageEndPoint.cpp
                ${GENERATED_DDS_FILES}
                )

# Integration test for lifecycle_mgr <-> adsw_manager handshake
add_executable ( LifecycleAdswIntegrationTest
                test_lifecycle_adsw_integration.cc
                ../endpoint/UpdateServiceListEndPoint.cpp
                ../endpoint/StartServiceEndPoint.cpp
                ../transmitter/DDSMessageEndPoint.cpp
                ../transmitter/MessageEndPoint.cpp
                ../logger/Logger.cpp
                ${GENERATED_DDS_FILES}
                )

# L3 E2E integration test with real autoware container
add_executable ( LifecycleAutowareE2ETest
                test_lifecycle_autoware_e2e.cc
                )

# L3 E2E stop service test with real autoware container
add_executable ( LifecycleAutowareStopE2ETest
                test_lifecycle_autoware_stop_e2e.cc
                ../transmitter/DDSMessageEndPoint.cpp
                ../transmitter/MessageEndPoint.cpp
                ../logger/Logger.cpp
                ${GENERATED_DDS_FILES}
                )

# L3 E2E polling verification test with real autoware container
add_executable ( LifecycleAutowarePollingE2ETest
                test_lifecycle_autoware_polling_e2e.cc
                )

# L3 E2E crash notification test with real autoware container
add_executable ( LifecycleAutowareCrashE2ETest
                test_lifecycle_autoware_crash_e2e.cc
                )

# L3 E2E recovery policy test with real autoware container
add_executable ( LifecycleAutowareRecoveryE2ETest
                test_lifecycle_autoware_recovery_e2e.cc
                )

# L3 E2E node-kill (PID polling) test with real autoware container
add_executable ( LifecycleAutowareNodeKillE2ETest
                test_lifecycle_autoware_node_kill_e2e.cc
                )

# L3 E2E launch-mode node-kill test with real autoware container
add_executable ( LifecycleAutowareLaunchKillE2ETest
                test_lifecycle_autoware_launch_kill_e2e.cc
                )

# L3 E2E direct-binary execution test with real autoware container
add_executable ( LifecycleAutowareDirectBinaryE2ETest
                test_lifecycle_autoware_direct_binary_e2e.cc
                )

# L3 E2E OTA mode transition test with real autoware container
add_executable ( LifecycleAutowareOtaE2ETest
                test_lifecycle_autoware_ota_e2e.cc
                ../transmitter/DDSMessageEndPoint.cpp
                ../transmitter/MessageEndPoint.cpp
                ../logger/Logger.cpp
                ${GENERATED_DDS_FILES}
                )

gtest_discover_tests ( LifecycleManagerTest )
gtest_discover_tests ( DDSTransmitterTest )
gtest_discover_tests ( DDSMessageEndPointTest )
gtest_discover_tests ( DDSMessageEndPointReadTest )
gtest_discover_tests ( EndPointFactoryTest )
gtest_discover_tests ( MWServiceManagerTest )
gtest_discover_tests ( LFPolicyManagerTest )
gtest_discover_tests ( ServiceRegistryTest )
gtest_discover_tests ( ServiceTest )
gtest_discover_tests ( StateManagerTest )
gtest_discover_tests ( StateMonitorTest )
gtest_discover_tests ( ArgumentConfigTest )
gtest_discover_tests ( MWServiceTest )
gtest_discover_tests ( HelperTest )
gtest_discover_tests ( ConcreteEndpointIntegrationTest )
gtest_discover_tests ( LifecycleAdswIntegrationTest )
gtest_discover_tests ( LifecycleAutowareE2ETest )
gtest_discover_tests ( LifecycleAutowareStopE2ETest )
gtest_discover_tests ( LifecycleAutowarePollingE2ETest )
gtest_discover_tests ( LifecycleAutowareCrashE2ETest )
gtest_discover_tests ( LifecycleAutowareRecoveryE2ETest )
gtest_discover_tests ( LifecycleAutowareNodeKillE2ETest )
gtest_discover_tests ( LifecycleAutowareLaunchKillE2ETest )
gtest_discover_tests ( LifecycleAutowareDirectBinaryE2ETest )
gtest_discover_tests ( LifecycleAutowareOtaE2ETest )


## Install
install ( TARGETS
                ArgumentConfigTest
                ServiceTest
                DDSTransmitterTest
                DDSMessageEndPointTest
                DDSMessageEndPointReadTest
                StateManagerTest
                MWServiceTest
                LFPolicyManagerTest
                StateMonitorTest
                MWServiceManagerTest
                EndPointFactoryTest
                LifecycleManagerTest
                ServiceRegistryTest
                HelperTest
                ConcreteEndpointIntegrationTest
                LifecycleAdswIntegrationTest
                LifecycleAutowareE2ETest
                LifecycleAutowareStopE2ETest
                LifecycleAutowarePollingE2ETest
                LifecycleAutowareCrashE2ETest
                LifecycleAutowareRecoveryE2ETest
                LifecycleAutowareNodeKillE2ETest
                LifecycleAutowareLaunchKillE2ETest
                LifecycleAutowareDirectBinaryE2ETest
                LifecycleAutowareOtaE2ETest
    RUNTIME DESTINATION bin/unittest/lifecycle_mgr
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

    
# Test IDL
install(
    DIRECTORY
        idl
    DESTINATION
        bin/unittest/lifecycle_mgr
    FILES_MATCHING
        PATTERN "*Simple.idl"
        PATTERN "*Complex.idl"
)

# Test Diag config
install(
    DIRECTORY
        config
    DESTINATION
        bin/unittest/lifecycle_mgr
    FILES_MATCHING PATTERN "*.yaml"
)
