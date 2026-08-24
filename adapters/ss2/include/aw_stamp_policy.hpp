// aw_stamp_policy.hpp -- put a canonical EgoState stamp onto the consumer's epoch.
//
// THE PROBLEM
//
// Canonical EgoState.stamp_ns is SIMULATION time. ss2_ego_mapping.hpp fills it from
// traffic_simulator's EntityStatus.time, which is
//
//     getCurrentScenarioTime() + step_time      (entity_manager.cpp:111)
//         = seconds_since_the_simulator_started_ - seconds_at_the_start_of_the_scenario_
//
// i.e. it counts from ZERO at scenario start. That is the right value to carry: it is the
// sample's own time, it is what a SiLS trace and the determinism gate compare, and
// overwriting it with arrival time would hide plant latency. Nothing here changes it.
//
// Autoware does not read it that way. Its consumers age a message against the node clock,
// so a stamp near zero is a message from 1970 -- 56 years stale. topic_state_monitor turns
// every one of them into an ERROR and autonomous mode never engages. Measured, on both the
// fixture and SS_v2: docs/sils/evidence/ss2-clock-survey-2026-08-11.md §1.
//
// So the conversion belongs HERE, at the boundary where sim time meets a wall-clock
// consumer -- not in ss2_ego_bridge, which would push wall-clock semantics back into the
// canonical contract the vECU and the trace gate depend on.
//
// WHY A LATCHED OFFSET IS ENOUGH (this was measured, not assumed)
//
// §3 of that evidence weighed a latched offset against a full /clock (tick) bridge, and
// could not choose because the drift between SS_v2's clock and the wall was unmeasured. It
// has now been measured over 2022 samples / 101 s (runs/ss2-clock/clock-drift.json):
//
//     drift slope    -6.8e-07 s/s   ->  1.2 ms after 30 minutes
//     jitter floor    2.7 ms        ->  larger than the drift over any run we do
//     dropped /clock  0 of 2022
//
// The drift is two orders of magnitude below the jitter it would have to beat to matter,
// so a constant offset captured once is as good as continuous tracking, and the tick
// bridge can wait for W3/B2-2 (SS_v2 as a barrier participant), where it is needed for a
// different reason -- ownership of time, not accuracy.
//
// WHY IT IS STILL WATCHED
//
// That measurement holds for the configuration it was taken in: realtime_factor 1.0 and
// SS_v2's use_sim_time false, which makes SimulationClock::getCurrentRosTime() return
// now() -- the wall clock (simulation_clock.cpp:41-47). Change either and sim time stops
// advancing at wall rate, the latched offset goes stale, and NOTHING would say so: the
// stamps stay plausible and merely drift out from under Autoware's freshness window.
//
// So the anchor is not just latched, it is checked against every later sample. Deviation
// beyond the tolerance is counted and reported. This detects the real failure -- sim time
// no longer tracking wall time -- rather than the config flag we guessed causes it.
//
// Header-only and free of ROS 2 and DDS so it can be unit-tested with a bare compiler,
// like the other headers in this directory. See test/test_stamp_policy.cpp.

#ifndef SILS_SS2_AW_STAMP_POLICY_HPP
#define SILS_SS2_AW_STAMP_POLICY_HPP

#include <cstdint>
#include <string>

