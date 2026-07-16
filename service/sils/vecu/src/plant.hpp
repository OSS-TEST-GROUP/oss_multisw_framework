// IPlant -- SiLS vECU plant-coupling port (architecture v3 Final §11 L3, IPlant port §6).
//
// The third hexagonal port (after IClockSource and IBehavior). A plant is the
// physical/external model the vECU controls in CLOSED LOOP: each tick the
// controller (an IBehavior) reads the plant's previous output, computes a command,
// and the plant integrates one dt to a new output that feeds back next tick. That
// live feedback is what separates L3 ("plant-coupled fidelity, FMU-based closed
// loop") from L2's open-loop controlled inputs (§7.4 "no live feedback").
//
// Port shape follows the doc: set_inputs(...) + step(dt) -> outputs. Two real
// implementations earn the abstraction (same rule as IClockSource/IBehavior):
//
//   - AnalyticPlant : a built-in first-order model x += (u - a*x)*dt. Responds to
//                     commands, so the loop is GENUINELY closed (mode="live"). It's
//                     the stand-in for an FMU behind the same port -- an FMI 2.0/3.0
//                     co-sim adapter drops in later without touching the tick loop.
//   - ReplayPlant   : plays back a recorded output-per-tick file (mode="replay").
//                     Deterministic and reproducible, but NOT plant-coupled (it
//                     ignores the command) -- the doc's "Replay" IPlant row, and the
//                     analogue of §7.4 input replay: a golden target, not L3-live.
//
// L3 conformance (§11) is judged from the trace by evaluate.py analyze_plant: a live
// plant with closed-loop feedback continuity (feedback_in[N] == output[N-1]) and a
// deterministic trajectory (constant dt, finite outputs). Model correlation against
// a reference trajectory is compare_golden (plant_step is a compared event).
//
// FMU step failure / NaN is a fail-fast condition (§14): a live model that produces
// a non-finite output must surface, which the trace contract flags downstream.

#ifndef SILS_VECU_PLANT_HPP
#define SILS_VECU_PLANT_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef SILS_HAVE_FMU
#include <cmath>
#include <limits>
#include <fmi4c.h>  // FMI 1/2/3 loader (build with -DSILS_ENABLE_FMU); FmuPlant only
#endif

