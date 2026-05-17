cmake_minimum_required(VERSION 3.5)

project("dds_bridge")

include_directories(.)
link_libraries(pthread fastcdr fastdds)

add_executable(${PROJECT_NAME}
    main.cc
    ota_bridge.cc
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install OtaManager IDL files alongside the binary so the bridge can
# load them at runtime via FastDDS dynamic IDL parser. The bridge is
# typically invoked with `./dds_bridge ./idl` and resolves
# "ota_mgr/OtamClientServiceMsg.idl" relative to that root.
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/idl/dds/common
    ${CMAKE_SOURCE_DIR}/idl/dds/ota_mgr
    # F012/F013: bridge also forwards lifecycle_mgr topics — explicit
    # install so the runtime IDL parser finds them regardless of which
    # other target happened to copy them first.
    ${CMAKE_SOURCE_DIR}/idl/dds/lifecycle
    DESTINATION bin/idl
)