namespace sils::ss2 {

enum class StampPolicy {
    // Publish EgoState.stamp_ns unchanged. Correct when the source already stamps on the
    // consumer's epoch -- real Autoware feeding aw_ego_state_bridge, or any plant on wall
    // time. Also the honest choice for a trace you intend to compare against a golden.
    kSource,
    // Add a constant, captured once, that moves sim time onto the receiver's epoch.
    kEpochAnchored,
};

// Accepts "source" and "epoch-anchored"/"epoch_anchored". Returns false on anything else
// rather than silently defaulting: picking the wrong one of these two is precisely the
// bug this header exists to fix, so a typo must not resolve to a working-looking node.
inline bool parse_stamp_policy(const std::string & text, StampPolicy & out)
{
    if (text == "source") {
        out = StampPolicy::kSource;
        return true;
    }
    if (text == "epoch-anchored" || text == "epoch_anchored") {
        out = StampPolicy::kEpochAnchored;
        return true;
    }
    return false;
}

inline const char * stamp_policy_name(StampPolicy p)
{
    return p == StampPolicy::kSource ? "source" : "epoch-anchored";
}

// Default tolerance: 20 ms.
//
// Sized against the consumer, not from taste. Autoware's topic_state_monitor and the
// planning stack judge staleness in the tens of milliseconds, so an anchor error that
// stays well inside 20 ms cannot change a decision. It is also ~7x the 2.7 ms jitter
// floor measured above, which keeps ordinary scheduling noise from crying wolf.
inline constexpr std::int64_t kDefaultAnchorToleranceNs = 20'000'000;

// ★ WHY THERE IS A WARM-UP, AND WHY IT IS NOT "THE FIRST SAMPLE"
//
// The canonical EgoState topic is TRANSIENT_LOCAL with KEEP_LAST(256). A bridge that
// attaches after the plant is already running therefore receives, before anything live,
// a BURST OF UP TO 256 HISTORICAL SAMPLES -- and the oldest arrives first.
//
// Anchoring on "the first sample" anchors on the oldest one in that backlog. MEASURED on
// a live domain-42 run against sils_ego_source at 20 ms:
//
//     STAMP ANCHOR STALE: max deviation -5110.5 ms      256 x 20 ms = 5120 ms
//
// Not drift -- a constant, and exactly the history depth. Every stamp the bridge published
// was five seconds in the past, permanently, which is precisely the freshness failure this
// header exists to prevent. The watchdog caught it; the first-sample rule caused it.
//
// So the anchor is taken as the MINIMUM of (receiver_now - source_stamp) over a short
// window. A stale sample inflates that difference, so the minimum is the least-delayed
// observation seen -- the newest sample of the burst, or a live one. The window is TIME
// based, not count based: the backlog arrives as a burst in a few milliseconds, so a
// count of N could still land inside it, while any wall-clock window comfortably longer
// than the burst cannot.
//
// During the window the anchor is still moving, so stamps computed from it are not yet
// trustworthy and MUST NOT be published -- latched() is false, and callers drop. Dropping
// the backlog is right on its own terms too: replaying five seconds of history to Autoware
// as current vehicle state would make the ego appear to teleport through its own past.
//
// 500 ms: ~100x the burst delivery time, 10x the 50 ms plant step, and short enough that
// the startup delay is invisible next to Autoware's own multi-second initialisation.
inline constexpr std::int64_t kDefaultAnchorWarmupNs = 500'000'000;

// ★ DRIFT AND DISCONTINUITY ARE DIFFERENT FAULTS AND GET OPPOSITE TREATMENT
//
// The anchor deliberately never re-fits, because a self-correcting anchor would absorb
// drift silently and destroy the only evidence that sim and wall have parted. That rule is
// right for drift. It is WRONG for a plant restart.
//
// MEASURED 2026-08-11: the fixture exhausted its --count and was restarted while the
// bridges stayed up. The new run's simulation clock begins at zero again, so the anchor --
// latched against the previous run ten minutes earlier -- was off by the entire gap:
//
//     STAMP ANCHOR STALE: max deviation 555366.8 ms
//
// The watchdog was right and the anchor was useless. Neither "keep the old anchor" nor
// "quietly follow the drift" is correct here; the samples simply belong to a different run.
//
// The two are cleanly separable, and the separation is not a heuristic:
//
//     drift          the source stamp keeps ADVANCING, the offset slowly changes
//     discontinuity  the source stamp goes BACKWARDS -- a clock that ran to 120 s does not
//                    return to 0 within one run
//
// So: a backwards step larger than the tolerance restarts the warm-up, exactly as if the
// bridge had just attached. Anything smaller is ordinary jitter and is counted as deviation.
//
// WHY THE STAMP AND NOT seq/run_id. Canonical EgoState carries both, and a seq reset is
// equally unambiguous. The stamp is used because it is the field the anchor is ABOUT: a
// writer restart that somehow kept its clock running needs no re-latch, and a clock reset
// needs one even if seq did not move. Deriving the trigger from the quantity being
// corrected keeps the two from disagreeing.
//
// This does NOT reopen self-correction. The anchor still moves only inside a warm-up
// window; a run boundary starts a new one, it does not nudge the old value.

class StampAnchor {
public:
    explicit StampAnchor(std::int64_t tolerance_ns = kDefaultAnchorToleranceNs,
                         std::int64_t warmup_ns = kDefaultAnchorWarmupNs)
    : tolerance_ns_(tolerance_ns), warmup_ns_(warmup_ns)
    {
    }

