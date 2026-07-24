// IInputFault -- inbound fault-injection port for the MORAI/AWSIM plant-coupling path
// (Option 2). The bus-coupled counterpart of IScenario (scenario.hpp), which perturbs the
// OUTBOUND request on the VISS path. Here we perturb the INBOUND ego STATE the vECU consumes,
// at the staging layer (between decode and the behavior), to test controller DEGRADATION --
// the half of SiLS value that "does the logic run" misses: does the controller stay SAFE when
// its sensor feed drops, goes NaN, or saturates?
//
// Faults map to robustness checks evaluate.py's analyze_fault verifies on the resulting
// control:
//   - Drop       : no fresh state this tick (sensor dropout) -> loop holds the previous value
//                  (ZOH). Check: control stays finite/bounded through the outage.
//   - Nan        : corrupt the state to NaN (bad sensor frame). Check: control does NOT
//                  propagate NaN (the controller must clamp/hold/safe -- §14 fail-fast else).
//   - OutOfRange : drive a state field far past physical range. Check: control stays bounded
//                  (the controller clamps / doesn't wind up).
//
// DETERMINISM IS THE CONTRACT (same as IScenario): faults are keyed on tick_seq from a fixed
// schedule -- never wall-clock or RNG -- so a fault run is itself golden-reproducible. Spec is
// "<tick>[-<tickEnd>]:<mode>[,...]", e.g. "10:drop,20-25:nan,40:oor". A tick outside every
// range is Fault::None. DDS-free (operates on the POD the behavior consumes) so it unit-tests
// without Fast-DDS.

#ifndef SILS_VECU_VEHICLE_INPUT_FAULT_HPP
#define SILS_VECU_VEHICLE_INPUT_FAULT_HPP

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "vehicle_sample.hpp"  // EgoStateSample

namespace sils {

enum class InputFault { None, Drop, Nan, OutOfRange };

inline const char* InputFaultName(InputFault f) {
    switch (f) {
        case InputFault::Drop: return "drop";
        case InputFault::Nan: return "nan";
        case InputFault::OutOfRange: return "oor";
        default: return "none";
    }
}

inline InputFault ParseInputFault(const std::string& s) {
    if (s == "drop") return InputFault::Drop;
    if (s == "nan") return InputFault::Nan;
    if (s == "oor" || s == "outofrange") return InputFault::OutOfRange;
    return InputFault::None;
}

class IInputFault {
public:
    virtual ~IInputFault() = default;
    // The fault (if any) to apply to this tick's inbound state.
    virtual InputFault OnTick(uint64_t tick_seq) = 0;
    virtual const char* name() const = 0;
    // Number of ticks the schedule perturbs (0 = nominal). Fail-fast (§14) when a fault run
    // is configured but would inject nothing.
    virtual std::size_t scheduled_faults() const = 0;
};

// Default: never perturbs -- the run is identical to a probe with no fault port.
class NominalInputFault : public IInputFault {
public:
    InputFault OnTick(uint64_t) override { return InputFault::None; }
    const char* name() const override { return "nominal"; }
    std::size_t scheduled_faults() const override { return 0; }
};

// Tick-indexed schedule parsed from a spec string. Ranges expand to per-tick entries so
// OnTick is a single map lookup. Deterministic: fixed at construction.
class ScheduledInputFault : public IInputFault {
public:
    explicit ScheduledInputFault(const std::string& spec) { Parse(spec); }

    InputFault OnTick(uint64_t tick_seq) override {
        auto it = schedule_.find(tick_seq);
        return it == schedule_.end() ? InputFault::None : it->second;
    }
    const char* name() const override { return "scheduled"; }
    std::size_t scheduled_faults() const override { return schedule_.size(); }

private:
    void Parse(const std::string& spec) {
        std::istringstream ss(spec);
        std::string item;
        while (std::getline(ss, item, ',')) {
            auto colon = item.find(':');
            if (colon == std::string::npos) continue;
            const std::string ticks = item.substr(0, colon);
            const InputFault f = ParseInputFault(item.substr(colon + 1));
            if (f == InputFault::None) continue;  // ignore unknown fault tokens
            uint64_t lo = 0, hi = 0;
            const auto dash = ticks.find('-');
            try {
                if (dash == std::string::npos) {
                    lo = hi = static_cast<uint64_t>(std::stoull(ticks));
                } else {
                    lo = static_cast<uint64_t>(std::stoull(ticks.substr(0, dash)));
                    hi = static_cast<uint64_t>(std::stoull(ticks.substr(dash + 1)));
                }
            } catch (...) {
                continue;  // skip malformed tick index; construction stays total
            }
            if (hi < lo) continue;
            for (uint64_t t = lo; t <= hi; ++t) schedule_[t] = f;
        }
    }

    std::map<uint64_t, InputFault> schedule_;
};

// SILS_INPUT_FAULTS non-empty -> scheduled; empty -> nominal.
inline std::unique_ptr<IInputFault> MakeInputFault(const std::string& spec) {
    if (!spec.empty()) return std::make_unique<ScheduledInputFault>(spec);
    return std::make_unique<NominalInputFault>();
}

// Apply a fault to the ego state the behavior is about to consume. `have_fresh` reflects
// whether a new sample arrived this tick; Drop clears it (the loop then holds the previous
// value = sensor dropout / ZOH). Nan/OutOfRange corrupt the state in place. Returns true if
// a fault was applied (for tracing). Kept as a free function so the port stays data-only.
inline bool ApplyInputFault(InputFault f, EgoStateSample& state, bool& have_fresh) {
    switch (f) {
        case InputFault::Drop:
            have_fresh = false;  // no fresh update this tick -> loop holds previous state
            return true;
        case InputFault::Nan: {
            const double nan = std::numeric_limits<double>::quiet_NaN();
            state.vx = state.vy = state.vz = nan;
            state.ax = state.ay = state.az = nan;
            state.steering_angle = nan;
            return true;
        }
        case InputFault::OutOfRange:
            state.vx = 1e6;              // wildly past any physical speed
            state.steering_angle = 1e3;  // rad, far past lock-to-lock
            return true;
        default:
            return false;
    }
}

}  // namespace sils

#endif  // SILS_VECU_VEHICLE_INPUT_FAULT_HPP
