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
elseif ( ${EXTENSION_PLATFORM} STREQUAL "GENERAL" )
    # Middleware-neutral variant: docker-container service management only, no ROS2
    # (adsw_manager 은 FastDDS+yaml-cpp, ROS2 링크 0). SiLS 등 비-ROS2 워크로드용.
    add_subdirectory ( general )
else ()
    message ( FATAL_ERROR "Invalid extension platform: ${EXTENSION_PLATFORM}" )
endif ()
