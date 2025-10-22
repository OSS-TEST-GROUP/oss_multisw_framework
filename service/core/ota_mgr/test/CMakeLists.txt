# CMakeLists.txt for OTA Manager Test Applications

cmake_minimum_required(VERSION 3.5)

# Test executable for OtamActorService
add_executable(test_otam_actor_service
    test_otam_actor_service.cc
)

# Link with required libraries
target_link_libraries(test_otam_actor_service PRIVATE
    OtamCommon
    pthread
    fastcdr
    fastdds
)

# Mock ArgoCD Server for testing OtamServerService
add_executable(mock_argocd_server
    mock_argocd_server.cc
)

# Link with required libraries
target_link_libraries(mock_argocd_server PRIVATE
    pthread
)

# Installation
install(TARGETS test_otam_actor_service mock_argocd_server
    RUNTIME DESTINATION bin/test
    LIBRARY DESTINATION lib/test
    ARCHIVE DESTINATION lib/test
)

# Install mock data files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mock_data
    DESTINATION bin/test
    FILES_MATCHING PATTERN "*.json"
)
