# in ota_mgr/otam_client_service/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

set(OTAM_CLIENT_SERVICE_SRCS
    otam_server_api_client_impl.cc
    otam_server_service.cc
)

# Create a static library for OtamServerService
# Static libraries are often preferred for internal components within a project
# as they get linked directly into the final executable.
add_library(OtamServerService STATIC ${OTAM_CLIENT_SERVICE_SRCS})

# Specify include directories for OtamServerService.
# PUBLIC means that any target linking to OtamServerService will also get these include paths.
# target_include_directories(OtamServerService PUBLIC
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> # For otam_client_service.h
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # For header files in otam_common
# )

# (Optional) If OtamServerService itself depends on other libraries, link them here.
# For now, it's self-contained with standard library includes.
target_link_libraries(OtamServerService PRIVATE
    CURL::libcurl
)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")