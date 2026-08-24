// Unit tests for aw_stamp_policy.hpp -- moving a simulation-time stamp onto the
// consumer's epoch, and noticing when that stops being valid.
//
// Dependency-free (no gtest, no ROS, no DDS) so it runs anywhere:
//     c++ -std=c++20 -I../include test_stamp_policy.cpp -o t && ./t
// or via adapters/ss2/run_tests.sh.
//
// WHAT IS ACTUALLY AT RISK HERE
//
// Two properties, and both fail silently in the real system:
//
//   1. SPACING MUST SURVIVE. The reason not to just stamp with arrival time is that it
//      erases plant latency and jitter. A conversion that quietly re-times the samples
//      would do the same damage while looking correct -- so the tests assert that output
//      deltas equal input deltas exactly, including when the receiver's clock jitters.
//
//   1b. THE ANCHOR MUST NOT COME FROM THE BACKLOG. EgoState is TRANSIENT_LOCAL, so a
//      late-attaching bridge is handed the writer's history oldest-first. Anchoring on
//      "the first sample" put every published stamp 5.11 s in the past on a live rig.
//      test_transient_local_backlog_does_not_poison_the_anchor is that run, in miniature.
//
//   2. THE WATCHDOG MUST FIRE. The latched offset is valid only while sim time advances
//      at wall rate. If it stops (realtime_factor != 1, use_sim_time flipped, a stalled
//      simulator) the stamps stay plausible and merely drift out of Autoware's freshness
//      window. Nothing else in the pipeline would report it. So the deviation counter is
//      not diagnostics, it is the detector, and it is tested as one.
//
// The numbers below use the measured configuration: 50 ms step (20 Hz), and the 2.7 ms
// jitter floor from runs/ss2-clock/clock-drift.json.

#include "aw_stamp_policy.hpp"

#include <cstdint>
#include <cstdio>
#include <string>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string & what)
{
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::printf("FAIL  %s\n", what.c_str());
    }
}

void check_eq(std::int64_t got, std::int64_t want, const std::string & what)
{
    ++g_checks;
    if (got != want) {
        ++g_failures;
        std::printf("FAIL  %s: got %lld want %lld\n", what.c_str(),
                    static_cast<long long>(got), static_cast<long long>(want));
    }
}

using sils::ss2::kDefaultAnchorToleranceNs;
using sils::ss2::parse_stamp_policy;
using sils::ss2::StampAnchor;
using sils::ss2::StampPolicy;

constexpr std::int64_t kMs = 1'000'000;
constexpr std::int64_t kSec = 1'000'000'000;
constexpr std::int64_t kStep = 50 * kMs;  // 20 Hz, the rate every SiLS run is taken at

// A plausible wall clock: 2026-08-11-ish in ns. The point is only that it is ~56 years
// from zero, which is exactly the gap that makes Autoware reject a sim-time stamp.
constexpr std::int64_t kEpochNow = 1'786'418'000LL * kSec;
constexpr std::int64_t kWarmup = 500 * kMs;

// Drive a clean warm-up: samples in lockstep with the receiver clock, so the minimum the
// anchor picks is the true offset. Returns the receiver time just past the window.
// Every post-warm-up test starts here, because latched() is false until it has run.
std::int64_t warm_up(StampAnchor & a, std::int64_t first_source, std::int64_t first_now)
{
    std::int64_t src = first_source;
    std::int64_t now = first_now;
    // One step past the window: the sample that CROSSES the boundary is what latches.
    while (now <= first_now + kWarmup) {
        a.Apply(src, now);
        src += kStep;
        now += kStep;
    }
    return now;
}

// --- the conversion ------------------------------------------------------------

void test_not_latched_during_warmup()
{
    StampAnchor anchor;
    check(!anchor.latched(), "not latched before any sample");
    anchor.Apply(0, kEpochNow);
    check(!anchor.latched(), "STILL not latched after the first sample");
    anchor.Apply(kStep, kEpochNow + kStep);
    check(!anchor.latched(), "still not latched a step later");

    warm_up(anchor, 0, kEpochNow);
    check(anchor.latched(), "latched once the window has passed");
}

void test_clean_warmup_gives_the_true_offset()
{
    // Scenario time starts at zero; that IS the bug the anchor corrects.
    StampAnchor anchor;
    const std::int64_t now = warm_up(anchor, 0, kEpochNow);
    check_eq(anchor.anchor_ns(), kEpochNow, "anchor is receiver_now - source_stamp");
    const std::int64_t out = anchor.Apply(now - kEpochNow, now);
    check_eq(out, now, "a live sample maps onto the receiver's now");
}

