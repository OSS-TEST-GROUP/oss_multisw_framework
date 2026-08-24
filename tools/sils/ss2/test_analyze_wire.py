#!/usr/bin/env python3
"""Tests for analyze_wire.py.

The point of these is not that the arithmetic is right -- it is that the analyzer never
turns "we did not learn it" into a verdict. A stage-1 report claiming VEHICLE frame on a
straight-line run would be worse than no report at all, because it closes a question that
is still open.
"""

from __future__ import annotations

import json
import math
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import analyze_wire as aw  # noqa: E402

WALL_EPOCH_NS = 1_754_000_000_000_000_000  # a plausible "now" in ns since the epoch


def state(tick, *, src_seq=None, stamp_ns=None, vx=None, vy=None, yaw=None,
          wall_time_ms=None, steering=0.0):
    e = {"run_id": "t", "instance_id": "v1", "event": "state_received", "tick_seq": tick,
         "steering": steering}
    if src_seq is not None:
        e["src_seq"] = src_seq
    if stamp_ns is not None:
        e["stamp_ns"] = stamp_ns
    if vx is not None:
        e["vx"] = vx
    if vy is not None:
        e["vy"] = vy
    if yaw is not None:
        e["yaw"] = yaw
    if wall_time_ms is not None:
        e["wall_time_ms"] = wall_time_ms
    return e


