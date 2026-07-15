#!/usr/bin/env python3
"""Convert SiLS conformance verdicts to JUnit XML for CI -- architecture v3 §9/§16.

The conformance evaluator (evaluate.py) produces a verdict most tooling can't
read directly. Nearly every CI system (GitLab, Jenkins, GitHub Actions via
test-reporter, ...) ingests JUnit XML, so this turns a fleet verdict into a
<testsuites> document: one <testcase> per SiLS run plus a fleet-completeness
case, with verdicts mapped to JUnit's failure/error/pass vocabulary:

  PASSED + conformant   -> passing testcase (no child)
  PASSED + violations   -> <failure type="conformance">   (ran, but broke an invariant)
  FAILED                -> <failure type="verify">         (verifier said no)
  ABORTED               -> <error   type="aborted">        (never reached a verdict)
  INCOMPLETE            -> <error   type="incomplete">     (trace truncated / in flight)

Two input modes:
  to_junit.py TRACE...[--resolved R]   evaluate traces, then convert (one-shot CI step)
  to_junit.py --from-verdict FILE|-    convert an evaluate.py JSON verdict (composable)

XML is built with xml.etree so escaping is handled for us. By default this exits
0 as long as it produced a report (the CI gate reads the XML); pass --exit-code
to also propagate a fleet FAIL as exit 1 for use in a shell `&&` chain.
"""

from __future__ import annotations

import argparse
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import evaluate as ev  # noqa: E402

EXIT_OK = 0
EXIT_FAIL = 1
EXIT_USAGE = 2

RUN_CLASS = "sils.conformance.run"
FLEET_CLASS = "sils.conformance.fleet"


def _run_case(parent: ET.Element, run: dict) -> tuple[int, int]:
    """Append one <testcase> for a run. Returns (failures, errors) it contributed."""
    case = ET.SubElement(parent, "testcase", {
        "name": run.get("run_id", "unknown"),
        "classname": RUN_CLASS,
        "time": f"{run.get('duration_sec', 0.0)}",
    })
    verdict = run.get("verdict", "INCOMPLETE")
    violations = run.get("violations", [])

    if verdict == "PASSED" and run.get("conformant"):
        return 0, 0  # clean pass, no child element

    body = "\n".join(f"- {v}" for v in violations) or f"verdict={verdict}"
    fidelity = run.get("fidelity_evidenced", "none")
    ev_ = run.get("evidence", {})
    body += (f"\n\nverdict={verdict} conformant={run.get('conformant')} "
             f"fidelity={fidelity} evidence={json.dumps(ev_)}")

    if verdict in ("ABORTED", "INCOMPLETE"):
        # Never reached a verdict -> an ERROR, not an assertion failure.
        kind, typ, msg = "error", verdict.lower(), f"run {verdict.lower()}"
        ET.SubElement(case, kind, {"type": typ, "message": msg}).text = body
        return 0, 1

    # PASSED-with-violations or FAILED -> a failure the run is accountable for.
    typ = "verify" if verdict == "FAILED" else "conformance"
    msg = (f"verdict {verdict}" if verdict == "FAILED"
           else f"{len(violations)} conformance violation(s)")
    ET.SubElement(case, "failure", {"type": typ, "message": msg}).text = body
    return 1, 0


