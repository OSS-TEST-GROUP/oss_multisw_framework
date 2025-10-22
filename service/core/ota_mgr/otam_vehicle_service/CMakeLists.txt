# in ota_mgr/otam_vehicle_service/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# Add source files for OtamVehicleService
set(OTAM_VEHICLE_SERVICE_SRCS
    otam_vehicle_service.cc
)

# Create a static library for OtamVehicleService
# Static libraries are often preferred for internal components within a project
# as they get linked directly into the final executable.
add_library(OtamVehicleService STATIC ${OTAM_VEHICLE_SERVICE_SRCS})

# (Optional) If OtamVehicleService itself depends on other libraries, link them here.
# For now, it's self-contained with standard library includes.
target_link_libraries(OtamVehicleService PRIVATE 
    OtamDdsImpl
)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")