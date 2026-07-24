# libsils_vecu example developer ECUs. Each subdirectory is a self-contained ECU that
# links the sils_vecu target and implements its own IBehavior/IPlant -- the reference
# for building your own ECU without touching core (docs/sils-extension-model.md P2).
add_subdirectory(brake-ecu)
# brake-ecu-native: same ECU, but publishes a native custom DDS type via its own
# IWireCodec + locally-generated IDL (docs/sils-extension-model.md P4b).
add_subdirectory(brake-ecu-native)
# steer-ecu: plant-coupling (Option 2) developer ECU -- implements IVehicleBehavior and
# runs via RunVehicleProbe (bus-coupled world), the counterpart of brake-ecu's RunProbe.
add_subdirectory(steer-ecu)
# vehicle-interface: the VCU -- sits between Autoware and the plant (Profile B: two inputs,
# control target + ego state -> unified command). docs/sils-vehicle-interface-autoware.md.
add_subdirectory(vehicle-interface)

# "How a developer's code plugs in" -- three plant-coupled ECUs, one per integration case:
#   plainc-ecu   : Case 1, plain C / Simulink-TargetLink auto-code wrapped in IVehicleBehavior
#   adaptive-ecu : Case 2, AUTOSAR Adaptive SWC (ara::com) bridged to the canonical contract
#   classic-vecu : Case 3, AUTOSAR Classic ECU loaded as a V-ECU FMU (gated on SILS_ENABLE_FMU)
add_subdirectory(plainc-ecu)
add_subdirectory(adaptive-ecu)
add_subdirectory(classic-vecu)
