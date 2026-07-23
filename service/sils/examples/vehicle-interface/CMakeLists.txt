# vehicle-interface (VCU) -- example developer ECU (Option 2 / Profile B) on libsils_vecu.
#
# Sits between Autoware and the plant: consumes a control TARGET + ego STATE, produces the
# unified command MORAI accepts. Implements IVehicleBehavior (src/vehicle_interface_behavior.hpp)
# and runs via sils::RunVehicleProbe -- same shape as steer-ecu, but with two inputs.
# Links sils_vecu (port headers + FastDDS + TickControl) and sils_vehicle_idl (canonical
# EgoState / VehicleControl / ControlTarget types the plant-coupling codec (de)serializes).

add_executable(vehicle_interface src/main.cc)
target_include_directories(vehicle_interface PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(vehicle_interface PRIVATE sils_vecu sils_vehicle_idl)

install(TARGETS vehicle_interface RUNTIME DESTINATION bin)
