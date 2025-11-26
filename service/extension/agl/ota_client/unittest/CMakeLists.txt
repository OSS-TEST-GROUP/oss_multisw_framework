cmake_minimum_required(VERSION 3.5)
project("agl_ota_client_unittest")

add_compile_options(
    -fno-access-control
    -ftest-coverage
)

enable_testing()
include (GoogleTest)

add_executable(${PROJECT_NAME}
    test_main.cpp
    OtacStatusStoreTest.cpp
    OtacManagerDdsImplTest.cpp
    OtacManagerServiceTest.cpp
    OtacMainTest.cpp
    ${GENERATED_OTA_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    agl_ota_core
    gtest
    gmock
    fastcdr
    fastdds
)

target_include_directories(${PROJECT_NAME} PRIVATE mock ${GENERATED_OTA_DIR})

gtest_discover_tests (${PROJECT_NAME})

install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin/unittest/agl/ota_client
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)