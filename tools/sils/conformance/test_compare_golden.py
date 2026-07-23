#!/usr/bin/env python3
"""Self-contained checks for compare_golden.py -- no pytest, just asserts.

Run: python3 tools/sils/conformance/test_compare_golden.py
"""

from __future__ import annotations

import copy
import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import compare_golden as cg  # noqa: E402


def golden():
    """A replay-style trace: run_start (baseline, not compared) + two publishes
    carrying a numeric 'speed', a received sample, and a pass."""
    return [
        {"event": "run_start", "wall_time_ms": 1000, "domain_id": 90, "run_id": "G"},
        {"event": "publish_intent", "seq": 0, "wall_time_ms": 1100, "speed": 10.0, "run_id": "G"},
        {"event": "publish_intent", "seq": 1, "wall_time_ms": 1200, "speed": 20.0, "run_id": "G"},
        {"event": "sample_received", "wall_time_ms": 1300, "run_id": "G"},
        {"event": "verify_pass", "wall_time_ms": 1400, "run_id": "G"},
        {"event": "run_pass", "wall_time_ms": 1500, "run_id": "G"},
    ]


def rules(**over):
    r = copy.deepcopy(cg.DEFAULT_RULES)
    r["input_mode"] = "replay"  # pass the §12.4 gate by default in unit tests
    for k, v in over.items():
        if isinstance(v, dict) and isinstance(r.get(k), dict):
            r[k] = {**r[k], **v}
        else:
            r[k] = v
    return r


def candidate(**mut):
    """A copy of golden with per-run identity changed (run_id=C) so ignore_fields
    is exercised; mut can override nothing here -- callers mutate the returned list."""
    c = copy.deepcopy(golden())
    for e in c:
        e["run_id"] = "C"
    return c


def test_identical_is_match():
    r = compare_result = cg.compare(golden(), candidate(), rules())
    assert r["verdict"] == "MATCH", r
    assert r["field_diffs"] == []


def test_run_id_difference_is_ignored():
    # candidate() already flips run_id to "C"; it must not register as a diff.
    r = cg.compare(golden(), candidate(), rules())
    assert r["verdict"] == "MATCH", r["field_diffs"]


def test_numeric_within_tolerance_matches():
    c = candidate()
    for e in c:
        if e["event"] == "publish_intent" and e["seq"] == 0:
            e["speed"] = 10.3
    r = cg.compare(golden(), c, rules(numeric_tolerance={"default": {"abs": 0.0, "rel": 0.0},
                                                          "fields": {"speed": {"abs": 0.5}}}))
    assert r["verdict"] == "MATCH", r["field_diffs"]


def test_numeric_beyond_tolerance_regresses():
    c = candidate()
    for e in c:
        if e["event"] == "publish_intent" and e["seq"] == 0:
            e["speed"] = 12.0
    r = cg.compare(golden(), c, rules(numeric_tolerance={"default": {"abs": 0.0, "rel": 0.0},
                                                         "fields": {"speed": {"abs": 0.5}}}))
    assert r["verdict"] == "REGRESSION"
    assert any(d["kind"] == "numeric" and d["field"] == "speed" for d in r["field_diffs"]), r["field_diffs"]


def test_no_tolerance_means_exact_numeric():
    c = candidate()
    for e in c:
        if e["event"] == "publish_intent" and e["seq"] == 0:
            e["speed"] = 10.0001
    r = cg.compare(golden(), c, rules())  # default tol 0/0 -> exact
    assert r["verdict"] == "REGRESSION", r


def test_value_diff_regresses():
    g = golden()
    g[1]["mode"] = "get"          # add a string field to golden pub#0
    c = candidate()
    c[1]["mode"] = "set"          # candidate differs
    r = cg.compare(g, c, rules())
    assert r["verdict"] == "REGRESSION"
    assert any(d["kind"] == "value" and d["field"] == "mode" for d in r["field_diffs"]), r["field_diffs"]


