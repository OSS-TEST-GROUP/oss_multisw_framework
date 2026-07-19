# brake-ecu -- example developer ECU built on the libsils_vecu SDK.
#
# In-tree this is added via service/sils/CMakeLists.txt (guarded by SILS_BUILD_EXAMPLES)
# so it compile-verifies against the real sils_vecu target and dogfoods the SDK boundary.
# An out-of-tree developer repo would instead `find_package(sils_vecu)` and link the same
# targets (T1-B install/export). Either way the target usage is identical:
#   - sils_vecu       : port headers + FastDDS/fastcdr + TickControl(clock) + FMU flag
#   - sils_viss_idl   : the VISS wire type this ECU publishes (viss_wire.hpp)
# The generated IDL is compiled ONCE into those libraries (vecu/CMakeLists.txt), so this
# example just links -- no re-glob / re-compile of VissRequest*/TickControl* (T1-B).

add_executable(brake_ecu src/main.cc)
# The developer's own headers (brake_behavior.hpp / brake_plant.hpp) live next to main.cc.
target_include_directories(brake_ecu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(brake_ecu PRIVATE sils_vecu sils_viss_idl)

install(TARGETS brake_ecu RUNTIME DESTINATION bin)

# FMU-path twin (src/main_fmu.cc): identical ECU, plant from an exported .fmu via
# sils::MakeFmuPlant. Only meaningful when the SDK carries FMU support (sils_vecu sets
# SILS_HAVE_FMU + links fmi4c under -DSILS_ENABLE_FMU), so gate the target on the same
# option -- otherwise it would compile but MakeFmuPlant could only ever error out.
if(SILS_ENABLE_FMU)
    add_executable(brake_ecu_fmu src/main_fmu.cc)
    target_include_directories(brake_ecu_fmu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(brake_ecu_fmu PRIVATE sils_vecu sils_viss_idl)
    install(TARGETS brake_ecu_fmu RUNTIME DESTINATION bin)
endif()