def straight_run(n=50, speed=10.0, dt_ns=50_000_000, t0_ns=0):
    """No heading change: V1 must stay INCONCLUSIVE no matter which frame is true."""
    return [
        state(i, src_seq=i, stamp_ns=t0_ns + i * dt_ns, vx=speed, vy=0.0, yaw=0.0,
              wall_time_ms=1_000 + i * (dt_ns // 1_000_000))
        for i in range(n)
    ]


def curve_run(n=50, speed=10.0, frame="vehicle", dt_ns=50_000_000, t0_ns=0,
              total_yaw=1.0):
    """A quarter-ish turn. In the vehicle frame vy stays 0; in the map frame the velocity
    vector rotates with the heading, so vy = |v|*sin(psi)."""
    out = []
    for i in range(n):
        psi = total_yaw * i / (n - 1)
        if frame == "vehicle":
            vx, vy = speed, 0.0
        else:
            vx, vy = speed * math.cos(psi), speed * math.sin(psi)
        out.append(state(i, src_seq=i, stamp_ns=t0_ns + i * dt_ns, vx=vx, vy=vy, yaw=psi,
                         wall_time_ms=1_000 + i * (dt_ns // 1_000_000)))
    return out


# --- V2 interop ------------------------------------------------------------------

def test_v2_passes_on_a_healthy_run():
    r = aw.analyze(straight_run(50), min_samples=20)["V2_interop"]
    assert r["verdict"] == "PASS"
    assert r["samples"] == 50


def test_v2_fails_with_no_samples_and_names_the_qos_trap():
    r = aw.analyze([{"run_id": "t", "event": "ready"}], min_samples=20)["V2_interop"]
    assert r["verdict"] == "FAIL"
    # The durability mismatch is silent at the DDS layer, so the message must point at it.
    assert "durability" in r["reason"].lower() or "SILS_DURABILITY" in r["reason"]


def test_v2_fails_when_src_seq_goes_backwards():
    ev = straight_run(30)
    ev[10]["src_seq"] = 3  # went backwards
    r = aw.analyze(ev, min_samples=5)["V2_interop"]
    assert r["verdict"] == "FAIL"
    assert r["src_seq_backwards"] >= 1
    assert "BACKWARDS" in r["reason"]


def test_v2_passes_when_src_seq_repeats_because_of_zero_order_hold():
    """The real-run regression: vehicle_run.hpp holds `latest` across ticks with no arrival,
    so a vECU ticking faster than the adapter re-reports the same src_seq. 78 such repeats
    in a healthy 596-sample run were once reported as FAIL."""
    ev = straight_run(30)
    for i in (10, 11, 20):
        ev[i]["src_seq"] = ev[i - 1]["src_seq"]
    r = aw.analyze(ev, min_samples=5)["V2_interop"]
    assert r["verdict"] == "PASS"
    assert r["ticks_holding_last_sample"] == 3
    assert r["src_seq_backwards"] == 0
    assert "ZOH" in r["reason"]


def test_v2_counts_distinct_seqs_not_rows_for_the_delivered_ratio():
    ev = straight_run(30)
    for i in range(10, 20):
        ev[i]["src_seq"] = ev[9]["src_seq"]
    r = aw.analyze(ev, min_samples=5)["V2_interop"]
    assert r["distinct_src_seq"] == 20
    assert r["src_seq_delivered_ratio"] < 1.0


def test_v2_tolerates_gaps_because_the_vecu_polls_slower_than_the_adapter():
    ev = [state(i, src_seq=i * 4, stamp_ns=i * 200_000_000, wall_time_ms=1000 + i * 200)
          for i in range(30)]
    r = aw.analyze(ev, min_samples=20)["V2_interop"]
    assert r["verdict"] == "PASS"
    assert r["src_seq_delivered_ratio"] < 0.3  # visible, but not a failure


def test_v2_fails_below_min_samples_and_says_it_is_not_an_interop_problem():
    r = aw.analyze(straight_run(5), min_samples=20)["V2_interop"]
    assert r["verdict"] == "FAIL"
    assert "interop" in r["reason"]


def test_v2_fails_a_stalled_stream_even_though_rows_are_plentiful():
    """The real-run regression: lanechange_left had already finished, so TRANSIENT_LOCAL
    handed over ONE retained sample which ZOH then re-reported for 595 ticks. Counting rows
    made that pass a min_samples=20 gate; counting distinct sequences does not."""
    ev = [state(i, src_seq=700, stamp_ns=1_400_000_000, vx=5.0, vy=0.0, yaw=0.0,
                wall_time_ms=1000 + i * 50)
          for i in range(596)]
    r = aw.analyze(ev, min_samples=20)["V2_interop"]
    assert r["verdict"] == "FAIL"
    assert r["distinct_src_seq"] == 1
    assert r["samples"] == 596
    assert "STALLED" in r["reason"]
    assert "TRANSIENT_LOCAL" in r["reason"]


def test_v3_blames_the_stream_not_the_field_when_only_one_sample_arrived():
    ev = [state(i, src_seq=700, stamp_ns=1_400_000_000, wall_time_ms=1000 + i * 50)
          for i in range(596)]
    r = aw.analyze(ev, min_samples=20)["V3_clock"]
    assert r["verdict"] == "INCONCLUSIVE"
    assert "stalled stream" in r["reason"]


def test_v3_rate_ignores_the_dead_tail_after_a_short_scenario():
    """The real-run regression: lanechange_left is 1.4 s of sim time, but the vECU ran for
    30 s and ZOH kept emitting rows. Measuring the wall span over ALL rows reported 0.05x
    real time -- an artifact of the dead tail, not the simulator's pace."""
    ev = [state(i, src_seq=i, stamp_ns=50_000_000 + i * 50_000_000,
                wall_time_ms=1000 + i * 50)
          for i in range(28)]
    last_seq, last_stamp = ev[-1]["src_seq"], ev[-1]["stamp_ns"]
    ev += [state(28 + i, src_seq=last_seq, stamp_ns=last_stamp,
                 wall_time_ms=1000 + 28 * 50 + i * 50)
           for i in range(500)]                       # 25 s of held rows
    r = aw.analyze(ev, min_samples=20)["V3_clock"]
    assert r["verdict"] == "SIM"
    assert r["distinct_samples"] == 28
    assert r["trace_rows_with_stamp"] == 528
    assert r["sim_per_wall"] == pytest.approx(1.0, abs=0.1), \
        "the dead tail leaked into the wall span"


def test_v3_blames_the_field_when_many_samples_share_a_frozen_stamp():
    ev = [state(i, src_seq=700 + i, stamp_ns=0, wall_time_ms=1000 + i * 50)
          for i in range(30)]
    r = aw.analyze(ev, min_samples=20)["V3_clock"]
    assert r["verdict"] == "INCONCLUSIVE"
    assert "unpopulated" in r["reason"]


# --- V1 frame --------------------------------------------------------------------

def test_v1_straight_line_is_inconclusive_not_vehicle():
    """The regression this whole file exists for: vy is 0 on a straight run in EITHER
    frame, so reporting VEHICLE here would falsely close the question."""
    r = aw.analyze(straight_run(50), min_samples=20)["V1_frame"]
    assert r["verdict"] == "INCONCLUSIVE"
    assert "straight" in r["reason"] or "turn" in r["reason"]


def test_v1_detects_vehicle_frame_on_a_curve():
    r = aw.analyze(curve_run(frame="vehicle"), min_samples=20)["V1_frame"]
    assert r["verdict"] == "VEHICLE"


def test_v1_detects_map_frame_on_a_curve():
    r = aw.analyze(curve_run(frame="map"), min_samples=20)["V1_frame"]
    assert r["verdict"] == "MAP"
    assert "rotate" in r["reason"]


def test_v1_inconclusive_when_the_field_is_absent():
    ev = [state(i, src_seq=i, stamp_ns=i * 1000) for i in range(30)]
    r = aw.analyze(ev, min_samples=20)["V1_frame"]
    assert r["verdict"] == "INCONCLUSIVE"
    assert "predates" in r["reason"]


def test_v1_inconclusive_when_stationary_through_the_turn():
    ev = curve_run(frame="map", speed=0.0)
    r = aw.analyze(ev, min_samples=20)["V1_frame"]
    assert r["verdict"] == "INCONCLUSIVE"


def test_v1_yaw_travel_unwraps_across_pi():
    """A run crossing +-pi must not register a phantom 2*pi turn."""
    yaws = [math.pi - 0.01, -math.pi + 0.01]
    assert aw._yaw_travel(yaws) < 0.05


def test_v1_ambiguous_ratio_is_inconclusive_not_forced():
    ev = curve_run(frame="vehicle")
    for e in ev:  # a real sideslip: small but well above the vehicle-frame ceiling
        e["vy"] = 0.05 * e["vx"]
    r = aw.analyze(ev, min_samples=20)["V1_frame"]
    assert r["verdict"] == "INCONCLUSIVE"
    assert "sideslip" in r["reason"]


# --- V3 clock --------------------------------------------------------------------

def test_v3_detects_sim_time():
    r = aw.analyze(straight_run(50, t0_ns=0), min_samples=20)["V3_clock"]
    assert r["verdict"] == "SIM"


def test_v3_detects_wall_clock():
    r = aw.analyze(straight_run(50, t0_ns=WALL_EPOCH_NS), min_samples=20)["V3_clock"]
    assert r["verdict"] == "WALL"
    assert "W1" in r["reason"] or "/clock" in r["reason"]


def test_v3_sim_time_tracking_real_time_is_reported_as_free_run():
    """dt_ns == wall delta: sim time exists but is not decoupled -- exactly B1-1's finding."""
    r = aw.analyze(straight_run(50, dt_ns=50_000_000), min_samples=20)["V3_clock"]
    assert r["verdict"] == "SIM"
    assert r["sim_per_wall"] == pytest.approx(1.0, abs=0.05)
    assert "free-run" in r["reason"]


def test_v3_reports_decoupled_sim_time_when_it_outpaces_the_wall_clock():
    ev = [state(i, src_seq=i, stamp_ns=i * 500_000_000, wall_time_ms=1000 + i * 50)
          for i in range(30)]
    r = aw.analyze(ev, min_samples=20)["V3_clock"]
    assert r["verdict"] == "SIM"
    assert r["sim_per_wall"] > 5
    assert "decoupled" in r["reason"]


def test_v3_detects_a_fixed_step_clock():
    """Measured on the real run: stamps 34050000000 .. 60050000000, all multiples of 50 ms.
    A fixed step means SS_v2 already has a notion of 'one step' for W1/W2 to drive."""
    ev = [state(i, src_seq=i, stamp_ns=34_050_000_000 + i * 50_000_000,
                wall_time_ms=1000 + i * 58)
          for i in range(40)]
    r = aw.analyze(ev, min_samples=20)["V3_clock"]
    assert r["verdict"] == "SIM"
    assert r["fixed_step_ns"] == 50_000_000
    assert "FIXED-STEP" in r["reason"]


def test_v3_fixed_step_survives_dropped_samples():
    """Gaps must not defeat the detector -- a missed step leaves a 2*dt gap, and the GCD of
    {dt, 2*dt} is still dt."""
    stamps = [0, 50, 100, 200, 250, 400, 450]  # ms, with holes
    ev = [state(i, src_seq=i, stamp_ns=s * 1_000_000, wall_time_ms=1000 + i * 50)
          for i, s in enumerate(stamps)]
    assert aw.analyze(ev, min_samples=5)["V3_clock"]["fixed_step_ns"] == 50_000_000


def test_v3_no_fixed_step_when_the_clock_jitters():
    """Jitter must NOT be read as a step. The offsets here are non-linear on purpose: a
    linear offset (even a weird one like i*7919) IS a fixed step and would be reported as
    one, correctly -- the detector only has to reject a clock whose gaps genuinely vary."""
    ev = [state(i, src_seq=i,
                stamp_ns=i * 50_000_000 + ((i * i * 37) % 13) * 100_003,
                wall_time_ms=1000 + i * 50)
          for i in range(40)]
    r = aw.analyze(ev, min_samples=20)["V3_clock"]
    assert r["verdict"] == "SIM"
    assert "fixed_step_ns" not in r, "jitter of ~0.1 ms was mistaken for a step"


def test_v3_two_stamps_do_not_prove_a_step():
    ev = [state(i, src_seq=i, stamp_ns=i * 50_000_000, wall_time_ms=1000 + i * 50)
          for i in range(2)]
    assert aw._fixed_step(sorted({e["stamp_ns"] for e in ev})) is None


def test_v3_frozen_stamp_is_inconclusive():
    ev = [state(i, src_seq=i, stamp_ns=0, wall_time_ms=1000 + i * 50) for i in range(30)]
    r = aw.analyze(ev, min_samples=20)["V3_clock"]
    assert r["verdict"] == "INCONCLUSIVE"


def test_v3_inconclusive_when_the_field_is_absent():
    ev = [state(i, src_seq=i, vx=1.0, vy=0.0, yaw=0.0) for i in range(30)]
    assert aw.analyze(ev, min_samples=20)["V3_clock"]["verdict"] == "INCONCLUSIVE"


# --- gate + IO -------------------------------------------------------------------

def test_gate_follows_v2_only():
    """A MAP-frame WALL-clock run still passes the stage-1 gate: those are measurements
    feeding stage 2, not stage-1 failures."""
    ev = curve_run(frame="map", t0_ns=WALL_EPOCH_NS)
    result = aw.analyze(ev, min_samples=20)
    assert result["V1_frame"]["verdict"] == "MAP"
    assert result["V3_clock"]["verdict"] == "WALL"
    assert result["gate"] == "PASS"


def test_result_records_which_simulator_it_measured():
    """SS2_REF defaults to the moving `master`, so a verdict without the SHA cannot be
    reproduced. It must be present either way -- explicitly 'unrecorded', never absent."""
    r = aw.analyze(straight_run(50), min_samples=20, ss2_commit="abc123")
    assert r["ss2_upstream_commit"] == "abc123"
    assert aw.analyze(straight_run(50), 20)["ss2_upstream_commit"] == "unrecorded"
    assert "abc123" in aw.render(r)


def test_render_covers_every_check(capsys):
    text = aw.render(aw.analyze(curve_run(frame="map"), min_samples=20))
    assert "V2 INTEROP" in text and "V1 FRAME" in text and "V3 CLOCK" in text


def test_main_exit_codes(tmp_path):
    ok = tmp_path / "ok.jsonl"
    ok.write_text("\n".join(json.dumps(e) for e in straight_run(50)), encoding="utf-8")
    assert aw.main([str(ok)]) == aw.EXIT_OK

    bad = tmp_path / "empty.jsonl"
    bad.write_text("", encoding="utf-8")
    assert aw.main([str(bad)]) == aw.EXIT_GATE_FAIL

    assert aw.main([str(tmp_path / "nope.jsonl")]) == aw.EXIT_USAGE


def test_main_rejects_a_corrupt_line(tmp_path):
    p = tmp_path / "corrupt.jsonl"
    p.write_text('{"run_id":"t","event":"ready"}\nnot json\n', encoding="utf-8")
    assert aw.main([str(p)]) == aw.EXIT_USAGE


def test_main_writes_json(tmp_path):
    src = tmp_path / "t.jsonl"
    src.write_text("\n".join(json.dumps(e) for e in curve_run(frame="map")), encoding="utf-8")
    out = tmp_path / "r.json"
    assert aw.main([str(src), "--json", str(out)]) == aw.EXIT_OK
    assert json.loads(out.read_text())["V1_frame"]["verdict"] == "MAP"


def test_blank_lines_are_skipped(tmp_path):
    p = tmp_path / "t.jsonl"
    p.write_text("\n\n".join(json.dumps(e) for e in straight_run(50)) + "\n\n",
                 encoding="utf-8")
    assert aw.main([str(p)]) == aw.EXIT_OK
