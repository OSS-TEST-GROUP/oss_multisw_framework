# brake-ecu -- example developer ECU built on the libsils_vecu SDK.
#
# In-tree this is added via service/sils/CMakeLists.txt (guarded by SILS_BUILD_EXAMPLES)
# so it compile-verifies against the real sils_vecu target and dogfoods the SDK boundary.
# An out-of-tree developer repo would instead find_package(sils_vecu) once that install/
# export lands; the target usage (link sils_vecu, list the generated IDL sources) is the
# same. Linking sils_vecu (INTERFACE) supplies the port headers + FastDDS + FMU flag.
#
# The one rough edge today: sils_vecu is header-only, so the consumer must still compile
# the generated IDL sources (VissRequest*, TickControl*) that RunProbe's wire type and
# lockstep clock reference. We re-glob them here exactly as vecu/CMakeLists.txt does.
# P4 (wire-type generalization) / SDK install-export will remove this duplication.

set(SILS_VISS_GEN_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl/vehicle_info_mgr")
set(SILS_TICK_GEN_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl/sils")

file(GLOB BRAKE_VISS_GENERATED_SRCS "${SILS_VISS_GEN_DIR}/VissRequest*.cxx")
file(GLOB BRAKE_TICK_GENERATED_SRCS "${SILS_TICK_GEN_DIR}/TickControl*.cxx")
if(NOT BRAKE_VISS_GENERATED_SRCS OR NOT BRAKE_TICK_GENERATED_SRCS)
    message(FATAL_ERROR
        "brake-ecu: generated IDL sources not found under ${SILS_VISS_GEN_DIR} / "
        "${SILS_TICK_GEN_DIR}. They are produced by the top-level add_subdirectory(idl) "
        "step; build with BUILD_SILS from the project root, not this dir standalone.")
endif()

add_executable(brake_ecu
    src/main.cc
    ${BRAKE_VISS_GENERATED_SRCS}
    ${BRAKE_TICK_GENERATED_SRCS}
)
# The developer's own headers (brake_behavior.hpp / brake_plant.hpp) live next to main.cc.
target_include_directories(brake_ecu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# One line gets the whole SDK: port headers, FastDDS/fastcdr, multidef link opt, FMU flag.
target_link_libraries(brake_ecu PRIVATE sils_vecu)

install(TARGETS brake_ecu RUNTIME DESTINATION bin)