def test_missing_sample_policy_fail_then_warn_then_ignore():
    c = candidate()
    c = [e for e in c if not (e["event"] == "publish_intent" and e["seq"] == 1)]  # drop pub#1
    assert cg.compare(golden(), c, rules(sample_policy={"missing": "fail", "extra": "fail"}))["verdict"] == "REGRESSION"
    warn = cg.compare(golden(), c, rules(sample_policy={"missing": "warn", "extra": "fail"}))
    assert warn["verdict"] == "MATCH" and warn["warnings"], warn
    ign = cg.compare(golden(), c, rules(sample_policy={"missing": "ignore", "extra": "fail"}))
    assert ign["verdict"] == "MATCH" and not ign["warnings"], ign
    # The dropped event is still reported structurally regardless of policy.
    assert len(warn["missing_samples"]) == 1


def test_extra_sample_policy():
    c = candidate()
    c.append({"event": "publish_intent", "seq": 2, "wall_time_ms": 1250, "speed": 30.0, "run_id": "C"})
    assert cg.compare(golden(), c, rules(sample_policy={"missing": "fail", "extra": "fail"}))["verdict"] == "REGRESSION"
    ok = cg.compare(golden(), c, rules(sample_policy={"missing": "fail", "extra": "ignore"}))
    assert ok["verdict"] == "MATCH", ok
    assert len(ok["extra_samples"]) == 1


def test_latency_window():
    c = candidate()
    for e in c:  # shift every candidate timestamp +30ms; baseline shifts too, so offsets unchanged
        e["wall_time_ms"] += 30
    r_ok = cg.compare(golden(), c, rules(latency_window_ms=10))
    assert r_ok["verdict"] == "MATCH", r_ok["latency_violations"]

    c2 = candidate()
    for e in c2:  # delay ONLY sample_received by 100ms -> its offset drifts
        if e["event"] == "sample_received":
            e["wall_time_ms"] += 100
    r_bad = cg.compare(golden(), c2, rules(latency_window_ms=50))
    assert r_bad["verdict"] == "REGRESSION"
    assert len(r_bad["latency_violations"]) == 1, r_bad["latency_violations"]


def test_lockstep_skips_wall_latency():
    # §8: lockstep decouples sim-time from wall time, so wall-clock latency offsets
    # are not part of the contract -- sim_time_ns (a compared field) is. A lockstep
    # run whose events drift in wall time but match on sim_time_ns must still MATCH.
    ls = lambda rid, off: [
        {"event": "clock_start", "mode": "lockstep", "wall_time_ms": 1000 + off, "run_id": rid},
        {"event": "plant_step", "tick_seq": 1, "sim_time_ns": 100000000, "dt_ns": 100000000,
         "feedback_in": 0.0, "command": 8.0, "output": 0.8, "wall_time_ms": 1005 + off, "run_id": rid},
        {"event": "plant_step", "tick_seq": 2, "sim_time_ns": 200000000, "dt_ns": 100000000,
         "feedback_in": 0.8, "command": 7.36, "output": 1.496, "wall_time_ms": 1200 + off, "run_id": rid},
    ]
    g = ls("G", 0)
    c = ls("C", 300)  # +300ms wall drift on the whole run, > window -- but lockstep
    tol = rules(latency_window_ms=50,
                numeric_tolerance={"default": {"abs": 0.0, "rel": 0.0},
                                   "fields": {"output": {"abs": 1e-6}, "command": {"abs": 1e-6},
                                              "feedback_in": {"abs": 1e-6}}})
    r = cg.compare(g, c, tol)
    assert r["verdict"] == "MATCH", (r["latency_violations"], r["field_diffs"])
    assert r["latency_violations"] == []
    assert any("lockstep" in w for w in r["warnings"]), r["warnings"]

    # A mismatched sim_time_ns (the real timing contract) still regresses.
    c2 = ls("C", 0)
    c2[2]["sim_time_ns"] = 250000000  # tick 2 sim time drifted
    r2 = cg.compare(g, c2, tol)
    assert r2["verdict"] == "REGRESSION"
    assert any(d["field"] == "sim_time_ns" for d in r2["field_diffs"]), r2["field_diffs"]


