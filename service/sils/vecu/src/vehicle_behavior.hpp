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

#include <string>
#include <vector>

#include "vehicle_sample.hpp"  // EgoStateSample / VehicleControlSample

namespace sils {

// One other vECU's command, read as an input. `name` is the CHANNEL NAME from the channel
// map -- not the peer's process name, which we cannot know from the bus. It is what the
// arbitration decision is logged against, so it has to be stable and human-meaningful:
// pick channel names in the map that say which ECU they came from.
struct ControlRequest {
    std::string name;
    VehicleControlSample sample;
};

struct VehicleBehaviorContext {
    uint64_t tick_seq = 0;
    int64_t sim_time_ns = 0;
    EgoStateSample state;      // latest ego state this tick (zero-init before any arrives)
    bool have_state = false;
    // Profile B (docs/sils/sils-vehicle-interface-autoware.md): upstream control target this
    // tick (Autoware, via the VCU). Additive -- state-only behaviors (longitudinal/steering)
    // simply ignore these; a Vehicle Interface reads them.
    ControlTargetSample target;
    bool have_target = false;
    // ARBITRATION (docs/sils/sils-vecu-arbitration.md): other vECUs' commands this tick, one
    // entry per `in` channel of type canonical/vehicle-control that has produced a sample.
    // Empty for every single-ECU configuration, which is why this is additive: a behavior
    // that ignores it behaves exactly as before.
    //
    // ORDER IS NOT MEANINGFUL. It follows channel-map order, and the arbitration rule must
    // not depend on it -- min/clamp are commutative precisely so that two runs that receive
    // the same set in a different order produce the same command (S3 lockstep determinism).
    std::vector<ControlRequest> requests;
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
