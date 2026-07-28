#!/usr/bin/env python3
"""import_openscenario.py -- import an ASAM OpenSCENARIO (.xosc) file into the SiLS
functional-requirements format (docs/sils-functional-verification-interop.md §4/§6 b).

The reuse thesis: a person who authored driving scenarios in another tool (CarMaker,
MORAI, Autoware scenario_simulator_v2, Simulink) should bring their .xosc with MINIMAL
change, not re-author. OpenSCENARIO is the cross-tool standard they all speak. This
importer maps the scenario's conditions to the requirements.yaml that evaluate.py's
analyze_requirements already checks -- so the pass/fail criteria come for free.

What it does:
  - scenario metadata (FileHeader author/description, entities) -> a `scenario:` block;
  - conditions whose SIGNAL exists in our trace vocabulary -> `requirements:` entries
    (SpeedCondition -> state_received.vx, AccelerationCondition -> control_published.
    acceleration), each carrying `source:` traceability back to the .xosc condition;
  - conditions we CANNOT judge from the current trace (collision, traveled distance,
    position, TTC ...) -> an explicit `unmapped:` list with the reason. NEVER silently
    dropped -- an importer that hid them would make a scenario look fully covered when
    it is not.

What it does NOT do (honest scope):
  - It does not EXECUTE the scenario (that needs a simulator + the plant bridge, #1). It
    imports the SPEC only.
  - OpenSCENARIO StopTrigger conditions do not themselves declare pass vs fail polarity;
    this maps `rule` to a bound and records `source`, but the user should REVIEW polarity
    (noted in the output header). Meeting a bound ("speed stays <= 30") is the common case.
  - Only a curated condition subset is mapped; extend MAPPABLE as real .xosc files arrive
    (earned-abstraction), rather than speculatively covering all of OpenSCENARIO.

Usage:
    import_openscenario.py scenario.xosc [-o requirements.yaml]
    import_openscenario.py scenario.xosc | gate.py run.jsonl --requirements /dev/stdin
"""
from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# Conditions whose observable signal exists in the SiLS trace (vehicle_run.hpp emits
# state_received.{vx,yaw,steering} and control_published.{steering_angle,acceleration,gear}).
# condition tag -> (trace signal, short kind).
MAPPABLE = {
    "SpeedCondition": ("state_received.vx", "speed"),
    "AccelerationCondition": ("control_published.acceleration", "acceleration"),
}

# Conditions we recognize but cannot judge from the current trace -- reported, not dropped.
UNMAPPED_KNOWN = {
    "TraveledDistanceCondition": "no odometry/distance signal in trace",
    "ReachPositionCondition": "no position signal in trace (state_received carries vx/yaw/steering only)",
    "DistanceCondition": "no position signal in trace",
    "RelativeDistanceCondition": "needs a second entity's signal (not in trace)",
    "CollisionCondition": "no collision signal (needs the simulator/plant, #1)",
    "TimeToCollisionCondition": "needs the simulator/plant (#1)",
    "StandStillCondition": "duration->tick mapping needs the run dt (future)",
    "OffroadCondition": "needs the simulator/plant (#1)",
}


def _tag(e: ET.Element) -> str:
    """Local tag name, stripping any XML namespace ({ns}Tag -> Tag)."""
    return e.tag.split("}", 1)[-1]


def _rule_to_range(rule: str, value: float) -> list | None:
    """Map an OpenSCENARIO comparison rule + threshold to an in_range bound. Returns None
    for rules we don't translate (notEqualTo)."""
    if rule in ("lessThan", "lessOrEqual"):
        return [None, value]          # signal must stay at/below the threshold
    if rule in ("greaterThan", "greaterOrEqual"):
        return [value, None]          # signal must stay at/above the threshold
    if rule == "equalTo":
        return [value, value]
    return None


