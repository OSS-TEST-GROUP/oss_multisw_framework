cmake_minimum_required(VERSION 3.5)

project ( "MultiMiddlewareFramework" )

## Build Options
set ( CMAKE_VERBOSE_MAKEFILE OFF)
set ( CMAKE_BUILD_TYPE "$ENV{BUILD_MODE}" )
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall" )

## Common Environment
set ( PROJECT_TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" )
set ( OUTPUT_DIR ${PROJECT_TOP_DIR}/output/${CMAKE_BUILD_TYPE} )

set ( EXTENSION_PLATFORM "ROS2" CACHE STRING "Target platform for extension framework" )

## Output directory Options
set ( CMAKE_INSTALL_PREFIX ${OUTPUT_DIR} )
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY "out/bin" )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY "out/lib" )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "out/lib" )

set ( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )
set ( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )

## Find package (pkgconfig)
#find_package ( PkgConfig )
#pkg_check_modules ( GLES glesv2 )
#if ( GLES_FOUND )
#    include_directories ( ${GLES_INCLUDE_DIRS} )
#    link_libraries ( ${GLES_LIBRARIES} )
#    link_directories ( ${GLES_LIBRARY_DIRS} )
#endif ()


## Global Definitions
#add_definitions ( -D_DEFINITTION )

## Global reference directories
# include_directories ()
# link_libraries ()
# link_directories ()


## Source Directory
add_subdirectory ( service )
