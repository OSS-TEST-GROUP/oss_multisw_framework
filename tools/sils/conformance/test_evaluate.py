#!/usr/bin/env python3
"""Self-contained checks for evaluate.py -- no pytest, just asserts.

Run: python3 tools/sils/conformance/test_evaluate.py

Fixtures are built in-process (a merged coordinator+probe+verify JSONL stream is
written to a temp file) rather than committed, so the expected-event contract
lives next to the assertions that depend on it.
"""

from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import evaluate as ev  # noqa: E402


def passing_events(run_id="r1", instance_id="viss-probe-1", domain=90):
    """A well-formed merged stream for one PASSED, fully-conformant run."""
    return [
        {"run_id": run_id, "event": "run_start", "wall_time_ms": 1000, "domain_id": domain,
         "backend": "local", "instance_id": instance_id},                                   # coordinator
        {"run_id": run_id, "event": "domain_leased", "wall_time_ms": 1001, "domain_id": domain, "pool": "90-92"},
        {"run_id": run_id, "instance_id": instance_id, "event": "run_start", "wall_time_ms": 1002, "domain_id": domain},  # probe
        {"run_id": run_id, "event": "process_running", "wall_time_ms": 1003, "pid": 1234},
        {"run_id": run_id, "instance_id": instance_id, "event": "endpoint_matched", "wall_time_ms": 1004},  # probe
        {"run_id": run_id, "instance_id": instance_id, "event": "ready", "wall_time_ms": 1005},             # probe
        {"run_id": run_id, "event": "vecu_ready", "wall_time_ms": 1006},
        {"run_id": run_id, "event": "verify_invoke", "wall_time_ms": 1007},
        {"run_id": run_id, "expected_instance_id": instance_id, "event": "verify_start", "wall_time_ms": 1008, "domain_id": domain},
        {"run_id": run_id, "expected_instance_id": instance_id, "event": "endpoint_matched", "wall_time_ms": 1009},
        {"run_id": run_id, "expected_instance_id": instance_id, "event": "sample_received", "wall_time_ms": 1010},
        {"run_id": run_id, "expected_instance_id": instance_id, "event": "verify_pass", "wall_time_ms": 1011},
        {"run_id": run_id, "event": "domain_released", "wall_time_ms": 1012},
        {"run_id": run_id, "event": "cleanup_done", "wall_time_ms": 1013},
        {"run_id": run_id, "event": "run_pass", "wall_time_ms": 1014},
    ]


def write_trace(events, name="run.trace.jsonl", extra_lines=None):
    d = Path(tempfile.mkdtemp(prefix="sils-conf-test-"))
    p = d / name
    lines = [json.dumps(e) for e in events]
    if extra_lines:
        lines.extend(extra_lines)
    p.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return p


def test_passing_run_is_conformant():
    p = write_trace(passing_events())
    events, err = ev.parse_trace(p)
    assert err is None
    r = ev.evaluate_run(events, str(p))
    assert r["verdict"] == "PASSED", r
    assert r["conformant"] is True, r["violations"]
    assert r["fidelity_evidenced"] == "L0"
    assert r["evidence"]["samples_received"] == 1
    assert r["violations"] == []
    assert r["instance_ids"] == ["viss-probe-1"]
    # duration is the wall_time_ms span (1000..1014ms) in seconds.
    assert r["duration_sec"] == 0.014, r["duration_sec"]


def test_pass_without_sample_is_flagged():
    events = [e for e in passing_events() if e["event"] != "sample_received"]
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert r["verdict"] == "PASSED"
    assert r["conformant"] is False
    assert any("PASS without evidence" in v for v in r["violations"]), r["violations"]
    assert r["fidelity_evidenced"] == "none"


def test_pass_without_verify_pass_is_flagged():
    events = [e for e in passing_events() if e["event"] != "verify_pass"]
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert r["conformant"] is False
    assert any("verify_pass" in v for v in r["violations"]), r["violations"]


def test_leaked_lease_is_flagged():
    events = [e for e in passing_events() if e["event"] != "domain_released"]
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert any("leaked lease" in v for v in r["violations"]), r["violations"]
    assert r["conformant"] is False


