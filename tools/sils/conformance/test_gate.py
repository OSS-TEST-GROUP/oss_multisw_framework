#!/usr/bin/env python3
"""Tests for gate.py -- the single fail-closed conformance gate."""
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gate  # noqa: E402

HERE = Path(__file__).resolve().parent
NOMINAL = HERE / "samples" / "nominal.trace.jsonl"
VEHICLE_LOCKSTEP = HERE / "samples" / "vehicle-lockstep.trace.jsonl"
VEHICLE_REQUIREMENTS = HERE / "samples" / "vehicle-requirements.yaml"


def write_trace(events, name="run.trace.jsonl"):
    d = Path(tempfile.mkdtemp(prefix="sils-gate-test-"))
    p = d / name
    p.write_text("\n".join(json.dumps(e) for e in events) + "\n", encoding="utf-8")
    return p


class GateTest(unittest.TestCase):
    # --- the committed nominal sample is the positive fixture ---------------
    def test_nominal_sample_passes_gate(self):
        self.assertTrue(NOMINAL.exists(), f"missing sample: {NOMINAL}")
        rc = gate.main([str(NOMINAL)])
        self.assertEqual(rc, gate.EXIT_OK)

    # --- bus-coupled (Option 2, S4) lockstep sample is a positive fixture ---
    def test_vehicle_lockstep_sample_passes_gate(self):
        self.assertTrue(VEHICLE_LOCKSTEP.exists(), f"missing sample: {VEHICLE_LOCKSTEP}")
        rc = gate.main([str(VEHICLE_LOCKSTEP)])
        self.assertEqual(rc, gate.EXIT_OK)

    # --- functional requirements fold into the FIDELITY verdict (§3) --------
    def test_vehicle_lockstep_passes_gate_with_requirements(self):
        # The sample requirements are satisfied by the golden -> gate still PASSES.
        self.assertTrue(VEHICLE_REQUIREMENTS.exists(), f"missing: {VEHICLE_REQUIREMENTS}")
        rc = gate.main([str(VEHICLE_LOCKSTEP), "--requirements", str(VEHICLE_REQUIREMENTS)])
        self.assertEqual(rc, gate.EXIT_OK)

    def test_unmet_requirement_fails_gate(self):
        # A requirement the golden violates (accel is -2.0/1.0, so max_abs 0.5 fails) ->
        # gate FAILS even though the run is otherwise conformant.
        spec = write_trace([], name="req.yaml")  # reuse tmpdir helper for a file path
        spec.write_text(
            "requirements:\n"
            "  - id: X\n    name: too-tight\n"
            "    signal: control_published.acceleration\n"
            "    assert: {max_abs: 0.5}\n", encoding="utf-8")
        rc = gate.main([str(VEHICLE_LOCKSTEP), "--requirements", str(spec)])
        self.assertEqual(rc, gate.EXIT_GATE_FAIL)

    # --- schema stage is fail-closed ---------------------------------------
    def test_unknown_event_fails_schema_stage(self):
        # A well-formed-but-unknown event -> schema stage fails -> gate fails,
        # even though evaluate might otherwise reach a verdict.
        p = write_trace([{"run_id": "r", "event": "totally_made_up", "wall_time_ms": 1}])
        self.assertEqual(gate.main([str(p)]), gate.EXIT_GATE_FAIL)

    # --- fidelity stage is fail-closed -------------------------------------
    def test_truncated_trace_fails_fidelity_stage(self):
        # Schema-valid events but no terminal verdict -> INCOMPLETE -> gate fails.
        p = write_trace([
            {"run_id": "r", "event": "run_start", "wall_time_ms": 1, "domain_id": 90,
             "instance_id": "viss-probe-1"},
            {"run_id": "r", "event": "endpoint_matched", "wall_time_ms": 2, "matched_count": 1},
        ])
        self.assertEqual(gate.main([str(p)]), gate.EXIT_GATE_FAIL)

    # --- JUnit output ------------------------------------------------------
    def test_junit_output_written(self):
        out = Path(tempfile.mkdtemp(prefix="sils-gate-junit-")) / "out.xml"
        rc = gate.main([str(NOMINAL), "--junit", str(out)])
        self.assertEqual(rc, gate.EXIT_OK)
        self.assertTrue(out.exists())
        xml = out.read_text(encoding="utf-8")
        self.assertIn("<testsuite", xml)
        self.assertIn("nominal", xml)

    # --- usage errors ------------------------------------------------------
    def test_target_without_contract_is_usage_error(self):
        self.assertEqual(gate.main([str(NOMINAL), "--target", "x.jsonl"]), gate.EXIT_USAGE)

    def test_no_trace_found_is_usage_error(self):
        empty = Path(tempfile.mkdtemp(prefix="sils-gate-empty-"))
        self.assertEqual(gate.main([str(empty)]), gate.EXIT_USAGE)

    # --- fleet completeness via --resolved is threaded through -------------
    def test_resolved_completeness_missing_instance_fails(self):
        # nominal has instance viss-probe-1; a resolved-run expecting a second
        # instance -> fleet completeness violation -> gate fails.
        resolved = Path(tempfile.mkdtemp(prefix="sils-gate-res-")) / "resolved-run.yaml"
        resolved.write_text(
            "containers:\n"
            "  - instance_id: viss-probe-1\n"
            "  - instance_id: viss-probe-2\n", encoding="utf-8")
        rc = gate.main([str(NOMINAL), "--resolved", str(resolved)])
        self.assertEqual(rc, gate.EXIT_GATE_FAIL)


if __name__ == "__main__":
    unittest.main()
