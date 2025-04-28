cmake_minimum_required(VERSION 3.5)

project ( "ros2" )

## Sub Directory
add_subdirectory ( adsw_manager )
add_subdirectory ( extension_communication )
add_subdirectory ( ota_client )
add_subdirectory ( security_client )
