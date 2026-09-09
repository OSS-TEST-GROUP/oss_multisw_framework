# in ota_manager/unittest/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# Add source files for mocks
set(OTAM_MOCK_SRCS
    mock/mock_service.cc
)

# Create a static library for the mocks
add_library(OtamMocks STATIC ${OTAM_MOCK_SRCS})
target_include_directories(OtamMocks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_include_directories(OtamMocks PRIVATE
    ${googletest_SOURCE_DIR}/googlemock/include
    ${googletest_SOURCE_DIR}/googletest/include
)

message(STATUS "googletest_SOURCE_DIR = ${googletest_SOURCE_DIR}")

# Add the test executable.
add_executable(otam_main_tests
    test_otam_main.cc
    test_otam_client_service.cc
    test_otam_vehicle_service.cc
    test_otam_actor_service.cc
    test_otam_get_applist.cc
    test_otam_diagnostic_service.cc
    test_otam_server_service.cc
)

# Link the test executable to GTest, GMock, OtamMain library, and our mocks.
target_link_libraries(otam_main_tests PRIVATE
    OtamMocks
    OtamMain
    OtamDdsImpl
    OtamClientService
    OtamActorService
    OtamVehicleService
    OtamDiagnosticService
    OtamServerService
    gtest
    gmock
    fastcdr
    fastdds
)

# Specify include directories for OtamMain header and mock headers.
target_include_directories(otam_main_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mock
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

install ( TARGETS otam_main_tests
    RUNTIME DESTINATION bin/unittest/ota_mgr/test
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# Register gtest cases with CTest so run_unittest.sh(ctest) picks them up
# (mirrors lifecycle_mgr/diagnostics unittest dirs). 순수 단위테스트만 등록 —
# test_otam_integration.cc 는 otam_main_tests 소스에 미포함이라 통합테스트는 딸려오지 않음.
enable_testing()
include(GoogleTest)
gtest_discover_tests(otam_main_tests)

##################
# otam_server_service
##################
# add_executable(otam_server_test 
#     test_otam_server_service.cc
# )

# target_link_libraries(otam_server_test PRIVATE
#     OtamServerService
#     gtest
#     gmock
# )

# target_include_directories(otam_server_test PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/mock
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
# )

# install ( TARGETS otam_server_test
#     RUNTIME DESTINATION bin/unittest/ota_mgr/test
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     )