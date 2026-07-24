#ifndef PLAINC_ECU_BEHAVIOR_HPP
#define PLAINC_ECU_BEHAVIOR_HPP

// PlainCBehavior -- Case 1 of "how a developer's code plugs into libsils_vecu": a PLAIN C,
// auto-generated control model (speedctl.h, as from Simulink Embedded Coder / TargetLink)
// adapted to the SDK's IVehicleBehavior port. The C model is SDK-agnostic and untouched;
// this ~20-line adapter is the ENTIRE integration. It is the lowest-friction case -- a
// model-based-design shop auto-codes to C and wraps it once.
//
// Seam (SI throughout, matching the canonical contract -- no unit conversion needed here;
// a real project puts any scaling/limiting at exactly this boundary):
//   EgoStateSample -> speedctl_ExtU -> speedctl_step -> speedctl_ExtY -> VehicleControlSample
//
// The C model is linked in C (extern "C"); the run loop (RunVehicleProbe) owns DDS, the
// channel-map reader/writer binding, the tick, and the trace -- as for steer-ecu.

extern "C" {
#include "speedctl.h"
}

#include "vehicle_behavior.hpp"  // sils::IVehicleBehavior / VehicleBehaviorContext

class PlainCBehavior : public sils::IVehicleBehavior {
public:
    PlainCBehavior() { speedctl_initialize(&params_); }

    // Let the ECU override the model's calibrations from its env knobs (optional).
    void set_v_setpoint(double v) { params_.v_setpoint = v; }
    void set_kp_speed(double k) { params_.kp_speed = k; }

    sils::VehicleControlSample OnTick(const sils::VehicleBehaviorContext& ctx) override {
        speedctl_ExtU u;
        u.vx = ctx.state.vx;
        u.steering_angle = ctx.state.steering_angle;

        speedctl_ExtY y;
        speedctl_step(&u, &params_, &y);  // the developer's generated model, verbatim

        sils::VehicleControlSample c;
        c.acceleration = y.accel_cmd;
        c.steering_angle = y.steer_cmd;
        c.gear = 3;  // DRIVE
        return c;
    }

    const char* name() const override { return "plainc-speedctl"; }

private:
    speedctl_Params params_;
};

#endif  // PLAINC_ECU_BEHAVIOR_HPP
