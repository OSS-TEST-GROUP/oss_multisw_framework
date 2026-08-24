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
                        # F013
                        ${GENERATED_DIR}/lifecycle/NotifyContainerStatePubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/NotifyContainerStateTypeObjectSupport.cxx
                        ${GENERATED_DIR}/lifecycle/StartContainerRequestPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/StartContainerRequestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/lifecycle/StopContainerRequestPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/StopContainerRequestTypeObjectSupport.cxx
                        ${GENERATED_DIR}/lifecycle/LifecycleCtrlRequestPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/LifecycleCtrlRequestTypeObjectSupport.cxx
                        # F014 per-service recovery policy (Request+Response defined in one IDL)
                        ${GENERATED_DIR}/lifecycle/SetRecoveryPolicyRequestPubSubTypes.cxx
                        ${GENERATED_DIR}/lifecycle/SetRecoveryPolicyRequestTypeObjectSupport.cxx
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
# GENERATED_DDS_FILES 는 on_notify_container_state 핸들러 테스트가 실제
# NotifyContainerState 를 만들어야 해서 필요하다 (생성 타입의 생성자는
# 헤더에 인라인돼 있지 않다). 나머지 핸들러는 포인터를 통과만 시키므로
# nullptr 로 검증한다.
add_executable ( LifecycleManagerTest LifecycleManagerTest.cpp ../manager/LifecycleManager.cpp
                ${GENERATED_DDS_FILES}
                )
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
                # F013 container actuator + ctrl/* endpoints
                ../endpoint/NotifyContainerStateEndPoint.cpp
                ../endpoint/StartContainerEndPoint.cpp
                ../endpoint/StopContainerEndPoint.cpp
                ../endpoint/CtrlStartServiceEndPoint.cpp
                ../endpoint/CtrlStopServiceEndPoint.cpp
                ../endpoint/CtrlSetServiceModeEndPoint.cpp
                ../endpoint/CtrlStartContainerEndPoint.cpp
                ../endpoint/CtrlStopContainerEndPoint.cpp
                # F014 per-service recovery policy endpoints
                ../endpoint/SetRecoveryPolicyEndPoint.cpp
                ../endpoint/CtrlSetRecoveryPolicyEndPoint.cpp
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
add_executable ( ServiceRegistryTest ServiceRegistryTest.cpp ../service/ServiceRegistry.cpp ../service/Container.cpp ../logger/Logger.cpp )
# UpdateServiceList 전용 바이너리. ServiceRegistryTest 와 달리 **진짜**
# Service.cpp 를 링크한다 — mock_service.hpp 심을 쓰면 UpdateServiceList 가
# 스스로 만드는 Service 객체들이 전역 mock 하나를 공유해 이름이 뭉개진다.
# 자세한 이유는 ServiceRegistryUpdateTest.cpp 상단 주석 참고.
add_executable ( ServiceRegistryUpdateTest ServiceRegistryUpdateTest.cpp
                 ../service/ServiceRegistry.cpp
                 ../service/Service.cpp
                 ../service/Container.cpp
                 ../logger/Logger.cpp
                 ${GENERATED_DDS_FILES}
                 )
add_executable ( ServiceTest ServiceTest.cpp ../service/Service.cpp )
# Container.cpp: ctrl/StartContainer·StopContainer 게이트가 ServiceRegistry 에서
# 받은 Container 를 다룬다. 링크해 두면 그 경로를 실제 객체로 테스트할 수 있고,
# 커버리지 0% 였던 Container.cpp / Container.hpp 도 함께 잡힌다.
add_executable ( StateManagerTest StateManagerTest.cpp ../state/StateManager.cpp
                 ../service/Container.cpp )
# Container 는 5인자 생성자만 Container.cpp 에 있고 나머지 접근자는 헤더에 있다.
# F008 컨테이너 집계 리포트 테스트가 실제 Container 를 만들어야 해서 링크한다
# (목/링크 심이 없는 순수 값 타입이라 중복 정의 걱정이 없다).
add_executable ( StateMonitorTest StateMonitorTest.cpp ../state/StateMonitor.cpp
                 ../service/Container.cpp )
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
                # F013/F014 ctrl·container 엔드포인트 10종.
                # 이들의 OnDataAvailable 콜백 디스패치는 Read<T>() 가 성공해야
                # 도달하고, Read 는 실제 DataReader 없이는 언제나 false 다.
                # 즉 실 DDS 왕복이 있는 이 타겟에서만 덮을 수 있다.
                ../endpoint/CtrlStartServiceEndPoint.cpp
                ../endpoint/CtrlStopServiceEndPoint.cpp
                ../endpoint/CtrlSetServiceModeEndPoint.cpp
                ../endpoint/CtrlSetRecoveryPolicyEndPoint.cpp
                ../endpoint/CtrlStartContainerEndPoint.cpp
                ../endpoint/CtrlStopContainerEndPoint.cpp
                ../endpoint/SetRecoveryPolicyEndPoint.cpp
                ../endpoint/StartContainerEndPoint.cpp
                ../endpoint/StopContainerEndPoint.cpp
                ../endpoint/NotifyContainerStateEndPoint.cpp
                ../transmitter/DDSMessageEndPoint.cpp
                ../transmitter/MessageEndPoint.cpp
                ${GENERATED_DDS_FILES}
                )

