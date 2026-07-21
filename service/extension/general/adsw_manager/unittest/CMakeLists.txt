cmake_minimum_required(VERSION 3.5)

project ( "adsw_general_unittest" )

# general adsw_manager 단위/통합 테스트. 1차에서는 ContainerController 의 Docker
# 통합 테스트만 둔다 — DDS·lifecycle_mgr 없이 실제 docker 로 컨테이너 제어를
# 검증(가장 격리된 Linux docker 확인 경로). 생성 IDL 불필요(순수 docker 셸).

include_directories (
    ..
    ${CMAKE_SOURCE_DIR}/service
)

link_libraries ( gtest pthread fastcdr fastdds yaml-cpp )

enable_testing()
include ( GoogleTest )

# Docker 통합 테스트 (실제 Docker 데몬 필요; 없으면 GTEST_SKIP)
add_executable ( ADSWGeneralContainerDockerIntegrationTest
                 ContainerControllerDockerIntegrationTest.cpp
                 ../control/ContainerController.cpp
                 ../service/Container.cpp
                 ../logger/Logger.cpp
)
gtest_discover_tests ( ADSWGeneralContainerDockerIntegrationTest )

install ( TARGETS
          ADSWGeneralContainerDockerIntegrationTest
    RUNTIME DESTINATION bin/unittest/adsw_manager_general
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