def test_failed_run_verdict():
    events = [e for e in passing_events() if e["event"] not in ("run_pass",)]
    events.append({"run_id": "r1", "event": "run_fail", "wall_time_ms": 1015, "verify_exit_code": 1})
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert r["verdict"] == "FAILED"
    assert r["conformant"] is False
    # A FAILED run must NOT get a spurious "no terminal event" violation.
    assert not any("no terminal event" in v for v in r["violations"]), r["violations"]


def test_ready_timeout_is_aborted():
    events = passing_events()[:4]  # up to process_running, then timeout
    events.append({"run_id": "r1", "event": "ready_timeout", "wall_time_ms": 2000})
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert r["verdict"] == "ABORTED"
    assert r["conformant"] is False


def test_incomplete_when_no_terminal():
    events = passing_events()[:6]  # stops after 'ready', no terminal/verify event
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert r["verdict"] == "INCOMPLETE"
    assert r["terminal_source"] is None
    assert any("no terminal event" in v for v in r["violations"]), r["violations"]


def test_coordinator_terminal_source():
    # A full coordinator run takes the coordinator verdict.
    p = write_trace(passing_events())
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert r["verdict"] == "PASSED"
    assert r["terminal_source"] == "coordinator"


def test_verifier_terminal_when_no_coordinator():
    # Raw-binary run: no run_pass, but the independent verifier concluded with
    # verify_pass -> accept it as a verifier-derived terminal (not truncation).
    events = [e for e in passing_events() if e["event"] != "run_pass"]
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert r["verdict"] == "PASSED", r
    assert r["terminal_source"] == "verifier"
    assert r["conformant"] is True, r["violations"]
    assert not any("no terminal event" in v for v in r["violations"]), r["violations"]


def test_verify_fail_is_verifier_terminal_failed():
    # verify_fail with no coordinator terminal -> FAILED (verifier-derived).
    events = [e for e in passing_events() if e["event"] != "run_pass"]
    for e in events:
        if e["event"] == "verify_pass":
            e["event"] = "verify_fail"
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert r["verdict"] == "FAILED", r
    assert r["terminal_source"] == "verifier"
    assert r["conformant"] is False


def test_domain_inconsistency_is_flagged():
    events = passing_events()
    # verify_start on a different domain than run_start/leased.
    for e in events:
        if e["event"] == "verify_start":
            e["domain_id"] = 91
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert any("domain inconsistency" in v for v in r["violations"]), r["violations"]
    assert r["conformant"] is False


def test_out_of_order_lifecycle_is_flagged():
    # Put verify_invoke BEFORE ready by reordering the list.
    events = passing_events()
    ready = next(e for e in events if e["event"] == "ready")
    verify = next(e for e in events if e["event"] == "verify_invoke")
    events.remove(verify)
    events.insert(events.index(ready), verify)  # verify_invoke now precedes ready
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert any("out-of-order" in v for v in r["violations"]), r["violations"]


def test_parse_skips_non_json_and_non_event_lines():
    p = write_trace(
        passing_events(),
        extra_lines=["not json at all", json.dumps({"note": "no event key"}), ""],
    )
    events, err = ev.parse_trace(p)
    assert err is None
    # Only the real events survive; the 3 junk lines are dropped.
    assert all("event" in e for e in events)
    r = ev.evaluate_run(events, str(p))
    assert r["conformant"] is True, r["violations"]


def test_fleet_all_pass_via_directory():
    d = Path(tempfile.mkdtemp(prefix="sils-conf-fleet-"))
    for rid, iid in (("r1", "wheelspeed-FL"), ("r2", "wheelspeed-FR")):
        (d / f"{rid}.trace.jsonl").write_text(
            "\n".join(json.dumps(e) for e in passing_events(run_id=rid, instance_id=iid)) + "\n"
        )
    paths = ev.collect_trace_paths([str(d)])
    assert len(paths) == 2
    results = [ev.evaluate_run(ev.parse_trace(p)[0], str(p)) for p in paths]
    fleet = ev.evaluate_fleet(results, None)
    assert fleet["fleet_verdict"] == "PASS", fleet
    assert fleet["runs_conformant"] == 2


