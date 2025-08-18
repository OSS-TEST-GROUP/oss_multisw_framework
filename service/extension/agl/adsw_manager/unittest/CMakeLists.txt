cmake_minimum_required(VERSION 3.5)
project("agl_adsw_manager_unittest")

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
    ContainerControllerTest.cpp
    AppControllerTest.cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE UNIT_TEST )

target_link_libraries(${PROJECT_NAME} PRIVATE
    agl_adsw_core
    GTest::GTest
    GTest::GMock
    Threads::Threads
    fastcdr
    fastdds
)

target_include_directories(${PROJECT_NAME} PRIVATE ../service_monitor mock)

target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--wrap=system")