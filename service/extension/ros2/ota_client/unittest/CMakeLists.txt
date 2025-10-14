cmake_minimum_required(VERSION 3.5)
project("ota_client_unittest")

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
    ota_client_core
    gtest
    gmock
    fastcdr
    fastdds
)

gtest_discover_tests (${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE mock ${GENERATED_OTA_DIR})

install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin/unittest/ota_client
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
