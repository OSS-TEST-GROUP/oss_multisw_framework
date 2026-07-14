#!/usr/bin/env python3
"""Self-contained checks for to_junit.py -- no pytest, just asserts.

Run: python3 tools/sils/conformance/test_to_junit.py

Every test re-parses the emitted XML with ElementTree, which both proves the
output is well-formed (escaping, structure) and lets us assert on it.
"""

from __future__ import annotations

import io
import json
import sys
import tempfile
import xml.etree.ElementTree as ET
from contextlib import redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import evaluate as ev  # noqa: E402
import to_junit as tj  # noqa: E402
from test_evaluate import passing_events, write_trace  # noqa: E402


def fleet_from(events_list, expected_ids=None):
    results = []
    for i, evs in enumerate(events_list):
        p = write_trace(evs, name=f"r{i}.trace.jsonl")
        results.append(ev.evaluate_run(ev.parse_trace(p)[0], str(p)))
    return ev.evaluate_fleet(results, expected_ids)


def parse(xml: str) -> ET.Element:
    return ET.fromstring(xml)


def test_passing_run_has_no_failure():
    fleet = fleet_from([passing_events()])
    xml = tj.serialize(tj.build_junit(fleet, "s"))
    root = parse(xml)
    suite = root.find("testsuite")
    assert suite.get("tests") == "1"
    assert suite.get("failures") == "0"
    assert suite.get("errors") == "0"
    case = suite.find("testcase")
    assert case.get("classname") == tj.RUN_CLASS
    assert list(case) == [], "a clean pass must have no child elements"
    assert float(case.get("time")) == 0.014


def test_failed_run_emits_failure():
    events = [e for e in passing_events() if e["event"] != "run_pass"]
    events.append({"run_id": "r1", "event": "run_fail", "wall_time_ms": 1015, "verify_exit_code": 1})
    fleet = fleet_from([events])
    root = parse(tj.serialize(tj.build_junit(fleet, "s")))
    suite = root.find("testsuite")
    assert suite.get("failures") == "1"
    fail = suite.find("testcase/failure")
    assert fail is not None
    assert fail.get("type") == "verify"
    assert "FAILED" in fail.get("message")


def test_pass_with_violation_is_conformance_failure():
    events = [e for e in passing_events() if e["event"] != "sample_received"]
    fleet = fleet_from([events])
    root = parse(tj.serialize(tj.build_junit(fleet, "s")))
    fail = root.find("testsuite/testcase/failure")
    assert fail is not None and fail.get("type") == "conformance", ET.tostring(root, encoding="unicode")
    assert "PASS without evidence" in fail.text


def test_aborted_run_emits_error_not_failure():
    events = passing_events()[:4]
    events.append({"run_id": "r1", "event": "ready_timeout", "wall_time_ms": 2000})
    fleet = fleet_from([events])
    root = parse(tj.serialize(tj.build_junit(fleet, "s")))
    suite = root.find("testsuite")
    assert suite.get("errors") == "1"
    assert suite.get("failures") == "0"
    err = suite.find("testcase/error")
    assert err is not None and err.get("type") == "aborted"


def test_incomplete_run_emits_error():
    fleet = fleet_from([passing_events()[:6]])  # no terminal event
    root = parse(tj.serialize(tj.build_junit(fleet, "s")))
    err = root.find("testsuite/testcase/error")
    assert err is not None and err.get("type") == "incomplete"


def test_fleet_completeness_case_only_when_checked():
    # Not checked (no resolved) -> no fleet-completeness testcase.
    fleet = fleet_from([passing_events()])
    root = parse(tj.serialize(tj.build_junit(fleet, "s")))
    names = [c.get("name") for c in root.iter("testcase")]
    assert "fleet-completeness" not in names, names

    # Checked & missing an instance -> fleet-completeness case with a failure.
    fleet2 = fleet_from([passing_events(instance_id="wheelspeed-FL")],
                        expected_ids=["wheelspeed-FL", "wheelspeed-FR"])
    root2 = parse(tj.serialize(tj.build_junit(fleet2, "s")))
    fc = [c for c in root2.iter("testcase") if c.get("name") == "fleet-completeness"]
    assert len(fc) == 1
    fail = fc[0].find("failure")
    assert fail is not None and fail.get("type") == "completeness"
    assert "wheelspeed-FR" in fail.text
    # Suite totals include the fleet case.
    assert root2.find("testsuite").get("tests") == "2"


def test_multiple_runs_time_aggregation():
    fleet = fleet_from([passing_events(run_id="a"), passing_events(run_id="b")])
    root = parse(tj.serialize(tj.build_junit(fleet, "s")))
    suite = root.find("testsuite")
    assert suite.get("tests") == "2"
    # each run spans 0.014s -> suite time 0.028
    assert abs(float(suite.get("time")) - 0.028) < 1e-6, suite.get("time")


def test_xml_escapes_special_chars():
    # A run_id with XML-hostile characters must round-trip through parsing.
    events = passing_events(run_id='r&"<>1')
    events = [e for e in events if e["event"] != "run_pass"]
    events.append({"run_id": 'r&"<>1', "event": "run_fail", "wall_time_ms": 1015})
    fleet = fleet_from([events])
    xml = tj.serialize(tj.build_junit(fleet, "s"))
    root = parse(xml)  # would raise if escaping were wrong
    assert root.find("testsuite/testcase").get("name") == 'r&"<>1'


def test_main_from_verdict_stdin():
    fleet = fleet_from([passing_events()])
    buf = io.StringIO()
    stdin_backup = sys.stdin
    sys.stdin = io.StringIO(json.dumps(fleet))
    try:
        with redirect_stdout(buf):
            rc = tj.main(["--from-verdict", "-"])
    finally:
        sys.stdin = stdin_backup
    assert rc == tj.EXIT_OK
    root = parse(buf.getvalue())
    assert root.find("testsuite").get("tests") == "1"


def test_main_exit_code_flag():
    # A FAILED fleet: default exit 0 (report produced), --exit-code -> 1.
    events = [e for e in passing_events() if e["event"] != "run_pass"]
    events.append({"run_id": "r1", "event": "run_fail", "wall_time_ms": 1015})
    p = write_trace(events)
    buf = io.StringIO()
    with redirect_stdout(buf):
        assert tj.main([str(p)]) == tj.EXIT_OK
    buf = io.StringIO()
    with redirect_stdout(buf):
        assert tj.main([str(p), "--exit-code"]) == tj.EXIT_FAIL


def test_main_writes_output_file():
    p = write_trace(passing_events())
    out = Path(tempfile.mkdtemp(prefix="sils-junit-")) / "junit.xml"
    assert tj.main([str(p), "-o", str(out)]) == tj.EXIT_OK
    root = ET.parse(out).getroot()  # parses from disk
    assert root.tag == "testsuites"
    assert root.get("tests") == "1"


def test_main_rejects_both_inputs():
    p = write_trace(passing_events())
    assert tj.main([str(p), "--from-verdict", "-"]) == tj.EXIT_USAGE


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
