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
    events = passing_events()[:6]  # stops after 'ready', no terminal event
    p = write_trace(events)
    r = ev.evaluate_run(ev.parse_trace(p)[0], str(p))
    assert r["verdict"] == "INCOMPLETE"
    assert any("no terminal event" in v for v in r["violations"]), r["violations"]


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
                   dropped=(), run_id="stg", instance_id="viss-probe-1"):
    """A PASSED run with TickInbox staging events: inputs received in order, then
    frozen per tick. `freezes` = list of (tick_seq, [recv_seqs])."""
    evs = [
        {"run_id": run_id, "event": "run_start", "wall_time_ms": 1000, "domain_id": 90,
         "backend": "local", "instance_id": instance_id},
        {"run_id": run_id, "instance_id": instance_id, "event": "ready", "wall_time_ms": 1001},
    ]
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
