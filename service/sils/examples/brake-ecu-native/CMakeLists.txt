# brake-ecu-native -- example ECU publishing a NATIVE custom DDS type (BrakeCmd),
# demonstrating the "bring your own IDL" path (docs/sils-extension-model.md P4b).
#
# The custom type is generated HERE by fastddsgen -- not added to the core idl/dds tree --
# so no core file changes and non-SILS builds don't generate it. An out-of-tree developer
# runs the identical fastddsgen command in their own repo; we invoke it inline to keep the
# example self-contained.

set(BRAKE_NATIVE_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")
file(MAKE_DIRECTORY ${BRAKE_NATIVE_GEN})

# fastddsgen is on PATH in the builder image (same tool idl/dds/CMakeLists.txt uses).
# Runs at configure time so the sources exist before the GLOB + compile below.
execute_process(
    COMMAND fastddsgen -replace -d ${BRAKE_NATIVE_GEN}
            ${CMAKE_CURRENT_SOURCE_DIR}/idl/BrakeCmd.idl
    RESULT_VARIABLE BRAKE_NATIVE_GEN_RESULT)
if(NOT BRAKE_NATIVE_GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "brake-ecu-native: fastddsgen failed for BrakeCmd.idl "
                        "(is fastddsgen on PATH? build with BUILD_SILS from the project root).")
endif()

file(GLOB BRAKE_NATIVE_SRCS "${BRAKE_NATIVE_GEN}/BrakeCmd*.cxx")
if(NOT BRAKE_NATIVE_SRCS)
    message(FATAL_ERROR "brake-ecu-native: no generated BrakeCmd*.cxx under ${BRAKE_NATIVE_GEN}")
endif()

# This ECU compiles NO VISS-generated sources: the wire port (wire.hpp) names no DDS type,
# the built-in VISS codecs live in viss_wire.hpp (not included here), and RunProbe's read
# path decodes through the injected codec -- so publishing AND the response/input readers
# are 100% native BrakeCmd. The only core-generated source still needed is TickControl,
# which the clock port uses for lockstep tick control (orthogonal to the wire type).
set(SILS_TICK_GEN_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl/sils")
file(GLOB SILS_TICK_GENERATED_SRCS "${SILS_TICK_GEN_DIR}/TickControl*.cxx")
if(NOT SILS_TICK_GENERATED_SRCS)
    message(FATAL_ERROR
        "brake-ecu-native: core generated IDL (TickControl) not found under "
        "${SILS_TICK_GEN_DIR}; build with BUILD_SILS from the project root.")
endif()

add_executable(brake_ecu_native
    src/main.cc
    ${BRAKE_NATIVE_SRCS}
    ${SILS_TICK_GENERATED_SRCS}
)
target_include_directories(brake_ecu_native PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${BRAKE_NATIVE_GEN}                              # generated BrakeCmd*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../brake-ecu/src)    # reuse BrakeBehavior / BrakePlant
target_link_libraries(brake_ecu_native PRIVATE sils_vecu)

install(TARGETS brake_ecu_native RUNTIME DESTINATION bin)
