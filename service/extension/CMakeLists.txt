cmake_minimum_required(VERSION 3.5)

project ( "extension" )

message ( "ExtentionPlatform : ${EXTENSION_PLATFORM}" )

## Sub Directory
if ( ${EXTENSION_PLATFORM} STREQUAL "AAutoSAR" )
    add_subdirectory ( adaptive_autoSAR )
elseif ( ${EXTENSION_PLATFORM} STREQUAL "AGL" )
    add_subdirectory ( agl )
elseif ( ${EXTENSION_PLATFORM} STREQUAL "ROS2" )
    add_subdirectory ( ros2 )
else ()
    message ( FATAL_ERROR "Invalid extension platform: ${EXTENSION_PLATFORM}" )
endif ()
