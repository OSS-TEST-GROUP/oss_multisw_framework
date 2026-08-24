#ifndef VEHICLE_INTERFACE_BEHAVIOR_HPP
#define VEHICLE_INTERFACE_BEHAVIOR_HPP

// VehicleInterfaceBehavior -- a developer-authored IVehicleBehavior implementing the VCU
// (Vehicle Interface) role (libsils_vecu example, Option 2 plant coupling / Profile B).
// docs/sils/sils-vehicle-interface-autoware.md.
//
// Unlike steer/brake (which CONTROL a single axis from state), the VCU sits between
// Autoware and the plant: it consumes the upstream control TARGET (ctx.target, from
// Autoware via the bridge) plus vehicle STATE, and produces the single unified command the
// plant (MORAI) accepts. So the "control logic" here is ARBITRATION + SAFETY, not path
// following (that is Autoware's).
//
//   - have_target : the target is the OWNER of both axes -- pass it through.
//   - no target   : SAFE STOP -- steady deceleration, steering neutral, gear DRIVE.
//   - other vECUs (ctx.requests) may only move the command in the SAFE direction. See below.
//
// ★ ARBITRATION -- AXIS OWNERSHIP + MONOTONE SAFETY OVERRIDE
//   full rules and the reasoning: docs/sils/sils-vecu-arbitration.md
//
//   longitudinal:  out = min(owner, req_1, req_2, …)      stronger braking wins
//   lateral:       out = clamp(owner, ±steer_limit)       limit only, never inject
//   gear:          owner only                             peers cannot change gear
//
// WHY min/clamp AND NOT A BLEND: the result must not depend on ARRIVAL ORDER. min and clamp
// are commutative and associative, so whichever peer's sample landed first, two runs that
// saw the same set produce the same command -- which is what keeps S3 lockstep golden
// reproduction intact. A weighted blend fails that and, worse, cannot be argued about:
// two individually safe commands can average into an unsafe one.
//
// WHAT THIS DELIBERATELY CANNOT DO: a peer cannot accelerate harder than the owner, or
// steer somewhere new. "This ECU knows better, go faster" is structurally impossible here.
// That is the price of the safety argument; changing it needs axis-ownership handover (a
// mode state machine), which is a different and much larger design.
//
// ⚠ THE NUMBERS HERE ARE NOT A SPEC. safe_decel and steer_limit are placeholders -- see §0
// and §7 of the arbitration doc. The rule shape is defensible without a spec (it can only
// fail safe); the VALUES come from vehicle/regulatory/OEM requirements we do not have.
// When a spec arrives it belongs in a declaration, not in this file.

#include <algorithm>
#include <cmath>
#include <string>

#include "vehicle_behavior.hpp"  // sils::IVehicleBehavior / VehicleBehaviorContext / VehicleControlSample

class VehicleInterfaceBehavior : public sils::IVehicleBehavior {
public:
    // steer_limit <= 0 disables the lateral clamp (pass the owner's steering through).
    explicit VehicleInterfaceBehavior(double safe_decel, double steer_limit = 0.0)
        : safe_decel_(safe_decel), steer_limit_(steer_limit) {}

    // What the last OnTick decided, for the run loop to trace. Kept as plain members rather
    // than returned, because IVehicleBehavior's contract is "return the control" and widening
    // it would touch every behavior in the tree for something only the VCU produces.
    struct Decision {
        bool arbitrated = false;       // did any peer request participate this tick
        std::string long_winner;       // channel name, or "owner" / "safe-stop"
        double owner_accel = 0.0;
        double chosen_accel = 0.0;
        bool steer_clamped = false;
    };
    const Decision& last_decision() const { return decision_; }

    sils::VehicleControlSample OnTick(const sils::VehicleBehaviorContext& ctx) override {
        sils::VehicleControlSample c;
        decision_ = Decision{};

        // --- owner: the upstream target, or safe stop when it is absent ------------------
        if (ctx.have_target) {
            c.steering_angle = ctx.target.steering_angle;
            c.acceleration = ctx.target.acceleration;
            c.gear = ctx.target.gear;
            decision_.long_winner = "owner";
        } else {
            // Not a special case: safe stop points the same way the override rule does, so
            // it is the same rule at its extreme. Peers can still brake HARDER than this.
            c.steering_angle = 0.0;
            c.acceleration = -safe_decel_;
            c.gear = 3;
            decision_.long_winner = "safe-stop";
        }
        decision_.owner_accel = c.acceleration;

        // --- peers: safe direction only ---------------------------------------------------
        for (const sils::ControlRequest& r : ctx.requests) {
            decision_.arbitrated = true;
            // NaN would win a naive comparison and silently take over the command. A peer
            // that emits NaN is broken; ignore its sample rather than let it drive.
            if (std::isnan(r.sample.acceleration)) continue;
            if (r.sample.acceleration < c.acceleration) {
                c.acceleration = r.sample.acceleration;
                decision_.long_winner = r.name;
            }
        }
        decision_.chosen_accel = c.acceleration;

        // --- lateral: clamp only ----------------------------------------------------------
        if (steer_limit_ > 0.0) {
            const double s = std::max(-steer_limit_, std::min(steer_limit_, c.steering_angle));
            decision_.steer_clamped = (s != c.steering_angle);
            c.steering_angle = s;
        }
        return c;
    }

    const char* name() const override { return "vehicle-interface"; }

private:
    double safe_decel_;    // m/s^2, magnitude of the safe-stop deceleration
    double steer_limit_;   // rad, 0 = no clamp
    Decision decision_;
};

#endif  // VEHICLE_INTERFACE_BEHAVIOR_HPP
