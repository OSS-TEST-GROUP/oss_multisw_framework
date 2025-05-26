# Top-level CMakeLists.txt for the OTA Manager project

cmake_minimum_required(VERSION 3.10) # Minimum CMake version
project(OtaManager CXX)             # Overall project name

# Set common C++ standard for the entire project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF) # Ensure strict adherence to standard

# Add source directory
add_subdirectory(otam_main)

# Add test directory
# Ensure that tests are only built when explicitly requested (e.g., by CI or developers)
# This is a common practice to avoid unnecessary test compilation in production builds.
option(BUILD_TESTING "Build the unit tests" ON) # Default to ON for development

if(BUILD_TESTING)
    add_subdirectory(unittest)
endif()

# (Optional) Install targets
# install(TARGETS OtamMain DESTINATION lib) # Example for library installation