#!/usr/bin/env python3
"""Tests for validate_trace.py -- the trace-schema conformance checker."""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import validate_trace as vt  # noqa: E402


def kinds(violations):
    return sorted(v.kind for v in violations)


class ValidateTraceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.schema = vt.load_schema(vt.DEFAULT_SCHEMA)

    def run_lines(self, lines, **kw):
        numbered = list(enumerate(lines, start=1))
        return list(vt.validate_lines(numbered, self.schema, **kw))

    # --- schema itself -----------------------------------------------------
    def test_schema_loads_and_is_v1(self):
        self.assertEqual(self.schema["schema_version"], 1)
        self.assertIn("run_start", self.schema["events"])
        self.assertIn("plant_step", self.schema["events"])
        self.assertIn("wire_start", self.schema["events"])

    # --- well-formed events pass ------------------------------------------
    def test_valid_vecu_events_pass(self):
        lines = [
            '{"run_id":"r","event":"run_start","instance_id":"i","seq":0,"wall_time_ms":1,"topic":"t","domain_id":42}',
            '{"run_id":"r","event":"wire_start","instance_id":"i","seq":0,"wall_time_ms":1,"topic":"t","name":"viss-envelope","type":"VissRequestTopic"}',
            '{"run_id":"r","event":"behavior_output","instance_id":"i","behavior":"p-controller","tick_seq":3,"value":1.5}',
            '{"run_id":"r","event":"plant_step","instance_id":"i","seq":3,"wall_time_ms":1,"topic":"t","tick_seq":3,"sim_time_ns":0,"dt_ns":1,"feedback_in":0.0,"command":1.5,"output":0.7}',
        ]
        self.assertEqual(self.run_lines(lines), [])

    def test_extra_fields_allowed(self):
        # forward-compatible: unknown extra fields are fine.
        lines = ['{"run_id":"r","event":"ready","instance_id":"i","seq":0,"wall_time_ms":1,"topic":"t","future_field":123}']
        self.assertEqual(self.run_lines(lines), [])

    # --- violations --------------------------------------------------------
    def test_missing_required_field(self):
        # plant_step without 'output'
        lines = ['{"run_id":"r","event":"plant_step","instance_id":"i","seq":3,"wall_time_ms":1,"topic":"t","tick_seq":3,"sim_time_ns":0,"dt_ns":1,"feedback_in":0.0,"command":1.5}']
        self.assertEqual(kinds(self.run_lines(lines)), ["missing-field"])

    def test_unknown_event(self):
        lines = ['{"run_id":"r","event":"totally_made_up"}']
        self.assertEqual(kinds(self.run_lines(lines)), ["unknown-event"])

    def test_unknown_event_allowed_when_opted_in(self):
        lines = ['{"run_id":"r","event":"totally_made_up"}']
        self.assertEqual(self.run_lines(lines, allow_unknown_events=True), [])

    def test_missing_base_fields(self):
        lines = ['{"event":"ready","instance_id":"i","seq":0,"wall_time_ms":1,"topic":"t"}']  # no run_id
        ks = kinds(self.run_lines(lines))
        self.assertIn("missing-base-field", ks)

    def test_non_object_line(self):
        lines = ['[1,2,3]']
        self.assertEqual(kinds(self.run_lines(lines)), ["not-object"])

    # --- non-JSON handling: lenient default vs --strict --------------------
    def test_non_json_skipped_by_default(self):
        lines = [
            'profiling:/home/ossuser:Cannot create directory',  # coverage noise
            '{"run_id":"r","event":"ready","instance_id":"i","seq":0,"wall_time_ms":1,"topic":"t"}',
        ]
        self.assertEqual(self.run_lines(lines), [])

    def test_non_json_flagged_in_strict(self):
        lines = ['profiling:/home/ossuser:Cannot create directory']
        self.assertEqual(kinds(self.run_lines(lines, strict=True)), ["not-json"])

    def test_blank_lines_ignored(self):
        self.assertEqual(self.run_lines(["", "   ", ""]), [])

    # --- producer filter ---------------------------------------------------
    def test_producer_filter_skips_other_producers(self):
        # a malformed clock_master event is ignored when we only validate vecu.
        lines = ['{"run_id":"r","event":"tick_grant"}']  # missing tick_seq/sim_time_ns/dt_ns/wall_time_ms
        self.assertEqual(self.run_lines(lines, producer="vecu"), [])
        self.assertTrue(self.run_lines(lines, producer="clock_master"))


if __name__ == "__main__":
    unittest.main()
