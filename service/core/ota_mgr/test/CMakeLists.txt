# CMakeLists.txt for OTA Manager Test Applications

cmake_minimum_required(VERSION 3.5)

# Test executable for OtamActorService
add_executable(test_otam_actor_service
    test_otam_actor_service.cc
)

# Link with required libraries
target_link_libraries(test_otam_actor_service PRIVATE
    OtamDdsImpl
    pthread
    fastcdr
    fastdds
)

# Mock VISS Server for testing OtamVehicleService
add_executable(mock_viss_server
    mock_viss_server.cc
)

# Link with required libraries
target_link_libraries(mock_viss_server PRIVATE
    OtamDdsImpl
    pthread
    fastcdr
    fastdds
    yaml-cpp
)

# Mock Lifecycle Service for testing OtamActorService
add_executable(mock_lifecycle_service
    mock_lifecycle_service.cc
)

# Link with required libraries
target_link_libraries(mock_lifecycle_service PRIVATE
    OtamDdsImpl
    pthread
    fastcdr
    fastdds
    yaml-cpp
)

# Installation
install(TARGETS test_otam_actor_service mock_viss_server mock_lifecycle_service
    RUNTIME DESTINATION bin/test
    LIBRARY DESTINATION lib/test
    ARCHIVE DESTINATION lib/test
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/mock_argocd_server.py ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    DESTINATION bin/test
)

# Install mock data files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mock_data
    DESTINATION bin/test
    FILES_MATCHING
    PATTERN "*.json"
    PATTERN "*.yaml"
)
