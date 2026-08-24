#!/usr/bin/env python3
"""import_openscenario.py -- import an ASAM OpenSCENARIO (.xosc) file into the SiLS
functional-requirements format (docs/sils/sils-functional-verification-interop.md §4/§6 b).

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

Parameters: OpenSCENARIO lets a condition write `value="$targetSpeed"` and declare the
value in a `<ParameterDeclarations>` block. Authoring tools lean on this heavily -- the
Web.Auto Scenario Editor emits parameterized scenarios -- so an importer that only read
literals would drop most real conditions as "not translatable". `$name` references are
resolved here (see resolve_parameters). Scoping is NOT honored: every
`ParameterDeclaration` in the document is collected into one flat table, so a
Story-scoped parameter shadowing a global one of the same name is not modeled. Arithmetic
expressions (`${a + b}`, OpenSCENARIO 1.1) are NOT evaluated -- they are reported as
unmapped with that reason rather than guessed at.

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


def collect_parameters(root: ET.Element) -> dict[str, str]:
    """Flat table of every `<ParameterDeclaration name=.. value=../>` in the document.

    Scope is deliberately ignored (see module docstring): OpenSCENARIO allows declarations
    at Storyboard/Story/Act level, but honoring that needs a scope stack we have no real
    scenario to justify yet. A duplicate name keeps the FIRST declaration and the caller
    reports the shadowing, so a silently wrong substitution can't happen unnoticed."""
    params: dict[str, str] = {}
    shadowed: list[str] = []
    for e in root.iter():
        if _tag(e) != "ParameterDeclaration":
            continue
        name, value = e.get("name"), e.get("value")
        if not name or value is None:
            continue
        if name in params and params[name] != value:
            shadowed.append(name)
            continue
        params.setdefault(name, value)
    if shadowed:
        params["__shadowed__"] = ",".join(sorted(set(shadowed)))
    return params


def resolve_parameters(value: str | None, params: dict[str, str]) -> tuple[str | None, str | None]:
    """Resolve an OpenSCENARIO `$name` parameter reference. Returns (resolved, error).

    - a plain literal passes straight through;
    - `$name` is looked up, and chained references ($a -> $b -> 3.0) are followed with a
      depth cap so a cyclic declaration cannot hang the importer;
    - `${...}` arithmetic (OpenSCENARIO 1.1) is NOT evaluated -- returning an error keeps
      it in the honest `unmapped:` list instead of silently guessing a threshold."""
    if value is None:
        return None, None
    if value.startswith("${"):
        return None, "parameter expression '${...}' is not evaluated by this importer"
    seen: list[str] = []
    cur = value
    for _ in range(8):                      # depth cap == cycle guard
        if not cur.startswith("$"):
            return cur, None
        name = cur[1:]
        if name in seen:
            return None, f"cyclic parameter reference: {' -> '.join(seen + [name])}"
        seen.append(name)
        if name not in params:
            return None, f"undeclared parameter '${name}' (no matching ParameterDeclaration)"
        cur = params[name]
    return None, f"parameter reference nested too deeply: {' -> '.join(seen)}"


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
    params = collect_parameters(root)
    shadowed = params.pop("__shadowed__", None)

    requirements: list[dict] = []
    unmapped: list[dict] = []
    idx = 0
    for cond in (e for e in root.iter() if _tag(e) == "Condition"):
        cname = cond.get("name", "")
        for e in cond.iter():
            t = _tag(e)
            if t in MAPPABLE:
                signal, kind = MAPPABLE[t]
                rule, raw = e.get("rule"), e.get("value")
                value, perr = resolve_parameters(raw, params)
                rng = None
                if perr is None and rule is not None and value is not None:
                    try:
                        rng = _rule_to_range(rule, float(value))
                    except ValueError:
                        rng = None
                if rng is None:
                    reason = perr or f"rule '{rule}' / value '{raw}' not translatable"
                    unmapped.append({"condition": t, "name": cname, "reason": reason})
                    continue
                source = {"xosc": src, "condition": t, "rule": rule, "value": float(value)}
                if raw != value:          # keep the authored form for traceability
                    source["parameter"] = raw
                idx += 1
                requirements.append({
                    "id": f"XOSC-{idx}",
                    "name": cname or f"{kind}-{idx}",
                    "signal": signal,
                    "assert": {"in_range": rng},
                    "source": source,
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
    if params:
        scenario["parameters"] = dict(sorted(params.items()))
    if shadowed:
        scenario["parameters_shadowed"] = shadowed

    return {"scenario": scenario, "requirements": requirements, "unmapped": unmapped}


class MissingDependency(RuntimeError):
    """PyYAML is absent, so we cannot render the requirements file at all."""


def to_yaml(result: dict) -> str:
    """Render the import result as requirements YAML (consumable by evaluate.py
    --requirements). `scenario`/`unmapped` are extra top-level keys analyze_requirements
    ignores; the header flags the polarity caveat and lists what was NOT mapped.

    Raises MissingDependency if PyYAML is absent. It must NOT return a placeholder
    comment: main() would happily write that to -o and exit 0, and feeding the result
    to gate.py --requirements then fails with exit 2 far from the real cause."""
    try:
        import yaml
    except ImportError as e:  # pragma: no cover -- depends on the environment
        raise MissingDependency(
            "PyYAML is required to render requirements YAML "
            "(pip install -r tools/sils/requirements.txt)") from e
    header = [
        "# Generated from OpenSCENARIO by import_openscenario.py -- REVIEW before use.",
        f"# source: {result['scenario'].get('source', '?')}",
        "# NOTE: OpenSCENARIO StopTrigger conditions do not declare pass/fail polarity;",
        "#       each mapped requirement is a BOUND the signal must respect -- verify direction.",
    ]
    if result["scenario"].get("parameters_shadowed"):
        header.append("# WARNING: parameter name(s) declared more than once with different values: "
                      f"{result['scenario']['parameters_shadowed']}")
        header.append("#          scope is not modeled -- the FIRST declaration was used. Verify.")
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

    try:
        text = to_yaml(result)
    except MissingDependency as e:
        print(f"# error: {e}", file=sys.stderr)
        return 2
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