def test_by_key_tolerates_intra_key_reorder_but_strict_catches_cross_key():
    # Move sample_received before the publishes (cross-key reorder).
    c = candidate()
    sample = next(e for e in c if e["event"] == "sample_received")
    c.remove(sample)
    c.insert(1, sample)  # right after run_start, before publish_intents
    # by_key: each key group still aligns positionally -> MATCH.
    assert cg.compare(golden(), c, rules(ordering="by_key"))["verdict"] == "MATCH"
    # strict: flat key sequence differs -> REGRESSION with ordering_diff.
    r = cg.compare(golden(), c, rules(ordering="strict"))
    assert r["verdict"] == "REGRESSION"
    assert r["ordering_diff"] is not None


def test_behavior_output_golden_scenario():
    # §11 L2: a behavior's per-tick value is golden-compared. Same values -> MATCH,
    # a drifted value -> REGRESSION (behavior_output is a compared event).
    bo = lambda rid, v2: [
        {"event": "behavior_output", "tick_seq": 1, "value": 10.0, "run_id": rid},
        {"event": "behavior_output", "tick_seq": 2, "value": v2, "run_id": rid},
    ]
    g = golden() + bo("G", 20.0)
    assert cg.compare(g, candidate() + bo("C", 20.0), rules())["verdict"] == "MATCH"
    r = cg.compare(g, candidate() + bo("C", 99.0), rules())
    assert r["verdict"] == "REGRESSION"
    assert any(d["field"] == "value" for d in r["field_diffs"]), r["field_diffs"]


def test_plant_step_model_correlation():
    # §9 L3: a plant's per-tick output is golden-compared under field tolerance.
    # Drift within tolerance -> MATCH; beyond -> REGRESSION (model correlation).
    ps = lambda rid, out2: [
        {"event": "plant_step", "tick_seq": 1, "dt_ns": 100_000_000,
         "feedback_in": 0.0, "command": 8.0, "output": 0.76, "run_id": rid},
        {"event": "plant_step", "tick_seq": 2, "dt_ns": 100_000_000,
         "feedback_in": 0.76, "command": 7.392, "output": out2, "run_id": rid},
    ]
    tol = rules(numeric_tolerance={"default": {"abs": 0.0, "rel": 0.0},
                                   "fields": {"output": {"abs": 1e-6}, "command": {"abs": 1e-6},
                                              "feedback_in": {"abs": 1e-6}}})
    g = golden() + ps("G", 1.4552)
    assert cg.compare(g, candidate() + ps("C", 1.4552 + 5e-7), tol)["verdict"] == "MATCH"
    r = cg.compare(g, candidate() + ps("C", 1.4552 + 0.01), tol)
    assert r["verdict"] == "REGRESSION"
    assert any(d["field"] == "output" for d in r["field_diffs"]), r["field_diffs"]


def test_input_mode_gate():
    assert cg.gate_input_mode(rules(input_mode="live")) is not None
    assert cg.gate_input_mode(rules(input_mode="local-staged")) is not None
    assert cg.gate_input_mode(rules(input_mode="replay")) is None
    assert cg.gate_input_mode(rules(input_mode="full-lockstep")) is None


def test_numeric_tolerance_none_from_empty_yaml():
    # YAML parses an empty `fields:`/`default:` as None; comparison must not crash
    # and must fall back to exact (0/0) tolerance.
    r = rules(numeric_tolerance={"default": None, "fields": None})
    c = candidate()
    c[1]["speed"] = 10.0001
    res = cg.compare(golden(), c, r)
    assert res["verdict"] == "REGRESSION"  # exact match required -> tiny drift fails
    # And an exact-equal candidate still matches without error.
    assert cg.compare(golden(), candidate(), r)["verdict"] == "MATCH"


def test_reference_rules_file_loads():
    # The shipped §12.4 rule set (config/sils/regression/golden-rules.yaml) must
    # load and compare without error despite its commented-out empty tolerance maps.
    ref = Path(__file__).resolve().parents[3] / "config/sils/regression/golden-rules.yaml"
    if not ref.exists():
        return  # tolerate running from a checkout without config/
    r, err = cg.load_rules(ref)
    assert err is None, err
    assert cg.compare(golden(), candidate(), r)["verdict"] == "MATCH"