def test_fleet_completeness_missing_instance():
    results = [ev.evaluate_run(ev.parse_trace(write_trace(passing_events(instance_id="wheelspeed-FL")))[0], "x")]
    fleet = ev.evaluate_fleet(results, expected_instance_ids=["wheelspeed-FL", "wheelspeed-FR"])
    assert fleet["fleet_verdict"] == "FAIL"
    assert any("missing instance" in v and "wheelspeed-FR" in v for v in fleet["fleet_violations"]), fleet


def lockstep_events(seqs=(1, 2, 3), dt=1_000_000, sims=None, complete=None,
                    barrier=None, timeout=None, run_id="lk", instance_id="viss-probe-1"):
    """A PASSED lockstep run: N TickGrants (sim advancing by dt) each closed by a
    tick_complete + tick_barrier, plus the usual L0 evidence and run_pass."""
    if sims is None:
        sims = [s * dt for s in seqs]
    complete = list(seqs) if complete is None else complete
    barrier = list(seqs) if barrier is None else barrier
    evs = [
        {"run_id": run_id, "event": "run_start", "wall_time_ms": 1000, "domain_id": 90,
         "backend": "local", "instance_id": instance_id},
        {"run_id": run_id, "instance_id": instance_id, "event": "ready", "wall_time_ms": 1002},
        {"run_id": run_id, "event": "clock_start", "wall_time_ms": 1003, "mode": "lockstep"},
    ]
    wt = 1010
    for i, s in enumerate(seqs):
        evs.append({"run_id": run_id, "event": "tick_grant", "wall_time_ms": wt,
                    "tick_seq": s, "sim_time_ns": sims[i], "dt_ns": dt}); wt += 1
        evs.append({"run_id": run_id, "instance_id": instance_id, "event": "publish_intent",
                    "wall_time_ms": wt, "tick_seq": s, "sim_time_ns": sims[i]}); wt += 1
        if s in complete:
            evs.append({"run_id": run_id, "instance_id": instance_id, "event": "tick_complete",
                        "wall_time_ms": wt, "tick_seq": s}); wt += 1
        if s in barrier:
            evs.append({"run_id": run_id, "event": "tick_barrier", "wall_time_ms": wt, "tick_seq": s}); wt += 1
    if timeout is not None:
        evs.append({"run_id": run_id, "event": "tick_timeout", "wall_time_ms": wt, "tick_seq": timeout}); wt += 1
    evs += [
        {"run_id": run_id, "instance_id": instance_id, "event": "endpoint_matched", "wall_time_ms": wt},
        {"run_id": run_id, "expected_instance_id": instance_id, "event": "sample_received", "wall_time_ms": wt + 1},
        {"run_id": run_id, "expected_instance_id": instance_id, "event": "verify_pass", "wall_time_ms": wt + 2},
        {"run_id": run_id, "event": "run_pass", "wall_time_ms": wt + 3},
    ]
    return evs


def staging_events(received=(1, 2, 3, 4, 5), freezes=((1, [1, 2]), (2, [3, 4]), (3, [5])),
                   dropped=(), replay=False, run_id="stg", instance_id="viss-probe-1"):
    """A PASSED run with TickInbox staging events: inputs received in order, then
    frozen per tick. `freezes` = list of (tick_seq, [recv_seqs]). replay=True marks
    it as tick-indexed replay (§7.4)."""
    evs = [
        {"run_id": run_id, "event": "run_start", "wall_time_ms": 1000, "domain_id": 90,
         "backend": "local", "instance_id": instance_id},
        {"run_id": run_id, "instance_id": instance_id, "event": "ready", "wall_time_ms": 1001},
    ]
    if replay:
        evs.append({"run_id": run_id, "instance_id": instance_id, "event": "replay_start",
                    "wall_time_ms": 1001, "file": "replay.txt"})
    wt = 1002
    for rs in received:
        evs.append({"run_id": run_id, "instance_id": instance_id, "event": "input_received",
                    "wall_time_ms": wt, "recv_seq": rs}); wt += 1
    for d in dropped:
        evs.append({"run_id": run_id, "instance_id": instance_id, "event": "input_dropped",
                    "wall_time_ms": wt, "recv_seq": d}); wt += 1
    for tick_seq, rs_list in freezes:
        evs.append({"run_id": run_id, "instance_id": instance_id, "event": "tick_input_frozen",
                    "wall_time_ms": wt, "tick_seq": tick_seq, "recv_seqs": rs_list,
                    "count": len(rs_list)}); wt += 1
    evs += [
        {"run_id": run_id, "instance_id": instance_id, "event": "endpoint_matched", "wall_time_ms": wt},
        {"run_id": run_id, "expected_instance_id": instance_id, "event": "sample_received", "wall_time_ms": wt + 1},
        {"run_id": run_id, "expected_instance_id": instance_id, "event": "verify_pass", "wall_time_ms": wt + 2},
        {"run_id": run_id, "event": "run_pass", "wall_time_ms": wt + 3},
    ]
    return evs


