#!/usr/bin/env python3
"""Golden-trace regression comparator -- architecture v3 Final §12.4.

Conformance (evaluate.py) answers "did this run pass on its own terms". Regression
answers a harder question: "does this run still match a known-good golden trace".
§12.4 spells out that this comparison is NOT byte-equality -- it is governed by
tolerance rules:

  - floating-point tolerance (abs/rel per field),
  - a latency window (matched events may drift in relative timing, within a bound),
  - unordered vs strict event ordering,
  - an explicit missing/extra sample policy (fail | warn | ignore),
  - input-mode gating: `live` and `local-staged` traces are non-deterministic by
    construction and MUST NOT be used as golden exact-match targets; only `replay`,
    `relay-staged`, or `full-lockstep` are (§12.4). A rules file declaring a rejected
    mode is refused (exit 2), not silently compared.
  - PLANT isolation: a golden captured under one plant/simulator may not be compared
    against a run under another (MORAI vs scenario_simulator_v2 vs FMU vs fixture have
    different vehicle dynamics). `capture_golden --plant` records it, `--plant` here
    declares the candidate's, and a declared mismatch is refused (exit 2) -- otherwise a
    plant swap reads as a code regression. See docs/sils/sils-simulator-loops.md.

Events are aligned by a key (default: the event name), grouped in order, and paired
positionally within each key group -- so N publish_intent events pair 0↔0,1↔1,...
and a surplus on either side surfaces as extra/missing under the sample policy.
Timing is compared as an OFFSET from each trace's first timestamp (absolute wall
clocks differ run-to-run; relative timing is the meaningful regression signal).

Usage:
  compare_golden.py --golden G.jsonl --candidate C.jsonl [--rules R.yaml] [--strict]
Exit: 0 = MATCH, 1 = REGRESSION, 2 = policy reject / usage error. Report -> stdout.
"""

from __future__ import annotations

import argparse
import copy
import json
import sys
from collections import OrderedDict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import evaluate as ev  # noqa: E402  (reuse the defensive JSONL parser)

EXIT_MATCH = 0
EXIT_REGRESSION = 1
EXIT_REJECT = 2

DEFAULT_RULES = {
    # Declared input mode of the traces being compared. None = unknown (gate skipped,
    # but a warning is emitted). See reject_input_modes.
    "input_mode": None,
    "reject_input_modes": ["live", "local-staged"],
    # Only these events participate in the comparison; coordinator bookkeeping
    # (domain_leased, cleanup_done, ...) is deliberately excluded.
    "compare_events": [
        "publish_intent", "sample_received", "endpoint_matched", "endpoint_unmatched",
        "verify_pass", "verify_fail", "run_pass", "run_fail",
        # protocol response (§11 L1): per-request status/error_code is compared, so a
        # request/response regression (e.g. ok->error) surfaces as a value diff.
        "response_received",
        # behavior output (§11 L2 golden scenario): its per-tick `value` is compared
        # under numeric_tolerance, so a behavior regression surfaces as a value diff.
        "behavior_output",
        # plant step (§9 L3 model correlation): per-tick output/command/feedback_in
        # are compared under numeric_tolerance, so a plant-trajectory regression
        # surfaces as a value diff. Set field tolerances in the rules file (a physical
        # signal rarely matches bit-exact across builds/FMU versions).
        "plant_step",
        # scenario fault (§11 negative path, IScenario): the injected fault type per tick
        # is compared, so a fault-injection run is itself golden-reproducible -- the same
        # schedule must inject the same fault at the same tick every run.
        "scenario_fault",
        # bus-coupled control output (Option 2, §9 over the DDS bus): the vECU's per-tick
        # command (steering_angle/acceleration/gear) is compared, so a plant-coupling
        # regression surfaces as a value diff -- the golden counterpart of plant_step for
        # the bus loop. Under lockstep (S3) these are bit-exact by construction (a pure
        # function of sim_time), so the default 0 tolerance holds; loosen per-field in the
        # rules file if a run couples to a non-deterministic external sim.
        "control_published",
    ],
    "key_fields": ["event"],
    # Per-run identity and transport noise never participate in value comparison;
    # wall_time_ms is superseded by the offset/latency check.
    "ignore_fields": [
        "run_id", "instance_id", "expected_instance_id", "line_index",
        "pid", "container", "wall_time_ms", "backend", "pool",
        # discovery/transport observation counts depend on endpoint-match timing
        # (when the verifier joins), not on deterministic vECU behavior -- comparing
        # them would flag cross-run join-order jitter as a regression.
        "matched_count", "received_count", "valid_count",
    ],
    "numeric_tolerance": {"default": {"abs": 0.0, "rel": 0.0}, "fields": {}},
    "latency_window_ms": None,          # None = skip latency comparison
    "ordering": "by_key",               # "by_key" | "strict"
    "sample_policy": {"missing": "fail", "extra": "fail"},
}


