#!/usr/bin/env python3
"""SiLS run conformance evaluator -- architecture v3 Final §5.2 / §9 / §11 / §12.

The Run Coordinator (service/sils/run_coordinator/run_phase1.sh) leaves a raw
trace.jsonl per run, but "did this run actually conform?" is a separate judgment
from "did the coordinator exit 0". This tool reads one or more trace files and
produces a machine-readable verdict:

  - maps the merged event stream onto the §5.2 run state machine
    (CREATED->...->PASSED|FAILED|ABORTED->CLEANED) and derives a terminal verdict,
  - checks conformance invariants that a green exit code alone doesn't prove:
      * lifecycle ordering (run_start before ready before verify before terminal),
      * pass-with-evidence (§11 L0: a PASS must be backed by endpoint match +
        >=1 received sample -- a "pass" with no sample is flagged, not trusted),
      * lease integrity (§12.2: a leased domain must be released -- a missing
        domain_released is a leaked-lease violation),
      * domain consistency (§12.1: every domain_id in the run agrees),
  - aggregates a fleet of runs (§13) into one verdict, and optionally cross-checks
    fleet completeness against a Topology Compiler resolved-run.yaml (§9): every
    resolved container's instance_id must be covered by exactly one run.

The trace is a MERGED stream from up to three producers (coordinator + probe +
verify), so this parses defensively: any JSON object line with an "event" field
is an event; anything else is ignored. Events without a run_id are attributed to
the file's dominant run_id.

Exit codes: 0 = fleet PASS, 1 = fleet FAIL (verdict or blocking violation),
2 = usage/IO error. Verdict JSON goes to stdout; a human summary to stderr.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

EXIT_PASS = 0
EXIT_FAIL = 1
EXIT_USAGE = 2

# Coordinator terminal events -> §5.2 terminal state.
TERMINAL = {
    "run_pass": "PASSED",
    "run_fail": "FAILED",
    "ready_timeout": "ABORTED",
    "start_failed": "ABORTED",
    "aborted_orphaned": "ABORTED",
}

# Canonical happy-path order (§5.2). Only milestones actually present are checked,
# so a run that legitimately stops early (ABORTED) isn't penalised for the tail.
HAPPY_ORDER = [
    "run_start",
    "ready",          # probe's readiness signal
    "vecu_ready",     # coordinator's mirror
    "verify_invoke",
    "verify_start",
    "sample_received",
    "verify_pass",
    "run_pass",
]


def parse_trace(path: Path) -> tuple[list[dict], str | None]:
    """Return (events, error). Each event is the parsed JSON object plus a
    0-based 'line_index' recording append order (the merged stream's real
    ordering signal -- wall_time_ms comes from three different processes)."""
    if not path.exists():
        return [], f"trace file not found: {path}"
    events: list[dict] = []
    idx = 0
    for raw in path.read_text(encoding="utf-8").splitlines():
        raw = raw.strip()
        if not raw:
            continue
        try:
            obj = json.loads(raw)
        except json.JSONDecodeError:
            continue  # probe/verify may emit non-event stdout; skip quietly
        if isinstance(obj, dict) and "event" in obj:
            obj["line_index"] = idx
            events.append(obj)
            idx += 1
    return events, None


def dominant_run_id(events: list[dict]) -> str:
    counts: dict[str, int] = {}
    for e in events:
        rid = e.get("run_id")
        if rid:
            counts[rid] = counts.get(rid, 0) + 1
    if not counts:
        return "unknown"
    return max(counts, key=lambda k: counts[k])


def first_index(events: list[dict], name: str) -> int | None:
    for e in events:
        if e.get("event") == name:
            return e["line_index"]
    return None


def evaluate_run(events: list[dict], path: str) -> dict:
    """Reduce one run's merged event stream to a verdict + violations."""
    run_id = dominant_run_id(events)
    names = [e.get("event") for e in events]
    violations: list[str] = []

    # --- terminal verdict (§5.2) --------------------------------------------
    verdict = "INCOMPLETE"
    for e in events:  # last terminal event wins if several somehow appear
        if e.get("event") in TERMINAL:
            verdict = TERMINAL[e["event"]]
    if verdict == "INCOMPLETE" and events:
        violations.append(
            "no terminal event (run_pass/run_fail/ready_timeout/start_failed/"
            "aborted_orphaned) -- trace truncated or run still in flight"
        )

    # --- lifecycle ordering (§5.2) ------------------------------------------
    present = [(n, first_index(events, n)) for n in HAPPY_ORDER]
    present = [(n, i) for n, i in present if i is not None]
    for (na, ia), (nb, ib) in zip(present, present[1:]):
        if ia > ib:
            violations.append(f"out-of-order lifecycle: '{na}' after '{nb}'")

    # --- evidence (§11 L0: endpoint match + received sample) ----------------
    samples = sum(1 for n in names if n == "sample_received")
    endpoints_matched = sum(1 for n in names if n == "endpoint_matched")
    verify_pass = "verify_pass" in names
    fidelity = "L0" if (samples >= 1 and endpoints_matched >= 1) else "none"

    if verdict == "PASSED":
        if samples < 1:
            violations.append(
                "PASS without evidence: run_pass but no sample_received "
                "(§11 L0 requires >=1 received sample)"
            )
        if endpoints_matched < 1:
            violations.append(
                "PASS without endpoint match: run_pass but no endpoint_matched "
                "(§11 L0 requires type/topic matching)"
            )
        if not verify_pass:
            violations.append(
                "PASS without verify_pass: coordinator run_pass but the verifier "
                "never reported verify_pass (verdict/verifier disagreement)"
            )

    # --- lease integrity (§12.2) --------------------------------------------
    leased = "domain_leased" in names
    released = "domain_released" in names
    if leased and not released:
        violations.append(
            "leaked lease: domain_leased with no domain_released -- the pool "
            "domain was never returned (§12.2)"
        )

    # --- domain consistency (§12.1) -----------------------------------------
    domains = sorted({e["domain_id"] for e in events if "domain_id" in e},
                     key=lambda d: str(d))
    if len(domains) > 1:
        violations.append(
            f"domain inconsistency: run touched multiple domain_ids {domains} "
            "(§12.1: one run == one domain)"
        )

    # --- identity (probe carries the vECU instance_id) ----------------------
    instance_ids = sorted({e["instance_id"] for e in events if e.get("instance_id")})

    # A run is CONFORMANT iff it reached PASSED with zero violations. Anything
    # else (FAILED/ABORTED/INCOMPLETE, or PASSED-with-violations) is not.
    conformant = verdict == "PASSED" and not violations

    return {
        "run_id": run_id,
        "trace": path,
        "verdict": verdict,
        "conformant": conformant,
        "fidelity_evidenced": fidelity,
        "evidence": {
            "endpoints_matched": endpoints_matched,
            "samples_received": samples,
            "verify_pass": verify_pass,
        },
        "domains": domains,
        "instance_ids": instance_ids,
        "violations": violations,
    }


