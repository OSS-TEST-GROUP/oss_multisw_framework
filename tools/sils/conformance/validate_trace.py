#!/usr/bin/env python3
"""Validate a SiLS vECU trace (JSONL) against the published trace schema.

The trace a run emits is the public conformance boundary (docs/sils-extension-model.md
§2 / docs/sils-trace-schema.md): evaluate.py / parity.py consume only this. This checker
is the schema's gatekeeper -- a developer runs it on their own ECU's trace to confirm the
events + fields the conformance tools rely on are actually present and well-formed, before
those tools (or the reviewer) ever see it.

Checks, per line:
  - the line is a single JSON object;
  - it carries the base fields (run_id, event);
  - the event is known to the schema (unknown -> violation, unless --allow-unknown-events);
  - every field the schema marks `required` for that event is present.
Unknown extra fields are allowed (forward-compatible). Fail-closed: any violation -> exit 1.

Usage:
    validate_trace.py TRACE.jsonl [--schema config/sils/trace-schema/v1.json]
    cat trace.jsonl | validate_trace.py -            # read stdin
    validate_trace.py t.jsonl --producer vecu        # only check vecu-produced events
"""
import argparse
import json
import os
import sys

DEFAULT_SCHEMA = os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))),
    "config", "sils", "trace-schema", "v1.json")


def load_schema(path):
    with open(path) as f:
        schema = json.load(f)
    if "events" not in schema or "base_required" not in schema:
        raise ValueError(f"{path}: not a trace schema (missing events/base_required)")
    return schema


class Violation:
    def __init__(self, line_no, kind, detail):
        self.line_no = line_no
        self.kind = kind
        self.detail = detail

    def __str__(self):
        return f"L{self.line_no}: {self.kind}: {self.detail}"


def validate_lines(lines, schema, allow_unknown_events=False, producer=None, strict=False):
    """Yield Violation for each problem found across an iterable of (line_no, text).

    By default non-JSON lines are skipped, matching how evaluate.py consumes a trace
    (the probe/verifier may interleave non-event stdout -- coverage 'profiling:' notes,
    DDS logs -- into the same stream). Pass strict=True to flag them, for validating a
    clean, extracted trace file where every line must be an event.
    """
    base = schema["base_required"]
    events = schema["events"]
    for line_no, text in lines:
        text = text.strip()
        if not text:
            continue
        try:
            obj = json.loads(text)
        except json.JSONDecodeError as e:
            if strict:
                yield Violation(line_no, "not-json", str(e))
            continue  # lenient (default): environmental stdout noise, skip like evaluate.py
        if not isinstance(obj, dict):
            yield Violation(line_no, "not-object", f"top-level {type(obj).__name__}, expected object")
            continue
        for b in base:
            if b not in obj:
                yield Violation(line_no, "missing-base-field", f"'{b}'")
        event = obj.get("event")
        if event is None:
            continue  # already reported as missing-base-field
        spec = events.get(event)
        if spec is None:
            if not allow_unknown_events:
                yield Violation(line_no, "unknown-event", f"'{event}' not in schema")
            continue
        if producer and spec.get("producer") != producer:
            continue  # caller only cares about one producer's events
        for field in spec.get("required", []):
            if field not in obj:
                yield Violation(line_no, "missing-field", f"event '{event}' requires '{field}'")


def validate_file(path, schema, **kw):
    if path == "-":
        lines = enumerate(sys.stdin, start=1)
        return list(validate_lines(lines, schema, **kw))
    with open(path) as f:
        return list(validate_lines(enumerate(f, start=1), schema, **kw))


def main(argv=None):
    ap = argparse.ArgumentParser(description="Validate a SiLS trace against the trace schema.")
    ap.add_argument("trace", help="trace JSONL file, or '-' for stdin")
    ap.add_argument("--schema", default=DEFAULT_SCHEMA, help="schema JSON (default: v1)")
    ap.add_argument("--allow-unknown-events", action="store_true",
                    help="treat unknown event names as OK (forward-compat) instead of a violation")
    ap.add_argument("--producer", help="only validate events from this producer (e.g. vecu)")
    ap.add_argument("--strict", action="store_true",
                    help="flag non-JSON lines as violations (default: skip them, like evaluate.py)")
    ap.add_argument("--max-print", type=int, default=50, help="max violations to print")
    ap.add_argument("--quiet", action="store_true", help="print only the summary line")
    args = ap.parse_args(argv)

    schema = load_schema(args.schema)
    violations = validate_file(args.trace, schema,
                               allow_unknown_events=args.allow_unknown_events,
                               producer=args.producer, strict=args.strict)
    if violations and not args.quiet:
        for v in violations[:args.max_print]:
            print(str(v), file=sys.stderr)
        if len(violations) > args.max_print:
            print(f"... and {len(violations) - args.max_print} more", file=sys.stderr)

    label = os.path.basename(args.trace)
    if violations:
        print(f"FAIL {label}: {len(violations)} violation(s) vs schema v{schema['schema_version']}")
        return 1
    print(f"PASS {label}: valid vs schema v{schema['schema_version']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
