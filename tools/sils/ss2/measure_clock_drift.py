#!/usr/bin/env python3
"""measure_clock_drift.py -- how far does SS_v2's /clock run from the wall clock?

WHY THIS NUMBER MATTERS

docs/sils/evidence/ss2-clock-survey-2026-08-11.md §3 leaves one design decision open, and
this measurement is what closes it:

  drift ~ 0        -> a fixed epoch offset is enough. Each Autoware bridge captures
                      (wall - sim) once at the first sample and adds it forever. No new
                      node, no new topic.
  drift != 0       -> the offset goes stale. A tick bridge republishing SS_v2's /clock
                      onto the Autoware domain is required, and every consumer has to run
                      use_sim_time:=true so they age messages against sim time, not wall.

Measuring it is not just "read /clock twice". SS_v2's SimulationClock advances by
realtime_factor / frame_rate per frame and is driven by the interpreter's own loop, so it
gains or loses against the wall exactly as fast as that loop runs early or late. The slope
is the answer; a single sample only shows the start-up offset, which is why §2 could not
tell the two apart from one reading.

WHAT IT REPORTS

  offset(t) = clock_sec(t) - wall_sec(t)

  intercept  the constant part -- what an epoch_offset policy would latch
  slope      seconds of drift per second of wall time. This is the verdict.
             |slope| * horizon = error an offset policy accumulates.
  residual   max deviation from the fitted line, i.e. how much of the error is jitter
             rather than drift, and thus the floor no linear correction can remove.

Also counts /clock gaps: the publisher is BEST_EFFORT (§2), so silent loss is possible and
would stall a downstream consumer's time without any error being raised.

Usage:
  measure_clock_drift.py [--seconds 120] [--json OUT]

Exit: 0 measured, 3 no /clock seen (nothing publishing, or wrong ROS_DOMAIN_ID).
"""

import argparse
import json
import statistics
import sys
import time

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSDurabilityPolicy, QoSHistoryPolicy, QoSProfile, QoSReliabilityPolicy
from rosgraph_msgs.msg import Clock

EXIT_NO_CLOCK = 3

# BEST_EFFORT to match the publisher measured in §2. A RELIABLE reader against a
# BEST_EFFORT writer does not match at all, and RxO mismatch is silent -- we would
# report "no /clock" for a simulator that is publishing perfectly well.
CLOCK_QOS = QoSProfile(
    reliability=QoSReliabilityPolicy.BEST_EFFORT,
    durability=QoSDurabilityPolicy.VOLATILE,
    history=QoSHistoryPolicy.KEEP_LAST,
    depth=10,
)


class ClockDriftProbe(Node):
    def __init__(self):
        super().__init__("sils_clock_drift_probe")
        # Explicitly NOT use_sim_time: this node's whole job is to compare sim time
        # against the wall, so it must keep its own clock independent.
        self.samples = []  # (wall_sec, clock_sec)
        self.create_subscription(Clock, "/clock", self._on_clock, CLOCK_QOS)

    def _on_clock(self, msg):
        wall = time.time()
        sim = msg.clock.sec + msg.clock.nanosec * 1e-9
        self.samples.append((wall, sim))


def linear_fit(xs, ys):
    """Least-squares slope/intercept. Written out rather than pulled from numpy so the
    probe runs in the plain sils-ss2 image with no extra install."""
    n = len(xs)
    mx = sum(xs) / n
    my = sum(ys) / n
    sxx = sum((x - mx) ** 2 for x in xs)
    if sxx == 0:
        return 0.0, my
    sxy = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    slope = sxy / sxx
    return slope, my - slope * mx


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--seconds", type=float, default=120.0)
    parser.add_argument("--json", type=str, default=None)
    args = parser.parse_args()

    rclpy.init()
    probe = ClockDriftProbe()

    deadline = time.time() + args.seconds
    while time.time() < deadline and rclpy.ok():
        rclpy.spin_once(probe, timeout_sec=0.1)

    samples = list(probe.samples)
    probe.destroy_node()
    rclpy.shutdown()

    if len(samples) < 2:
        print(f"no /clock: {len(samples)} sample(s) in {args.seconds:.0f}s.")
        print("  Is a simulator up, and is ROS_DOMAIN_ID the same one it publishes on?")
        return EXIT_NO_CLOCK

    t0_wall = samples[0][0]
    xs = [w - t0_wall for w, _ in samples]                  # wall seconds since start
    ys = [s - w for w, s in samples]                        # offset = sim - wall

    slope, intercept = linear_fit(xs, ys)
    residuals = [y - (slope * x + intercept) for x, y in zip(xs, ys)]
    max_abs_residual = max(abs(r) for r in residuals)

    # Publish interval, for the gap count. The nominal step is realtime_factor/frame_rate,
    # but measure it rather than assume -- the launch may have been given another rate.
    gaps = [xs[i + 1] - xs[i] for i in range(len(xs) - 1)]
    median_gap = statistics.median(gaps)
    # 3x median: loose enough not to fire on ordinary scheduling jitter, tight enough that
    # a genuinely dropped sample (2x) plus its jitter still trips it.
    dropped = [g for g in gaps if g > 3 * median_gap]

    span = xs[-1]
    sim_span = samples[-1][1] - samples[0][1]

    print(f"samples            {len(samples)} over {span:.1f}s wall / {sim_span:.1f}s sim")
    print(f"publish interval   median {median_gap * 1000:.1f} ms  max {max(gaps) * 1000:.1f} ms")
    print(f"gaps > 3x median   {len(dropped)}"
          + (f"  (largest {max(dropped) * 1000:.0f} ms)" if dropped else ""))
    print()
    print(f"offset at start    {intercept:+.6f} s      <- what epoch_offset would latch")
    print(f"drift slope        {slope:+.6e} s/s   <- THE VERDICT")
    print(f"max residual       {max_abs_residual * 1000:.2f} ms    <- jitter floor")
    print()
    for horizon in (60, 300, 1800):
        print(f"  a fixed offset is off by {abs(slope) * horizon * 1000:8.1f} ms "
              f"after {horizon:4d}s")
    print()

    # 20 ms over 5 minutes. Chosen against the consumer, not from taste: Autoware's
    # topic_state_monitor and the planning stack judge freshness in the tens of
    # milliseconds, so an error that stays well under that for a scenario-length run
    # cannot change a decision -- and a scenario-length run is 300 s (global_timeout).
    threshold = 0.020 / 300
    if abs(slope) < threshold:
        verdict = "offset-sufficient"
        print(f"VERDICT  {verdict}: |slope| < {threshold:.2e} s/s.")
        print("         A fixed epoch offset in the two Autoware bridges is enough;")
        print("         the tick bridge can wait for W3/B2-2.")
    else:
        verdict = "tick-bridge-required"
        print(f"VERDICT  {verdict}: |slope| >= {threshold:.2e} s/s.")
        print("         A latched offset goes stale within a scenario. Republish SS_v2's")
        print("         /clock onto the Autoware domain and set use_sim_time:=true.")

    if args.json:
        with open(args.json, "w") as f:
            json.dump({
                "samples": len(samples),
                "wall_span_sec": span,
                "sim_span_sec": sim_span,
                "median_publish_interval_sec": median_gap,
                "max_publish_interval_sec": max(gaps),
                "gaps_over_3x_median": len(dropped),
                "offset_intercept_sec": intercept,
                "drift_slope_sec_per_sec": slope,
                "max_residual_sec": max_abs_residual,
                "verdict": verdict,
            }, f, indent=2)
        print(f"\nwrote {args.json}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
