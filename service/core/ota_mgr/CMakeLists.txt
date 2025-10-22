# Top-level CMakeLists.txt for the OTA Manager project

cmake_minimum_required(VERSION 3.5) # Minimum CMake version
project(OtaManager CXX)             # Overall project name

# Find FastDDS package
# if(NOT fastcdr_FOUND)
#     find_package(fastcdr 2 REQUIRED)
# endif()

# if(NOT fastdds_FOUND)
#     find_package(fastdds 3 REQUIRED)
# endif()

set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")
set(GENERATED_OTA_MGR_DIR "${GENERATED_DIR}/ota_mgr")
set(GENERATED_LIFECYCLE_MGR_DIR "${GENERATED_DIR}/lifecycle")

message(STATUS "BUILD_INTERFACE:CMAKE_CURRENT_SOURCE_DIR = $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
message(STATUS "INSTALL_INTERFACE:include = $<INSTALL_INTERFACE:include>")
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

include_directories (
    ${GENERATED_DIR}  # For generated/idl
    ${GENERATED_OTA_MGR_DIR}  # For generated/idl/ota_mgr
    ${CMAKE_INSTALL_PREFIX}
    ${CMAKE_INSTALL_PREFIX}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    # $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> # For otam_client_service.h
    # $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # For header files in otam_common
)

link_libraries ( 
    pthread fastcdr fastdds
)

# Add source directory
add_subdirectory(otam_common)
add_subdirectory(otam_main)
add_subdirectory(otam_client_service)
add_subdirectory(otam_server_service)
add_subdirectory(otam_diagnostic_service)
add_subdirectory(otam_vehicle_service)
add_subdirectory(otam_actor_service)

# Add test directory
# Ensure that tests are only built when explicitly requested (e.g., by CI or developers)
# This is a common practice to avoid unnecessary test compilation in production builds.
# option(BUILD_UNITTEST "Build the unit tests" ON) # Default to ON for development

if(BUILD_UNITTEST)
    add_subdirectory(unittest)
endif()

# Add test application directory for integration testing
option(BUILD_TEST "Build the test applications" ON) # Default to ON for development

if(BUILD_TEST)
#    add_subdirectory(test)
endif()

# (Optional) Install targets
# install(TARGETS OtamMain DESTINATION lib) # Example for library installation
add_executable ( ${PROJECT_NAME} main.cc )
target_link_libraries(${PROJECT_NAME} PRIVATE 
    OtamMain
    OtamClientService
    OtamServerService
    OtamDiagnosticService
    OtamVehicleService
    OtamActorService
)

install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# idl
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/idl/dds/ota_mgr
    DESTINATION bin/idl
)

# config
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
    DESTINATION bin/config)