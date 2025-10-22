# in ota_mgr/otam_diagnostic_service/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# Add source files for OtamDiagnosticService
set(OTAM_DIAGNOSTIC_SERVICE_SRCS
    otam_diagnostic_service.cc
)

# Create a static library for OtamDiagnosticService
# Static libraries are often preferred for internal components within a project
# as they get linked directly into the final executable.
add_library(OtamDiagnosticService STATIC ${OTAM_DIAGNOSTIC_SERVICE_SRCS})

# (Optional) If OtamDiagnosticService itself depends on other libraries, link them here.
# For now, it's self-contained with standard library includes.
target_link_libraries(OtamDiagnosticService PRIVATE 
    OtamDdsImpl
)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")