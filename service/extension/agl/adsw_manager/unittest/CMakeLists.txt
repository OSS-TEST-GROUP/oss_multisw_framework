cmake_minimum_required(VERSION 3.5)
project("agl_adsw_manager_unittest")

add_executable(${PROJECT_NAME}
    test_main.cpp
    ADSWManagerTest.cpp
    ServiceRegistryTest.cpp
    AppStateReaderTest.cpp
    ServiceMonitorTest.cpp
    ServiceControllerTest.cpp
    MessageTransmitterTest.cpp
    StopAppHandlerTest.cpp
    StartAppHandlerTest.cpp
    NotifyAppStateWriterTest.cpp
    AppStateHandlerTest.cpp
    DDSManagerTest.cpp
    AppControllerTest.cpp
    ${GENERATED_LIFECYCLE_FILES}
    ${GENERATED_AGL_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    agl_adsw_core
    gtest
    gmock
    fastcdr
    fastdds
)

target_include_directories(${PROJECT_NAME} PRIVATE ../service_monitor mock ${GENERATED_IDL_DIR})

install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin/unittest/agl/adsw_manager
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)