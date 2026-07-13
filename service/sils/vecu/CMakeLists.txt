cmake_minimum_required(VERSION 3.14)
project(sils_vecu CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Phase 1 Vertical PoC (architecture v3 Final, §17 Phase 1 Scope Gate):
# one Contract Catalog endpoint (viss-request, config/sils/contracts/mmf-endpoints.yaml),
# WallClock only, Docker backend, no Relay/lockstep/replay/fault/FMI.
#
# Reuses the project-wide IDL generation from idl/dds/CMakeLists.txt (add_subdirectory(idl)
# runs before add_subdirectory(service) at the top level, so by the time this file
# configures, idl/dds/vehicle_info_mgr/VissRequest.idl has already been generated into
# ${CMAKE_INSTALL_PREFIX}/generated/idl/vehicle_info_mgr/). We do not re-generate or
# duplicate that IDL here -- it is the single Contract Catalog source of truth (§9.1).

if(NOT fastcdr_FOUND)
    find_package(fastcdr 2 REQUIRED)
endif()

if(NOT fastdds_FOUND)
    find_package(fastdds 3 REQUIRED)
endif()

set(SILS_VISS_GEN_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl/vehicle_info_mgr")

# fastddsgen's exact output filenames vary slightly by version (PubSubTypes /
# TypeObjectSupport pairs); glob rather than hardcode so this doesn't silently
# break on a version bump. Fail loudly at configure time if generation hasn't
# happened yet, instead of a confusing link error later.
file(GLOB SILS_VISS_GENERATED_SRCS "${SILS_VISS_GEN_DIR}/VissRequest*.cxx")
if(NOT SILS_VISS_GENERATED_SRCS)
    message(FATAL_ERROR
        "sils_vecu: no generated sources found under ${SILS_VISS_GEN_DIR}. "
        "Expected idl/dds/vehicle_info_mgr/VissRequest.idl to have been generated "
        "by add_subdirectory(idl) already (top-level CMakeLists.txt adds idl/ before "
        "service/). Check that BUILD_SILS was not configured standalone without the "
        "top-level idl/dds generation step.")
endif()

add_executable(sils_vecu_viss_probe
    src/viss_probe_main.cc
    ${SILS_VISS_GENERATED_SRCS}
)
target_include_directories(sils_vecu_viss_probe PRIVATE ${SILS_VISS_GEN_DIR})
target_link_libraries(sils_vecu_viss_probe PRIVATE fastdds fastcdr)

add_executable(sils_vecu_viss_verify
    src/viss_verify_main.cc
    ${SILS_VISS_GENERATED_SRCS}
)
target_include_directories(sils_vecu_viss_verify PRIVATE ${SILS_VISS_GEN_DIR})
target_link_libraries(sils_vecu_viss_verify PRIVATE fastdds fastcdr)

install(TARGETS sils_vecu_viss_probe sils_vecu_viss_verify RUNTIME DESTINATION bin)