def load_rules(path: Path | None) -> tuple[dict, str | None]:
    rules = copy.deepcopy(DEFAULT_RULES)
    if path is None:
        return rules, None
    try:
        import yaml
    except ImportError:  # pragma: no cover
        return rules, "PyYAML not available; cannot read --rules"
    if not path.exists():
        return rules, f"rules file not found: {path}"
    user = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    # Shallow merge with one level of nesting for the dict-valued keys.
    for k, v in user.items():
        if isinstance(v, dict) and isinstance(rules.get(k), dict):
            merged = copy.deepcopy(rules[k])
            merged.update(v)
            rules[k] = merged
        else:
            rules[k] = v
    return rules, None


def event_key(e: dict, key_fields: list[str]) -> tuple:
    return tuple(str(e.get(f, "")) for f in key_fields)


def group_in_order(events: list[dict], key_fields: list[str]) -> "OrderedDict[tuple, list[dict]]":
    g: "OrderedDict[tuple, list[dict]]" = OrderedDict()
    for e in events:
        g.setdefault(event_key(e, key_fields), []).append(e)
    return g


def baseline_ms(events: list[dict]) -> float | None:
    for e in events:
        wt = e.get("wall_time_ms")
        if isinstance(wt, (int, float)):
            return float(wt)
    return None


def within_tolerance(g: float, c: float, tol: dict) -> bool:
    allowed = float(tol.get("abs", 0.0)) + float(tol.get("rel", 0.0)) * abs(g)
    return abs(g - c) <= allowed


def compare_fields(g: dict, c: dict, rules: dict, ctx: str) -> list[dict]:
    diffs: list[dict] = []
    ignore = set(rules["ignore_fields"])
    ntol = rules["numeric_tolerance"]
    for field, gval in g.items():
        if field in ignore or field == "event":
            continue
        if field not in c:
            diffs.append({"where": ctx, "field": field, "kind": "field_missing",
                          "golden": gval, "candidate": None})
            continue
        cval = c[field]
        if isinstance(gval, (int, float)) and isinstance(cval, (int, float)) \
                and not isinstance(gval, bool) and not isinstance(cval, bool):
            # `or {}` guards against YAML parsing an empty `fields:`/`default:` as None.
            tol = (ntol.get("fields") or {}).get(field, ntol.get("default") or {})
            if not within_tolerance(float(gval), float(cval), tol):
                diffs.append({"where": ctx, "field": field, "kind": "numeric",
                              "golden": gval, "candidate": cval, "tolerance": tol})
        elif gval != cval:
            diffs.append({"where": ctx, "field": field, "kind": "value",
                          "golden": gval, "candidate": cval})
    return diffs


def is_lockstep(events: list[dict]) -> bool:
    """A run is lockstep iff its clock announced mode 'lockstep' (or it carries tick
    grants). In lockstep, sim_time_ns is decoupled from wall time by design (§8), so
    wall-clock latency offsets are NOT part of the reproducibility contract -- the
    deterministic timing axis is sim_time_ns, which is compared exactly as a field."""
    for e in events:
        if e.get("event") == "clock_start" and e.get("mode") == "lockstep":
            return True
        if e.get("event") == "tick_grant":
            return True
    return False


