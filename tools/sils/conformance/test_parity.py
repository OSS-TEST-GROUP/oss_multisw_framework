#!/usr/bin/env python3
"""Self-contained checks for parity.py (L4 target parity) -- no pytest, just asserts.

Run: python3 tools/sils/conformance/test_parity.py
"""

from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import parity as p  # noqa: E402

CONTRACT = {
    "attestation": {"arch": "aarch64", "gateway": "real", "protocol": "real"},
    "compare_events": ["behavior_output", "plant_step", "response_received"],
    "key_fields": ["event", "tick_seq", "request_id"],
    "default_tolerance": {"abs": 0.0, "rel": 0.0},
    "signals": {"output": {"abs": 0.05, "rel": 0.005}, "command": {"abs": 0.05, "rel": 0.005}},
    "timing_window_ms": 10,
    "sample_policy": {"missing": "fail", "extra": "fail"},
}


def target_trace(outputs=(1.0, 2.0, 3.0)):
    evs = []
    for i, o in enumerate(outputs, start=1):
        evs.append({"event": "behavior_output", "tick_seq": i, "value": float(i)})
        evs.append({"event": "plant_step", "tick_seq": i, "output": o, "command": float(i)})
        evs.append({"event": "response_received", "request_id": f"sils-{i}", "status": "ok"})
    return evs


def sils_trace(outputs=(1.0, 2.0, 3.0), arch="aarch64", gateway="real", protocol="real",
               attest=True):
    evs = []
    if attest:
        evs.append({"event": "target_attestation", "arch": arch, "gateway": gateway,
                    "protocol": protocol})
    evs.extend(target_trace(outputs))
    return evs


def test_parity_pass_is_l4():
    r = p.judge(target_trace(), sils_trace(), CONTRACT)
    assert r["verdict"] == "PARITY", r
    assert r["fidelity_evidenced"] == "L4"
    assert r["values_match"] and r["attestation_ok"]


def test_within_tolerance_still_parity():
    # plant output off by 0.02 < abs 0.05 -> still parity
    r = p.judge(target_trace((1.0, 2.0, 3.0)), sils_trace((1.02, 1.98, 3.0)), CONTRACT)
    assert r["verdict"] == "PARITY", r["comparison"]["field_diffs"]


def test_value_divergence_is_below_l4():
    r = p.judge(target_trace((1.0, 2.0, 3.0)), sils_trace((1.0, 5.0, 3.0)), CONTRACT)
    assert r["verdict"] == "DIVERGENCE", r
    assert r["fidelity_evidenced"] == "below-L4"
    assert not r["values_match"]


def test_discrete_field_must_match_exactly():
    # response status differs -> divergence (default tolerance, not a signal)
    sils = sils_trace()
    for e in sils:
        if e.get("event") == "response_received" and e["request_id"] == "sils-2":
            e["status"] = "error"
    r = p.judge(target_trace(), sils, CONTRACT)
    assert r["verdict"] == "DIVERGENCE", r


def test_values_match_but_no_attestation_event():
    r = p.judge(target_trace(), sils_trace(attest=False), CONTRACT)
    assert r["verdict"] == "PARITY_UNATTESTED", r
    assert r["fidelity_evidenced"] == "L3"
    assert r["values_match"] and not r["attestation_ok"]
    assert any("no target_attestation" in f for f in r["attestation_failures"]), r


def test_values_match_but_wrong_arch():
    r = p.judge(target_trace(), sils_trace(arch="x86_64"), CONTRACT)
    assert r["verdict"] == "PARITY_UNATTESTED", r
    assert any("arch" in f and "aarch64" in f for f in r["attestation_failures"]), r


def test_values_match_but_stubbed_gateway():
    r = p.judge(target_trace(), sils_trace(gateway="sim"), CONTRACT)
    assert r["verdict"] == "PARITY_UNATTESTED", r
    assert any("gateway" in f for f in r["attestation_failures"]), r


def test_no_attestation_clause_rests_on_values():
    c = dict(CONTRACT)
    c.pop("attestation")
    r = p.judge(target_trace(), sils_trace(attest=False), c)
    assert r["verdict"] == "PARITY", r  # no clause -> values alone decide
    assert r["attestation_ok"]


def test_field_diffs_relabeled_target_sils():
    r = p.judge(target_trace((1.0, 2.0, 3.0)), sils_trace((1.0, 5.0, 3.0)), CONTRACT)
    d = r["comparison"]["field_diffs"][0]
    assert "target" in d and "sils" in d and "golden" not in d and "candidate" not in d, d


def test_contract_to_rules_maps_signals_and_timing():
    rules = p.contract_to_rules(CONTRACT)
    assert rules["compare_events"] == CONTRACT["compare_events"]
    assert rules["numeric_tolerance"]["fields"]["output"] == {"abs": 0.05, "rel": 0.005}
    assert rules["latency_window_ms"] == 10


def test_shipped_contract_parses_and_is_complete():
    repo = Path(__file__).resolve().parents[3]
    c, err = p.load_contract(repo / "config/sils/parity/viss-target-parity.yaml")
    assert err is None, err
    assert c.get("attestation", {}).get("arch") == "aarch64", c
    assert c.get("compare_events") and c.get("signals"), c
    # it must project onto valid compare rules without error
    rules = p.contract_to_rules(c)
    assert rules["compare_events"] and "output" in rules["numeric_tolerance"]["fields"]


def test_main_exit_codes():
    with tempfile.TemporaryDirectory() as d:
        import yaml
        dp = Path(d)
        (dp / "target.jsonl").write_text(
            "\n".join(json.dumps(e) for e in target_trace()) + "\n", encoding="utf-8")
        (dp / "sils_ok.jsonl").write_text(
            "\n".join(json.dumps(e) for e in sils_trace()) + "\n", encoding="utf-8")
        (dp / "sils_div.jsonl").write_text(
            "\n".join(json.dumps(e) for e in sils_trace((1.0, 9.0, 3.0))) + "\n", encoding="utf-8")
        (dp / "c.yaml").write_text(yaml.safe_dump(CONTRACT), encoding="utf-8")

        rc = p.main(["--target", str(dp / "target.jsonl"), "--sils", str(dp / "sils_ok.jsonl"),
                     "--contract", str(dp / "c.yaml"), "--quiet"])
        assert rc == p.EXIT_PARITY, rc
        rc = p.main(["--target", str(dp / "target.jsonl"), "--sils", str(dp / "sils_div.jsonl"),
                     "--contract", str(dp / "c.yaml"), "--quiet"])
        assert rc == p.EXIT_NO_PARITY, rc
        rc = p.main(["--target", str(dp / "target.jsonl"), "--sils", str(dp / "sils_ok.jsonl"),
                     "--contract", str(dp / "missing.yaml"), "--quiet"])
        assert rc == p.EXIT_ERROR, rc


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
    print(f"\n{len(tests) - failed}/{len(tests)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
