#!/usr/bin/env python3
"""gate.py -- the single SiLS conformance gate.

The kit ships three checkers (validate_trace / evaluate / parity, see README.md). A
developer or CI shouldn't have to wire them by hand: this chains them into ONE fail-closed
command run on a run's trace(s), which is the whole point of "the trace is the conformance
boundary" (docs/sils/sils-extension-model.md §2). It reuses the checkers as libraries -- no
logic is duplicated here; gate.py only orchestrates and reduces their verdicts to one exit.

Stages (each fail-closed; a failed stage does not stop later stages, so one run surfaces
every problem, but ANY failure makes the gate exit non-zero):
  1. SCHEMA   validate_trace : every trace is well-formed against the trace schema
  2. FIDELITY evaluate       : the fleet reaches a PASS verdict (conformant at some level)
  3. PARITY   parity (opt.)  : with --target + --contract, each trace matches the real
                               target reference within the parity contract (L4)

--junit writes the fidelity verdict as JUnit XML for CI ingestion (same document as
to_junit.py). The individual tools remain available for composing custom flows.

Usage:
  gate.py TRACE... [--resolved R] [--schema S] [--strict]
  gate.py runs/my-run/ --resolved runs/my-run/resolved-run.yaml --junit out.xml
  gate.py sils.jsonl --target target.jsonl --contract parity.yaml   # add L4 parity
Exit: 0 all requested stages pass, 1 a gate failed, 2 usage/IO error.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import evaluate as ev  # noqa: E402
import parity as par  # noqa: E402
import to_junit as tj  # noqa: E402
import validate_trace as vt  # noqa: E402

EXIT_OK = 0
EXIT_GATE_FAIL = 1
EXIT_USAGE = 2


def _schema_stage(trace_paths: list[Path], schema: dict, strict: bool) -> tuple[bool, list[str]]:
    """Stage 1: every trace well-formed against the schema. Returns (ok, summary_lines)."""
    lines: list[str] = []
    ok = True
    for tp in trace_paths:
        violations = vt.validate_file(str(tp), schema, strict=strict)
        if violations:
            ok = False
            lines.append(f"  XX {tp.name}: {len(violations)} schema violation(s)")
            for v in violations[:5]:
                lines.append(f"       {v}")
            if len(violations) > 5:
                lines.append(f"       ... and {len(violations) - 5} more")
        else:
            lines.append(f"  OK {tp.name}")
    return ok, lines


def _fidelity_stage(trace_paths: list[Path], resolved: Path | None, requirements=None
                    ) -> tuple[bool, list[str], dict | None, str | None]:
    """Stage 2: evaluate the fleet. Returns (ok, summary_lines, fleet, error).
    `requirements` (parsed spec) folds developer functional pass/fail into the verdict."""
    results = []
    for tp in trace_paths:
        events, err = ev.parse_trace(tp)
        if err:
            return False, [], None, err
        results.append(ev.evaluate_run(events, str(tp), requirements))
    expected_ids = None
    if resolved is not None:
        expected_ids, err = ev.load_resolved_instance_ids(resolved)
        if err:
            return False, [], None, err
    fleet = ev.evaluate_fleet(results, expected_ids)
    lines = [f"  fleet {fleet['fleet_verdict']}: {fleet['runs_conformant']}/{fleet['runs_total']} "
             "runs conformant"]
    for r in fleet["runs"]:
        mark = "OK " if r["conformant"] else "XX "
        lines.append(f"  {mark}{Path(r['run_id']).name if r.get('run_id') else '?'}: "
                     f"{r['verdict']} (fidelity {r.get('fidelity_evidenced', 'none')})")
        for v in r.get("violations", []):
            lines.append(f"       {v}")
    for v in fleet.get("fleet_violations", []):
        lines.append(f"  !! fleet: {v}")
    return fleet["fleet_verdict"] == "PASS", lines, fleet, None


def _parity_stage(trace_paths: list[Path], target: Path, contract_path: Path
                  ) -> tuple[bool, list[str], str | None]:
    """Stage 3 (optional): each trace vs the real target reference. Returns (ok, lines, err)."""
    contract, cerr = par.load_contract(contract_path)
    if cerr:
        return False, [], cerr
    target_events, terr = ev.parse_trace(target)
    if terr:
        return False, [], terr
    lines: list[str] = []
    ok = True
    for tp in trace_paths:
        sils_events, serr = ev.parse_trace(tp)
        if serr:
            return False, [], serr
        report = par.judge(target_events, sils_events, contract)
        passed = report["verdict"] == "PARITY"
        ok = ok and passed
        mark = "OK " if passed else "XX "
        lines.append(f"  {mark}{tp.name}: {report['verdict']} "
                     f"(fidelity {report['fidelity_evidenced']})")
        for f in report.get("attestation_failures", []):
            lines.append(f"       attest: {f}")
    return ok, lines, None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("traces", nargs="+", help="trace.jsonl file(s) or run dir(s)")
    parser.add_argument("--schema", default=vt.DEFAULT_SCHEMA, help="trace schema JSON (default: v1)")
    parser.add_argument("--strict", action="store_true",
                        help="schema stage flags non-JSON lines (default: skip, like evaluate.py)")
    parser.add_argument("--resolved", type=Path, default=None,
                        help="resolved-run.yaml for fleet completeness")
    parser.add_argument("--target", type=Path, default=None,
                        help="real-target reference trace for the L4 parity stage")
    parser.add_argument("--contract", type=Path, default=None,
                        help="parity contract YAML (required with --target)")
    parser.add_argument("--junit", type=Path, default=None,
                        help="write the fidelity verdict as JUnit XML here")
    parser.add_argument("--requirements", type=Path, default=None,
                        help="optional functional requirements YAML (folds into FIDELITY verdict)")
    parser.add_argument("--suite-name", default="sils-conformance")
    args = parser.parse_args(argv)

    if (args.target is None) != (args.contract is None):
        print("# error: --target and --contract must be given together", file=sys.stderr)
        return EXIT_USAGE

    try:
        schema = vt.load_schema(args.schema)
    except (OSError, ValueError) as e:
        print(f"# error: cannot load schema: {e}", file=sys.stderr)
        return EXIT_USAGE

    trace_paths = ev.collect_trace_paths(args.traces)
    if not trace_paths:
        print("# error: no trace files found", file=sys.stderr)
        return EXIT_USAGE

    gate_ok = True

    # Stage 1: schema.
    schema_ok, schema_lines = _schema_stage(trace_paths, schema, args.strict)
    gate_ok = gate_ok and schema_ok
    print(f"[1/{'3' if args.target else '2'}] SCHEMA  {'PASS' if schema_ok else 'FAIL'}",
          file=sys.stderr)
    for ln in schema_lines:
        print(ln, file=sys.stderr)

    # Stage 2: fidelity.
    requirements, rerr = ev.load_requirements(args.requirements)
    if rerr:
        print(f"# error: {rerr}", file=sys.stderr)
        return EXIT_USAGE
    fidelity_ok, fid_lines, fleet, ferr = _fidelity_stage(trace_paths, args.resolved, requirements)
    if ferr:
        print(f"# error: fidelity stage: {ferr}", file=sys.stderr)
        return EXIT_USAGE
    gate_ok = gate_ok and fidelity_ok
    print(f"[2/{'3' if args.target else '2'}] FIDELITY {'PASS' if fidelity_ok else 'FAIL'}",
          file=sys.stderr)
    for ln in fid_lines:
        print(ln, file=sys.stderr)

    # Stage 3 (optional): parity.
    if args.target:
        parity_ok, par_lines, perr = _parity_stage(trace_paths, args.target, args.contract)
        if perr:
            print(f"# error: parity stage: {perr}", file=sys.stderr)
            return EXIT_USAGE
        gate_ok = gate_ok and parity_ok
        print(f"[3/3] PARITY  {'PASS' if parity_ok else 'FAIL'}", file=sys.stderr)
        for ln in par_lines:
            print(ln, file=sys.stderr)

    # Optional JUnit (the fidelity fleet verdict, same document as to_junit.py).
    if args.junit and fleet is not None:
        xml = tj.serialize(tj.build_junit(fleet, args.suite_name))
        args.junit.write_text(xml, encoding="utf-8")
        print(f"# wrote {args.junit}", file=sys.stderr)

    print(f"\nGATE {'PASS' if gate_ok else 'FAIL'}", file=sys.stderr)
    return EXIT_OK if gate_ok else EXIT_GATE_FAIL


if __name__ == "__main__":
    sys.exit(main())