def _stg(events):
    return ev.evaluate_run(ev.parse_trace(write_trace(events))[0], "x")


def test_no_staging_when_absent():
    r = ev.evaluate_run(ev.parse_trace(write_trace(passing_events()))[0], "x")
    assert r["staging"]["input_mode"] == "none"


def test_staging_deterministic_is_conformant():
    r = _stg(staging_events())
    assert r["staging"]["input_mode"] == "local-staged"
    assert r["staging"]["inputs_received"] == 5
    assert r["staging"]["inputs_staged"] == 5
    assert r["staging"]["deterministic"] is True, r["violations"]
    assert r["conformant"] is True


def test_staging_duplicate_consumption_flagged():
    r = _stg(staging_events(freezes=((1, [1, 2]), (2, [3, 4]), (3, [4, 5]))))  # 4 twice
    assert any("more than one tick" in v for v in r["violations"]), r["violations"]
    assert r["staging"]["deterministic"] is False


def test_staging_out_of_reception_order_flagged():
    r = _stg(staging_events(freezes=((1, [3, 4]), (2, [1, 2]), (3, [5]))))
    assert any("reception order" in v for v in r["violations"]), r["violations"]


def test_staging_phantom_input_flagged():
    r = _stg(staging_events(freezes=((1, [1, 2]), (2, [3, 4]), (3, [5, 9]))))  # 9 never received
    assert any("never-received" in v for v in r["violations"]), r["violations"]


def test_staging_lost_input_flagged():
    # received 6, never frozen, not dropped. Loss is a completeness/policy issue
    # (§7.3), NOT an order-determinism break: order stays deterministic, but the
    # run is still non-conformant and reports inputs_unstaged.
    r = _stg(staging_events(received=(1, 2, 3, 4, 5, 6)))
    assert any("never staged into a tick nor dropped" in v for v in r["violations"]), r["violations"]
    assert r["staging"]["inputs_unstaged"] == 1
    assert r["staging"]["deterministic"] is True, "order preserved -> still deterministic"
    assert r["conformant"] is False, "but loss still fails conformance"


def test_staging_dropped_input_ok():
    # 6 received but explicitly dropped -> no "lost" violation
    r = _stg(staging_events(received=(1, 2, 3, 4, 5, 6), dropped=(6,)))
    assert r["staging"]["inputs_dropped"] == 1
    assert not any("never staged" in v for v in r["violations"]), r["violations"]
    assert r["staging"]["deterministic"] is True


def test_replay_mode_detected():
    r = _stg(staging_events(replay=True))
    assert r["staging"]["input_mode"] == "replay"
    assert r["staging"]["deterministic"] is True
    assert r["conformant"] is True


def test_behavior_none_when_absent():
    r = ev.evaluate_run(ev.parse_trace(write_trace(passing_events()))[0], "x")
    assert r["behavior"]["name"] == "none"
    assert r["behavior"]["outputs"] == 0


def test_behavior_reported_and_conformant():
    evs = passing_events()
    evs.insert(1, {"run_id": "r1", "event": "behavior_start", "wall_time_ms": 1001, "name": "accumulate"})
    evs.insert(2, {"run_id": "r1", "instance_id": "viss-probe-1", "event": "behavior_output",
                   "wall_time_ms": 1002, "behavior": "accumulate", "tick_seq": 1, "value": 3})
    r = ev.evaluate_run(ev.parse_trace(write_trace(evs))[0], "x")
    assert r["behavior"]["name"] == "accumulate"
    assert r["behavior"]["outputs"] == 1
    assert r["conformant"] is True


