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

# Verifier-derived terminal (§5.2 fallback for standalone runs). When the run is
# driven by raw binaries with no Run Coordinator in the loop, no coordinator
# terminal is ever emitted -- but the independent verifier still concludes with
# verify_pass/verify_fail. That is not truncation: the verifier only reports pass
# once received_count >= min_samples, so its verdict is a complete signal. We
# accept it (recording terminal_source="verifier") so manual/CI raw-binary runs
# are judgeable, while coordinator runs still take the coordinator verdict.
VERIFIER_TERMINAL = {
    "verify_pass": "PASSED",
    "verify_fail": "FAILED",
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


def analyze_lockstep(events: list[dict]) -> tuple[dict, list[str]]:
    """Classify the run's clock mode and, for lockstep runs, check tick determinism
    (§7/§8.2). Returns (clock_info, violations).

    A run is lockstep iff it carries `tick_grant` events (emitted by the ClockMaster).
    Determinism requires: tick sequences strictly +1 from 1 (no gaps/dupes), sim_time
    advancing by a constant dt each grant, every granted tick closed by a barrier /
    tick_complete, and no tick_timeout. A `wall` run (no tick_grant) is reported as
    non-deterministic by construction and gets no lockstep violations.
    """
    grants = [e for e in events if e.get("event") == "tick_grant"]
    if not grants:
        return {"mode": "wall", "ticks": 0, "deterministic": False}, []

    violations: list[str] = []
    seqs = [e.get("tick_seq") for e in grants]

    # strictly increasing by 1 from 1
    expected = list(range(1, len(grants) + 1))
    if seqs != expected:
        violations.append(
            f"lockstep tick sequence not 1..N by 1 (§8.2): got {seqs}"
        )

    # sim_time advances by a constant dt
    sims = [e.get("sim_time_ns") for e in grants]
    dt_decl = grants[0].get("dt_ns")
    if all(isinstance(s, (int, float)) for s in sims) and len(sims) >= 2:
        deltas = {sims[i + 1] - sims[i] for i in range(len(sims) - 1)}
        if len(deltas) != 1:
            violations.append(
                f"lockstep sim_time not advancing by a constant dt (§8.1): deltas={sorted(deltas)}"
            )
        elif dt_decl is not None and next(iter(deltas)) != dt_decl:
            violations.append(
                f"lockstep sim_time step {next(iter(deltas))} != declared dt_ns {dt_decl}"
            )

    # every granted tick is closed (a barrier for it, or >=1 tick_complete)
    barrier_seqs = {e.get("tick_seq") for e in events if e.get("event") == "tick_barrier"}
    complete_seqs = {e.get("tick_seq") for e in events if e.get("event") == "tick_complete"}
    for s in seqs:
        if s not in barrier_seqs and s not in complete_seqs:
            violations.append(f"lockstep tick {s} granted but never completed (open barrier, §8.2)")
    # a timeout is a hard determinism break
    if any(e.get("event") == "tick_timeout" for e in events):
        to = sorted({e.get("tick_seq") for e in events if e.get("event") == "tick_timeout"})
        violations.append(f"lockstep tick_timeout at {to} -- barrier did not close (§8.2)")

    clock = {
        "mode": "lockstep",
        "ticks": len(grants),
        "dt_ns": dt_decl,
        "deterministic": not violations,
    }
    return clock, violations


def analyze_staging(events: list[dict]) -> tuple[dict, list[str]]:
    """Check TickInbox input-staging determinism (§7.3). Returns (staging_info, violations).

    A run stages input iff it carries `tick_input_frozen` events (the vECU froze a
    set of already-received inputs at a tick boundary). The §7.3 attribution rules:
      - inputs are consumed in RECEPTION order -> concatenating each tick's frozen
        recv_seqs in tick order must be non-decreasing,
      - no input consumed in more than one tick (no duplicate across freeze sets),
      - a frozen recv_seq must actually have been received,
      - every received input is eventually staged, unless dropped under a declared
        late-sample policy (`input_dropped`).
    """
    frozen = [e for e in events if e.get("event") == "tick_input_frozen"]
    if not frozen:
        return {"input_mode": "none"}, []

    # replay (§7.4): input is pre-recorded and delivered tick-indexed, so which
    # input lands in which tick is deterministic BY CONSTRUCTION (no wall-clock
    # arrival race). Detected via replay_start or a source:"replay" freeze marker.
    # Cross-run reproducibility is verified by compare_golden (§7.4 golden target),
    # not here; the order/dup/phantom invariants below still run as a sanity check.
    is_replay = any(e.get("event") == "replay_start" for e in events) or \
        any(e.get("source") == "replay" for e in frozen)
    mode = "replay" if is_replay else "local-staged"

    received = [e["recv_seq"] for e in events
                if e.get("event") == "input_received" and "recv_seq" in e]
    dropped = {e["recv_seq"] for e in events
               if e.get("event") == "input_dropped" and "recv_seq" in e}

    all_frozen: list = []
    for e in frozen:
        rs = e.get("recv_seqs")
        if isinstance(rs, list):
            all_frozen.extend(rs)

    # §7.2 defines local-staged determinism as "consumption ORDER fixed" -- these
    # three checks are the reproducibility invariant and set `deterministic`.
    order_violations: list[str] = []
    seen = set()
    dups = {s for s in all_frozen if s in seen or seen.add(s)}
    if dups:
        order_violations.append(f"staging consumed input(s) {sorted(dups)} in more than one tick (§7.3)")
    if all_frozen != sorted(all_frozen):
        order_violations.append("staging not in reception order: frozen recv_seqs across ticks "
                                "are not non-decreasing (§7.2)")
    recv_set = set(received)
    phantom = sorted(set(all_frozen) - recv_set)
    if phantom:
        order_violations.append(f"staging froze never-received input(s) {phantom} (§7.3)")

    # Input loss (received but never consumed) is a SEPARATE, completeness/policy
    # concern (§7.3 late-sample policy), not an order-determinism break -- it still
    # fails conformance, but doesn't flip `deterministic`.
    completeness_violations: list[str] = []
    lost = sorted(recv_set - set(all_frozen) - dropped)
    if lost:
        head = lost if len(lost) <= 10 else lost[:10] + ["..."]
        completeness_violations.append(
            f"{len(lost)} received input(s) never staged into a tick nor dropped "
            f"{head} -- declare a late-sample policy or pace the clock so input lands "
            "in ticks (§7.2/§7.3)")

    staging = {
        "input_mode": mode,
        "inputs_received": len(received),
        "inputs_staged": len(all_frozen),
        "inputs_dropped": len(dropped),
        "inputs_unstaged": len(lost),
        "ticks_with_input": len(frozen),
        "deterministic": not order_violations,
    }
    return staging, order_violations + completeness_violations


def analyze_behavior(events: list[dict]) -> tuple[dict, list[str]]:
    """Report the vECU behavior (§11 L2) and sanity-check its per-tick output.
    Behavior *reproducibility* (same tick-indexed input -> same output) is verified
    by golden-scenario comparison (compare_golden.py, §11 L2), not here; this just
    surfaces which behavior ran and that each output carries a numeric value."""
    starts = [e for e in events if e.get("event") == "behavior_start"]
    outputs = [e for e in events if e.get("event") == "behavior_output"]
    if not starts and not outputs:
        return {"name": "none", "outputs": 0}, []
    name = starts[-1].get("name") if starts else outputs[0].get("behavior", "unknown")
    violations: list[str] = []
    missing_val = [o.get("tick_seq") for o in outputs
                   if not isinstance(o.get("value"), (int, float))]
    if missing_val:
        violations.append(f"behavior_output without a numeric value at tick(s) {missing_val[:10]} (§11 L2)")
    return {"name": name, "outputs": len(outputs)}, violations


# Plant feedback continuity tolerance -- feedback_in[N] should equal output[N-1]
# exactly (it IS the previous output fed back), but doubles round-trip through JSON
# text, so allow a tiny epsilon.
PLANT_EPS = 1e-9


def _finite(x) -> bool:
    return isinstance(x, (int, float)) and x == x and x not in (float("inf"), float("-inf"))


def analyze_plant(events: list[dict]) -> tuple[dict, list[str]]:
    """Check IPlant closed-loop determinism (§9 L3, §7 IPlant). Returns (plant_info, violations).

    A run is plant-coupled iff it carries `plant_step` events -- each records one
    tick of the loop: the controller read the plant's previous output (`feedback_in`),
    computed a `command`, and the plant integrated one `dt` to a new `output`.

    L3 ("plant-coupled fidelity, FMU-based closed loop") is evidenced only when the
    loop is genuinely CLOSED and LIVE:
      - closed loop: feedback_in[N] == output[N-1] -- the controller consumed the
        real previous plant output, not a fixed reference. Broken continuity means
        the loop is actually open (a real defect for a live plant).
      - live: the plant responds to commands (mode="live", e.g. AnalyticPlant/FMU),
        as opposed to mode="replay" which plays back recorded outputs (deterministic
        and reproducible, but NOT plant-coupled -- like input replay in §7.4).
      - deterministic: constant dt, monotone ticks, all-finite outputs (FMU step
        NaN/Inf is a fail-fast condition per §14).
    Model correlation against a reference trajectory (§9) is verified by
    compare_golden (plant_step is a compared event), not here."""
    steps = [e for e in events if e.get("event") == "plant_step"]
    if not steps:
        return {"model": "none"}, []

    resets = [e for e in events if e.get("event") == "plant_reset"]
    model = resets[-1].get("model") if resets else steps[0].get("model", "unknown")
    mode = (resets[-1].get("mode") if resets else steps[0].get("mode")) or "live"
    violations: list[str] = []

    # finite outputs/commands (§14 fail-fast on FMU NaN/Inf)
    nonfinite = [s.get("tick_seq") for s in steps
                 if not _finite(s.get("output")) or not _finite(s.get("command"))]
    if nonfinite:
        violations.append(f"plant_step produced non-finite output/command at tick(s) "
                          f"{nonfinite[:10]} -- FMU step NaN/Inf is fail-fast (§14)")

    # deterministic integration: monotone ticks + constant dt
    ticks = [s.get("tick_seq") for s in steps]
    monotone = all(isinstance(a, int) and isinstance(b, int) and b > a
                   for a, b in zip(ticks, ticks[1:]))
    if not monotone:
        violations.append("plant steps not in strictly increasing tick order")
    dts = {s.get("dt_ns") for s in steps if "dt_ns" in s}
    if len(dts) > 1:
        violations.append(f"plant dt not constant across steps: saw {sorted(dts)} "
                          "-- integration step must be fixed for a deterministic trajectory")

    # closed-loop continuity: feedback_in[N] == output[N-1]. Only meaningful for a
    # live plant; a replay plant ignores the command so the loop isn't "live".
    closed = True
    if mode == "live" and monotone and not nonfinite:
        for prev, cur in zip(steps, steps[1:]):
            fb, out_prev = cur.get("feedback_in"), prev.get("output")
            if _finite(fb) and _finite(out_prev) and abs(fb - out_prev) > PLANT_EPS:
                closed = False
                violations.append(
                    f"plant loop not closed at tick {cur.get('tick_seq')}: feedback_in "
                    f"{fb} != previous output {out_prev} -- the controller did not consume "
                    "the live plant output (loop is open, not L3 closed-loop §9)")
                break
    elif mode == "replay":
        closed = False  # playback, not a live coupled loop

    deterministic = monotone and len(dts) <= 1 and not nonfinite
    plant = {
        "model": model,
        "mode": mode,
        "steps": len(steps),
        "closed_loop": closed,
        "deterministic": deterministic,
    }
    return plant, violations


def evaluate_run(events: list[dict], path: str) -> dict:
    """Reduce one run's merged event stream to a verdict + violations."""
    run_id = dominant_run_id(events)
    names = [e.get("event") for e in events]
    violations: list[str] = []

    # --- terminal verdict (§5.2) --------------------------------------------
    verdict = "INCOMPLETE"
    terminal_source = None
    for e in events:  # last coordinator terminal wins if several somehow appear
        if e.get("event") in TERMINAL:
            verdict = TERMINAL[e["event"]]
            terminal_source = "coordinator"
    if verdict == "INCOMPLETE":  # no coordinator terminal -> fall back to verifier
        for e in events:
            if e.get("event") in VERIFIER_TERMINAL:
                verdict = VERIFIER_TERMINAL[e["event"]]
                terminal_source = "verifier"
    if verdict == "INCOMPLETE" and events:
        violations.append(
            "no terminal event (run_pass/run_fail/verify_pass/verify_fail/"
            "ready_timeout/start_failed/aborted_orphaned) -- trace truncated "
            "or run still in flight"
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
                "PASS without evidence: terminal PASS but no sample_received "
                "(§11 L0 requires >=1 received sample)"
            )
        if endpoints_matched < 1:
            violations.append(
                "PASS without endpoint match: terminal PASS but no endpoint_matched "
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

    # --- clock mode & lockstep determinism (§7/§8) --------------------------
    clock, lockstep_violations = analyze_lockstep(events)
    violations.extend(lockstep_violations)

    # --- input staging determinism (TickInbox, §7.3) ------------------------
    staging, staging_violations = analyze_staging(events)
    violations.extend(staging_violations)

    # --- behavior (§11 L2) --------------------------------------------------
    behavior, behavior_violations = analyze_behavior(events)
    violations.extend(behavior_violations)

    # --- plant coupling (§9 L3, IPlant closed loop) -------------------------
    plant, plant_violations = analyze_plant(events)
    violations.extend(plant_violations)
    # L3 is evidenced only by a live, closed, deterministic plant loop (on top of
    # the L0 endpoint/sample evidence). Replay/broken/open plants do not lift it.
    if (plant.get("model", "none") != "none" and plant.get("mode") == "live"
            and plant.get("closed_loop") and plant.get("deterministic")
            and fidelity == "L0"):
        fidelity = "L3"

    # --- identity (probe carries the vECU instance_id) ----------------------
    instance_ids = sorted({e["instance_id"] for e in events if e.get("instance_id")})

    # --- duration (best-effort, from the wall_time_ms span) -----------------
    # Coordinator events are single-writer wall_time_ms; probe/verify share the
    # host clock, so max-min over all events approximates the run's wall duration.
    # Reported for CI dashboards (e.g. JUnit testcase time); 0.0 if <2 timestamps.
    wall_times = [e["wall_time_ms"] for e in events
                  if isinstance(e.get("wall_time_ms"), (int, float))]
    duration_sec = round((max(wall_times) - min(wall_times)) / 1000.0, 3) \
        if len(wall_times) >= 2 else 0.0

    # A run is CONFORMANT iff it reached PASSED with zero violations. Anything
    # else (FAILED/ABORTED/INCOMPLETE, or PASSED-with-violations) is not.
    conformant = verdict == "PASSED" and not violations

    return {
        "run_id": run_id,
        "trace": path,
        "verdict": verdict,
        "terminal_source": terminal_source,
        "conformant": conformant,
        "duration_sec": duration_sec,
        "clock": clock,
        "staging": staging,
        "behavior": behavior,
        "plant": plant,
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
        "checked_completeness": expected_instance_ids is not None,
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