void test_mid_scenario_attach()
{
    // The bridge may attach mid-scenario; the anchor must discount the elapsed sim time.
    StampAnchor anchor;
    warm_up(anchor, 56 * kSec, kEpochNow);
    check_eq(anchor.anchor_ns(), kEpochNow - 56 * kSec, "anchor discounts elapsed sim time");
}

// ★ THE REGRESSION. A TRANSIENT_LOCAL backlog is delivered oldest-first as a burst, so
// "anchor on the first sample" anchors on the STALEST one. Measured on a live rig: every
// published stamp sat 5.11 s in the past, forever. This is that scenario in miniature.
void test_transient_local_backlog_does_not_poison_the_anchor()
{
    StampAnchor anchor;

    // 256 historical samples (the writer's KEEP_LAST depth), spanning 5.12 s of source
    // time, all delivered within 5 ms of receiver time. The oldest is 5.12 s stale.
    const std::int64_t burst_now = kEpochNow;
    const std::int64_t newest_source = 5120 * kMs;
    for (int i = 0; i < 256; ++i) {
        const std::int64_t source = newest_source - (255 - i) * kStep;
        anchor.Apply(source, burst_now + i * 20'000);   // 20 us apart: 5.1 ms total
    }

    // Then live samples at their natural rate, until the window closes.
    std::int64_t src = newest_source;
    std::int64_t now = burst_now + 5 * kMs;
    while (now <= burst_now + kWarmup) {
        src += kStep;
        now += kStep;
        anchor.Apply(src, now);
    }

    check(anchor.latched(), "latched after the window");

    // If the anchor had come from the oldest backlog sample it would be ~5.12 s too large.
    const std::int64_t truth = burst_now - newest_source;   // the live relationship
    const std::int64_t error = anchor.anchor_ns() - truth;
    check(error >= 0 && error < 10 * kMs,
          "anchor comes from the freshest observation, not the backlog");
    check(anchor.warmup_samples() > 256, "the whole backlog was consumed by the warm-up");

    // And the live stream must now sit at the receiver's clock, not 5 s behind it.
    const std::int64_t out = anchor.Apply(src + kStep, now + kStep);
    const std::int64_t lag = (now + kStep) - out;
    check(lag > -10 * kMs && lag < 10 * kMs, "live stamps land on the receiver's now");
}

void test_spacing_is_preserved_exactly()
{
    // Property 1. The receiver's clock is deliberately given jitter that the source does
    // NOT have; the output must follow the SOURCE.
    StampAnchor anchor;
    std::int64_t now = warm_up(anchor, 0, kEpochNow);
    std::int64_t source = now - kEpochNow;

    const std::int64_t jitter[] = {0, 3 * kMs, -2 * kMs, 7 * kMs, -1 * kMs};
    std::int64_t previous = anchor.Apply(source, now);

    for (int i = 0; i < 5; ++i) {
        source += kStep;
        now += kStep;
        const std::int64_t out = anchor.Apply(source, now + jitter[i]);
        check_eq(out - previous, kStep, "output delta equals the source step, not the receiver's");
        previous = out;
    }
}

void test_irregular_source_spacing_survives()
{
    // A dropped plant frame is real information -- the trace and the freshness monitor are
    // both supposed to see the 100 ms hole. Smoothing it away would be a lie.
    StampAnchor anchor;
    const std::int64_t now = warm_up(anchor, 0, kEpochNow);
    const std::int64_t source = now - kEpochNow;
    const std::int64_t first = anchor.Apply(source, now);
    const std::int64_t out = anchor.Apply(source + 2 * kStep, now + 2 * kStep);
    check_eq(out - first, 2 * kStep, "a skipped source frame stays skipped");
}

// --- the watchdog ---------------------------------------------------------------

void test_clean_run_reports_no_deviation()
{
    StampAnchor anchor;
    std::int64_t now = warm_up(anchor, 0, kEpochNow);
    for (int i = 1; i <= 200; ++i) {
        anchor.Apply(now - kEpochNow, now);
        now += kStep;
    }
    check_eq(static_cast<std::int64_t>(anchor.exceedances()), 0, "perfect tracking: no exceedances");
    check_eq(anchor.max_deviation_ns(), 0, "perfect tracking: no deviation");
}

void test_jitter_below_tolerance_does_not_fire()
{
    // The measured floor is 2.7 ms against a 20 ms tolerance. If this fired, every real
    // run would report a fault and the signal would be worthless.
    StampAnchor anchor;
    std::int64_t now = warm_up(anchor, 0, kEpochNow);
    const std::int64_t jitter[] = {2'700'000, -2'700'000, 1'100'000, -900'000};
    for (int i = 0; i < 4; ++i) {
        anchor.Apply(now - kEpochNow, now + jitter[i]);
        now += kStep;
    }
    check_eq(static_cast<std::int64_t>(anchor.exceedances()), 0, "2.7 ms jitter stays quiet");
    check_eq(anchor.max_deviation_ns(), 2'700'000, "but it is still recorded");
}

void test_realtime_factor_drift_is_caught()
{
    // Property 2, the failure this exists for. realtime_factor 0.5: sim time advances at
    // half wall rate, so (receiver_now - source_stamp) grows by 50 ms of wall per 50 ms
    // step. It must trip, and it must trip EARLY -- 20 ms of error is 0.4 s of wall time
    // here, well inside a scenario.
    StampAnchor anchor;
    const std::int64_t t0 = warm_up(anchor, 0, kEpochNow);
    const std::int64_t s0 = t0 - kEpochNow;

    bool fired_within_a_second = false;
    for (int i = 1; i <= 40; ++i) {           // 40 steps = 2 s of sim, 4 s of wall
        anchor.Apply(s0 + i * kStep, t0 + 2 * i * kStep);   // wall runs at 2x sim
        if (i == 20 && anchor.exceedances() > 0) fired_within_a_second = true;
    }
    check(fired_within_a_second, "half-rate sim is caught within a second of sim time");
    check(anchor.max_deviation_ns() > 0, "deviation is positive when sim lags wall");
}

void test_stalled_simulator_is_caught_with_the_opposite_sign()
{
    // The other direction: sim time frozen while the wall moves on. Sign matters -- it is
    // what distinguishes "sim is slow" from "sim has stopped", and an absolute value would
    // report the two identically.
    StampAnchor anchor;
    const std::int64_t t0 = warm_up(anchor, 0, kEpochNow);
    const std::int64_t s0 = t0 - kEpochNow;
    anchor.Apply(s0, t0);                       // one clean live sample
    anchor.Apply(s0, t0 + 500 * kMs);           // then sim time freezes for 500 ms
    check(anchor.exceedances() == 1, "a 500 ms stall trips the tolerance");
    check(anchor.max_deviation_ns() == 500 * kMs, "and is recorded with a positive sign");

    // Sim running FAST relative to wall gives a negative deviation.
    StampAnchor fast;
    const std::int64_t f0 = warm_up(fast, 0, kEpochNow);
    fast.Apply((f0 - kEpochNow) + 500 * kMs, f0);
    check(fast.max_deviation_ns() == -500 * kMs, "sim ahead of wall reads negative");
}

void test_anchor_never_moves_after_latching()
{
    // A self-correcting anchor would be worse than a fixed one: it would absorb the drift
    // silently, keep the stamps looking fresh, and destroy the only evidence that sim and
    // wall have parted. Fixed-and-watched is the deliberate choice.
    StampAnchor anchor;
    const std::int64_t t0 = warm_up(anchor, 0, kEpochNow);
    const std::int64_t s0 = t0 - kEpochNow;
    const std::int64_t latched = anchor.anchor_ns();
    for (int i = 1; i <= 10; ++i) {
        anchor.Apply(s0 + i * kStep, t0 + 3 * i * kStep);
    }
    check_eq(anchor.anchor_ns(), latched, "anchor is latched once and never re-fitted");

    // ...and NOT even by a sample that would lower the minimum. The warm-up is the only
    // place the anchor moves; a self-correcting anchor would absorb drift silently and
    // destroy the evidence that sim and wall have parted.
    anchor.Apply(s0 + 11 * kStep, t0 + 11 * kStep - 2 * kSec);
    check_eq(anchor.anchor_ns(), latched, "a smaller observation after latching is ignored");
}

// ★ THE SECOND REGRESSION. The plant exhausted its sample count and was restarted while
// the bridges stayed up; the new run's clock began at zero and the anchor -- latched ten
// minutes earlier -- was 555 s wrong. Drift must never self-correct; a run boundary must.
void test_plant_restart_relatches()
{
    StampAnchor anchor;
    std::int64_t now = warm_up(anchor, 0, kEpochNow);
    std::int64_t src = now - kEpochNow;

    // A healthy first run.
    for (int i = 0; i < 100; ++i) {
        src += kStep;
        now += kStep;
        anchor.Apply(src, now);
    }
    const std::int64_t first_anchor = anchor.anchor_ns();
    check_eq(static_cast<std::int64_t>(anchor.run_boundaries()), 0, "no boundary during one run");
    check_eq(static_cast<std::int64_t>(anchor.exceedances()), 0, "first run is clean");

    // Plant dies. Ten minutes of wall time pass. It restarts from sim time zero.
    now += 600 * kSec;
    anchor.Apply(0, now);

    check_eq(static_cast<std::int64_t>(anchor.run_boundaries()), 1, "the backwards jump is caught");
    check_eq(anchor.last_run_gap_ns(), src, "the gap is the old run's elapsed sim time");
    check(!anchor.latched(), "a run boundary reopens the warm-up, it does not nudge the anchor");

    // The new run warms up and lands on the receiver's clock again.
    std::int64_t nsrc = 0;
    const std::int64_t new_start_now = now;
    while (now <= new_start_now + kWarmup) {
        nsrc += kStep;
        now += kStep;
        anchor.Apply(nsrc, now);
    }
    check(anchor.latched(), "re-latched on the new run");
    check(anchor.anchor_ns() != first_anchor, "and the anchor actually moved to the new run");

    const std::int64_t out = anchor.Apply(nsrc + kStep, now + kStep);
    const std::int64_t lag = (now + kStep) - out;
    check(lag > -10 * kMs && lag < 10 * kMs, "new-run stamps land on the receiver's now");

    // ...and the old run's alarm state is gone, not latched forever on a healthy new run.
    check_eq(static_cast<std::int64_t>(anchor.exceedances()), 0, "deviation stats reset with the run");
}

// The boundary rule must not fire on ordinary out-of-order jitter, or every jittery plant
// would re-latch constantly -- which IS self-correction by another name.
void test_small_backwards_step_is_jitter_not_a_boundary()
{
    StampAnchor anchor;                       // 20 ms tolerance
    std::int64_t now = warm_up(anchor, 0, kEpochNow);
    std::int64_t src = now - kEpochNow;
    anchor.Apply(src, now);

    now += kStep;
    anchor.Apply(src - 5 * kMs, now);         // 5 ms backwards: inside tolerance
    check_eq(static_cast<std::int64_t>(anchor.run_boundaries()), 0, "5 ms backwards is jitter");
    check(anchor.latched(), "and does not reopen the warm-up");
}

void test_custom_tolerance()
{
    StampAnchor tight(1 * kMs);
    const std::int64_t t0 = warm_up(tight, 0, kEpochNow);
    const std::int64_t s0 = t0 - kEpochNow;
    tight.Apply(s0, t0);
    tight.Apply(s0 + kStep, t0 + kStep + 2 * kMs);
    check_eq(static_cast<std::int64_t>(tight.exceedances()), 1, "a tighter tolerance fires sooner");

    // A zero-length warm-up degenerates to the old first-sample behaviour. Kept reachable
    // for a source with no history (VOLATILE), but never the default -- see the header.
    StampAnchor immediate(kDefaultAnchorToleranceNs, 0);
    immediate.Apply(0, kEpochNow);
    check(immediate.latched(), "a zero warm-up latches on the first sample");
    check_eq(immediate.anchor_ns(), kEpochNow, "and takes it verbatim");
}

// --- policy parsing --------------------------------------------------------------

void test_parse()
{
    StampPolicy p = StampPolicy::kSource;

    check(parse_stamp_policy("source", p) && p == StampPolicy::kSource, "parses 'source'");
    check(parse_stamp_policy("epoch-anchored", p) && p == StampPolicy::kEpochAnchored,
          "parses 'epoch-anchored'");
    check(parse_stamp_policy("epoch_anchored", p) && p == StampPolicy::kEpochAnchored,
          "parses the underscore spelling too");

    // The important one. A typo must NOT resolve to a plausible default -- choosing wrong
    // between these two is the exact bug this header was written to fix, and a node that
    // starts up looking healthy on a misspelled policy hides it again.
    check(!parse_stamp_policy("epoch", p), "rejects a near-miss");
    check(!parse_stamp_policy("", p), "rejects empty");
    check(!parse_stamp_policy("Source", p), "rejects wrong case rather than guessing");

    check_eq(kDefaultAnchorToleranceNs, 20 * kMs, "default tolerance is 20 ms");
    check_eq(sils::ss2::kDefaultAnchorWarmupNs, kWarmup, "default warm-up is 500 ms");
}

}  // namespace

int main()
{
    test_not_latched_during_warmup();
    test_clean_warmup_gives_the_true_offset();
    test_mid_scenario_attach();
    test_transient_local_backlog_does_not_poison_the_anchor();
    test_plant_restart_relatches();
    test_small_backwards_step_is_jitter_not_a_boundary();
    test_spacing_is_preserved_exactly();
    test_irregular_source_spacing_survives();
    test_clean_run_reports_no_deviation();
    test_jitter_below_tolerance_does_not_fire();
    test_realtime_factor_drift_is_caught();
    test_stalled_simulator_is_caught_with_the_opposite_sign();
    test_anchor_never_moves_after_latching();
    test_custom_tolerance();
    test_parse();

    std::printf("%s  test_stamp_policy: %d checks, %d failures\n",
                g_failures == 0 ? "PASS" : "FAIL", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