def compare(golden: list[dict], candidate: list[dict], rules: dict) -> dict:
    key_fields = rules["key_fields"]
    compare_events = set(rules["compare_events"])

    g_base = baseline_ms(golden)
    c_base = baseline_ms(candidate)

    gf = [e for e in golden if e.get("event") in compare_events]
    cf = [e for e in candidate if e.get("event") in compare_events]

    gg = group_in_order(gf, key_fields)
    cg = group_in_order(cf, key_fields)

    diffs: list[dict] = []
    missing: list[dict] = []
    extra: list[dict] = []
    latency: list[dict] = []

    lat_win = rules["latency_window_ms"]
    # Wall-clock latency is meaningless for a lockstep run (§8: sim-time is decoupled
    # from wall time). Auto-disable the window when either side is lockstep; sim_time_ns
    # equality (a compared field) carries the timing contract instead. A warning below
    # records that the wall-latency check was skipped.
    lockstep = is_lockstep(golden) or is_lockstep(candidate)
    if lockstep:
        lat_win = None

    for key, glist in gg.items():
        clist = cg.get(key, [])
        n = min(len(glist), len(clist))
        for i in range(n):
            ctx = f"{'|'.join(key)}#{i}"
            diffs.extend(compare_fields(glist[i], clist[i], rules, ctx))
            if lat_win is not None and g_base is not None and c_base is not None:
                gw, cw = glist[i].get("wall_time_ms"), clist[i].get("wall_time_ms")
                if isinstance(gw, (int, float)) and isinstance(cw, (int, float)):
                    goff, coff = gw - g_base, cw - c_base
                    if abs(goff - coff) > float(lat_win):
                        latency.append({"where": ctx, "golden_offset_ms": goff,
                                        "candidate_offset_ms": coff, "window_ms": lat_win})
        for e in glist[n:]:
            missing.append({"key": "|".join(key), "event": e.get("event")})
    for key, clist in cg.items():
        glen = len(gg.get(key, []))
        for e in clist[glen:]:
            extra.append({"key": "|".join(key), "event": e.get("event")})

    # Strict ordering: the flat key sequence must match position-for-position.
    ordering_diff = None
    if rules["ordering"] == "strict":
        gseq = [event_key(e, key_fields) for e in gf]
        cseq = [event_key(e, key_fields) for e in cf]
        if gseq != cseq:
            ordering_diff = {"golden_len": len(gseq), "candidate_len": len(cseq),
                             "first_divergence": _first_divergence(gseq, cseq)}

    # Apply the missing/extra sample policy to decide what actually fails.
    # policy=fail -> counts as a hard regression; warn -> recorded but non-fatal;
    # ignore -> dropped entirely.
    pol = rules["sample_policy"]
    hard = list(diffs) + list(latency)
    if ordering_diff:
        hard.append({"kind": "ordering", **ordering_diff})
    warnings: list[str] = []
    if lockstep and rules["latency_window_ms"] is not None:
        warnings.append("lockstep run: wall-clock latency window skipped "
                        "(sim_time_ns carries the timing contract, §8)")

    def apply_policy(items: list, kind: str, label: str) -> None:
        if not items:
            return
        p = pol.get(label, "fail")
        if p == "fail":
            hard.append({"kind": kind, "count": len(items), "items": items})
        elif p == "warn":
            warnings.append(f"{len(items)} {label} sample(s) [policy=warn]")
        # p == "ignore": nothing

    apply_policy(missing, "missing_samples", "missing")
    apply_policy(extra, "extra_samples", "extra")

    verdict = "MATCH" if not hard else "REGRESSION"
    return {
        "verdict": verdict,
        "field_diffs": diffs,
        "latency_violations": latency,
        "missing_samples": missing,
        "extra_samples": extra,
        "ordering_diff": ordering_diff,
        "warnings": warnings,
        "compared_events": len(gf),
    }


def _first_divergence(a: list, b: list) -> int:
    for i in range(min(len(a), len(b))):
        if a[i] != b[i]:
            return i
    return min(len(a), len(b))


def gate_input_mode(rules: dict) -> str | None:
    """Return a rejection reason if the declared input mode may not be golden-matched
    (§12.4), else None. An unknown (None) mode is allowed but warned about by caller."""
    mode = rules.get("input_mode")
    if mode is not None and mode in rules.get("reject_input_modes", []):
        return (f"input_mode '{mode}' is non-deterministic and cannot be a golden "
                f"exact-match target (§12.4); use replay/relay-staged/full-lockstep")
    return None


def golden_plant(golden: list[dict]) -> str | None:
    """The plant/simulator identity capture_golden stamped into `golden_meta`, or None
    for a golden captured before/without --plant."""
    for e in golden:
        if e.get("event") == "golden_meta":
            return e.get("plant")
    return None


