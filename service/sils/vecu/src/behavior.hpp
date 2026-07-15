// IBehavior -- SiLS vECU per-tick behavior port (architecture v3 Final §11 L2).
//
// The second hexagonal port (after IClockSource). A behavior computes what the
// vECU emits each tick, optionally from internal state and this tick's frozen
// input set (§7.3), so the probe body stays behavior-agnostic:
//
//   BehaviorOutput out = behavior->OnTick({tick_seq, sim_time_ns, staged_count});
//   <wrap out.request_json in the VISS envelope and publish>;  trace out.value
//
//   - VissRequestBehavior  : the Phase 1 behavior -- a stateless VISS get-request
//                            (preserves the exact wire payload).
//   - AccumulatorBehavior  : a stateful L2 example -- accumulates the count of
//                            staged inputs and emits the running total, a small
//                            state machine that consumes §7.3 staged input.
//
// L2 conformance (§11) is verified by GOLDEN SCENARIO: the same tick-indexed input
// must reproduce the same per-tick `value` sequence. compare_golden.py checks that
// (behavior_output is a compared event); this header just has to be deterministic
// given (tick_seq, staged_count). Introduced now because there are two real
// implementations -- same earned-abstraction rule as IClockSource.

#ifndef SILS_VECU_BEHAVIOR_HPP
#define SILS_VECU_BEHAVIOR_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

namespace sils {

struct BehaviorContext {
    uint64_t tick_seq = 0;
    int64_t sim_time_ns = 0;
    std::size_t staged_input_count = 0;  // this tick's frozen input set size (§7.3)
    double plant_feedback = 0.0;         // plant output read back this tick (§9 L3 closed loop)
};

struct BehaviorOutput {
    std::string request_json;  // inner VISS request; the probe wraps it in the envelope
    double value = 0.0;        // scalar output for trace + golden comparison (§11 L2)
};

class IBehavior {
public:
    virtual ~IBehavior() = default;
    virtual BehaviorOutput OnTick(const BehaviorContext& ctx) = 0;
    virtual const char* name() const = 0;
};

// Phase 1 behavior: a VISS get-request. Stateless; value = tick_seq (identical wire
// payload to the original probe so the default run is unchanged).
class VissRequestBehavior : public IBehavior {
public:
    VissRequestBehavior(std::string action, std::string path)
        : action_(std::move(action)), path_(std::move(path)) {}

    BehaviorOutput OnTick(const BehaviorContext& ctx) override {
        std::ostringstream inner;
        inner << R"({"action":")" << action_ << R"(","path":")" << path_
              << R"(","requestId":"sils-)" << ctx.tick_seq << R"("})";
        return {inner.str(), static_cast<double>(ctx.tick_seq)};
    }
    const char* name() const override { return "viss-request"; }

private:
    std::string action_;
    std::string path_;
};

// L2 example: a stateful accumulator that consumes staged input (§7.3). State is the
// running total of staged inputs; each tick adds this tick's count and emits it.
// Deterministic given the tick-indexed staged-count stream -> golden-reproducible.
class AccumulatorBehavior : public IBehavior {
public:
    BehaviorOutput OnTick(const BehaviorContext& ctx) override {
        acc_ += static_cast<double>(ctx.staged_input_count);
        std::ostringstream inner;
        inner << R"({"action":"set","path":"Vehicle.InputCount","value":)" << acc_
              << R"(,"requestId":"sils-)" << ctx.tick_seq << R"("})";
        return {inner.str(), acc_};
    }
    const char* name() const override { return "accumulate"; }

private:
    double acc_ = 0.0;
};

// L3 controller: a proportional controller closing the loop around an IPlant. It
// reads the plant's fed-back output (ctx.plant_feedback) and emits the actuation
// command u = Kp*(setpoint - feedback). value = command, so behavior_output carries
// the command and the probe feeds that same command into the plant -- one number,
// no divergence between what's published and what's actuated. Deterministic given
// (Kp, setpoint) and the fixed dt/feedback stream -> the closed loop is golden-
// reproducible (§9 model correlation).
class PControllerBehavior : public IBehavior {
public:
    PControllerBehavior(double kp, double setpoint) : kp_(kp), setpoint_(setpoint) {}
    BehaviorOutput OnTick(const BehaviorContext& ctx) override {
        double command = kp_ * (setpoint_ - ctx.plant_feedback);
        std::ostringstream inner;
        inner << R"({"action":"set","path":"Vehicle.Actuation","value":)" << command
              << R"(,"requestId":"sils-)" << ctx.tick_seq << R"("})";
        return {inner.str(), command};
    }
    const char* name() const override { return "p-controller"; }

private:
    double kp_;
    double setpoint_;
};

inline std::unique_ptr<IBehavior> MakeBehavior(
    const std::string& kind, const std::string& action, const std::string& path) {
    if (kind == "accumulate") {
        return std::make_unique<AccumulatorBehavior>();
    }
    return std::make_unique<VissRequestBehavior>(action, path);
}

// Overload carrying controller gains (SILS_CTRL_KP / SILS_CTRL_SETPOINT) for the
// L3 "p-controller" behavior; other kinds ignore them.
inline std::unique_ptr<IBehavior> MakeBehavior(
    const std::string& kind, const std::string& action, const std::string& path,
    double kp, double setpoint) {
    if (kind == "p-controller") {
        return std::make_unique<PControllerBehavior>(kp, setpoint);
    }
    return MakeBehavior(kind, action, path);
}

}  // namespace sils

#endif  // SILS_VECU_BEHAVIOR_HPP