namespace sils {

// Config for the FMU-backed plant (SILS_PLANT=fmu). value references address the
// FMU's input (command) and output variables per its modelDescription.xml.
struct FmuConfig {
    std::string file;      // path to the .fmu
    int input_vr = -1;     // command input value reference; <0 = FMU takes no command
    int output_vr = 0;     // output value reference read back as the plant output
};

class IPlant {
public:
    virtual ~IPlant() = default;
    // Reset internal state to `initial` (the tick-0 output the controller first reads).
    virtual void Reset(double initial) = 0;
    // Stage this tick's command (the controller's actuation) before Step.
    virtual void SetInputs(double command) = 0;
    // Integrate one step of dt and return the new output; also becomes output().
    virtual double Step(int64_t dt_ns) = 0;
    // Current output -- what the controller reads back as feedback next tick.
    virtual double output() const = 0;
    virtual const char* name() const = 0;
    virtual const char* mode() const = 0;  // "live" | "replay"
    // FMI version string for the trace (e.g. "2.0"/"3.0"); nullptr for non-FMU plants.
    virtual const char* fmi_version() const { return nullptr; }
};

// First-order lag: x' = u - a*x, forward-Euler x += (u - a*x)*dt. A real, command-
// responsive model (no external library) -- the loop is closed and live. Converges
// to u/a; with a P-controller u = Kp*(sp - x) it settles at Kp*sp/(a+Kp).
class AnalyticPlant : public IPlant {
public:
    explicit AnalyticPlant(double drag) : a_(drag) {}
    void Reset(double initial) override { x_ = initial; }
    void SetInputs(double command) override { u_ = command; }
    double Step(int64_t dt_ns) override {
        double dt = static_cast<double>(dt_ns) / 1e9;
        x_ += (u_ - a_ * x_) * dt;
        return x_;
    }
    double output() const override { return x_; }
    const char* name() const override { return "first-order"; }
    const char* mode() const override { return "live"; }

private:
    double a_;         // drag coefficient
    double x_ = 0.0;   // state / output
    double u_ = 0.0;   // staged command
};

// Playback plant: returns a pre-recorded output per Step, in file order, ignoring
// the command. File format matches ReplaySource -- '#' comments, blank lines
// ignored, whitespace-separated "<tick_seq> <output>" (tick_seq is documentary; the
// values are consumed in order). Deterministic golden target, not a live loop.
class ReplayPlant : public IPlant {
public:
    bool Load(const std::string& path, std::string& error) {
        std::ifstream f(path);
        if (!f.is_open()) { error = "cannot open plant replay file: " + path; return false; }
        std::string line;
        int lineno = 0;
        while (std::getline(f, line)) {
            ++lineno;
            std::string trimmed = line.substr(0, line.find('#'));
            std::istringstream ls(trimmed);
            uint64_t tick;
            double out;
            if (!(ls >> tick)) continue;  // blank / comment-only
            if (!(ls >> out)) {
                error = "plant replay " + path + ":" + std::to_string(lineno) +
                        ": expected 'tick_seq output'";
                return false;
            }
            outs_.push_back(out);
        }
        return true;
    }
    void Reset(double initial) override { x_ = initial; i_ = 0; }
    void SetInputs(double /*command*/) override {}  // playback ignores the command
    double Step(int64_t /*dt_ns*/) override {
        if (i_ < outs_.size()) x_ = outs_[i_++];
        return x_;
    }
    double output() const override { return x_; }
    const char* name() const override { return "replay"; }
    const char* mode() const override { return "replay"; }

private:
    std::vector<double> outs_;
    std::size_t i_ = 0;
    double x_ = 0.0;
};

#ifdef SILS_HAVE_FMU
// FMU-backed plant (FMI 2.0/3.0 co-simulation, §11 L3, third IPlant implementation).
// The port maps 1:1 onto FMI co-sim: SetInputs->fmiX_setReal/setFloat64,
// Step->fmiX_doStep, output->fmiX_getReal/getFloat64. So a real, validated physics
// FMU (from Simulink/Dymola/AMESim, or a Reference-FMU) drops in behind IPlant with
// NO change to the tick loop / evaluate / golden -- it emits the same plant_step
// contract as AnalyticPlant. A doStep that fails or yields non-finite output surfaces
// as NaN, which analyze_plant fail-fasts (§14). fmi4c provides FMI 1/2/3 loading.
class FmuPlant : public IPlant {
public:
    ~FmuPlant() override {
        if (i2_) fmi2_freeInstance(i2_);
        if (i3_) fmi3_freeInstance(i3_);
        if (fmu_) fmi4c_freeFmu(fmu_);
    }

    bool Load(const FmuConfig& cfg, std::string& error) {
        input_vr_ = cfg.input_vr;
        output_vr_ = cfg.output_vr;
        has_input_ = cfg.input_vr >= 0;
        fmu_ = fmi4c_loadFmu(cfg.file.c_str(), "sils_fmu");
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
            x_ = ReadOutput();
        } else if (v == fmiVersion3) {
            fmi_ver_ = "3.0";
            i3_ = fmi3_instantiateCoSimulation(fmu_, fmi3False, fmi3False, fmi3False,
                                               fmi3False, nullptr, 0, nullptr, nullptr, nullptr);
            if (!i3_) { error = "fmi3_instantiateCoSimulation failed"; return false; }
            if (fmi3_enterInitializationMode(i3_, fmi3False, 0.0, 0.0, fmi3False, 0.0) != fmi3OK ||
                fmi3_exitInitializationMode(i3_) != fmi3OK) {
                error = "fmi3 initialization failed"; return false;
            }
            x_ = ReadOutput();
        } else {
            error = "unsupported FMI version (need 2.0 or 3.0)"; return false;
        }
        return true;
    }

