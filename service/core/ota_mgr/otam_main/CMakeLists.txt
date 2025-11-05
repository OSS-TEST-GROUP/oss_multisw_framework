# in ota_manager/otam_main/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# Add source files for OtamMain
set(OTAM_MAIN_SRCS
    otam_main.cc
)

# Create a static library for OtamMain
# Static libraries are often preferred for internal components within a project
# as they get linked directly into the final executable.
add_library(OtamMain STATIC ${OTAM_MAIN_SRCS})

# Specify include directories for OtamMain.
# PUBLIC means that any target linking to OtamMain will also get these include paths.
target_include_directories(OtamMain PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> # For otam_main.h
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # For header files in otam_common
)

# (Optional) If OtamMain itself depends on other libraries, link them here.
# For now, it's self-contained with standard library includes.
# target_link_libraries(OtamMain PRIVATE SomeOtherLibrary)