// FmuBehavior -- an IVehicleBehavior whose control law is supplied by an exported .fmu
// instead of hand-written C++. It is to IVehicleBehavior what FmuPlant (plant.hpp) is to
// IPlant: the FMI co-sim step drives the port. Each tick it writes the ego STATE to the
// FMU's input value references, doStep()s one dt, and reads the FMU's output value
// references back as the VehicleControl. MIMO (several inputs/outputs), unlike the SISO
// FmuPlant -- a controller consumes state fields and emits actuation fields.
//
// This is the seam for Case 3 (AUTOSAR Classic): a Classic ECU's application cannot be
// compiled standalone (it depends on a generated RTE + BSW), so the developer's AUTOSAR
// toolchain / a commercial virtualization tool exports it as a **V-ECU FMU**, which drops
// in here with NO C++ written. It equally accepts a controller FMU from Simulink/TargetLink
// (an alternative to the plain-C path, Case 1). We do NOT generate the V-ECU -- RTE/BSW
// virtualization is the toolchain's job; we provide the FMI socket + canonical binding.
//
// Determinism: FMI co-sim doStep(t, h) is a pure function of its inputs, so an FMU behavior
// is golden-reproducible under lockstep exactly like a C++ behavior -- one tick = one
// doStep. dt is taken from the tick's sim_time advance (the run loop stamps sim_time_ns).

#ifndef SILS_VECU_VEHICLE_FMU_BEHAVIOR_HPP
#define SILS_VECU_VEHICLE_FMU_BEHAVIOR_HPP

#include <memory>
#include <string>

#include "plant.hpp"             // FmuHasBinaryForHost + fmi4c include (both under SILS_HAVE_FMU)
#include "vehicle_behavior.hpp"  // sils::IVehicleBehavior / VehicleBehaviorContext

namespace sils {

// Which FMU value references carry which canonical signals. A negative VR = "not wired":
// an unwired input is simply not written; an unwired output leaves its control field at 0
// (gear defaults to DRIVE). Read from env by the example main (SILS_FMU_VR_*), mirroring
// how the SISO FmuPlant reads SILS_FMU_INPUT_VR / _OUTPUT_VR.
struct FmuBehaviorConfig {
    std::string file;
    // inputs: canonical ego state -> FMU
    int vx_vr = -1;          // longitudinal speed, m/s
    int steering_in_vr = -1; // measured road-wheel angle, rad
    // outputs: FMU -> canonical vehicle control
    int accel_out_vr = -1;   // acceleration command, m/s^2
    int steer_out_vr = -1;   // steering command, rad
    int gear_out_vr = -1;    // gear (real, rounded); <0 = default DRIVE(3)
    int64_t fallback_dt_ns = 10000000;  // dt for the first tick / non-advancing sim time
};

#ifdef SILS_HAVE_FMU

class FmuBehavior : public IVehicleBehavior {
public:
    ~FmuBehavior() override {
        if (i2_) fmi2_freeInstance(i2_);
        if (i3_) fmi3_freeInstance(i3_);
        if (fmu_) fmi4c_freeFmu(fmu_);
    }

    bool Load(const FmuBehaviorConfig& cfg, std::string& error) {
        cfg_ = cfg;
        if (!FmuHasBinaryForHost(cfg.file, error)) return false;  // fail-closed before fmi4c
        fmu_ = fmi4c_loadFmu(cfg.file.c_str(), "sils_vecu_fmu");
        if (!fmu_) { error = "fmi4c_loadFmu failed for " + cfg.file; return false; }
        const fmiVersion_t v = fmi4c_getFmiVersion(fmu_);
        if (v == fmiVersion2) {
            fmi_ver_ = "2.0";
            i2_ = fmi2_instantiate(fmu_, fmi2CoSimulation, nullptr, nullptr, nullptr,
                                   nullptr, nullptr, fmi2False, fmi2False);
            if (!i2_) { error = "fmi2_instantiate failed"; return false; }
            if (fmi2_setupExperiment(i2_, fmi2False, 0.0, 0.0, fmi2False, 0.0) != fmi2OK ||
                fmi2_enterInitializationMode(i2_) != fmi2OK ||
                fmi2_exitInitializationMode(i2_) != fmi2OK) {
                error = "fmi2 initialization failed"; return false;
            }
        } else if (v == fmiVersion3) {
            fmi_ver_ = "3.0";
            i3_ = fmi3_instantiateCoSimulation(fmu_, fmi3False, fmi3False, fmi3False,
                                               fmi3False, nullptr, 0, nullptr, nullptr, nullptr);
            if (!i3_) { error = "fmi3_instantiateCoSimulation failed"; return false; }
            if (fmi3_enterInitializationMode(i3_, fmi3False, 0.0, 0.0, fmi3False, 0.0) != fmi3OK ||
                fmi3_exitInitializationMode(i3_) != fmi3OK) {
                error = "fmi3 initialization failed"; return false;
            }
        } else {
            error = "unsupported FMI version (need 2.0 or 3.0)"; return false;
        }
        return true;
    }

