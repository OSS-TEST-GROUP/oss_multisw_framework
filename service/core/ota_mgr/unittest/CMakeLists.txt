# in ota_manager/unittest/CMakeLists.txt

# Do not call project() here, it's defined in the top-level CMakeLists.txt

# --- Optional: Configure GTest/GMock via FetchContent ---
# This approach fetches and builds GTest/GMock as part of your project,
# which can simplify setup and ensure consistent versions across environments.
# If you prefer to find system-installed GTest/GMock, remove this block
# and ensure 'find_package(GTest REQUIRED)' is used in test/CMakeLists.txt.
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0 # Or a newer stable release
)

# FetchContent_MakeAvailable will automatically add gtest and gmock targets
# (e.g., GTest::gtest, GMock::gmock) which can then be linked.
FetchContent_MakeAvailable(googletest)
# --- End of Optional GTest/GMock FetchContent block ---

# Add source files for mocks
set(OTAM_MOCK_SRCS
    mock/mock_service.cc
)

# Add header files for mocks
set(OTAM_MOCK_HDRS
    mock/mock_service.h
)

# Create a static library for the mocks
add_library(OtamMocks STATIC ${OTAM_MOCK_SRCS} ${OTAM_MOCK_HDRS})
target_include_directories(OtamMocks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../otam_main
)

target_include_directories(OtamMocks PRIVATE
    ${googletest_SOURCE_DIR}/googlemock/include
)

# Add the test executable.
add_executable(otam_main_tests test_otam_main.cc)

# Link the test executable to GTest, GMock, OtamMain library, and our mocks.
target_link_libraries(otam_main_tests PRIVATE
    gtest
    gmock
    OtamMain
    OtamMocks
)

# Specify include directories for OtamMain header and mock headers.
target_include_directories(otam_main_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/otam_main
    ${CMAKE_CURRENT_SOURCE_DIR}/mock
)

# Add the tests to CTest.
include(GoogleTest)
gtest_discover_tests(otam_main_tests)