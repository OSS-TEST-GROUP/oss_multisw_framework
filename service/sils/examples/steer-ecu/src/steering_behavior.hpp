#ifndef STEER_ECU_BEHAVIOR_HPP
#define STEER_ECU_BEHAVIOR_HPP

// SteeringBehavior -- a developer-authored IVehicleBehavior (libsils_vecu example, Option 2
// plant coupling). The "your control logic" half of a plant-coupled ECU: it implements the
// SDK's IVehicleBehavior port (vehicle_behavior.hpp) and nothing else. RunVehicleProbe
// (vehicle_run.hpp) calls OnTick once per tick with the latest ego STATE, stamps seq/stamp,
// and publishes the returned VehicleControl -- exactly as RunProbe does for an IBehavior.
//
// Semantics: a proportional steering controller. Drives the road-wheel angle toward a
// target -- steering = Kp * (target - current_angle). Longitudinal is left to another ECU
// (acceleration 0). Deterministic given (kp, target) and the fixed state stream, so the
// loop is golden-reproducible. The plant-coupling analogue of brake-ecu's BrakeBehavior.

#include "vehicle_behavior.hpp"  // sils::IVehicleBehavior / VehicleBehaviorContext / VehicleControlSample

class SteeringBehavior : public sils::IVehicleBehavior {
public:
    SteeringBehavior(double kp, double target_angle) : kp_(kp), target_angle_(target_angle) {}

    sils::VehicleControlSample OnTick(const sils::VehicleBehaviorContext& ctx) override {
        sils::VehicleControlSample c;
        c.steering_angle = kp_ * (target_angle_ - ctx.state.steering_angle);  // P toward target
        c.acceleration = 0.0;   // steering ECU: longitudinal owned by another ECU
        c.gear = 3;             // DRIVE
        return c;
    }

    const char* name() const override { return "steering-controller"; }

private:
    double kp_;
    double target_angle_;   // target road-wheel angle, rad
};

#endif  // STEER_ECU_BEHAVIOR_HPP
