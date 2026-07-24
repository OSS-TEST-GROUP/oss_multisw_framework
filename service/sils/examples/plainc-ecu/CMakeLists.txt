# plainc-ecu -- Case 1 example: a plain-C / model-based auto-code controller on libsils_vecu.
#
# The generated model (src/speedctl.c) compiles as C; the adapter + main (src/*.hpp, main.cc)
# as C++. CMake mixes them in one target with no special handling -- the point of the case:
# the developer's auto-code is just another translation unit. Links the same targets as
# steer-ecu (sils_vecu for the ports/loop, sils_vehicle_idl for the canonical DDS types).
add_executable(plainc_ecu src/main.cc src/speedctl.c)
target_include_directories(plainc_ecu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(plainc_ecu PRIVATE sils_vecu sils_vehicle_idl)

install(TARGETS plainc_ecu RUNTIME DESTINATION bin)
