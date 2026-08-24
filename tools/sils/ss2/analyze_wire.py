#!/usr/bin/env python3
"""analyze_wire.py -- stage-1 (wiring) verdict for SiLS loop B.

Reads a vECU trace produced while the SS_v2 adapter (adapters/ss2) was publishing, and
answers the three questions that stage 1 exists to answer. See
docs/sils/sils-ss2-bringup.md §6 and docs/sils/sils-simulator-loops.md §7.3.

  V2  INTEROP   Does a Fast DDS 2.6 writer (the adapter, pinned to the ROS 2 Humble
                stack) actually reach our Fast DDS 3.x reader? This is the stage-1
                gate: everything above it is pointless if the wire does not carry.
  V1  FRAME     Is EgoState.velocity in the VEHICLE frame (canonical, per
                idl/dds/sils/EgoState.idl) or the MAP frame? SS_v2 publishes
                action_status.twist and the IDL comment asserts vehicle frame -- an
                assertion nobody has checked. Wrong here means the whole control loop
                is wrong on every curve, silently, while straight-line runs look fine.
  V3  CLOCK     Is stamp_ns simulation time or wall clock? Feeds the stage-2 time-sync
                design (W1/W2): sim time means SS_v2 already exposes a usable clock,
                wall clock means we must patch one in.

ONLY V2 IS A GATE. V1 and V3 are measurements: they can come back INCONCLUSIVE (not
enough yaw travel, field absent) and that is a legitimate outcome that must not be
reported as a pass or a failure. Conflating "we did not learn it" with "it is fine" is
exactly how a wrong frame convention survives to stage 3.

Usage:
  analyze_wire.py TRACE [--min-samples N] [--json OUT]
Exit: 0 V2 passed, 1 V2 failed, 2 usage/IO error.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

EXIT_OK = 0
EXIT_GATE_FAIL = 1
EXIT_USAGE = 2

# Wall-clock stamps are ns since the Unix epoch: ~1.7e18 now, and >1e18 since 2001. A
# simulation clock that restarts each run cannot plausibly reach that -- 1e18 ns is 31.7
# years of scenario. The gap between the two hypotheses is ~9 orders of magnitude, so the
# threshold does not need to be precise.
WALL_CLOCK_NS_FLOOR = 1_000_000_000_000_000_000

# V1 needs the ego to actually turn. Below this much yaw travel the two hypotheses predict
# the same thing (vy ~= 0 either way) and the honest answer is INCONCLUSIVE.
MIN_YAW_TRAVEL_RAD = 0.15  # ~8.6 deg

# Map frame with heading psi gives vy = |v|*sin(psi - psi0); vehicle frame gives vy ~= 0.
# Classify on the ratio |vy|/|v| at the sample with the most yaw travel: well above the
# noise floor of a straight-line run, well below sin(MIN_YAW_TRAVEL_RAD) ~= 0.149.
VEHICLE_FRAME_MAX_RATIO = 0.02
MAP_FRAME_MIN_RATIO = 0.08


def read_trace(path: Path) -> list[dict]:
    """Parse JSONL, skipping blank lines. Non-JSON lines are an error, not a warning:
    the trace is the conformance boundary and a corrupt line means the run is unusable."""
    events: list[dict] = []
    with path.open(encoding="utf-8") as fh:
        for lineno, raw in enumerate(fh, 1):
            line = raw.strip()
            if not line:
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(f"{path}:{lineno}: not JSON: {exc}") from exc
            if not isinstance(obj, dict):
                raise ValueError(f"{path}:{lineno}: expected a JSON object")
            events.append(obj)
    return events


def check_interop(states: list[dict], min_samples: int) -> dict:
    """V2: did samples cross the 2.6 -> 3.x boundary, and did they arrive intact?

    THE TICK LOOP IS ZERO-ORDER HOLD. vehicle_run.hpp keeps `have_state` sticky and holds
    `latest` across ticks with no arrival, so a tick that finds nothing REPEATS the previous
    src_seq. Repeats are therefore the normal signature of a vECU ticking faster than the
    adapter publishes -- not duplicate delivery, and there is no way to tell the two apart
    from the trace anyway, because ZOH erases the difference.
    (The first version of this checker failed a perfectly healthy 596-sample run over 78
    such repeats. Repeats are counted and reported; only a BACKWARDS step fails.)

    Failure shapes, which point at different fixes:
      no samples          -> discovery/QoS/domain: nothing ever matched
      src_seq goes DOWN   -> real reordering, which RELIABLE must not permit
      too few samples     -> the wire carries; a duration/rate problem, not interop
    """
    detail: dict = {"samples": len(states), "min_samples": min_samples}

    if not states:
        return {
            **detail,
            "verdict": "FAIL",
            "reason": "no state_received at all -- the reader never matched the adapter's "
                      "writer. Check, in order: same DDS domain, SILS_DURABILITY not "
                      "'volatile' against the adapter's TRANSIENT_LOCAL writer, adapter "
                      "actually publishing (its own log line), same host/network.",
        }

    src_seqs = [e["src_seq"] for e in states if isinstance(e.get("src_seq"), int)]
    detail["with_src_seq"] = len(src_seqs)

    backwards = [(a, b) for a, b in zip(src_seqs, src_seqs[1:]) if b < a]
    held = sum(1 for a, b in zip(src_seqs, src_seqs[1:]) if b == a)
    detail["src_seq_backwards"] = len(backwards)
    detail["ticks_holding_last_sample"] = held

    if backwards:
        a, b = backwards[0]
        return {
            **detail,
            "verdict": "FAIL",
            "reason": f"src_seq went BACKWARDS ({a} -> {b}, {len(backwards)} occurrence(s)). "
                      "That is reordering, which RELIABLE must not permit -- suspect a second "
                      "publisher on the same topic, or two adapter instances sharing a run_id.",
        }

    # DISTINCT sequences, not trace rows, are the measure of how much the WIRE carried.
    # Under ZOH a stalled stream still emits one row per tick forever: a run where exactly
    # one sample arrived and was then held for 595 ticks looks like "596 samples" if you
    # count rows. That run must not pass -- it is the signature of a simulator that had
    # already exited, with TRANSIENT_LOCAL handing us its one retained sample.
    distinct = len(set(src_seqs)) if src_seqs else len(states)
    if src_seqs:
        detail["distinct_src_seq"] = distinct
        # Gaps are NOT a failure: a vECU ticking SLOWER than the adapter legitimately never
        # sees some seqs. Reported so an odd number is visible without failing a healthy run.
        span = src_seqs[-1] - src_seqs[0] + 1
        detail["src_seq_span"] = span
        detail["src_seq_delivered_ratio"] = round(distinct / span, 4) if span > 0 else None

    if distinct < min_samples:
        stalled = ""
        if src_seqs and distinct <= 2 and len(states) > 10 * distinct:
            stalled = (f" {len(states)} rows but only {distinct} distinct sequence(s): the "
                       "stream STALLED. Almost always a short cpp_mock_scenario that had "
                       "already finished -- the vECU then receives only the writer's "
                       "TRANSIENT_LOCAL retained sample. Start the vECU FIRST, then launch "
                       "the scenario.")
        return {
            **detail,
            "verdict": "FAIL",
            "reason": f"only {distinct} distinct sample(s), need {min_samples}. The wire "
                      f"carries, so this is a duration/rate problem, not interop.{stalled}",
        }

    note = ""
    if held:
        note = (f" {held} tick(s) re-reported the held sample (ZOH), i.e. the vECU ticked "
                "faster than the adapter published -- expected, not a defect.")
    return {
        **detail,
        "verdict": "PASS",
        "reason": f"{distinct} distinct sample(s) crossed the Fast DDS 2.6 -> 3.x boundary; "
                  f"src_seq never went backwards.{note}",
    }


def _yaw_travel(yaws: list[float]) -> float:
    """Total |unwrapped| yaw change. Unwrapping matters: a run that crosses +-pi would
    otherwise report a ~2pi jump and look like a hard turn that never happened."""
    total = 0.0
    for a, b in zip(yaws, yaws[1:]):
        d = math.atan2(math.sin(b - a), math.cos(b - a))
        total += abs(d)
    return total


def check_frame(states: list[dict]) -> dict:
    """V1: vehicle frame or map frame?

    Compares |vy|/|v| at the sample whose yaw is furthest from the run's initial yaw.
    That sample is where the two hypotheses disagree most, so it is the most informative
    single point -- and using one well-chosen point rather than an average avoids letting
    a long straight section wash out a short curve.
    """
    usable = [
        e for e in states
        if isinstance(e.get("vx"), (int, float))
        and isinstance(e.get("vy"), (int, float))
        and isinstance(e.get("yaw"), (int, float))
    ]
    if not usable:
        return {
            "verdict": "INCONCLUSIVE",
            "reason": "no state_received carries vy/yaw. Either the vECU predates the field "
                      "(service/sils/vecu/src/vehicle_run.hpp) or nothing was received.",
        }

    yaws = [float(e["yaw"]) for e in usable]
    travel = _yaw_travel(yaws)
    detail = {"samples": len(usable), "yaw_travel_rad": round(travel, 4)}

    if travel < MIN_YAW_TRAVEL_RAD:
        return {
            **detail,
            "verdict": "INCONCLUSIVE",
            "reason": f"the ego turned only {travel:.3f} rad (need {MIN_YAW_TRAVEL_RAD}). On a "
                      "straight path both frames predict vy ~= 0, so this run cannot tell them "
                      "apart. Re-run on a scenario with a curve.",
        }

    yaw0 = yaws[0]
    # Pick the sample with the largest heading change AND a non-trivial speed: at |v| ~= 0
    # the ratio is numerically meaningless whatever the frame.
    best = None
    best_dpsi = 0.0
    for e in usable:
        speed = math.hypot(float(e["vx"]), float(e["vy"]))
        if speed < 0.1:
            continue
        dpsi = abs(math.atan2(math.sin(float(e["yaw"]) - yaw0), math.cos(float(e["yaw"]) - yaw0)))
        if dpsi > best_dpsi:
            best, best_dpsi = e, dpsi

    if best is None:
        return {
            **detail,
            "verdict": "INCONCLUSIVE",
            "reason": "the ego turned, but never moved faster than 0.1 m/s while doing so; "
                      "|vy|/|v| is numerically meaningless at rest.",
        }

    speed = math.hypot(float(best["vx"]), float(best["vy"]))
    ratio = abs(float(best["vy"])) / speed
    detail.update({
        "at_heading_change_rad": round(best_dpsi, 4),
        "speed_m_s": round(speed, 4),
        "lateral_ratio": round(ratio, 4),
    })

    if ratio <= VEHICLE_FRAME_MAX_RATIO:
        return {
            **detail,
            "verdict": "VEHICLE",
            "reason": f"|vy|/|v| = {ratio:.4f} after {best_dpsi:.3f} rad of heading change. "
                      "Lateral velocity stays ~0 through the curve, which is the vehicle "
                      "frame -- matches what EgoState.idl already claims. No adapter change.",
        }
    if ratio >= MAP_FRAME_MIN_RATIO:
        return {
            **detail,
            "verdict": "MAP",
            "reason": f"|vy|/|v| = {ratio:.4f} after {best_dpsi:.3f} rad of heading change, "
                      f"vs sin({best_dpsi:.3f}) = {math.sin(best_dpsi):.4f} predicted for the "
                      "map frame. The adapter MUST rotate velocity by -yaw before publishing; "
                      "until it does, every curved run feeds the vECU a wrong lateral velocity.",
        }
    return {
        **detail,
        "verdict": "INCONCLUSIVE",
        "reason": f"|vy|/|v| = {ratio:.4f} sits between the vehicle-frame ceiling "
                  f"({VEHICLE_FRAME_MAX_RATIO}) and the map-frame floor ({MAP_FRAME_MIN_RATIO}). "
                  "Neither hypothesis explains it; a real sideslip angle would also land here, "
                  "so inspect the raw samples rather than guessing.",
    }


def check_clock(states: list[dict]) -> dict:
    """V3: simulation time or wall clock?

    Primary discriminator is magnitude -- a sim clock restarting near 0 cannot reach ns
    since the epoch. When stamps ARE wall clock the answer is already settled, so the rate
    comparison below only runs for the sim-time branch, where it distinguishes "sim time
    that happens to track real time" (free-run, what B1-1 found) from "sim time decoupled
    from real time" (which would mean SS_v2 is already steppable).
    """
    all_with_stamp = [e for e in states if isinstance(e.get("stamp_ns"), int)]

    # ONLY the rows that carried a NEW src_seq. The sim-vs-wall RATE is meaningless
    # otherwise: when a short scenario ends, the vECU keeps ticking and ZOH keeps
    # re-reporting the last sample, so the wall span runs on while sim time stands still.
    # A 1.4 s scenario observed for 30 s reported 0.05x real time -- an artifact of the
    # dead tail, not a property of the simulator.
    usable: list[dict] = []
    seen_seq: set = set()
    for e in all_with_stamp:
        sq = e.get("src_seq")
        if isinstance(sq, int):
            if sq in seen_seq:
                continue
            seen_seq.add(sq)
        usable.append(e)

    if len(usable) < 2:
        if len(all_with_stamp) > 10:
            why = (f"{len(all_with_stamp)} rows carry a stamp but they are all the SAME "
                   "sample held by ZOH -- a stalled stream, not a stamp problem (see V2).")
        else:
            why = ("either the vECU predates the field, or almost nothing arrived (see V2).")
        return {
            "verdict": "INCONCLUSIVE",
            "reason": f"fewer than 2 distinct stamped samples: {why}",
        }

    first, last = usable[0], usable[-1]
    detail = {
        "distinct_samples": len(usable),
        "trace_rows_with_stamp": len(all_with_stamp),
        "first_stamp_ns": first["stamp_ns"],
        "last_stamp_ns": last["stamp_ns"],
    }

    if first["stamp_ns"] >= WALL_CLOCK_NS_FLOOR:
        return {
            **detail,
            "verdict": "WALL",
            "reason": f"stamp_ns starts at {first['stamp_ns']} (>1e18 = ns since the Unix "
                      "epoch), so the source stamps WALL CLOCK, not simulation time. Stage 2 "
                      "needs its own clock: W1 ExternalClockSource or the W2 /clock bridge.",
        }

    sim_span_ns = last["stamp_ns"] - first["stamp_ns"]
    detail["sim_span_ns"] = sim_span_ns
    if sim_span_ns <= 0:
        # Two very different causes, and blaming the adapter for the wrong one wastes a
        # debugging session: a stalled STREAM (one sample held across every tick) says
        # nothing about the field, whereas a stamp that stays put while new sequences keep
        # arriving really is an unpopulated field.
        distinct_seq = len(usable)
        if distinct_seq <= 1:
            why = ("only one distinct sample ever arrived, so there is nothing for the stamp "
                   "to advance ACROSS. This is a stalled stream, not a stamp problem -- see V2.")
        else:
            why = (f"{distinct_seq} distinct samples arrived and the stamp never moved, so the "
                   "field really is unpopulated by the adapter.")
        return {
            **detail,
            "verdict": "INCONCLUSIVE",
            "reason": f"stamp_ns did not advance (span {sim_span_ns} ns): {why}",
        }

    wall_ms = [e["wall_time_ms"] for e in usable if isinstance(e.get("wall_time_ms"), int)]
    if len(wall_ms) >= 2 and wall_ms[-1] > wall_ms[0]:
        wall_span_ns = (wall_ms[-1] - wall_ms[0]) * 1_000_000
        ratio = sim_span_ns / wall_span_ns
        detail["wall_span_ns"] = wall_span_ns
        detail["sim_per_wall"] = round(ratio, 4)
        pace = "tracks real time (free-run, consistent with B1-1)" if 0.9 <= ratio <= 1.1 \
            else f"advances at {ratio:.2f}x real time -- decoupled from the wall clock"
        note = f" It {pace}."
    else:
        note = ""

    # Is the clock QUANTIZED? This is the question stage 2 actually cares about, and it is
    # separate from sim-vs-wall. A clock that only ever lands on multiples of some dt is a
    # FIXED-STEP integrator, which means SS_v2 already has an internal notion of "one step"
    # -- W1/W2 then only has to drive the existing step rather than introduce one. A
    # continuously-varying stamp would mean the opposite, and a much bigger patch.
    step_ns = _fixed_step(sorted({e["stamp_ns"] for e in usable}))
    if step_ns:
        detail["fixed_step_ns"] = step_ns
        note += (f" Every stamp is a multiple of {step_ns / 1e6:.3g} ms, so the source is a "
                 "FIXED-STEP clock -- stage 2 can drive an existing step instead of "
                 "inventing one (W1/W2).")

    return {
        **detail,
        "verdict": "SIM",
        "reason": f"stamp_ns starts at {first['stamp_ns']}, far below ns-since-epoch, so the "
                  f"source stamps SIMULATION time.{note}",
    }


# A step below this is indistinguishable from stamp jitter and would be over-read.
MIN_FIXED_STEP_NS = 1_000_000  # 1 ms


def _fixed_step(distinct_stamps: list[int]) -> int | None:
    """GCD of the gaps between distinct stamps -- the largest dt they are all multiples of.

    Returns None when there is no meaningful common step. Uses the GCD of the DIFFERENCES
    rather than of the values so an arbitrary epoch offset does not matter: a run starting
    at 34.05 s with a 50 ms step has stamps that share no useful factor, but their gaps do.
    """
    if len(distinct_stamps) < 3:
        return None  # two points always share a "step"; that proves nothing
    step = 0
    for a, b in zip(distinct_stamps, distinct_stamps[1:]):
        step = math.gcd(step, b - a)
    return step if step >= MIN_FIXED_STEP_NS else None


def analyze(events: list[dict], min_samples: int, ss2_commit: str | None = None) -> dict:
    states = [e for e in events if e.get("event") == "state_received"]
    interop = check_interop(states, min_samples)
    return {
        # WHICH simulator produced this. SS2_REF defaults to the moving `master` branch, so
        # a verdict without the upstream SHA cannot be reproduced or compared against a
        # later one -- carried in the JSON so the artifact stands alone.
        "ss2_upstream_commit": ss2_commit or "unrecorded",
        "trace_events": len(events),
        "state_received": len(states),
        "V2_interop": interop,
        "V1_frame": check_frame(states),
        "V3_clock": check_clock(states),
        "gate": interop["verdict"],
    }


_MARK = {
    "PASS": "OK", "FAIL": "XX",
    "VEHICLE": "OK", "MAP": "!!", "SIM": "OK", "WALL": "!!",
    "INCONCLUSIVE": "--",
}


def render(result: dict) -> str:
    lines = [
        "SiLS loop B -- stage 1 (wiring) verdict",
        f"  trace events: {result['trace_events']}   state_received: {result['state_received']}",
        f"  SS_v2 upstream: {result.get('ss2_upstream_commit', 'unrecorded')}",
        "",
    ]
    for key, title in (
        ("V2_interop", "V2 INTEROP  Fast DDS 2.6 -> 3.x"),
        ("V1_frame", "V1 FRAME    EgoState.velocity"),
        ("V3_clock", "V3 CLOCK    EgoState.stamp_ns"),
    ):
        r = result[key]
        lines.append(f"  [{_MARK.get(r['verdict'], '??')}] {title}: {r['verdict']}")
        lines.append(f"       {r['reason']}")
        extras = {k: v for k, v in r.items() if k not in ("verdict", "reason")}
        if extras:
            lines.append("       " + "  ".join(f"{k}={v}" for k, v in extras.items()))
        lines.append("")
    lines.append(
        "  GATE: V2 " + result["gate"]
        + ("  (V1/V3 are measurements, not gates -- INCONCLUSIVE means unlearned, not fine)"
           if result["gate"] == "PASS" else "")
    )
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("trace", type=Path, help="vECU JSONL trace")
    parser.add_argument("--min-samples", type=int, default=20,
                        help="minimum state_received for V2 to pass (default 20)")
    parser.add_argument("--json", type=Path, help="also write the full result as JSON")
    parser.add_argument("--ss2-commit", default=None,
                        help="upstream scenario_simulator_v2 SHA (from "
                             "docker/sils/ss2-upstream.pin); recorded in the result")
    args = parser.parse_args(argv)

    try:
        events = read_trace(args.trace)
    except (OSError, ValueError) as exc:
        print(f"analyze_wire: {exc}", file=sys.stderr)
        return EXIT_USAGE

    result = analyze(events, args.min_samples, args.ss2_commit)
    print(render(result))

    if args.json:
        try:
            args.json.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        except OSError as exc:
            print(f"analyze_wire: cannot write {args.json}: {exc}", file=sys.stderr)
            return EXIT_USAGE

    return EXIT_OK if result["gate"] == "PASS" else EXIT_GATE_FAIL


if __name__ == "__main__":
    sys.exit(main())
