cmake_minimum_required(VERSION 3.5)

project("dds_bridge")

include_directories(.)
link_libraries(pthread fastcdr fastdds)

add_executable(${PROJECT_NAME}
    main.cc
    dds_bridge.cc
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install IDL files alongside the binary so the bridge can load them at
# runtime via the FastDDS dynamic IDL parser. The bridge is typically
# invoked with `./dds_bridge ./idl` and resolves include paths like
# "ota_mgr/OtamClientServiceMsg.idl" or "lifecycle/NotifyServiceState.idl"
# relative to that root.
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/idl/dds/common
    ${CMAKE_SOURCE_DIR}/idl/dds/ota_mgr
    # F012/F013: bridge also forwards lifecycle_mgr topics — explicit
    # install so the runtime IDL parser finds them regardless of which
    # other target happened to copy them first.
    ${CMAKE_SOURCE_DIR}/idl/dds/lifecycle
    DESTINATION bin/idl
)