def gate_plant(golden: list[dict], candidate_plant: str | None) -> tuple[str | None, str | None]:
    """Plant isolation. Returns (rejection_reason, warning).

    Two simulators (MORAI vs scenario_simulator_v2 vs an FMU vs a fixture) have different
    vehicle dynamics, so the same vECU legitimately produces different values under each.
    Comparing a golden captured under one against a run under another surfaces as a
    REGRESSION with nothing to say the PLANT changed rather than the code -- the exact
    false signal a regression gate exists to avoid. So a declared mismatch is a policy
    REJECT (exit 2), never a regression verdict.

    Enforcement mirrors the input_mode gate: it only binds when both sides declare. A
    legacy golden with no recorded plant, or a caller that passes no --plant, is allowed
    through with a warning that the gate did not run -- silently upgrading those to hard
    failures would break every golden captured before this field existed."""
    declared = golden_plant(golden)
    if declared is not None and candidate_plant is not None:
        if declared != candidate_plant:
            return (f"plant mismatch: golden was captured under plant '{declared}' but the "
                    f"candidate ran under '{candidate_plant}'. Different plants have "
                    f"different dynamics -- a cross-plant compare cannot distinguish a code "
                    f"regression from a plant change. Use the golden set for '{candidate_plant}'."), None
        return None, None
    if declared is None and candidate_plant is None:
        return None, ("plant identity declared on neither golden nor candidate; "
                      "plant-isolation gate not enforced")
    missing = "golden (capture_golden --plant)" if declared is None else "candidate (--plant)"
    return None, (f"plant identity missing on {missing}; plant-isolation gate not enforced "
                  f"(other side declares '{declared or candidate_plant}')")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--golden", type=Path, required=True, help="known-good golden trace.jsonl")
    parser.add_argument("--candidate", type=Path, required=True, help="candidate run trace.jsonl")
    parser.add_argument("--rules", type=Path, default=None, help="tolerance rules YAML (§12.4)")
    parser.add_argument("--strict", action="store_true", help="force strict event ordering (overrides rules)")
    parser.add_argument("--plant", default=None, metavar="ID",
                        help="plant/simulator the CANDIDATE ran under (e.g. morai, "
                             "scenario_simulator_v2). Compared against the golden's recorded "
                             "plant; a mismatch is refused (exit 2) rather than reported as "
                             "a regression")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)

    rules, err = load_rules(args.rules)
    if err:
        print(f"# error: {err}", file=sys.stderr)
        return EXIT_REJECT
    if args.strict:
        rules["ordering"] = "strict"

    reason = gate_input_mode(rules)
    if reason:
        print(json.dumps({"verdict": "REJECTED", "reason": reason}, indent=2))
        if not args.quiet:
            print(f"# REJECTED: {reason}", file=sys.stderr)
        return EXIT_REJECT

    golden, gerr = ev.parse_trace(args.golden)
    cand, cerr = ev.parse_trace(args.candidate)
    if gerr or cerr:
        print(f"# error: {gerr or cerr}", file=sys.stderr)
        return EXIT_REJECT

    # Plant isolation runs BEFORE the value comparison: if the plants differ there is no
    # meaning in the field diffs at all, so producing them would only invite reading them.
    plant_reason, plant_warn = gate_plant(golden, args.plant)
    if plant_reason:
        print(json.dumps({"verdict": "REJECTED", "reason": plant_reason}, indent=2))
        if not args.quiet:
            print(f"# REJECTED: {plant_reason}", file=sys.stderr)
        return EXIT_REJECT

    report = compare(golden, cand, rules)
    if rules.get("input_mode") is None:
        report["warnings"].append("input_mode not declared in rules; §12.4 gate not enforced")
    if plant_warn:
        report["warnings"].append(plant_warn)
    print(json.dumps(report, indent=2))

    if not args.quiet:
        print(f"# {report['verdict']}: {report['compared_events']} golden events compared, "
              f"{len(report['field_diffs'])} field diff(s), "
              f"{len(report['latency_violations'])} latency, "
              f"{len(report['missing_samples'])} missing, {len(report['extra_samples'])} extra",
              file=sys.stderr)
        for w in report["warnings"]:
            print(f"#   warn: {w}", file=sys.stderr)

    return EXIT_MATCH if report["verdict"] == "MATCH" else EXIT_REGRESSION


if __name__ == "__main__":
    sys.exit(main())