def parse_xosc(text: str, src: str = "scenario.xosc") -> dict:
    """Parse an OpenSCENARIO XML string into {scenario, requirements, unmapped}."""
    root = ET.fromstring(text)

    requirements: list[dict] = []
    unmapped: list[dict] = []
    idx = 0
    for cond in (e for e in root.iter() if _tag(e) == "Condition"):
        cname = cond.get("name", "")
        for e in cond.iter():
            t = _tag(e)
            if t in MAPPABLE:
                signal, kind = MAPPABLE[t]
                rule, value = e.get("rule"), e.get("value")
                rng = None
                if rule is not None and value is not None:
                    try:
                        rng = _rule_to_range(rule, float(value))
                    except ValueError:
                        rng = None
                if rng is None:
                    unmapped.append({"condition": t, "name": cname,
                                     "reason": f"rule '{rule}' / value '{value}' not translatable"})
                    continue
                idx += 1
                requirements.append({
                    "id": f"XOSC-{idx}",
                    "name": cname or f"{kind}-{idx}",
                    "signal": signal,
                    "assert": {"in_range": rng},
                    "source": {"xosc": src, "condition": t, "rule": rule, "value": float(value)},
                })
            elif t in UNMAPPED_KNOWN:
                unmapped.append({"condition": t, "name": cname, "reason": UNMAPPED_KNOWN[t]})

    scenario: dict = {"source": src}
    for e in root.iter():
        if _tag(e) == "FileHeader":
            for k in ("author", "description", "revMajor", "revMinor", "date"):
                if e.get(k) is not None:
                    scenario[k] = e.get(k)
            break
    entities = [e.get("name") for e in root.iter()
                if _tag(e) == "ScenarioObject" and e.get("name")]
    if entities:
        scenario["entities"] = entities

    return {"scenario": scenario, "requirements": requirements, "unmapped": unmapped}


def to_yaml(result: dict) -> str:
    """Render the import result as requirements YAML (consumable by evaluate.py
    --requirements). `scenario`/`unmapped` are extra top-level keys analyze_requirements
    ignores; the header flags the polarity caveat and lists what was NOT mapped."""
    try:
        import yaml
    except ImportError:  # pragma: no cover
        return "# error: PyYAML not available to render YAML\n"
    header = [
        "# Generated from OpenSCENARIO by import_openscenario.py -- REVIEW before use.",
        f"# source: {result['scenario'].get('source', '?')}",
        "# NOTE: OpenSCENARIO StopTrigger conditions do not declare pass/fail polarity;",
        "#       each mapped requirement is a BOUND the signal must respect -- verify direction.",
    ]
    if result["unmapped"]:
        header.append("# UNMAPPED conditions (no trace signal -- need the plant/simulator, #1):")
        for u in result["unmapped"]:
            header.append(f"#   - {u['condition']}"
                          f"{(' [' + u['name'] + ']') if u.get('name') else ''}: {u['reason']}")
    body = {"scenario": result["scenario"], "requirements": result["requirements"]}
    return "\n".join(header) + "\n" + yaml.safe_dump(body, sort_keys=False, allow_unicode=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("xosc", type=Path, help="OpenSCENARIO .xosc file")
    parser.add_argument("-o", "--out", type=Path, default=None,
                        help="write requirements YAML here (default: stdout)")
    args = parser.parse_args(argv)

    if not args.xosc.exists():
        print(f"# error: file not found: {args.xosc}", file=sys.stderr)
        return 2
    try:
        result = parse_xosc(args.xosc.read_text(encoding="utf-8"), src=args.xosc.name)
    except ET.ParseError as e:
        print(f"# error: not valid XML: {e}", file=sys.stderr)
        return 2

    text = to_yaml(result)
    if args.out is not None:
        args.out.write_text(text, encoding="utf-8")
        print(f"# wrote {args.out}", file=sys.stderr)
    else:
        sys.stdout.write(text)

    n_map, n_un = len(result["requirements"]), len(result["unmapped"])
    print(f"# mapped {n_map} requirement(s), {n_un} unmapped condition(s)", file=sys.stderr)
    for u in result["unmapped"]:
        print(f"#   unmapped: {u['condition']} -- {u['reason']}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
