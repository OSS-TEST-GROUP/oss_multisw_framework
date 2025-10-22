# in ota_mgr/otam_client_service/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# Add source files for OtamClientService
set(OTAM_CLIENT_SERVICE_SRCS
    otam_client_service.cc
)

# Create a static library for OtamClientService
# Static libraries are often preferred for internal components within a project
# as they get linked directly into the final executable.
add_library(OtamClientService STATIC ${OTAM_CLIENT_SERVICE_SRCS})

# (Optional) If OtamClientService itself depends on other libraries, link them here.
# For now, it's self-contained with standard library includes.
target_link_libraries(OtamClientService PRIVATE 
    OtamDdsImpl
)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")