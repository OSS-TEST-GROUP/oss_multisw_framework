cmake_minimum_required(VERSION 3.5)

project ( "core" )

## Sub Directory
add_subdirectory ( diagnostics )
add_subdirectory ( lifecycle_mgr )
add_subdirectory ( ota_mgr )
add_subdirectory ( security_mgr )
#add_subdirectory ( vehicle_info_mgr )
