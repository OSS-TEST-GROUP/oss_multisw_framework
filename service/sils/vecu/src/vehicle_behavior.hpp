// IVehicleBehavior -- control port for the MORAI/AWSIM plant-coupling path (Option 2).
//
// The bus-coupled counterpart of IBehavior (behavior.hpp): given this tick's latest ego
// STATE, compute the vehicle CONTROL to publish. The run loop (vehicle_run.hpp) injects
// one and calls OnTick each tick; it stamps seq/stamp_ns from the tick, so a behavior only
// fills the control fields. A developer's plant-coupled ECU implements this and hands it to
// RunVehicleProbe -- exactly as an ECU implements IBehavior for RunProbe. See the steering
// ECU example (service/sils/examples/steer-ecu) for a developer-authored impl.
//
// DDS-free (PODs from vehicle_sample.hpp) so control logic compiles and unit-tests without
// Fast-DDS. The one built-in here is LongitudinalBehavior -- the reference control the
// default vehicle probe (vehicle_probe_main.cc) runs, mirroring how behavior.hpp ships
// VissRequestBehavior for the default VISS probe. Concrete ECU control (steering, etc.)
// lives with its example/ECU, not in the SDK.

#ifndef SILS_VECU_VEHICLE_BEHAVIOR_HPP
#define SILS_VECU_VEHICLE_BEHAVIOR_HPP

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

// Reference longitudinal P-controller (the default vehicle probe's behavior): drive
// acceleration to hold a target speed; steering held 0. accel = Kp * (v_setpoint - vx).
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

}  // namespace sils

#endif  // SILS_VECU_VEHICLE_BEHAVIOR_HPP
