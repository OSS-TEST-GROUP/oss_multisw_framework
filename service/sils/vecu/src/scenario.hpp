// IScenario -- SiLS vECU fault-injection port (architecture v3 Final §11 L1/L2 negative path).
//
// The fourth hexagonal port (after IClockSource, IBehavior, IPlant). A scenario decides,
// per tick, whether to perturb what the vECU puts on the wire -- promoting negative-path
// testing (dropped / corrupted / duplicated requests) from ad-hoc env flags to a first-
// class port, so the probe body stays fault-agnostic:
//
//   ScenarioAction act = scenario->OnTick({tick_seq, sim_time_ns});
//   switch (act.fault) { Drop -> don't publish; Corrupt -> publish garbage; Duplicate -> publish twice }
//
// DETERMINISM IS THE CONTRACT. Faults are keyed on tick_seq from a fixed schedule -- never
// wall-clock or RNG -- so a run WITH faults is itself golden-reproducible: the same schedule
// injects the same fault at the same tick every run (compare_golden treats scenario_fault as
// a compared event). This mirrors how §11 L1/L2 conformance is proven, and lets the injected
// faults line up with evaluate.py's existing L1 detectors:
//   - Drop      -> a tick with no request_sent            -> analyze_protocol "unanswered"/gap
//   - Corrupt   -> a request the SUT can't parse           -> no correlated response (unanswered)
//   - Duplicate -> the same requestId sent twice           -> analyze_protocol "duplicate"
//
//   - NominalScenario       : injects nothing (default; the run is byte-for-byte unchanged).
//   - ScheduledFaultScenario: a tick-indexed fault schedule parsed from a spec string
//                             ("3:drop,7:corrupt,9:duplicate"), deterministic + reproducible.
//
// Two real implementations -> same earned-abstraction rule as the other ports.

#ifndef SILS_VECU_SCENARIO_HPP
#define SILS_VECU_SCENARIO_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace sils {

// The perturbation applied to a single tick's outbound request. Kept small and closed:
// each fault maps to a negative path evaluate.py already recognizes.
enum class Fault { None, Drop, Corrupt, Duplicate };

inline const char* FaultName(Fault f) {
    switch (f) {
        case Fault::Drop: return "drop";
        case Fault::Corrupt: return "corrupt";
        case Fault::Duplicate: return "duplicate";
        default: return "none";
    }
}

inline Fault ParseFault(const std::string& s) {
    if (s == "drop") return Fault::Drop;
    if (s == "corrupt") return Fault::Corrupt;
    if (s == "duplicate") return Fault::Duplicate;
    return Fault::None;
}

struct ScenarioContext {
    uint64_t tick_seq = 0;
    int64_t sim_time_ns = 0;
};

struct ScenarioAction {
    Fault fault = Fault::None;
};

class IScenario {
public:
    virtual ~IScenario() = default;
    virtual ScenarioAction OnTick(const ScenarioContext& ctx) = 0;
    virtual const char* name() const = 0;
    // How many faults this scenario will inject over its schedule (0 = nominal). Used to
    // fail-fast (§14) when a fault run is configured but would inject nothing.
    virtual std::size_t scheduled_faults() const = 0;
};

// Default: never perturbs. The nominal run stays identical to a probe with no scenario.
class NominalScenario : public IScenario {
public:
    ScenarioAction OnTick(const ScenarioContext&) override { return {}; }
    const char* name() const override { return "nominal"; }
    std::size_t scheduled_faults() const override { return 0; }
};

// Tick-indexed fault schedule. Spec is "<tick>:<fault>[,<tick>:<fault>...]", e.g.
// "3:drop,7:corrupt,9:duplicate". A tick not in the map gets Fault::None. Deterministic:
// the map is fixed at construction, so replays inject identically.
class ScheduledFaultScenario : public IScenario {
public:
    explicit ScheduledFaultScenario(const std::string& spec) { Parse(spec); }

    ScenarioAction OnTick(const ScenarioContext& ctx) override {
        auto it = schedule_.find(ctx.tick_seq);
        return {it == schedule_.end() ? Fault::None : it->second};
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
            std::string tick_s = item.substr(0, colon);
            Fault f = ParseFault(item.substr(colon + 1));
            if (f == Fault::None) continue;  // ignore unknown fault tokens
            try {
                schedule_[static_cast<uint64_t>(std::stoull(tick_s))] = f;
            } catch (...) {
                // skip malformed tick index; construction stays total
            }
        }
    }

    std::map<uint64_t, Fault> schedule_;
};

// SILS_SCENARIO=scheduled + SILS_SCENARIO_FAULTS="3:drop,7:corrupt"; anything else -> nominal.
inline std::unique_ptr<IScenario> MakeScenario(const std::string& kind, const std::string& spec) {
    if (kind == "scheduled") {
        return std::make_unique<ScheduledFaultScenario>(spec);
    }
    return std::make_unique<NominalScenario>();
}

}  // namespace sils

#endif  // SILS_VECU_SCENARIO_HPP
