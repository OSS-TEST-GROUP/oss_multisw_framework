cmake_minimum_required(VERSION 3.5)

project ( "MultiMiddlewareFramework" )

## Common Environment
set ( CMAKE_VERBOSE_MAKEFILE OFF)
set ( CMAKE_BUILD_TYPE "$ENV{BUILD_MODE}" )
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall" )

set ( PROJECT_TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" )
set ( OUTPUT_DIR ${PROJECT_TOP_DIR}/output/${CMAKE_BUILD_TYPE} )

## Build Options
set ( EXTENSION_PLATFORM "ROS2" CACHE STRING "Target platform for extension framework" )
set ( BUILD_UNITTEST FALSE CACHE STRING "Build the unittest" )

## Output directory Options
set ( CMAKE_INSTALL_PREFIX ${OUTPUT_DIR} )
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY "out/bin" )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY "out/lib" )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "out/lib" )

set ( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )
set ( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )

## Find package (pkgconfig)
if(NOT fastcdr_FOUND)
    find_package(fastcdr 2 REQUIRED)
endif()

if(NOT fastdds_FOUND)
    find_package(fastdds 3 REQUIRED)
endif()

if (BUILD_UNITTEST)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    )
    #set(INSTALL_GTEST OFF)
    FetchContent_MakeAvailable(googletest)
endif()

## Global Definitions
#add_definitions ( -D_DEFINITTION )

## Global reference directories
# include_directories ()
# link_libraries ()
# link_directories ()


## Source Directory
add_subdirectory ( idl )
add_subdirectory ( service )
