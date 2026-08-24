cmake_minimum_required(VERSION 3.5)

project ( "adsw_general_unittest" )

# general adsw_manager 단위/통합 테스트. 1차에서는 ContainerController 의 Docker
# 통합 테스트만 둔다 — DDS·lifecycle_mgr 없이 실제 docker 로 컨테이너 제어를
# 검증(가장 격리된 Linux docker 확인 경로). 생성 IDL 불필요(순수 docker 셸).

## Compiler Options
# -fno-access-control: 저장소의 다른 5개 유닛테스트(lifecycle_mgr, ros2/agl
# adsw_manager, diagnostics, agl/ota_client)가 이미 쓰는 규약이다. general 만
# 빠져 있어서, 예컨대 ContainerServiceController::container_controller_ 가
# private 이고 세터도 없어 가짜 컨테이너 컨트롤러를 물려줄 수 없었다 —
# ProcessServiceStartRequest 의 컨테이너 선기동 로직이 통째로 미커버였던 이유다.
# 프로덕션 헤더에 테스트 전용 세터를 뚫는 것보다 이쪽이 낫다.
add_compile_options(
    -fno-access-control
)

include_directories (
    ..
    .
    ${CMAKE_SOURCE_DIR}/service
    ${CMAKE_INSTALL_PREFIX}/generated/idl
)

link_libraries ( gtest gmock pthread fastcdr fastdds yaml-cpp )

enable_testing()
include ( GoogleTest )

# ── 유닛테스트 ──────────────────────────────────────────────────────────────
# ContainerController 의 외부 프로세스 실행 seam(RunCommand/CaptureCommand)을
# 서브클래싱해 가로채므로 docker 없이 돈다. Logger 는 mock/MockLogger.hpp 가
# 링크 심으로 정의하므로 실제 Logger.cpp 를 링크하지 않는다.
add_executable ( ADSWGeneralContainerControllerTest
                 ContainerControllerTest.cpp
                 ../control/ContainerController.cpp
                 ../service/Container.cpp
)
gtest_discover_tests ( ADSWGeneralContainerControllerTest )

# ServiceRegistry / Service. YAML 로드·저장과 정규식 추출은 순수 로직이라
# tmpdir 만 있으면 되고, ADSWManager 싱글턴은 mock/MockADSWManager.hpp 가
# 링크 심으로 가로챈다 (실물을 링크하면 DDS participant 가 뜬다).
add_executable ( ADSWGeneralServiceRegistryTest
                 ServiceRegistryTest.cpp
                 ../service/ServiceRegistry.cpp
                 ../service/Service.cpp
                 ../service/Container.cpp
                 ../control/ContainerController.cpp
                 # LoadServiceSaved 가 GetServiceController()->GetContainer() 를
                 # 요구한다. 실물을 링크해 그 경로까지 함께 덮는다.
                 ../control/ContainerServiceController.cpp
                 ../monitor/ServiceMonitor.cpp
                 ../monitor/UpdateServiceInfo.cpp
                 ../transmitter/DDSTransmitter.cpp
                 ../transmitter/DDSMessageEndpoint.cpp
                 ../transmitter/MessageQueue.cpp
                 ${GENERATED_DDS_STATUS_FILES}
                 ${GENERATED_DDS_LIFECYCLE_FILES}
)
gtest_discover_tests ( ADSWGeneralServiceRegistryTest )

# ContainerServiceController / ServiceMonitor / MessageQueue.
# general 판 제어 흐름의 본체. ADSWManager 는 링크 심으로 가로챈다.
add_executable ( ADSWGeneralControllerTest
                 ContainerServiceControllerTest.cpp
                 ../control/ContainerServiceController.cpp
                 ../control/ContainerController.cpp
                 ../monitor/ServiceMonitor.cpp
                 ../monitor/UpdateServiceInfo.cpp
                 ../service/ServiceRegistry.cpp
                 ../service/Service.cpp
                 ../service/Container.cpp
                 ../transmitter/DDSTransmitter.cpp
                 ../transmitter/DDSMessageEndpoint.cpp
                 ../transmitter/MessageQueue.cpp
                 ${GENERATED_DDS_STATUS_FILES}
                 ${GENERATED_DDS_LIFECYCLE_FILES}
)
gtest_discover_tests ( ADSWGeneralControllerTest )

# DDSMessageEndpoint / DDSTransmitter.
# 실제 participant 없이 초기화 전후의 방어 로직과 리스너 콜백을 검증한다.
# 실 pub→sub 왕복은 통합 레인 소관.
add_executable ( ADSWGeneralDDSTransmitterTest
                 DDSTransmitterTest.cpp
                 ../transmitter/DDSTransmitter.cpp
                 ../transmitter/DDSMessageEndpoint.cpp
                 ../transmitter/MessageQueue.cpp
                 ${GENERATED_DDS_STATUS_FILES}
                 ${GENERATED_DDS_LIFECYCLE_FILES}
)
gtest_discover_tests ( ADSWGeneralDDSTransmitterTest )

# Docker 통합 테스트 (실제 Docker 데몬 필요; 없으면 GTEST_SKIP)
add_executable ( ADSWGeneralContainerDockerIntegrationTest
                 ContainerControllerDockerIntegrationTest.cpp
                 ../control/ContainerController.cpp
                 ../service/Container.cpp
                 ../logger/Logger.cpp
)
gtest_discover_tests ( ADSWGeneralContainerDockerIntegrationTest )

install ( TARGETS
          ADSWGeneralContainerControllerTest
          ADSWGeneralServiceRegistryTest
          ADSWGeneralControllerTest
          ADSWGeneralDDSTransmitterTest
          ADSWGeneralContainerDockerIntegrationTest
    RUNTIME DESTINATION bin/unittest/adsw_manager_general
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
