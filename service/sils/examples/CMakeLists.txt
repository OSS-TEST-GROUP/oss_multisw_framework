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
