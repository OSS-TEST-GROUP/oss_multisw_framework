#ifndef ADAPTIVE_ECU_BEHAVIOR_HPP
#define ADAPTIVE_ECU_BEHAVIOR_HPP

// AdaptiveBehavior -- the bridge between an AUTOSAR Adaptive SWC (adaptive_swc.hpp) and the
// SDK's IVehicleBehavior port. It plays the role the AP runtime + ara::com binding play in
// production: on each tick it translates the canonical ego STATE into the SWC's consumed
// service event and Deliver()s it (running the SWC's control), then picks up the SWC's
// offered command event and translates it back to the canonical VehicleControl -- including
// the AP-gear -> canonical-gear anti-corruption mapping (the "bridge" work).
//
// This adapter is the ENTIRE SiLS-specific integration for an Adaptive ECU. The developer's
// SWC (adaptive_swc.hpp) and its ara::com usage are untouched; only the shim is swapped for
// the vendor runtime in production. Case 2 of the three plug-in cases (see README.md).

#include <cstdint>

#include "adaptive_swc.hpp"
#include "vehicle_behavior.hpp"  // sils::IVehicleBehavior / VehicleBehaviorContext

class AdaptiveBehavior : public sils::IVehicleBehavior {
public:
    void set_target_speed(double v) { swc_.set_target_speed(v); }
    void set_kp(double k) { swc_.set_kp(k); }

    sils::VehicleControlSample OnTick(const sils::VehicleBehaviorContext& ctx) override {
        // (1) Runtime delivers the subscribed VehicleStatus event -> SWC handler runs and
        //     Send()s a ControlCommand (all inside Deliver(), event-driven, AP idiom).
        ap::EgoStatus s;
        s.velocity = ctx.state.vx;
        s.steering = ctx.state.steering_angle;
        swc_.ego_status.Deliver(s);

        // (2) Runtime picks up the offered ControlCommand and translates AP -> canonical.
        sils::VehicleControlSample c;
        ap::ControlCommand cmd;
        if (swc_.control.TakeLast(cmd)) {
            c.acceleration = cmd.acceleration;  // SI, no conversion (AP authored in SI)
            c.steering_angle = cmd.steering;
            c.gear = MapGear(cmd.gear);  // anti-corruption: AP enum -> canonical 0..3
        }
        return c;
    }

    const char* name() const override { return "adaptive-drivecontroller"; }

private:
    // AP GearCommand (Autoware-like) -> canonical (0=P 1=R 2=N 3=D).
    static uint8_t MapGear(int ap_gear) {
        switch (ap_gear) {
            case ap::AP_DRIVE: return 3;
            case ap::AP_REVERSE: return 1;
            case ap::AP_PARK: return 0;
            default: return 2;  // NEUTRAL
        }
    }

    ap::DriveController swc_;
};

#endif  // ADAPTIVE_ECU_BEHAVIOR_HPP