def test_behavior_nonnumeric_value_flagged():
    evs = passing_events()
    evs.insert(1, {"run_id": "r1", "event": "behavior_output", "wall_time_ms": 1002,
                   "behavior": "x", "tick_seq": 1, "value": "not-a-number"})
    r = ev.evaluate_run(ev.parse_trace(write_trace(evs))[0], "x")
    assert any("without a numeric value" in v for v in r["violations"]), r["violations"]


def test_wall_run_reports_wall_clock():
    r = ev.evaluate_run(ev.parse_trace(write_trace(passing_events()))[0], "x")
    assert r["clock"]["mode"] == "wall"
    assert r["clock"]["deterministic"] is False
    assert r["conformant"] is True  # wall is fine, just not deterministic


def test_lockstep_deterministic_is_conformant():
    r = ev.evaluate_run(ev.parse_trace(write_trace(lockstep_events()))[0], "x")
    assert r["verdict"] == "PASSED", r
    assert r["clock"]["mode"] == "lockstep"
    assert r["clock"]["ticks"] == 3
    assert r["clock"]["deterministic"] is True, r["violations"]
    assert r["conformant"] is True


def test_lockstep_sequence_gap_flagged():
    r = ev.evaluate_run(ev.parse_trace(write_trace(lockstep_events(seqs=(1, 2, 4))))[0], "x")
    assert r["clock"]["deterministic"] is False
    assert any("tick sequence not 1..N" in v for v in r["violations"]), r["violations"]
    assert r["conformant"] is False


def test_lockstep_nonconstant_dt_flagged():
    # sim_time jumps that aren't a constant step
    r = ev.evaluate_run(ev.parse_trace(write_trace(
        lockstep_events(seqs=(1, 2, 3), sims=[1_000_000, 2_000_000, 5_000_000])))[0], "x")
    assert any("constant dt" in v for v in r["violations"]), r["violations"]


def test_lockstep_open_barrier_flagged():
    # tick 3 granted but never completed/barriered
    r = ev.evaluate_run(ev.parse_trace(write_trace(
        lockstep_events(seqs=(1, 2, 3), complete=[1, 2], barrier=[1, 2])))[0], "x")
    assert any("tick 3 granted but never completed" in v for v in r["violations"]), r["violations"]
    assert r["clock"]["deterministic"] is False


def test_lockstep_timeout_flagged():
    r = ev.evaluate_run(ev.parse_trace(write_trace(lockstep_events(timeout=2)))[0], "x")
    assert any("tick_timeout" in v for v in r["violations"]), r["violations"]


def plant_events(mode="live", a=0.5, Kp=0.8, sp=10.0, dt_ns=100_000_000, ticks=6,
                 break_loop_at=None, nonfinite_at=None, dts=None, model="first-order",
                 fmi_version=None):
    """A closed-loop run: first-order plant x += (u - a*x)*dt driven by a P-controller
    u = Kp*(sp - x). feedback_in[N] == output[N-1] by construction (loop is closed)."""
    dt = dt_ns / 1e9
    reset = {"run_id": "pl", "event": "plant_reset", "wall_time_ms": 1004, "model": model,
             "mode": mode, "initial": 0.0}
    if fmi_version:
        reset["fmi_version"] = fmi_version
    ev_list = [
        {"run_id": "pl", "event": "domain_leased", "wall_time_ms": 1000, "domain_id": 90, "pool": "90-92"},
        {"run_id": "pl", "instance_id": "viss-probe-1", "event": "run_start", "wall_time_ms": 1001, "domain_id": 90},
        {"run_id": "pl", "instance_id": "viss-probe-1", "event": "endpoint_matched", "wall_time_ms": 1002},
        {"run_id": "pl", "instance_id": "viss-probe-1", "event": "ready", "wall_time_ms": 1003},
        reset,
        {"run_id": "pl", "event": "verify_invoke", "wall_time_ms": 1005},
        {"run_id": "pl", "expected_instance_id": "viss-probe-1", "event": "verify_start", "wall_time_ms": 1006, "domain_id": 90},
        {"run_id": "pl", "expected_instance_id": "viss-probe-1", "event": "endpoint_matched", "wall_time_ms": 1007},
    ]
    x = 0.0
    wt = 1010
    for n in range(1, ticks + 1):
        fb = x
        u = Kp * (sp - fb)
        x = x + (u - a * x) * dt
        out = x
        this_dt = dts[n - 1] if dts else dt_ns
        if break_loop_at == n:
            fb = fb + 1.0  # controller did NOT consume the live previous output
        if nonfinite_at == n:
            out = float("nan")
        ev_list.append({"run_id": "pl", "event": "plant_step", "wall_time_ms": wt, "tick_seq": n,
                        "sim_time_ns": n * dt_ns, "dt_ns": this_dt,
                        "feedback_in": round(fb, 9), "command": round(u, 9),
                        "output": (out if out != out else round(out, 9))})
        ev_list.append({"run_id": "pl", "expected_instance_id": "viss-probe-1", "event": "sample_received", "wall_time_ms": wt + 1, "received_count": n})
        wt += 5
    ev_list.append({"run_id": "pl", "expected_instance_id": "viss-probe-1", "event": "verify_pass", "wall_time_ms": wt, "received_count": ticks, "valid_count": ticks, "min_samples": 1})
    ev_list.append({"run_id": "pl", "event": "domain_released", "wall_time_ms": wt + 1})
    return ev_list


