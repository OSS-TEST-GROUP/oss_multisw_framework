#!/usr/bin/env python3
"""Self-contained checks for capture_golden.py -- no pytest, just asserts.

Run: python3 tools/sils/conformance/test_capture_golden.py

The load-bearing test is the roundtrip: a captured golden must be consumable by
compare_golden against a fresh run with different identity and absolute timing.
"""

from __future__ import annotations

import copy
import io
import json
import sys
import tempfile
from contextlib import redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import capture_golden as capg  # noqa: E402
import compare_golden as cg  # noqa: E402
import evaluate as ev  # noqa: E402
from test_evaluate import passing_events, write_trace  # noqa: E402


def replay_rules(**over):
    r = copy.deepcopy(cg.DEFAULT_RULES)
    r["input_mode"] = "replay"
    r["latency_window_ms"] = 50
    for k, v in over.items():
        r[k] = v
    return r


def capture_to_file(source_events, extra_args=None):
    src = write_trace(source_events, name="src.trace.jsonl")
    out = src.parent / "golden.jsonl"
    argv = ["--trace", str(src), "--out", str(out), "--input-mode", "replay"]
    if extra_args:
        argv += extra_args
    rc = capg.main(argv)
    return rc, out


def test_capture_roundtrip_matches():
    # Capture a golden from a PASSED run...
    rc, gpath = capture_to_file(passing_events(run_id="CAP", instance_id="probe-cap"))
    assert rc == capg.EXIT_OK, "capture of a conformant run should succeed"

    golden, err = ev.parse_trace(gpath)
    assert err is None

    # ...then compare a FRESH run: different run_id/instance_id and +5000ms absolute
    # clock. Relative timing is identical, so it must MATCH.
    cand = passing_events(run_id="LIVE-999", instance_id="probe-live")
    for e in cand:
        e["wall_time_ms"] += 5000
    result = cg.compare(golden, cand, replay_rules())
    assert result["verdict"] == "MATCH", result
    assert result["latency_violations"] == [], result["latency_violations"]


def test_normalization_strips_identity_and_rebases_time():
    rc, gpath = capture_to_file(passing_events(run_id="CAP", instance_id="probe-cap"))
    assert rc == capg.EXIT_OK
    events, _ = ev.parse_trace(gpath)
    real = [e for e in events if e.get("event") != "golden_meta"]
    # per-run identity gone
    for e in real:
        for k in ("run_id", "instance_id", "expected_instance_id", "domain_id", "pid",
                  "container", "backend", "pool"):
            assert k not in e, f"{k} should be stripped from golden: {e}"
    # first timestamped event rebased to offset 0
    first_ts = next(e["wall_time_ms"] for e in real if "wall_time_ms" in e)
    assert first_ts == 0, f"expected offset-0 anchor, got {first_ts}"


def test_meta_event_present_and_custom_meta():
    rc, gpath = capture_to_file(passing_events(),
                                extra_args=["--meta", "catalog_version=0.1.0", "--meta", "image_digest=sha256:abc"])
    assert rc == capg.EXIT_OK
    events, _ = ev.parse_trace(gpath)
    meta = events[0]
    assert meta["event"] == "golden_meta"
    assert meta["input_mode"] == "replay"
    assert meta["source_verdict"] == "PASSED"
    assert meta["catalog_version"] == "0.1.0"
    assert meta["image_digest"] == "sha256:abc"
    # compare_golden must ignore golden_meta (it is not a compare_event).
    cand = passing_events()
    assert cg.compare(events, cand, replay_rules())["verdict"] == "MATCH"


def test_refuses_non_conformant_run():
    truncated = passing_events()[:6]  # no terminal event -> INCOMPLETE
    src = write_trace(truncated, name="trunc.trace.jsonl")
    out = src.parent / "g.jsonl"
    buf = io.StringIO()
    with redirect_stdout(buf):
        rc = capg.main(["--trace", str(src), "--out", str(out), "--input-mode", "replay"])
    assert rc == capg.EXIT_REFUSED, "must refuse to baseline a non-conformant run"
    assert not out.exists(), "no golden should be written on refusal"
    # --force overrides.
    with redirect_stdout(buf):
        rc2 = capg.main(["--trace", str(src), "--out", str(out), "--input-mode", "replay", "--force"])
    assert rc2 == capg.EXIT_OK and out.exists()


def test_refuses_bad_input_mode():
    src = write_trace(passing_events(), name="s.trace.jsonl")
    out = src.parent / "g.jsonl"
    buf = io.StringIO()
    with redirect_stdout(buf):
        rc = capg.main(["--trace", str(src), "--out", str(out), "--input-mode", "live"])
    assert rc == capg.EXIT_REFUSED, "live mode is not a valid golden target (§12.4)"
    with redirect_stdout(buf):
        assert capg.main(["--trace", str(src), "--out", str(out), "--input-mode", "live", "--force"]) == capg.EXIT_OK


def test_lean_keeps_only_compare_events_plus_anchor():
    rc, gpath = capture_to_file(passing_events(), extra_args=["--lean"])
    assert rc == capg.EXIT_OK
    events, _ = ev.parse_trace(gpath)
    real = [e for e in events if e.get("event") != "golden_meta"]
    compare_events = set(cg.DEFAULT_RULES["compare_events"])
    non_compare = [e for e in real if e["event"] not in compare_events]
    # At most one non-compare event survives: the timing anchor.
    assert len(non_compare) <= 1, [e["event"] for e in non_compare]
    # A lean golden must still roundtrip-match a full candidate.
    cand = passing_events(run_id="X")
    for e in cand:
        e["wall_time_ms"] += 100
    assert cg.compare(events, cand, replay_rules())["verdict"] == "MATCH"


def test_meta_parse_error():
    src = write_trace(passing_events(), name="s.trace.jsonl")
    buf = io.StringIO()
    with redirect_stdout(buf):
        rc = capg.main(["--trace", str(src), "--input-mode", "replay", "--meta", "novalue"])
    assert rc == capg.EXIT_USAGE


def test_missing_trace_is_usage_error():
    assert capg.main(["--trace", "/nope/x.jsonl", "--input-mode", "replay"]) == capg.EXIT_USAGE


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
