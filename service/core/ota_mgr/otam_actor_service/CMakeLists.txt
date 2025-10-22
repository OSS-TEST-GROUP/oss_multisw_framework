# in ota_mgr/otam_actor_service/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# Add source files for OtamActorService
set(OTAM_ACTOR_SERVICE_SRCS
    otam_actor_service.cc
)

# Create a static library for OtamActorService
# Static libraries are often preferred for internal components within a project
# as they get linked directly into the final executable.
add_library(OtamActorService STATIC ${OTAM_ACTOR_SERVICE_SRCS})

# (Optional) If OtamActorService itself depends on other libraries, link them here.
# For now, it's self-contained with standard library includes.
target_link_libraries(OtamActorService PRIVATE 
    OtamDdsImpl
)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")