    void Reset(double /*initial*/) override {}  // FMU init done in Load (start from modelDescription)
    void SetInputs(double command) override {
        if (!has_input_) return;
        if (i2_) {
            fmi2ValueReference vr = static_cast<fmi2ValueReference>(input_vr_);
            fmi2Real val = command;
            fmi2_setReal(i2_, &vr, 1, &val);
        } else if (i3_) {
            fmi3ValueReference vr = static_cast<fmi3ValueReference>(input_vr_);
            fmi3Float64 val = command;
            fmi3_setFloat64(i3_, &vr, 1, &val, 1);
        }
    }
    double Step(int64_t dt_ns) override {
        const double dt = static_cast<double>(dt_ns) / 1e9;
        if (i2_) {
            if (fmi2_doStep(i2_, time_, dt, fmi2True) != fmi2OK) return Fail();
        } else if (i3_) {
            fmi3Boolean ev = fmi3False, term = fmi3False, er = fmi3False;
            fmi3Float64 last = 0.0;
            if (fmi3_doStep(i3_, time_, dt, fmi3True, &ev, &term, &er, &last) != fmi3OK) return Fail();
        } else {
            return Fail();
        }
        time_ += dt;
        x_ = ReadOutput();
        return x_;
    }
    double output() const override { return x_; }
    const char* name() const override { return "fmu"; }
    const char* mode() const override { return "live"; }
    const char* fmi_version() const override { return fmi_ver_; }

private:
    double Fail() { x_ = std::numeric_limits<double>::quiet_NaN(); return x_; }  // §14 fail-fast
    double ReadOutput() {
        if (i2_) {
            fmi2ValueReference vr = static_cast<fmi2ValueReference>(output_vr_);
            fmi2Real val = 0.0;
            fmi2_getReal(i2_, &vr, 1, &val);
            return val;
        }
        if (i3_) {
            fmi3ValueReference vr = static_cast<fmi3ValueReference>(output_vr_);
            fmi3Float64 val = 0.0;
            fmi3_getFloat64(i3_, &vr, 1, &val, 1);
            return val;
        }
        return x_;
    }

    fmuHandle* fmu_ = nullptr;
    fmi2InstanceHandle* i2_ = nullptr;
    fmi3InstanceHandle* i3_ = nullptr;
    const char* fmi_ver_ = "";
    double time_ = 0.0;
    double x_ = 0.0;
    int input_vr_ = -1;
    int output_vr_ = 0;
    bool has_input_ = false;
};
#endif  // SILS_HAVE_FMU

// Public convenience for the FMU plant path (extension-model §3.C). A developer ECU
// that ships an exported .fmu writes `auto p = sils::MakeFmuPlant(cfg.fmu, err)` and
// gets an IPlant with no IPlant subclass to author -- the symmetric counterpart of the
// code path's `std::make_unique<MyPlant>(...)`. Returns nullptr on load/config error
// (message in `error`), or when built without -DSILS_ENABLE_FMU. This builds the exact
// same FmuPlant that MakePlant("fmu", ...) does; MakePlant delegates here.
inline std::unique_ptr<IPlant> MakeFmuPlant([[maybe_unused]] const FmuConfig& fmu,
                                            std::string& error) {
#ifdef SILS_HAVE_FMU
    auto p = std::make_unique<FmuPlant>();
    if (!p->Load(fmu, error)) return nullptr;
    return p;
#else
    error = "FMU plant requested but FMU support not compiled in "
            "(rebuild with -DSILS_ENABLE_FMU and fmi4c available)";
    return nullptr;
#endif
}

// Factory. kind "first-order" (live, drag=a), "replay" (needs replay_file), or "fmu"
// (needs fmu config; requires -DSILS_ENABLE_FMU / fmi4c). Any other value (including
// "none") returns nullptr -> plant coupling off (open loop).
inline std::unique_ptr<IPlant> MakePlant(
    const std::string& kind, double drag, const std::string& replay_file,
    const FmuConfig& fmu, std::string& error) {
    if (kind == "first-order") {
        return std::make_unique<AnalyticPlant>(drag);
    }
    if (kind == "replay") {
        auto p = std::make_unique<ReplayPlant>();
        if (!p->Load(replay_file, error)) return nullptr;
        return p;
    }
    if (kind == "fmu") {
        return MakeFmuPlant(fmu, error);
    }
    return nullptr;
}

}  // namespace sils

#endif  // SILS_VECU_PLANT_HPP
