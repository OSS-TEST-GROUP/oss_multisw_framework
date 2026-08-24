#!/usr/bin/env python3
"""Capture a normalized golden trace from a run -- architecture v3 Final §12.4.

compare_golden.py consumes a golden trace but has no way to PRODUCE one. This is
its other half: it takes a run's trace.jsonl and emits a normalized golden suitable
as a regression baseline, so the loop is run -> normalize -> register golden ->
compare future runs against it.

Normalization makes the golden deterministic and reviewable, and consistent with
how compare_golden will read it:

  - strips per-run identity/noise fields (run_id, instance_id, domain_id, pid,
    container, backend, pool, ...) -- fields compare_golden ignores anyway, but a
    checked-in golden shouldn't carry a random capture run's identifiers,
  - rebases wall_time_ms to an OFFSET from the run's first timestamp, so the golden
    stores relative timing (what §12.4's latency window compares) rather than an
    absolute capture-time clock,
  - drives the strip/keep decisions from the SAME rules file the comparison will
    use, so capture and compare never disagree about what matters,
  - prepends an informational `golden_meta` event (source run, capture note, any
    --meta k=v like catalog_version / image_digest for §11 version linkage);
    compare_golden ignores it (not a compare_event).

Guards (a golden should be trustworthy):
  - refuses to baseline a run that isn't PASSED + conformant (evaluate.py) unless
    --force,
  - refuses a non-deterministic input mode (live / local-staged) per §12.4 unless
    --force -- the same gate compare_golden enforces.

Usage:
  capture_golden.py --trace RUN.trace.jsonl [--rules R] [--out G] [--lean]
                    [--input-mode replay] [--meta k=v ...] [--force]
Exit: 0 = golden written, 1 = refused (non-conformant / bad mode), 2 = usage/IO.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import evaluate as ev  # noqa: E402
import compare_golden as cg  # noqa: E402

EXIT_OK = 0
EXIT_REFUSED = 1
EXIT_USAGE = 2

# domain_id is per-run (leased from a pool) so it must not survive into a golden;
# add it to whatever the rules already ignore. wall_time_ms is NOT stripped -- it
# is rebased to a relative offset instead.
EXTRA_STRIP = {"domain_id"}


def normalize(events: list[dict], rules: dict, lean: bool) -> list[dict]:
    strip = (set(rules["ignore_fields"]) | EXTRA_STRIP) - {"wall_time_ms"}
    compare_events = set(rules["compare_events"])

    base = cg.baseline_ms(events)
    out: list[dict] = []
    anchored = False
    for e in events:
        name = e.get("event")
        # In lean mode keep only compare_events, plus the first timestamped event
        # as a timing anchor (so a lean golden's baseline still matches a full
        # candidate's run_start baseline).
        keep = (name in compare_events)
        if lean and not keep:
            if not anchored and isinstance(e.get("wall_time_ms"), (int, float)):
                anchored = True  # keep this one purely as the offset-0 anchor
            else:
                continue
        ne = {k: v for k, v in e.items() if k not in strip}
        if base is not None and isinstance(ne.get("wall_time_ms"), (int, float)):
            ne["wall_time_ms"] = ne["wall_time_ms"] - base
        out.append(ne)
    return out


def parse_meta(pairs: list[str]) -> dict:
    meta: dict[str, str] = {}
    for p in pairs or []:
        if "=" not in p:
            raise ValueError(f"--meta must be key=value, got '{p}'")
        k, v = p.split("=", 1)
        meta[k] = v
    return meta


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, required=True, help="source run trace.jsonl")
    parser.add_argument("--rules", type=Path, default=None, help="rules YAML shared with compare_golden (§12.4)")
    parser.add_argument("--out", type=Path, default=None, help="write golden here (default stdout)")
    parser.add_argument("--lean", action="store_true", help="keep only compare_events (+timing anchor)")
    parser.add_argument("--input-mode", default=None, help="declared capture mode; overrides rules.input_mode")
    parser.add_argument("--plant", default=None, metavar="ID",
                        help="plant/simulator identity this run used (e.g. morai, "
                             "scenario_simulator_v2, fmu, fixture). Stamped into golden_meta "
                             "so compare_golden --plant can refuse a cross-plant comparison")
    parser.add_argument("--meta", action="append", default=[], metavar="K=V",
                        help="extra golden_meta fields (e.g. catalog_version=0.1.0)")
    parser.add_argument("--force", action="store_true", help="baseline even a non-conformant / non-deterministic run")
    args = parser.parse_args(argv)

    rules, err = cg.load_rules(args.rules)
    if err:
        print(f"# error: {err}", file=sys.stderr)
        return EXIT_USAGE
    if args.input_mode is not None:
        rules["input_mode"] = args.input_mode

    try:
        meta = parse_meta(args.meta)
    except ValueError as e:
        print(f"# error: {e}", file=sys.stderr)
        return EXIT_USAGE

    events, perr = ev.parse_trace(args.trace)
    if perr:
        print(f"# error: {perr}", file=sys.stderr)
        return EXIT_USAGE
    if not events:
        print(f"# error: no events in {args.trace}", file=sys.stderr)
        return EXIT_USAGE

    # Guard 1: input-mode gate (§12.4), same policy as compare_golden.
    reason = cg.gate_input_mode(rules)
    if reason and not args.force:
        print(f"# refused: {reason} (use --force to override)", file=sys.stderr)
        return EXIT_REFUSED

    # Guard 2: only a PASSED + conformant run makes a trustworthy baseline.
    run = ev.evaluate_run(events, str(args.trace))
    if not run["conformant"] and not args.force:
        print(f"# refused: source run is not conformant (verdict={run['verdict']}, "
              f"violations={len(run['violations'])}); a golden must be a known-good run. "
              f"Use --force to override.", file=sys.stderr)
        for v in run["violations"]:
            print(f"#   - {v}", file=sys.stderr)
        return EXIT_REFUSED

    normalized = normalize(events, rules, args.lean)

    # `plant` identifies WHICH plant/simulator produced these values. Two simulators have
    # different vehicle dynamics, so their traces legitimately differ -- comparing across
    # them reads as a REGRESSION with no hint that the plant, not the code, changed.
    # compare_golden --plant enforces the isolation; this records the golden's side of it.
    plant = args.plant if args.plant is not None else meta.get("plant")

    meta_event = {
        "event": "golden_meta",
        "source_run_id": run["run_id"],
        "source_verdict": run["verdict"],
        "input_mode": rules.get("input_mode"),
        "plant": plant,
        "lean": args.lean,
        "normalized_events": len(normalized),
        "forced": bool(args.force),
    }
    meta_event.update(meta)
    meta_event["plant"] = plant          # --meta plant=X must not diverge from the field

    lines = [json.dumps(meta_event)] + [json.dumps(e) for e in normalized]
    text = "\n".join(lines) + "\n"

    if plant is None:
        print("# warn: no --plant declared; this golden cannot be plant-isolated and "
              "compare_golden will not be able to refuse a cross-plant comparison. "
              "Declare it whenever more than one plant/simulator is in play.",
              file=sys.stderr)

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(text, encoding="utf-8")
        print(f"# wrote golden {args.out} ({len(normalized)} normalized event(s), "
              f"source={run['run_id']}, mode={rules.get('input_mode')}, "
              f"plant={plant or '<undeclared>'})", file=sys.stderr)
    else:
        sys.stdout.write(text)
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
