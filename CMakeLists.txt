cmake_minimum_required(VERSION 3.5)

project ( "MultiMiddlewareFramework" )

## Common Environment
set ( CMAKE_VERBOSE_MAKEFILE OFF)
# CMAKE_BUILD_TYPE: prefer (1) -DCMAKE_BUILD_TYPE on the cmake CLI,
# (2) the BUILD_MODE env var (used by docker/scripts/build.sh), then
# (3) Debug as a safe default. Without the fallback an unset env var
# leaves it empty, which cascades into OUTPUT_DIR = "output/<arch>//core"
# (double slash) and `make install` ends up writing to the wrong tree.
if ( NOT CMAKE_BUILD_TYPE )
    if ( DEFINED ENV{BUILD_MODE} AND NOT "$ENV{BUILD_MODE}" STREQUAL "" )
        set ( CMAKE_BUILD_TYPE "$ENV{BUILD_MODE}" )
    else()
        set ( CMAKE_BUILD_TYPE "Debug" )
    endif()
endif()
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall" )

set ( PROJECT_TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" )
set ( OUTPUT_DIR ${PROJECT_TOP_DIR}/output/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE} )

## Build Options
set ( EXTENSION_PLATFORM "ROS2" CACHE STRING "Target platform for extension framework" )
set ( BUILD_EXTENSION FALSE CACHE STRING "Build the extension framework for target platform" )
set ( BUILD_UNITTEST FALSE CACHE STRING "Build the unittest" )
set ( BUILD_SILS FALSE CACHE STRING "Build the SiLS vECU Phase 1 vertical-slice PoC" )

## Output directory Options
if (NOT BUILD_EXTENSION)
	set ( OUTPUT_DIR "${OUTPUT_DIR}/core" )
else()
	string ( TOLOWER "${EXTENSION_PLATFORM}" EXT_PLATFORM )
	set ( OUTPUT_DIR "${OUTPUT_DIR}/extension/${EXT_PLATFORM}" )
endif()
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

find_package(yaml-cpp REQUIRED)
find_package(CURL REQUIRED)

if (BUILD_UNITTEST)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    )
    #set(INSTALL_GTEST OFF)
    FetchContent_MakeAvailable(googletest)
    # Code Coverage
    add_compile_options(-O0 -g --coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
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