def load_resolved_instance_ids(resolved_path: Path) -> tuple[list[str], str | None]:
    try:
        import yaml
    except ImportError:  # pragma: no cover
        return [], "PyYAML not available; cannot read --resolved"
    if not resolved_path.exists():
        return [], f"resolved-run file not found: {resolved_path}"
    data = yaml.safe_load(resolved_path.read_text(encoding="utf-8")) or {}
    ids = [c.get("instance_id") for c in data.get("containers", []) if c.get("instance_id")]
    return ids, None


def evaluate_fleet(run_results: list[dict], expected_instance_ids: list[str] | None) -> dict:
    """Aggregate per-run verdicts into a fleet verdict (§13)."""
    fleet_violations: list[str] = []

    # Fleet completeness (§9): every expected instance must be covered exactly once.
    if expected_instance_ids is not None:
        seen: dict[str, int] = {}
        for r in run_results:
            for iid in r["instance_ids"]:
                seen[iid] = seen.get(iid, 0) + 1
        for iid in expected_instance_ids:
            n = seen.get(iid, 0)
            if n == 0:
                fleet_violations.append(f"missing instance in fleet trace: '{iid}' expected but never seen")
            elif n > 1:
                fleet_violations.append(f"duplicate instance in fleet trace: '{iid}' seen in {n} runs")
        extra = sorted(set(seen) - set(expected_instance_ids))
        for iid in extra:
            fleet_violations.append(f"unexpected instance in fleet trace: '{iid}' not in resolved-run")

    all_conformant = all(r["conformant"] for r in run_results) and run_results != []
    passed = all_conformant and not fleet_violations
    return {
        "fleet_verdict": "PASS" if passed else "FAIL",
        "runs_total": len(run_results),
        "runs_conformant": sum(1 for r in run_results if r["conformant"]),
        "fleet_violations": fleet_violations,
        "runs": run_results,
    }


def collect_trace_paths(inputs: list[str]) -> list[Path]:
    """Accept files and directories; a directory contributes its *.trace.jsonl."""
    paths: list[Path] = []
    for item in inputs:
        p = Path(item)
        if p.is_dir():
            paths.extend(sorted(p.glob("*.trace.jsonl")))
        else:
            paths.append(p)
    return paths


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", nargs="+", help="trace.jsonl file(s) or a run dir containing *.trace.jsonl")
    parser.add_argument("--resolved", type=Path, default=None,
                        help="optional resolved-run.yaml to cross-check fleet completeness (§9)")
    parser.add_argument("--quiet", action="store_true", help="suppress the human summary on stderr")
    args = parser.parse_args(argv)

    trace_paths = collect_trace_paths(args.traces)
    if not trace_paths:
        print("# error: no trace files found", file=sys.stderr)
        return EXIT_USAGE

    run_results: list[dict] = []
    for tp in trace_paths:
        events, err = parse_trace(tp)
        if err:
            print(f"# error: {err}", file=sys.stderr)
            return EXIT_USAGE
        run_results.append(evaluate_run(events, str(tp)))

    expected_ids = None
    if args.resolved is not None:
        expected_ids, err = load_resolved_instance_ids(args.resolved)
        if err:
            print(f"# error: {err}", file=sys.stderr)
            return EXIT_USAGE

    fleet = evaluate_fleet(run_results, expected_ids)
    print(json.dumps(fleet, indent=2))

    if not args.quiet:
        print(f"# fleet {fleet['fleet_verdict']}: "
              f"{fleet['runs_conformant']}/{fleet['runs_total']} runs conformant",
              file=sys.stderr)
        for r in fleet["runs"]:
            mark = "OK " if r["conformant"] else "XX "
            print(f"#   {mark}{r['run_id']} [{r['verdict']}] fidelity={r['fidelity_evidenced']}",
                  file=sys.stderr)
            for v in r["violations"]:
                print(f"#       - {v}", file=sys.stderr)
        for v in fleet["fleet_violations"]:
            print(f"#   ! {v}", file=sys.stderr)

    return EXIT_PASS if fleet["fleet_verdict"] == "PASS" else EXIT_FAIL


if __name__ == "__main__":
    sys.exit(main())
