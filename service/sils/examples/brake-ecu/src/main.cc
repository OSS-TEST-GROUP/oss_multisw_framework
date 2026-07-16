// brake-ecu -- a complete developer ECU built on libsils_vecu (code-path plant).
//
// This is the whole executable: parse the SILS_* environment, assemble the two ports
// this ECU customizes (BrakeBehavior + BrakePlant), and hand them to sils::RunProbe,
// which owns everything else (DDS setup, the tick loop, the trace contract, teardown).
// It is the reference shape every developer ECU follows -- compare with the default
// probe (service/sils/vecu/src/viss_probe_main.cc), which is identical except it picks
// the built-in ports from the factories instead of constructing custom ones.
//
// ECU-specific parameters that the SDK's ProbeConfig doesn't carry (brake gain, max
// force) are read straight from the environment with the SDK's own helpers -- the SDK
// owns the COMMON contract (timing/QoS/topic/attestation), the developer owns their
// model's knobs. Reused SDK fields: SILS_CTRL_KP / SILS_CTRL_SETPOINT (controller),
// SILS_PLANT_DRAG (drag), SILS_PLANT_INIT (initial speed).

#include <memory>

#include "brake_behavior.hpp"
#include "brake_plant.hpp"
#include "config.hpp"     // sils::ConfigFromEnv / GetEnvDoubleOr
#include "run_loop.hpp"   // sils::RunProbe

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    // ECU-specific knobs (this ECU's own env contract, documented in its descriptor).
    const double max_force = sils::GetEnvDoubleOr("BRAKE_MAX_FORCE", 100.0);
    const double gain = sils::GetEnvDoubleOr("BRAKE_GAIN", 0.02);

    auto behavior = std::make_unique<BrakeBehavior>(cfg.ctrl_kp, cfg.ctrl_setpoint, max_force);
    auto plant = std::make_unique<BrakePlant>(gain, cfg.plant_drag);   // ── code path
    // FMU path (extension-model §3.C) would replace the line above with:
    //   auto plant = sils::MakeFmuPlant(cfg);   // (public API arrives in P3)

    return sils::RunProbe(cfg, std::move(behavior), std::move(plant));
}
