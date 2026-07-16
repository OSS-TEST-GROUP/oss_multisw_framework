# libsils_vecu example developer ECUs. Each subdirectory is a self-contained ECU that
# links the sils_vecu target and implements its own IBehavior/IPlant -- the reference
# for building your own ECU without touching core (docs/sils-extension-model.md P2).
add_subdirectory(brake-ecu)
