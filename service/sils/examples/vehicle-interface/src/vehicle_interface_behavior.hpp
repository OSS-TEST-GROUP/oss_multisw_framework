#ifndef VEHICLE_INTERFACE_BEHAVIOR_HPP
#define VEHICLE_INTERFACE_BEHAVIOR_HPP

// VehicleInterfaceBehavior -- a developer-authored IVehicleBehavior implementing the VCU
// (Vehicle Interface) role (libsils_vecu example, Option 2 plant coupling / Profile B).
// docs/sils-vehicle-interface-autoware.md.
//
// Unlike steer/brake (which CONTROL a single axis from state), the VCU sits between
// Autoware and the plant: it consumes the upstream control TARGET (ctx.target, from
// Autoware via the bridge) plus vehicle STATE, and produces the single unified command the
// plant (MORAI) accepts. So the "control logic" here is ARBITRATION + SAFETY, not path
// following (that is Autoware's).
//
// S1 policy (minimal but real):
//   - have_target : map the target through to the unified VehicleControl (near pass-through;
//                   clamps/rate-limits go here later).
//   - no target   : SAFE STOP -- command a steady deceleration, steering neutral, gear DRIVE.
//                   (A time-based command timeout is the S3 refinement; the have_target gate
//                   is the S1 stand-in for it -- integration-review's "control command
//                   timeout -> safe state".)

#include "vehicle_behavior.hpp"  // sils::IVehicleBehavior / VehicleBehaviorContext / VehicleControlSample

class VehicleInterfaceBehavior : public sils::IVehicleBehavior {
public:
    explicit VehicleInterfaceBehavior(double safe_decel) : safe_decel_(safe_decel) {}

    sils::VehicleControlSample OnTick(const sils::VehicleBehaviorContext& ctx) override {
        sils::VehicleControlSample c;
        if (ctx.have_target) {
            // Arbitration/mapping: pass Autoware's target through as the unified command.
            // (Physical clamps / rate limits would be applied here.)
            c.steering_angle = ctx.target.steering_angle;
            c.acceleration = ctx.target.acceleration;
            c.gear = ctx.target.gear;
        } else {
            // Safety: no upstream command -> safe stop (steady decel, straight, DRIVE).
            c.steering_angle = 0.0;
            c.acceleration = -safe_decel_;
            c.gear = 3;
        }
        return c;
    }

    const char* name() const override { return "vehicle-interface"; }

private:
    double safe_decel_;   // m/s^2, magnitude of the safe-stop deceleration
};

#endif  // VEHICLE_INTERFACE_BEHAVIOR_HPP
