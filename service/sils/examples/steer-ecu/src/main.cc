// steer-ecu -- a complete developer ECU built on libsils_vecu (Option 2 plant coupling).
//
// The whole executable: parse the SILS_* environment, build the one port this ECU
// customizes (SteeringBehavior), and hand it to sils::RunVehicleProbe, which owns
// everything else (DDS setup, channel-map reader/writer binding, the tick loop, the trace
// contract, teardown). This is the plant-coupling counterpart of brake-ecu/src/main.cc
// (which uses RunProbe for the VISS request model) -- same "developer authors a port,
// SDK runs it" shape.
//
// ECU-specific knob (target steering angle) is read straight from the environment with the
// SDK helper -- the SDK owns the COMMON contract (timing/QoS/topic/channel-map), the
// developer owns their control's knobs. Reused SDK field: SILS_CTRL_KP (gain). The channel
// map (channelmap/steer-min.yaml) is selected via SILS_CHANNELMAP.

#include <memory>

#include "config.hpp"            // sils::ConfigFromEnv / GetEnvDoubleOr
#include "steering_behavior.hpp"
#include "vehicle_run.hpp"       // sils::RunVehicleProbe

int main() {
    const sils::ProbeConfig cfg = sils::ConfigFromEnv();

    // ECU-specific knob (this ECU's own env contract): target road-wheel angle, rad.
    const double target_angle = sils::GetEnvDoubleOr("STEER_SETPOINT", 0.1);

    auto behavior = std::make_unique<SteeringBehavior>(cfg.ctrl_kp, target_angle);
    return sils::RunVehicleProbe(cfg, std::move(behavior));
}
