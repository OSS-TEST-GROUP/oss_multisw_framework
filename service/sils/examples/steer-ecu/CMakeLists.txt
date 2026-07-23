# steer-ecu -- example developer ECU (Option 2 plant coupling) built on libsils_vecu.
#
# The plant-coupling counterpart of brake-ecu: it implements the SDK's IVehicleBehavior
# (src/steering_behavior.hpp) and runs via sils::RunVehicleProbe, instead of IBehavior +
# RunProbe. In-tree this is added via service/sils/CMakeLists.txt (guarded by
# SILS_BUILD_EXAMPLES); an out-of-tree repo would find_package(sils_vecu) and link the same
# targets. Target usage:
#   - sils_vecu        : port headers (vehicle_behavior/vehicle_run/…) + FastDDS + TickControl
#   - sils_vehicle_idl : the canonical EgoState/VehicleControl types the plant-coupling
#                        codec (vehicle_wire.hpp) (de)serializes -- a plant-coupled ECU links
#                        this instead of the VISS sils_viss_idl.

add_executable(steer_ecu src/main.cc)
target_include_directories(steer_ecu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(steer_ecu PRIVATE sils_vecu sils_vehicle_idl)

install(TARGETS steer_ecu RUNTIME DESTINATION bin)
