#ifndef BRAKE_ECU_BEHAVIOR_HPP
#define BRAKE_ECU_BEHAVIOR_HPP

// BrakeBehavior -- a developer-authored IBehavior (libsils_vecu example, code path).
//
// This is the "your control logic" half of a developer ECU: it implements the SDK's
// IBehavior port (behavior.hpp) and nothing else. The SDK's RunProbe (run_loop.hpp)
// calls OnTick once per tick, wraps the returned request_json in the VISS envelope,
// publishes it, and traces `value` -- exactly as it does for the built-in behaviors.
//
// Semantics: a proportional brake controller. It reads the vehicle speed fed back from
// the plant this tick (ctx.plant_feedback, §9 L3 closed loop) and commands a brake
// force proportional to how far speed is ABOVE the target, clamped to [0, max_force]
// (a brake only decelerates -- it never produces negative force). Deterministic given
// (kp, setpoint, max_force) and the fixed dt/feedback stream, so the closed loop is
// golden-reproducible like the built-in p-controller.

#include <algorithm>
#include <sstream>
#include <string>

#include "behavior.hpp"  // sils::IBehavior / BehaviorContext / BehaviorOutput (from sils_vecu)

class BrakeBehavior : public sils::IBehavior {
public:
    BrakeBehavior(double kp, double target_speed, double max_force)
        : kp_(kp), target_speed_(target_speed), max_force_(max_force) {}

    sils::BehaviorOutput OnTick(const sils::BehaviorContext& ctx) override {
        const double speed = ctx.plant_feedback;             // current vehicle speed (plant output)
        const double over = speed - target_speed_;           // how far above the target we are
        double force = kp_ * over;                            // proportional brake force
        force = std::clamp(force, 0.0, max_force_);           // brake-only, saturated actuator

        std::ostringstream inner;
        inner << R"({"action":"set","path":"Vehicle.Brake.TargetForce","value":)" << force
              << R"(,"requestId":"sils-)" << ctx.tick_seq << R"("})";
        return {inner.str(), force};  // value = commanded force -> traced + fed into the plant
    }

    const char* name() const override { return "brake-controller"; }

private:
    double kp_;
    double target_speed_;
    double max_force_;
};

#endif  // BRAKE_ECU_BEHAVIOR_HPP
