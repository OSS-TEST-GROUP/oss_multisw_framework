// vehicle-interface -- the VCU: a developer ECU that sits between Autoware and the plant
// (libsils_vecu example, Option 2 / Profile B). docs/sils/sils-vehicle-interface-autoware.md.
//
// Parses env, builds VehicleInterfaceBehavior, hands it to sils::RunVehicleProbe. The run
// loop binds the channel map's inputs -- a `canonical/control-target` channel (Autoware,
// via the bridge) AND a `canonical/ego-state` channel (MORAI) -- and the output
// `canonical/vehicle-control` channel (to the plant). Same "developer authors a port, SDK
// runs it" shape as steer-ecu; the difference is two inputs (Profile B).
//
// ECU-specific knob (safe-stop deceleration) via the SDK helper. The channel map
// (channelmap/vehicle-interface.yaml) is selected with SILS_CHANNELMAP.

#include <memory>

#include "config.hpp"                    // sils::ConfigFromEnv / GetEnvDoubleOr
#include "vehicle_interface_behavior.hpp"
#include "vehicle_run.hpp"               // sils::RunVehicleProbe

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    // ECU-specific knob: safe-stop deceleration magnitude, m/s^2 (used when no target).
    const double safe_decel = sils::GetEnvDoubleOr("VI_SAFE_DECEL", 2.0);
    // Lateral clamp for arbitration (rad). DEFAULT 0 = OFF, deliberately: a clamp is a
    // vehicle limit and we do not have one. Turning it on with an invented number would put
    // a fabricated requirement into every run's output. docs/sils/sils-vecu-arbitration.md §7.
    const double steer_limit = sils::GetEnvDoubleOr("VI_STEER_LIMIT_RAD", 0.0);

    auto behavior = std::make_unique<VehicleInterfaceBehavior>(safe_decel, steer_limit);
    return sils::RunVehicleProbe(cfg, std::move(behavior));
}
