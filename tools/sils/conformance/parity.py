#!/usr/bin/env python3
"""L4 target-parity judge (§11 Target-like Fidelity) -- compare a SiLS run against a
REAL-TARGET reference trace, gated by an environment attestation.

L4 is NOT "the SiLS trace reproduces itself" (that is golden reproducibility, §12.4,
compare_golden.py). L4 is "the SiLS vECU behaves like the real ECU": the SAME stimulus
is run against the physical/target ECU and against the SiLS vECU, and their observable
interface (behavior output, protocol responses, plant trajectory) must agree within a
contracted tolerance. The target trace is the AUTHORITY; the SiLS trace is judged
against it.

Two things must both hold to claim L4 -- and this is the whole point of the level:
  1. VALUE/TIMING PARITY: the compared events agree within the parity contract's
     per-signal tolerance and timing window (reuses compare_golden's diff engine, with
     target = reference, sils = candidate).
  2. ATTESTATION: the SiLS run actually used the real target-like components the level
     requires -- ARM64 + real gateway/protocol (§11: "ARM64는 L4 구성 요소 중 하나이며
     L4 전체를 자동으로 충족하지 않는다"). A SiLS run whose values happen to match a
     reference but ran on x86 with a stubbed gateway is NOT L4; it is at best L3. The
     attestation is read from the SiLS trace's `target_attestation` event and checked
     against the contract.

Verdicts:
  PARITY              value/timing parity AND attestation satisfied  -> fidelity L4 (exit 0)
  PARITY_UNATTESTED   values match the reference but attestation failed/absent -> L3 (exit 1)
  DIVERGENCE          value/timing diverges from the target reference -> below L4 (exit 1)

Usage:
  parity.py --target target.jsonl --sils sils.jsonl --contract parity-contract.yaml
  parity.py ... --quiet
Exit: 0 PARITY(L4), 1 no-parity (divergence or unattested), 2 usage/IO error.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import compare_golden as cg  # noqa: E402  (reuse the value/timing diff engine)
import evaluate as ev  # noqa: E402       (reuse parse_trace)

EXIT_PARITY = 0
EXIT_NO_PARITY = 1
EXIT_ERROR = 2

# The attestation keys L4 cares about, and the value that counts as "real target-like".
# A missing key, or any value other than the contracted one, fails the attestation.
ATTEST_KEYS = ("arch", "gateway", "protocol")


def load_contract(path: Path) -> tuple[dict, str | None]:
    try:
        import yaml
    except ImportError:  # pragma: no cover
        return {}, "PyYAML not available; cannot read --contract"
    if not path.exists():
        return {}, f"parity contract not found: {path}"
    try:
        return yaml.safe_load(path.read_text(encoding="utf-8")) or {}, None
    except Exception as e:  # noqa: BLE001
        return {}, f"cannot parse parity contract {path}: {e}"


def contract_to_rules(contract: dict) -> dict:
    """Project the parity contract onto compare_golden's rules dict. The target is a real
    (often physical) system, so signals carry per-field numeric tolerance and timing a
    wall-clock window -- unlike golden's default of bit-exact."""
    import copy
    rules = copy.deepcopy(cg.DEFAULT_RULES)
    if contract.get("compare_events"):
        rules["compare_events"] = contract["compare_events"]
    if contract.get("key_fields"):
        rules["key_fields"] = contract["key_fields"]
    # signals -> per-field numeric tolerance (default stays exact unless a signal overrides)
    signals = contract.get("signals") or {}
    rules["numeric_tolerance"] = {"default": contract.get("default_tolerance", {"abs": 0.0, "rel": 0.0}),
                                  "fields": signals}
    rules["latency_window_ms"] = contract.get("timing_window_ms")
    if contract.get("sample_policy"):
        rules["sample_policy"] = contract["sample_policy"]
    return rules


def find_attestation(events: list[dict]) -> dict | None:
    """The last target_attestation event in the SiLS trace (probe emits one at startup)."""
    att = None
    for e in events:
        if e.get("event") == "target_attestation":
            att = e
    return att


def check_attestation(sils_events: list[dict], contract: dict) -> tuple[bool, list[str], dict]:
    """Verify the SiLS run attests the real target-like components the contract requires.
    Returns (ok, failures, observed)."""
    required = contract.get("attestation") or {}
    if not required:
        # No attestation clause: the contract does not gate on environment. Parity then
        # rests on values alone -- but we say so explicitly rather than silently pass.
        return True, [], {"note": "contract declares no attestation clause"}
    att = find_attestation(sils_events)
    if att is None:
        return False, ["SiLS trace has no target_attestation event -- cannot verify the run "
                       "used ARM64/real gateway (§11 L4 attestation)"], {}
    failures: list[str] = []
    observed: dict = {}
    for key in ATTEST_KEYS:
        want = required.get(key)
        if want is None:
            continue
        got = att.get(key)
        observed[key] = got
        if str(got) != str(want):
            failures.append(f"attestation {key}={got!r} but L4 requires {want!r} "
                            "(real target-like component absent)")
    return not failures, failures, observed


def judge(target: list[dict], sils: list[dict], contract: dict) -> dict:
    rules = contract_to_rules(contract)
    comparison = cg.compare(target, sils, rules)
    # Relabel golden/candidate -> target/sils in the user-facing divergence detail.
    for d in comparison.get("field_diffs", []):
        if "golden" in d:
            d["target"] = d.pop("golden")
        if "candidate" in d:
            d["sils"] = d.pop("candidate")

    values_match = comparison["verdict"] == "MATCH"
    attest_ok, attest_failures, observed = check_attestation(sils, contract)

    if values_match and attest_ok:
        verdict, fidelity = "PARITY", "L4"
    elif values_match and not attest_ok:
        verdict, fidelity = "PARITY_UNATTESTED", "L3"
    else:
        verdict, fidelity = "DIVERGENCE", "below-L4"

    return {
        "verdict": verdict,
        "fidelity_evidenced": fidelity,
        "values_match": values_match,
        "attestation_ok": attest_ok,
        "attestation_required": contract.get("attestation") or {},
        "attestation_observed": observed,
        "attestation_failures": attest_failures,
        "comparison": comparison,
        "compared_events": comparison.get("compared_events", 0),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--target", type=Path, required=True,
                        help="real-target reference trace.jsonl (the authority)")
    parser.add_argument("--sils", type=Path, required=True, help="SiLS vECU run trace.jsonl")
    parser.add_argument("--contract", type=Path, required=True, help="parity contract YAML")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)

    contract, cerr = load_contract(args.contract)
    if cerr:
        print(f"# error: {cerr}", file=sys.stderr)
        return EXIT_ERROR
    target, terr = ev.parse_trace(args.target)
    sils, serr = ev.parse_trace(args.sils)
    if terr or serr:
        print(f"# error: {terr or serr}", file=sys.stderr)
        return EXIT_ERROR

    report = judge(target, sils, contract)
    print(json.dumps(report, indent=2))

    if not args.quiet:
        c = report["comparison"]
        print(f"# {report['verdict']} (fidelity {report['fidelity_evidenced']}): "
              f"{report['compared_events']} events compared, "
              f"{len(c['field_diffs'])} field diff(s), "
              f"{len(c['latency_violations'])} timing, "
              f"attestation {'OK' if report['attestation_ok'] else 'FAILED'}", file=sys.stderr)
        for f in report["attestation_failures"]:
            print(f"#   attest: {f}", file=sys.stderr)

    return EXIT_PARITY if report["verdict"] == "PARITY" else EXIT_NO_PARITY


if __name__ == "__main__":
    sys.exit(main())