# F013/F014 ctrl·container 엔드포인트 10종 — 유닛 레인용 방어 경로.
# 실 DDS 를 쓰지 않는다. reader_ 가 nullptr 일 때 조용히 끝나는지, 콜백이
# 없을 때 터지지 않는지를 본다. 자세한 이유는 CtrlEndPointTest.cpp 상단 주석 참고.
#
# 예전 주석은 "커버리지는 유닛 레인 실행분으로만 측정되므로 통합 레인에 넣으면
# 수치에 반영되지 않는다"고 적혀 있었는데, 그 전제는 더 이상 맞지 않는다.
# coverage 캡처 전에 run_integration.sh 를 돌리도록 바뀐 뒤로 통합 레인 실행분도
# 함께 잡힌다 (Jenkinsfile.unittest 의 'Integration ctest (for coverage)' 스테이지).
# 근거: StartServiceEndPoint.cpp 의 `if (on_data_cb_)` 는 통합 레인에서만
# 실행되는데 현재 두 분기 모두 taken 으로 잡힌다.
# 그래서 콜백 디스패치는 위 통합 타겟에서 덮고, 이 타겟은 방어 경로만 맡는다.
add_executable ( CtrlEndPointTest
                CtrlEndPointTest.cpp
                ../endpoint/CtrlStartServiceEndPoint.cpp
                ../endpoint/CtrlStopServiceEndPoint.cpp
                ../endpoint/CtrlSetServiceModeEndPoint.cpp
                ../endpoint/CtrlSetRecoveryPolicyEndPoint.cpp
                ../endpoint/CtrlStartContainerEndPoint.cpp
                ../endpoint/CtrlStopContainerEndPoint.cpp
                ../endpoint/SetRecoveryPolicyEndPoint.cpp
                ../endpoint/StartContainerEndPoint.cpp
                ../endpoint/StopContainerEndPoint.cpp
                ../endpoint/NotifyContainerStateEndPoint.cpp
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

# ── Autoware E2E: inject repo root as compile-time default for path resolution ─
# Tests use autoware_e2e_common.h which reads MMF_E2E_ROOT_DEFAULT when the
# MMF_E2E_ROOT env var is not set. CMAKE_SOURCE_DIR is the top-level project dir.
# CI runners should additionally set MMF_E2E_ROOT / MMF_E2E_OUTPUT env vars so
# the tests resolve to the actual build output inside the runner container.
set(AUTOWARE_E2E_TARGETS
    LifecycleAutowareE2ETest
    LifecycleAutowareStopE2ETest
    LifecycleAutowarePollingE2ETest
    LifecycleAutowareCrashE2ETest
    LifecycleAutowareRecoveryE2ETest
    LifecycleAutowareNodeKillE2ETest
    LifecycleAutowareLaunchKillE2ETest
    LifecycleAutowareDirectBinaryE2ETest
    LifecycleAutowareOtaE2ETest
)
foreach(target ${AUTOWARE_E2E_TARGETS})
    target_compile_definitions(${target} PRIVATE
        MMF_E2E_ROOT_DEFAULT="${CMAKE_SOURCE_DIR}"
    )
endforeach()

gtest_discover_tests ( LifecycleManagerTest )
gtest_discover_tests ( DDSTransmitterTest )
gtest_discover_tests ( DDSMessageEndPointTest )
gtest_discover_tests ( DDSMessageEndPointReadTest )
gtest_discover_tests ( EndPointFactoryTest )
gtest_discover_tests ( MWServiceManagerTest )
gtest_discover_tests ( LFPolicyManagerTest )
gtest_discover_tests ( ServiceRegistryTest )
gtest_discover_tests ( ServiceRegistryUpdateTest )
gtest_discover_tests ( ServiceTest )
gtest_discover_tests ( StateManagerTest )
gtest_discover_tests ( StateMonitorTest )
gtest_discover_tests ( ArgumentConfigTest )
gtest_discover_tests ( MWServiceTest )
gtest_discover_tests ( HelperTest )
gtest_discover_tests ( CtrlEndPointTest )
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
