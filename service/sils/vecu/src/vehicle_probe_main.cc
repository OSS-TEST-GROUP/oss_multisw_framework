// sils_vecu_vehicle_probe -- the DEFAULT plant-coupling probe (Option 2), the bus-coupled
// counterpart of sils_vecu_viss_probe. Reads env, builds the reference LongitudinalBehavior,
// and hands it to RunVehicleProbe (vehicle_run.hpp) which owns DDS/channel-map/tick-loop/
// trace. docs/sils/sils-morai-plant-adapter-design.md §5.
//
// This is the SDK's reference probe (like viss_probe uses the built-in VissRequestBehavior).
// A developer's own plant-coupled ECU authors its own IVehicleBehavior and calls
// RunVehicleProbe the same way -- see service/sils/examples/steer-ecu for a steering ECU.

#include <memory>

#include "config.hpp"
#include "vehicle_behavior.hpp"
#include "vehicle_run.hpp"

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();
    auto behavior = std::make_unique<sils::LongitudinalBehavior>(cfg.ctrl_kp, cfg.ctrl_setpoint);
    return sils::RunVehicleProbe(cfg, std::move(behavior));
}
