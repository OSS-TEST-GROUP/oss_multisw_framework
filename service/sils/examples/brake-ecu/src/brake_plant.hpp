#ifndef BRAKE_ECU_PLANT_HPP
#define BRAKE_ECU_PLANT_HPP

// BrakePlant -- a developer-authored IPlant (libsils_vecu example, CODE path).
//
// This is the "physical model" half, written directly in C++ (the alternative is the
// FMU path -- an exported .fmu behind sils::MakeFmuPlant; see the extension-model doc
// §3.C). It implements the SDK's IPlant port (plant.hpp): the SDK stages the brake
// command (SetInputs), integrates one tick (Step), and reads the new speed back to the
// controller (output). RunProbe emits the same plant_step trace as any other plant, so
// evaluate.py/golden treat this ECU identically to a built-in one.
//
// Model: longitudinal vehicle speed. State x = speed (m/s). Command u = brake force.
//   dx/dt = -(gain * u) - (drag * x)     [brake decel + aero/rolling drag]
// Forward-Euler integrated; speed is clamped at >= 0 (a car doesn't roll backwards
// under braking). A first-order, dependency-free live model -- the loop is closed.

#include <algorithm>
#include <cstdint>

#include "plant.hpp"  // sils::IPlant (from sils_vecu)

class BrakePlant : public sils::IPlant {
public:
    BrakePlant(double gain, double drag) : gain_(gain), drag_(drag) {}

    void Reset(double initial) override { x_ = initial; }   // initial speed (SILS_PLANT_INIT)
    void SetInputs(double command) override { u_ = command; }  // this tick's brake force

    double Step(int64_t dt_ns) override {
        const double dt = static_cast<double>(dt_ns) / 1e9;
        x_ += (-(gain_ * u_) - drag_ * x_) * dt;
        x_ = std::max(0.0, x_);  // no reverse motion under braking
        return x_;
    }

    double output() const override { return x_; }
    const char* name() const override { return "brake-vehicle"; }
    const char* mode() const override { return "live"; }

private:
    double gain_;      // brake force -> deceleration gain (1/mass, effectively)
    double drag_;      // speed-proportional drag coefficient
    double x_ = 0.0;   // state / output: vehicle speed
    double u_ = 0.0;   // staged command: brake force
};

#endif  // BRAKE_ECU_PLANT_HPP