def test_plant_none_when_absent():
    r = ev.evaluate_run(ev.parse_trace(write_trace(passing_events()))[0], "x")
    assert r["plant"] == {"model": "none"}
    assert r["fidelity_evidenced"] == "L0"


def test_plant_closed_loop_is_l3_conformant():
    r = ev.evaluate_run(ev.parse_trace(write_trace(plant_events()))[0], "x")
    assert r["verdict"] == "PASSED", r
    assert r["plant"]["model"] == "first-order"
    assert r["plant"]["mode"] == "live"
    assert r["plant"]["closed_loop"] is True, r["violations"]
    assert r["plant"]["deterministic"] is True
    assert r["fidelity_evidenced"] == "L3", r
    assert r["conformant"] is True, r["violations"]


def test_plant_open_loop_flagged():
    # feedback_in at some tick != previous output -> loop is open, not L3.
    r = ev.evaluate_run(ev.parse_trace(write_trace(plant_events(break_loop_at=4)))[0], "x")
    assert r["plant"]["closed_loop"] is False
    assert r["fidelity_evidenced"] != "L3"
    assert any("loop not closed" in v for v in r["violations"]), r["violations"]
    assert r["conformant"] is False


def test_plant_replay_mode_not_l3():
    # replay plant plays back outputs -- deterministic, but not a live coupled loop.
    r = ev.evaluate_run(ev.parse_trace(write_trace(plant_events(mode="replay")))[0], "x")
    assert r["plant"]["mode"] == "replay"
    assert r["plant"]["closed_loop"] is False
    assert r["fidelity_evidenced"] != "L3"


def test_plant_nonfinite_output_flagged():
    r = ev.evaluate_run(ev.parse_trace(write_trace(plant_events(nonfinite_at=3)))[0], "x")
    assert any("non-finite" in v for v in r["violations"]), r["violations"]
    assert r["plant"]["deterministic"] is False
    assert r["fidelity_evidenced"] != "L3"


def test_plant_fmu_is_l3_with_metadata():
    # An FMU-backed plant emits the SAME plant_step contract, so it is judged L3 by
    # analyze_plant unchanged; the FMI metadata is surfaced for the report.
    r = ev.evaluate_run(ev.parse_trace(write_trace(
        plant_events(model="BouncingBall", fmi_version="3.0")))[0], "x")
    assert r["verdict"] == "PASSED", r
    assert r["plant"]["model"] == "BouncingBall"
    assert r["plant"]["fmi_version"] == "3.0"
    assert r["plant"]["closed_loop"] is True
    assert r["fidelity_evidenced"] == "L3", r


def test_plant_nonconstant_dt_flagged():
    r = ev.evaluate_run(ev.parse_trace(write_trace(
        plant_events(ticks=3, dts=[100_000_000, 100_000_000, 200_000_000])))[0], "x")
    assert any("dt not constant" in v for v in r["violations"]), r["violations"]
    assert r["plant"]["deterministic"] is False


