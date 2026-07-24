// adaptive-ecu -- Case 2: an AUTOSAR Adaptive software component plugged into libsils_vecu.
//
// The developer's SWC (src/adaptive_swc.hpp) talks only to ara::com; src/adaptive_behavior.hpp
// bridges ara::com <-> the canonical DDS contract (playing the AP runtime); RunVehicleProbe
// owns DDS/tick/trace. Because an Adaptive app is already a POSIX service-oriented process,
// there is no RTE/BSW to virtualize -- the seam is just the event binding. See README.md.

#include <memory>

#include "adaptive_behavior.hpp"
#include "config.hpp"        // sils::ConfigFromEnv / GetEnvDoubleOr
#include "vehicle_run.hpp"   // sils::RunVehicleProbe

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    auto behavior = std::make_unique<AdaptiveBehavior>();
    behavior->set_target_speed(sils::GetEnvDoubleOr("SILS_CTRL_SETPOINT", 10.0));
    behavior->set_kp(cfg.ctrl_kp);

    return sils::RunVehicleProbe(cfg, std::move(behavior));
}
