cmake_minimum_required(VERSION 3.5)

project ( "extension" )

## Sub Directory
if ( ${EXTENSION_PLATFORM} STREQUAL "AdaptiveAutoSAR" )
    add_subdirectory ( adaptive_autoSAR )
elseif ( ${EXTENSION_PLATFORM} STREQUAL "AGL" )
    add_subdirectory ( agl )
elseif ( ${EXTENSION_PLATFORM} STREQUAL "ROS2" )
    add_subdirectory ( ros2 )
else ()
    message ( "Not defined platform for extension framework" )
endif ()