    VehicleControlSample OnTick(const VehicleBehaviorContext& ctx) override {
        // Inputs: canonical state -> FMU value references.
        Set(cfg_.vx_vr, ctx.state.vx);
        Set(cfg_.steering_in_vr, ctx.state.steering_angle);

        // Advance one step. dt from the tick's sim-time advance (deterministic under lockstep).
        int64_t dt_ns = ctx.sim_time_ns - prev_sim_time_ns_;
        if (dt_ns <= 0) dt_ns = cfg_.fallback_dt_ns;
        prev_sim_time_ns_ = ctx.sim_time_ns;
        DoStep(static_cast<double>(dt_ns) / 1e9);

        // Outputs: FMU value references -> canonical control (anti-corruption stays with the
        // FMU's own units; gear rounded + clamped to 0..3).
        VehicleControlSample c;
        c.acceleration = Get(cfg_.accel_out_vr);
        c.steering_angle = Get(cfg_.steer_out_vr);
        if (cfg_.gear_out_vr >= 0) {
            long g = static_cast<long>(Get(cfg_.gear_out_vr) + 0.5);
            c.gear = static_cast<uint8_t>(g < 0 ? 0 : (g > 3 ? 3 : g));
        } else {
            c.gear = 3;  // DRIVE
        }
        return c;
    }

    const char* name() const override { return "fmu-behavior"; }
    const char* fmi_version() const { return fmi_ver_; }

private:
    void Set(int vr, double val) {
        if (vr < 0) return;
        if (i2_) {
            fmi2ValueReference r = static_cast<fmi2ValueReference>(vr);
            fmi2Real v = val;
            fmi2_setReal(i2_, &r, 1, &v);
        } else if (i3_) {
            fmi3ValueReference r = static_cast<fmi3ValueReference>(vr);
            fmi3Float64 v = val;
            fmi3_setFloat64(i3_, &r, 1, &v, 1);
        }
    }
    double Get(int vr) {
        if (vr < 0) return 0.0;
        if (i2_) {
            fmi2ValueReference r = static_cast<fmi2ValueReference>(vr);
            fmi2Real v = 0.0;
            fmi2_getReal(i2_, &r, 1, &v);
            return v;
        }
        if (i3_) {
            fmi3ValueReference r = static_cast<fmi3ValueReference>(vr);
            fmi3Float64 v = 0.0;
            fmi3_getFloat64(i3_, &r, 1, &v, 1);
            return v;
        }
        return 0.0;
    }
    void DoStep(double dt) {
        if (i2_) {
            fmi2_doStep(i2_, time_, dt, fmi2True);
        } else if (i3_) {
            fmi3Boolean ev = fmi3False, term = fmi3False, er = fmi3False;
            fmi3Float64 last = 0.0;
            fmi3_doStep(i3_, time_, dt, fmi3True, &ev, &term, &er, &last);
        }
        time_ += dt;
    }

    FmuBehaviorConfig cfg_;
    fmuHandle* fmu_ = nullptr;
    fmi2InstanceHandle* i2_ = nullptr;
    fmi3InstanceHandle* i3_ = nullptr;
    const char* fmi_ver_ = "";
    double time_ = 0.0;
    int64_t prev_sim_time_ns_ = 0;
};

#endif  // SILS_HAVE_FMU

// Public factory (symmetric with MakeFmuPlant): build an FMU-backed vehicle behavior, or
// nullptr on load/config error (message in `error`) / when built without -DSILS_ENABLE_FMU.
inline std::unique_ptr<IVehicleBehavior> MakeFmuBehavior(
    [[maybe_unused]] const FmuBehaviorConfig& cfg, std::string& error) {
#ifdef SILS_HAVE_FMU
    auto b = std::make_unique<FmuBehavior>();
    if (!b->Load(cfg, error)) return nullptr;
    return b;
#else
    error = "FMU behavior requested but FMU support not compiled in "
            "(rebuild with -DSILS_ENABLE_FMU and fmi4c available)";
    return nullptr;
#endif
}

}  // namespace sils

#endif  // SILS_VECU_VEHICLE_FMU_BEHAVIOR_HPP
