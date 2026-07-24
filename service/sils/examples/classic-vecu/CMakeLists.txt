# classic-vecu -- Case 3 example: an AUTOSAR Classic ECU loaded as a V-ECU FMU.
#
# Meaningful only when the SDK carries FMU support (sils_vecu sets SILS_HAVE_FMU + links
# fmi4c under -DSILS_ENABLE_FMU) -- FmuBehavior/MakeFmuBehavior can otherwise only error out.
# So gate the target on the same option as brake_ecu_fmu. The sample V-ECU FMU is built
# separately and reproducibly from models/vecu-fmu/ (build.sh); nothing here compiles it.
if(SILS_ENABLE_FMU)
    add_executable(classic_vecu src/main.cc)
    target_include_directories(classic_vecu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(classic_vecu PRIVATE sils_vecu sils_vehicle_idl)
    install(TARGETS classic_vecu RUNTIME DESTINATION bin)
else()
    message(STATUS "classic-vecu: skipped (needs -DSILS_ENABLE_FMU for the V-ECU FMU path)")
endif()