def build_junit(fleet: dict, suite_name: str) -> ET.Element:
    runs = fleet.get("runs", [])
    fleet_violations = fleet.get("fleet_violations", [])
    checked_completeness = fleet.get("checked_completeness", False)

    suite = ET.Element("testsuite", {"name": suite_name})
    failures = errors = 0
    total_time = 0.0

    for run in runs:
        f, e = _run_case(suite, run)
        failures += f
        errors += e
        total_time += float(run.get("duration_sec", 0.0) or 0.0)

    # Fleet-level completeness becomes its own testcase only when a resolved-run
    # was supplied to check against -- otherwise there's nothing to assert and a
    # phantom "fleet-completeness" case would be misleading.
    tests = len(runs)
    if checked_completeness:
        tests += 1
        fc = ET.SubElement(suite, "testcase", {
            "name": "fleet-completeness", "classname": FLEET_CLASS, "time": "0",
        })
        if fleet_violations:
            body = "\n".join(f"- {v}" for v in fleet_violations)
            ET.SubElement(fc, "failure", {
                "type": "completeness",
                "message": f"{len(fleet_violations)} fleet completeness violation(s)",
            }).text = body
            failures += 1

    suite.set("tests", str(tests))
    suite.set("failures", str(failures))
    suite.set("errors", str(errors))
    suite.set("skipped", "0")
    suite.set("time", f"{round(total_time, 3)}")

    root = ET.Element("testsuites", {
        "name": suite_name,
        "tests": str(tests),
        "failures": str(failures),
        "errors": str(errors),
        "skipped": "0",
        "time": f"{round(total_time, 3)}",
    })
    root.append(suite)
    return root


def serialize(root: ET.Element) -> str:
    ET.indent(root, space="  ")  # py3.9+; pretty output for humans reading CI logs
    body = ET.tostring(root, encoding="unicode")
    return '<?xml version="1.0" encoding="UTF-8"?>\n' + body + "\n"


def _fleet_from_traces(trace_inputs: list[str], resolved: Path | None) -> tuple[dict | None, str | None]:
    trace_paths = ev.collect_trace_paths(trace_inputs)
    if not trace_paths:
        return None, "no trace files found"
    results = []
    for tp in trace_paths:
        events, err = ev.parse_trace(tp)
        if err:
            return None, err
        results.append(ev.evaluate_run(events, str(tp)))
    expected_ids = None
    if resolved is not None:
        expected_ids, err = ev.load_resolved_instance_ids(resolved)
        if err:
            return None, err
    fleet = ev.evaluate_fleet(results, expected_ids)
    return fleet, None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", nargs="*", help="trace.jsonl file(s) or run dir(s)")
    parser.add_argument("--from-verdict", metavar="FILE",
                        help="read an evaluate.py JSON verdict instead of traces ('-' = stdin)")
    parser.add_argument("--resolved", type=Path, default=None,
                        help="resolved-run.yaml for fleet completeness (trace mode only)")
    parser.add_argument("-o", "--output", type=Path, default=None, help="write XML here (default stdout)")
    parser.add_argument("--suite-name", default="sils-conformance")
    parser.add_argument("--exit-code", action="store_true",
                        help="exit 1 when the fleet verdict is FAIL (default: always 0 on success)")
    args = parser.parse_args(argv)

    if args.from_verdict:
        if args.traces:
            print("# error: pass traces OR --from-verdict, not both", file=sys.stderr)
            return EXIT_USAGE
        try:
            text = sys.stdin.read() if args.from_verdict == "-" else Path(args.from_verdict).read_text(encoding="utf-8")
            fleet = json.loads(text)
        except (OSError, json.JSONDecodeError) as e:
            print(f"# error: cannot read verdict: {e}", file=sys.stderr)
            return EXIT_USAGE
    else:
        if not args.traces:
            print("# error: provide trace file(s) or --from-verdict", file=sys.stderr)
            return EXIT_USAGE
        fleet, err = _fleet_from_traces(args.traces, args.resolved)
        if err:
            print(f"# error: {err}", file=sys.stderr)
            return EXIT_USAGE

    root = build_junit(fleet, args.suite_name)
    xml = serialize(root)

    if args.output:
        args.output.write_text(xml, encoding="utf-8")
        print(f"# wrote {args.output} "
              f"({root.get('tests')} tests, {root.get('failures')} failures, {root.get('errors')} errors)",
              file=sys.stderr)
    else:
        sys.stdout.write(xml)

    if args.exit_code and fleet.get("fleet_verdict") == "FAIL":
        return EXIT_FAIL
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
