cmake_minimum_required(VERSION 3.5)

project ( "diagnostics_unittest" )

## Compiler Options
add_compile_options(
    -fno-access-control
    -ftest-coverage
)

## gtest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
set(INSTALL_GTEST OFF)
FetchContent_MakeAvailable(googletest)

## Source
include_directories ( .. )

## Library
link_libraries ( gtest gmock )

## Target binary
add_executable ( DiagnosticManagerTest DiagnosticManagerTest.cpp )
add_executable ( DiagInfoRegistryTest DiagInfoRegistryTest.cpp )
add_executable ( DiagInfoTest DiagInfoTest.cpp )
add_executable ( ServiceExecutorTest ServiceExecutorTest.cpp )
add_executable ( ServiceTaskTest ServiceTaskTest.cpp )
add_executable ( RequestBaseTest RequestBaseTest.cpp )

## Install
install ( TARGETS DiagnosticManagerTest
                  DiagInfoRegistryTest
                  DiagInfoTest
                  ServiceExecutorTest
                  ServiceTaskTest
                  RequestBaseTest
    RUNTIME DESTINATION bin/unittest/diagnostics
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
