#ifndef ADAPTIVE_ECU_SWC_HPP
#define ADAPTIVE_ECU_SWC_HPP

// adaptive_swc.hpp -- the DEVELOPER'S AUTOSAR Adaptive software component. This is the code
// an AP team actually authors: it talks only to `ara::com` events (here the shim; in
// production the vendor runtime) and knows nothing about libsils_vecu, DDS topics, or the
// canonical EgoState/VehicleControl types. Port it to SiLS by keeping THIS file unchanged
// and swapping the shim for the real AP runtime.
//
// The component subscribes to a VehicleStatus service event and offers a ControlCommand
// service event -- a service-oriented control loop, the AP idiom. Datatypes below mirror
// what an ARXML ServiceInterface would generate; the AP gear enum uses Autoware-like values
// ON PURPOSE so the canonical mapping (done in the adapter, not here) is visibly needed.

#include "ara_com_shim.hpp"

namespace ap {

// AP service datatypes (as generated from an ARXML ServiceInterface). SI units.
struct EgoStatus {
    double velocity = 0.0;  // m/s
    double steering = 0.0;  // rad
};

enum ApGear { AP_NEUTRAL = 1, AP_DRIVE = 2, AP_REVERSE = 20, AP_PARK = 22 };  // Autoware-like

struct ControlCommand {
    double acceleration = 0.0;  // m/s^2
    double steering = 0.0;      // rad
    int gear = AP_NEUTRAL;      // AP enum -- adapter maps to canonical 0..3
};

// The developer's component. Reacts to each delivered status sample by computing a command
// and Send()ing it -- exactly as it would against a real AP runtime.
class DriveController {
public:
    DriveController() {
        ego_status.SetReceiveHandler([this](const EgoStatus& s) { OnEgoStatus(s); });
    }

    // ara::com events (Proxy in / Skeleton out). The runtime (real AP, or the adapter's
    // shim here) drives ego_status and drains control.
    ara::com::ProxyEvent<EgoStatus> ego_status;
    ara::com::SkeletonEvent<ControlCommand> control;

    void set_target_speed(double v) { v_set_ = v; }
    void set_kp(double k) { kp_ = k; }

private:
    void OnEgoStatus(const EgoStatus& s) {
        ControlCommand cmd;
        cmd.acceleration = kp_ * (v_set_ - s.velocity);      // hold target speed
        cmd.steering = kp_steer_ * (steer_set_ - s.steering);  // hold target wheel angle
        cmd.gear = AP_DRIVE;
        control.Send(cmd);
    }

    double kp_ = 0.8;
    double v_set_ = 10.0;
    double kp_steer_ = 0.6;
    double steer_set_ = 0.1;
};

}  // namespace ap

#endif  // ADAPTIVE_ECU_SWC_HPP
