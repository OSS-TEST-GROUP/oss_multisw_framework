cmake_minimum_required(VERSION 3.5)
project("agl_adsw_manager_unittest")

add_compile_options(
    -fno-access-control
    -ftest-coverage
)

enable_testing()
include (GoogleTest)

add_executable(${PROJECT_NAME}
    test_main.cpp
    ADSWManagerTest.cpp
    ServiceRegistryTest.cpp
    AppStateReaderTest.cpp
    ServiceMonitorTest.cpp
    ServiceControllerTest.cpp
    MessageTransmitterTest.cpp
    StopAppHandlerTest.cpp
    StartAppHandlerTest.cpp
    NotifyAppStateWriterTest.cpp
    AppStateHandlerTest.cpp
    DDSManagerTest.cpp
    AppControllerTest.cpp
    MainTest.cpp
    UpdateAppListHandlerTest.cpp
    ${GENERATED_LIFECYCLE_FILES}
    ${GENERATED_AGL_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    agl_adsw_core
    gtest
    gmock
    fastcdr
    fastdds
)

target_include_directories(${PROJECT_NAME} PRIVATE ../service_monitor mock ${GENERATED_IDL_DIR})

# 단위테스트는 helpers/docker(+systemctl) stub 을 PATH 에 얹어 실제 docker 대신 쓴다.
# CI 의 Run-Unittest 컨테이너는 .build/idl/output 만 마운트하고 service/ 소스트리는
# 마운트하지 않으므로, 소스경로(${CMAKE_CURRENT_SOURCE_DIR}/helpers)는 런타임에 부재해
# execvp 가 ENOENT 로 실패한다. 빌드 디렉토리(.build 하위, Run 에 마운트됨)로 복사하고
# 그 경로를 가리킨다. file(COPY) 는 실행 비트를 보존한다.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/helpers DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_compile_definitions(${PROJECT_NAME} PRIVATE
  UNIT_TEST
  MOCK_SYSCTL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/unittest/helpers"
  MOCK_SYSCTL_LOG="${CMAKE_CURRENT_BINARY_DIR}/sysctl.log"
  ADSW_HELPERS_DIR="${CMAKE_CURRENT_BINARY_DIR}/helpers"
)

gtest_discover_tests (${PROJECT_NAME})

install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin/unittest/agl/adsw_manager
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)