cmake_minimum_required(VERSION 3.5)
project("agl_ota_client_unittest")

include(ExternalProject)
find_package(Threads REQUIRED)

ExternalProject_Add(GoogleTest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/lib
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(GoogleTest source_dir)
ExternalProject_Get_Property(GoogleTest binary_dir)

include_directories(
    ${source_dir}/googletest/include
    ${source_dir}/googlemock/include
)

add_library(GTest::GTest UNKNOWN IMPORTED)
set_target_properties(GTest::GTest PROPERTIES
    IMPORTED_LOCATION ${binary_dir}/lib/libgtest.a
    IMPORTED_LINK_INTERFACE_LIBRARIES Threads::Threads
)
add_dependencies(GTest::GTest GoogleTest)

add_library(GTest::GMock UNKNOWN IMPORTED)
set_target_properties(GTest::GMock PROPERTIES
    IMPORTED_LOCATION ${binary_dir}/lib/libgmock.a
    IMPORTED_LINK_INTERFACE_LIBRARIES Threads::Threads
)
add_dependencies(GTest::GMock GoogleTest)

add_executable(${PROJECT_NAME}
    test_main.cpp
    OtacStatusStoreTest.cpp
    OtacManagerDdsImplTest.cpp
    OtacManagerServiceTest.cpp
    OtacMainTest.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    agl_ota_core
    GTest::GTest
    GTest::GMock
    Threads::Threads
    fastcdr
    fastdds
)

target_include_directories(${PROJECT_NAME} PRIVATE mock)