def l1_events(pairs=5, error_at=None, drop_response=None, phantom=False,
              duplicate=None, bad_status=None, error_no_code=False, qos_reliability="RELIABLE",
              qos_incompatible=False):
    """A request/response protocol run (§11 L1). Each request_sent gets a correlated
    response_received unless perturbed."""
    ev_list = [
        {"run_id": "l1", "event": "domain_leased", "wall_time_ms": 1000, "domain_id": 90, "pool": "90-92"},
        {"run_id": "l1", "instance_id": "viss-probe-1", "event": "run_start", "wall_time_ms": 1001, "domain_id": 90},
        {"run_id": "l1", "instance_id": "viss-probe-1", "event": "endpoint_matched", "wall_time_ms": 1002},
        {"run_id": "l1", "event": "qos_negotiated", "wall_time_ms": 1003, "role": "probe-writer",
         "reliability": qos_reliability, "durability": "TRANSIENT_LOCAL"},
        {"run_id": "l1", "instance_id": "viss-probe-1", "event": "ready", "wall_time_ms": 1004},
        {"run_id": "l1", "event": "verify_invoke", "wall_time_ms": 1005},
        {"run_id": "l1", "expected_instance_id": "viss-probe-1", "event": "verify_start", "wall_time_ms": 1006, "domain_id": 90},
        {"run_id": "l1", "expected_instance_id": "viss-probe-1", "event": "endpoint_matched", "wall_time_ms": 1007},
    ]
    if qos_incompatible:
        ev_list.append({"run_id": "l1", "event": "qos_incompatible", "wall_time_ms": 1008,
                        "role": "probe-writer", "reason": "durability RxO"})
    wt = 1010
    for n in range(1, pairs + 1):
        rid = f"sils-{n}"
        ev_list.append({"run_id": "l1", "event": "request_sent", "wall_time_ms": wt,
                        "request_id": rid, "path": "Vehicle.Speed", "action": "get"})
        if drop_response != n:
            st = bad_status if (bad_status and n == pairs) else ("error" if error_at == n else "ok")
            r = {"run_id": "l1", "event": "response_received", "wall_time_ms": wt + 2,
                 "request_id": rid, "status": st}
            if st == "error" and not error_no_code:
                r["error_code"] = "UNKNOWN_PATH"
            ev_list.append(r)
            if duplicate == n:  # emit a second response for the same id
                ev_list.append(dict(r, wall_time_ms=wt + 3))
        ev_list.append({"run_id": "l1", "expected_instance_id": "viss-probe-1",
                        "event": "sample_received", "wall_time_ms": wt + 4, "received_count": n})
        wt += 6
    if phantom:
        ev_list.append({"run_id": "l1", "event": "response_received", "wall_time_ms": wt,
                        "request_id": "sils-999", "status": "ok"})
    ev_list.append({"run_id": "l1", "expected_instance_id": "viss-probe-1", "event": "verify_pass",
                    "wall_time_ms": wt + 1, "received_count": pairs, "valid_count": pairs, "min_samples": 1})
    ev_list.append({"run_id": "l1", "event": "domain_released", "wall_time_ms": wt + 2})
    return ev_list


def test_protocol_none_when_absent():
    r = ev.evaluate_run(ev.parse_trace(write_trace(passing_events()))[0], "x")
    assert r["protocol"] == {"mode": "none"}


def test_protocol_reqresp_is_l1_conformant():
    r = ev.evaluate_run(ev.parse_trace(write_trace(l1_events(error_at=5)))[0], "x")
    assert r["verdict"] == "PASSED", r
    assert r["protocol"]["conformant"] is True, r["violations"]
    assert r["protocol"]["matched"] == 5
    assert r["protocol"]["errors"] == 1  # well-formed error counts, not a violation
    assert r["protocol"]["qos_conformant"] is True
    assert r["fidelity_evidenced"] == "L1", r
    assert r["conformant"] is True, r["violations"]


def test_protocol_unanswered_request_flagged():
    r = ev.evaluate_run(ev.parse_trace(write_trace(l1_events(drop_response=3)))[0], "x")
    assert r["protocol"]["unanswered"] == 1
    assert r["fidelity_evidenced"] != "L1"
    assert any("unanswered" in v for v in r["violations"]), r["violations"]


