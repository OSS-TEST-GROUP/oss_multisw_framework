// brake-ecu-native -- the brake ECU publishing a NATIVE custom DDS type (BrakeCmd)
// instead of the VISS envelope (libsils_vecu example, P4b).
//
// Compare with ../brake-ecu/src/main.cc: the behavior and plant are IDENTICAL (reused
// verbatim from the sibling example) -- the ONLY change is the injected wire codec. That
// is the whole demonstration: swapping the DDS type on the wire is one constructor, with
// no change to control logic, plant, RunProbe, config, or trace contract.
//
//     ../brake-ecu:        RunProbe(cfg, behavior, plant, MakeWireCodec(...));        // VISS envelope
//     here (brake-native): RunProbe(cfg, behavior, plant, BrakeCmdWireCodec{});       // native BrakeCmd

#include <memory>

#include "brake_behavior.hpp"   // reused from ../brake-ecu/src (wire-agnostic)
#include "brake_cmd_wire.hpp"
#include "brake_plant.hpp"      // reused from ../brake-ecu/src
#include "config.hpp"
#include "run_loop.hpp"

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    const double max_force = sils::GetEnvDoubleOr("BRAKE_MAX_FORCE", 100.0);
    const double gain = sils::GetEnvDoubleOr("BRAKE_GAIN", 0.02);

    auto behavior = std::make_unique<BrakeBehavior>(cfg.ctrl_kp, cfg.ctrl_setpoint, max_force);
    auto plant = std::make_unique<BrakePlant>(gain, cfg.plant_drag);

    // The one line that differs from ../brake-ecu: publish the native BrakeCmd type.
    return sils::RunProbe(cfg, std::move(behavior), std::move(plant),
                          std::make_unique<BrakeCmdWireCodec>());
}
