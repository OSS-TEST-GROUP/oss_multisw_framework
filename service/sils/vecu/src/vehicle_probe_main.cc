// sils_vecu_vehicle_probe -- MORAI/AWSIM plant-coupling probe (Option 2). Thin entry point:
// read env, build the control behavior, hand it to RunVehicleProbe (vehicle_run.hpp) which
// owns DDS/channel-map/tick-loop/trace. Mirrors the VISS probe's shape (config -> ports ->
// RunProbe), but for the bus-coupled world model. docs/sils-morai-plant-adapter-design.md §5.
//
// Behavior is selected by SILS_VEHICLE_BEHAVIOR (longitudinal | steering); a developer's
// own plant-coupled ECU builds its own IVehicleBehavior and calls RunVehicleProbe directly.
// A "steering ECU" and a "longitudinal ECU" are the SAME binary with a different behavior +
// channel map (process = instance), matching the VISS probe/brake variants.

#include <iostream>
#include <memory>

#include "config.hpp"
#include "vehicle_behavior.hpp"
#include "vehicle_run.hpp"

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    auto behavior = sils::MakeVehicleBehavior(cfg.vehicle_behavior, cfg);
    if (!behavior) {
        std::cerr << "vehicle_probe: unknown SILS_VEHICLE_BEHAVIOR='" << cfg.vehicle_behavior
                  << "' (expected 'longitudinal' or 'steering')" << std::endl;
        return 2;
    }
    return sils::RunVehicleProbe(cfg, std::move(behavior));
}