def test_protocol_phantom_response_flagged():
    r = ev.evaluate_run(ev.parse_trace(write_trace(l1_events(phantom=True)))[0], "x")
    assert r["protocol"]["phantom"] == 1
    assert any("no matching request" in v for v in r["violations"]), r["violations"]


def test_protocol_duplicate_response_flagged():
    r = ev.evaluate_run(ev.parse_trace(write_trace(l1_events(duplicate=2)))[0], "x")
    assert r["protocol"]["duplicated"] == 1
    assert any("more than one response" in v for v in r["violations"]), r["violations"]


def test_protocol_bad_status_and_illformed_error_flagged():
    r = ev.evaluate_run(ev.parse_trace(write_trace(l1_events(bad_status="weird")))[0], "x")
    assert any("invalid status" in v for v in r["violations"]), r["violations"]
    r2 = ev.evaluate_run(ev.parse_trace(write_trace(l1_events(error_at=2, error_no_code=True)))[0], "x")
    assert any("no error_code" in v for v in r2["violations"]), r2["violations"]


def test_protocol_qos_violations_flagged():
    r = ev.evaluate_run(ev.parse_trace(write_trace(l1_events(qos_reliability="BEST_EFFORT")))[0], "x")
    assert r["protocol"]["qos_conformant"] is False
    assert any("reliability" in v for v in r["violations"]), r["violations"]
    r2 = ev.evaluate_run(ev.parse_trace(write_trace(l1_events(qos_incompatible=True)))[0], "x")
    assert r2["protocol"]["qos_conformant"] is False
    assert any("QoS incompatible" in v for v in r2["violations"]), r2["violations"]


def test_protocol_l3_takes_precedence_over_l1():
    # A run that shows BOTH a conformant protocol and a live closed plant loop
    # reports the higher level (L3).
    combined = plant_events() + [
        {"run_id": "pl", "event": "request_sent", "wall_time_ms": 1500, "request_id": "r1", "path": "p", "action": "get"},
        {"run_id": "pl", "event": "response_received", "wall_time_ms": 1502, "request_id": "r1", "status": "ok"},
    ]
    r = ev.evaluate_run(ev.parse_trace(write_trace(combined))[0], "x")
    assert r["protocol"]["conformant"] is True
    assert r["plant"]["closed_loop"] is True
    assert r["fidelity_evidenced"] == "L3", r


def test_fleet_completeness_unexpected_instance():
    results = [ev.evaluate_run(ev.parse_trace(write_trace(passing_events(instance_id="rogue")))[0], "x")]
    fleet = ev.evaluate_fleet(results, expected_instance_ids=["wheelspeed-FL"])
    assert fleet["fleet_verdict"] == "FAIL"
    assert any("unexpected instance" in v and "rogue" in v for v in fleet["fleet_violations"]), fleet
    assert any("missing instance" in v and "wheelspeed-FL" in v for v in fleet["fleet_violations"]), fleet


def test_main_exit_codes():
    # Passing single run -> exit 0.
    p = write_trace(passing_events())
    assert ev.main([str(p), "--quiet"]) == ev.EXIT_PASS
    # Failing run -> exit 1.
    events = [e for e in passing_events() if e["event"] != "run_pass"]
    events.append({"run_id": "r1", "event": "run_fail", "wall_time_ms": 1015, "verify_exit_code": 1})
    p2 = write_trace(events)
    assert ev.main([str(p2), "--quiet"]) == ev.EXIT_FAIL
    # No such file -> usage error.
    assert ev.main(["/nonexistent/nope.trace.jsonl", "--quiet"]) == ev.EXIT_USAGE


def main() -> int:
    tests = [v for k, v in globals().items() if k.startswith("test_") and callable(v)]
    failed = 0
    for t in tests:
        try:
            t()
            print(f"PASS {t.__name__}")
        except AssertionError as e:
            failed += 1
            print(f"FAIL {t.__name__}: {e}")
        except Exception as e:  # noqa: BLE001
            failed += 1
            print(f"ERROR {t.__name__}: {type(e).__name__}: {e}")
    print(f"\n{len(tests) - failed}/{len(tests)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