def test_load_rules_merge_nested():
    d = Path(tempfile.mkdtemp(prefix="sils-cmp-rules-"))
    rp = d / "rules.yaml"
    rp.write_text(
        "input_mode: replay\n"
        "latency_window_ms: 25\n"
        "sample_policy:\n  missing: warn\n",  # only override 'missing'; 'extra' keeps default
        encoding="utf-8",
    )
    r, err = cg.load_rules(rp)
    assert err is None
    assert r["latency_window_ms"] == 25
    assert r["sample_policy"]["missing"] == "warn"
    assert r["sample_policy"]["extra"] == "fail"  # default preserved through merge


def _write(events, name):
    d = Path(tempfile.mkdtemp(prefix="sils-cmp-"))
    p = d / name
    p.write_text("\n".join(json.dumps(e) for e in events) + "\n", encoding="utf-8")
    return p


def test_main_exit_codes():
    import io
    from contextlib import redirect_stdout
    g = _write(golden(), "golden.jsonl")
    c_ok = _write(candidate(), "cand.jsonl")
    rp = _write([], "x")  # placeholder path reuse dir
    rules_path = rp.parent / "rules.yaml"
    rules_path.write_text("input_mode: replay\n", encoding="utf-8")

    buf = io.StringIO()
    with redirect_stdout(buf):
        assert cg.main(["--golden", str(g), "--candidate", str(c_ok),
                        "--rules", str(rules_path), "--quiet"]) == cg.EXIT_MATCH

    # regression
    c_bad_events = candidate()
    c_bad_events[1]["speed"] = 99.0
    c_bad = _write(c_bad_events, "cand2.jsonl")
    buf = io.StringIO()
    with redirect_stdout(buf):
        assert cg.main(["--golden", str(g), "--candidate", str(c_bad),
                        "--rules", str(rules_path), "--quiet"]) == cg.EXIT_REGRESSION

    # rejected input mode
    reject_rules = rp.parent / "reject.yaml"
    reject_rules.write_text("input_mode: live\n", encoding="utf-8")
    buf = io.StringIO()
    with redirect_stdout(buf):
        assert cg.main(["--golden", str(g), "--candidate", str(c_ok),
                        "--rules", str(reject_rules), "--quiet"]) == cg.EXIT_REJECT


def _veh(steer=0.05, accel=1.0, gear=3):
    """A minimal lockstep bus-coupled trace with two control_published ticks."""
    return [
        {"event": "clock_start", "mode": "lockstep", "wall_time_ms": 1000, "run_id": "G"},
        {"event": "control_published", "instance_id": "vi", "tick_seq": 1, "behavior": "vehicle-interface",
         "steering_angle": 0.0, "acceleration": -2.0, "gear": 3, "wall_time_ms": 1010, "run_id": "G"},
        {"event": "control_published", "instance_id": "vi", "tick_seq": 2, "behavior": "vehicle-interface",
         "steering_angle": steer, "acceleration": accel, "gear": gear, "wall_time_ms": 1020, "run_id": "G"},
    ]


def test_control_published_identical_is_match():
    g, c = _veh(), _veh()
    for e in c:
        e["run_id"] = "C"
    r = cg.compare(g, c, rules(input_mode="local-staged"))
    assert r["verdict"] == "MATCH", r


def test_control_published_value_diff_regresses():
    # A steering regression (0.05 -> 0.06) must surface under the default 0 tolerance.
    g = _veh()
    c = _veh(steer=0.06)
    for e in c:
        e["run_id"] = "C"
    r = cg.compare(g, c, rules(input_mode="local-staged"))
    assert r["verdict"] == "REGRESSION", r
    assert any(d["field"] == "steering_angle" and d["kind"] == "numeric" for d in r["field_diffs"]), r["field_diffs"]


def test_control_published_gear_diff_regresses():
    # gear is exact (a value diff), not tolerance-compared.
    g = _veh()
    c = _veh(gear=2)
    for e in c:
        e["run_id"] = "C"
    r = cg.compare(g, c, rules(input_mode="local-staged"))
    assert any(d["field"] == "gear" for d in r["field_diffs"]), r["field_diffs"]


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
