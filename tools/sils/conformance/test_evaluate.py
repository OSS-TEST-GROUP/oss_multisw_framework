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
