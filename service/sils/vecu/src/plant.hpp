// IPlant -- SiLS vECU plant-coupling port (architecture v3 Final §9 L3, §7 IPlant).
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
// L3 conformance (§9) is judged from the trace by evaluate.py analyze_plant: a live
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

namespace sils {

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

// Factory. kind "first-order" (live, drag=a) or "replay" (needs replay_file). Any
// other value (including "none") returns nullptr -> plant coupling off (open loop).
inline std::unique_ptr<IPlant> MakePlant(
    const std::string& kind, double drag, const std::string& replay_file, std::string& error) {
    if (kind == "first-order") {
        return std::make_unique<AnalyticPlant>(drag);
    }
    if (kind == "replay") {
        auto p = std::make_unique<ReplayPlant>();
        if (!p->Load(replay_file, error)) return nullptr;
        return p;
    }
    return nullptr;
}

}  // namespace sils

#endif  // SILS_VECU_PLANT_HPP