    // source_stamp_ns: the sample's own (simulation) time.
    // receiver_now_ns:  the consumer's clock at the moment this sample was taken in.
    //
    // Returns the stamp to publish -- but ONLY MEANINGFUL ONCE latched() IS TRUE. Callers
    // must check and drop while it is false; see the warm-up note above.
    //
    // Once latched, every call returns source_stamp + anchor, so the SPACING between
    // samples is exactly the source's -- plant latency and jitter survive the conversion,
    // which is the whole reason not to just stamp with arrival time.
    std::int64_t Apply(std::int64_t source_stamp_ns, std::int64_t receiver_now_ns)
    {
        const std::int64_t observed = receiver_now_ns - source_stamp_ns;

        // Run boundary: the source clock went backwards. Start over -- including the
        // warm-up, because the new run replays its own TRANSIENT_LOCAL backlog.
        if (have_last_source_ && source_stamp_ns < last_source_ns_ - tolerance_ns_) {
            ++run_boundaries_;
            last_run_gap_ns_ = last_source_ns_ - source_stamp_ns;
            latched_ = false;
            warmup_started_ = false;
            warmup_samples_ = 0;
            // The previous run's deviation history says nothing about this one, and leaving
            // it would keep the STALE warning latched forever on a healthy new run.
            last_deviation_ns_ = 0;
            max_abs_deviation_ns_ = 0;
            max_deviation_signed_ns_ = 0;
            exceedances_ = 0;
        }
        last_source_ns_ = source_stamp_ns;
        have_last_source_ = true;

        if (!latched_) {
            if (!warmup_started_) {
                warmup_started_ = true;
                warmup_end_ns_ = receiver_now_ns + warmup_ns_;
                anchor_ns_ = observed;
            } else if (observed < anchor_ns_) {
                // Smaller delay = less stale. Only ever downward, and only in here.
                anchor_ns_ = observed;
            }
            ++warmup_samples_;
            if (receiver_now_ns >= warmup_end_ns_) latched_ = true;
            return source_stamp_ns + anchor_ns_;
        }

        // Signed: the sign says WHICH WAY it went. Sim running fast (stamps arriving
        // "early") and sim stalling are different faults with different fixes, and an
        // absolute value would report them identically.
        last_deviation_ns_ = observed - anchor_ns_;
        const std::int64_t magnitude =
            last_deviation_ns_ < 0 ? -last_deviation_ns_ : last_deviation_ns_;
        if (magnitude > max_abs_deviation_ns_) {
            max_abs_deviation_ns_ = magnitude;
            max_deviation_signed_ns_ = last_deviation_ns_;
        }
        if (magnitude > tolerance_ns_) {
            ++exceedances_;
        }

        return source_stamp_ns + anchor_ns_;
    }

    // False while the warm-up window is still open. Callers MUST NOT publish a stamp
    // produced before this turns true.
    bool latched() const { return latched_; }
    std::int64_t anchor_ns() const { return anchor_ns_; }
    std::int64_t tolerance_ns() const { return tolerance_ns_; }
    std::int64_t warmup_ns() const { return warmup_ns_; }
    // How many samples the warm-up consumed. A number near the writer's history depth is
    // the signature of a TRANSIENT_LOCAL backlog -- i.e. this bridge attached late.
    std::uint64_t warmup_samples() const { return warmup_samples_; }
    // How many times the source clock jumped backwards, i.e. how many plant restarts this
    // bridge has ridden through. Nonzero is not a fault by itself -- it is worth surfacing
    // because a bridge that has outlived several runs is easy to mistake for a fresh one.
    std::uint64_t run_boundaries() const { return run_boundaries_; }
    std::int64_t last_run_gap_ns() const { return last_run_gap_ns_; }
    std::int64_t last_deviation_ns() const { return last_deviation_ns_; }
    // Signed value of the largest-magnitude deviation seen.
    std::int64_t max_deviation_ns() const { return max_deviation_signed_ns_; }
    std::uint64_t exceedances() const { return exceedances_; }

private:
    std::int64_t tolerance_ns_;
    std::int64_t warmup_ns_;
    bool warmup_started_ = false;
    std::int64_t warmup_end_ns_ = 0;
    std::uint64_t warmup_samples_ = 0;
    bool latched_ = false;
    std::int64_t anchor_ns_ = 0;
    std::int64_t last_deviation_ns_ = 0;
    std::int64_t max_abs_deviation_ns_ = 0;
    std::int64_t max_deviation_signed_ns_ = 0;
    std::uint64_t exceedances_ = 0;
    bool have_last_source_ = false;
    std::int64_t last_source_ns_ = 0;
    std::uint64_t run_boundaries_ = 0;
    std::int64_t last_run_gap_ns_ = 0;
};

}  // namespace sils::ss2

#endif  // SILS_SS2_AW_STAMP_POLICY_HPP
