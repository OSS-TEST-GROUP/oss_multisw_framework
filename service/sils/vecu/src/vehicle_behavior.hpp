// IVehicleBehavior -- control port for the MORAI/AWSIM plant-coupling path (Option 2).
//
// The bus-coupled counterpart of IBehavior (behavior.hpp): given this tick's latest ego
// STATE, compute the vehicle CONTROL to publish. Introduced now because a SECOND real
// consumer exists -- longitudinal (the S1 stand-in) AND steering -- which is exactly the
// earned-abstraction bar the other ports use (clock/behavior/plant each waited for two
// impls). The run loop (vehicle_run.hpp) injects one and calls OnTick each tick; it stamps
// seq/stamp_ns from the tick, so a behavior only fills the control fields.
//
// DDS-free (PODs from vehicle_sample.hpp) so control logic compiles and unit-tests without
// Fast-DDS. Built-ins are simple P-controllers -- a developer adds their own IVehicleBehavior
// the same way they add a custom IBehavior.

#ifndef SILS_VECU_VEHICLE_BEHAVIOR_HPP
#define SILS_VECU_VEHICLE_BEHAVIOR_HPP

#include <memory>
#include <string>

#include "config.hpp"          // sils::ProbeConfig
#include "vehicle_sample.hpp"  // EgoStateSample / VehicleControlSample

namespace sils {

struct VehicleBehaviorContext {
    uint64_t tick_seq = 0;
    int64_t sim_time_ns = 0;
    EgoStateSample state;   // latest ego state seen this tick (zero-initialized before any arrives)
    bool have_state = false;
};

class IVehicleBehavior {
public:
    virtual ~IVehicleBehavior() = default;
    // Compute this tick's control. seq/stamp_ns are set by the run loop, not here.
    virtual VehicleControlSample OnTick(const VehicleBehaviorContext& ctx) = 0;
    virtual const char* name() const = 0;
};

// Longitudinal P-controller (the S1 stand-in, preserved exactly): drive acceleration to
// hold a target speed; steering held 0. accel = Kp * (v_setpoint - vx).
class LongitudinalBehavior : public IVehicleBehavior {
public:
    LongitudinalBehavior(double kp, double v_setpoint) : kp_(kp), v_setpoint_(v_setpoint) {}
    VehicleControlSample OnTick(const VehicleBehaviorContext& ctx) override {
        VehicleControlSample c;
        c.steering_angle = 0.0;
        c.acceleration = kp_ * (v_setpoint_ - ctx.state.vx);
        c.gear = 3;  // DRIVE
        return c;
    }
    const char* name() const override { return "longitudinal-p"; }

private:
    double kp_;
    double v_setpoint_;
};

// Steering P-controller (the second consumer): drive the road-wheel angle toward a target
// steering angle; longitudinal left to another ECU (acceleration 0). steering = Kp *
// (steer_setpoint - steering_angle). Same shape as longitudinal, different axis.
class SteeringBehavior : public IVehicleBehavior {
public:
    SteeringBehavior(double kp, double steer_setpoint) : kp_(kp), steer_setpoint_(steer_setpoint) {}
    VehicleControlSample OnTick(const VehicleBehaviorContext& ctx) override {
        VehicleControlSample c;
        c.steering_angle = kp_ * (steer_setpoint_ - ctx.state.steering_angle);
        c.acceleration = 0.0;
        c.gear = 3;  // DRIVE
        return c;
    }
    const char* name() const override { return "steering-p"; }

private:
    double kp_;
    double steer_setpoint_;
};

// Factory. kind "longitudinal" (default) or "steering"; any other value returns nullptr so
// the caller fails fast on an unknown SILS_VEHICLE_BEHAVIOR (§14). Reuses cfg.ctrl_kp for
// both gains; cfg.ctrl_setpoint is the target speed, cfg.steer_setpoint the target angle.
inline std::unique_ptr<IVehicleBehavior> MakeVehicleBehavior(const std::string& kind,
                                                             const ProbeConfig& cfg) {
    if (kind == "longitudinal") {
        return std::make_unique<LongitudinalBehavior>(cfg.ctrl_kp, cfg.ctrl_setpoint);
    }
    if (kind == "steering") {
        return std::make_unique<SteeringBehavior>(cfg.ctrl_kp, cfg.steer_setpoint);
    }
    return nullptr;
}

}  // namespace sils

#endif  // SILS_VECU_VEHICLE_BEHAVIOR_HPP
