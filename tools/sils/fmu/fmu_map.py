#!/usr/bin/env python3
"""Resolve + validate FMU variables by NAME from modelDescription.xml (§11 L3).

FmuPlant (service/sils/vecu/src/plant.hpp) addresses the FMU's input/output by raw
`valueReference` (SILS_FMU_INPUT_VR / SILS_FMU_OUTPUT_VR) -- which means hand-reading
modelDescription.xml and pasting integers. This tool resolves a variable by NAME and
validates it (causality, data type, unit) against what FmuPlant can drive, then emits
the value references so a descriptor / Run Coordinator can bind the FMU by variable
name instead of a brittle integer (FMU guide docs/sils/sils-fmu-plant.md §5.2).

FmuPlant drives Float64 co-simulation I/O only:
  - FMI 3.0: <Float64 name=.. valueReference=.. causality=.. unit=../>
  - FMI 2.0: <ScalarVariable name=.. valueReference=.. causality=..><Real unit=../></ScalarVariable>
so a mapped input/output MUST be that real-valued type; Float32/Int/Bool/String are
rejected (FmuPlant has no setter for them).

Usage:
  fmu_map.py --model-description md.xml  --input <name> --output <name>
  fmu_map.py --fmu model.fmu            --output <name>          # input optional (autonomous FMU)
  fmu_map.py ... --expect-input-unit m/s --expect-output-unit m/s   # optional unit assertions
  fmu_map.py ... --json                 # JSON instead of shell KEY='value'
Exit: 0 resolved, 1 validation error, 2 usage/IO.
"""

from __future__ import annotations

import argparse
import json
import sys
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path

# Real-valued variable types FmuPlant can set/get (FMI 3.0 Float64 / FMI 2.0 Real).
REAL_TYPES = {"Float64", "Real"}
# Output-ish causalities: an output the controller reads back as feedback.
OUTPUT_CAUSALITIES = {"output", "local", "calculated"}


def load_model_description(args) -> tuple[str, str | None]:
    """Return (xml_text, error). Reads --model-description directly, or the
    modelDescription.xml inside a --fmu zip."""
    if args.model_description:
        p = Path(args.model_description)
        if not p.exists():
            return "", f"model-description not found: {p}"
        return p.read_text(encoding="utf-8"), None
    p = Path(args.fmu)
    if not p.exists():
        return "", f"fmu not found: {p}"
    try:
        with zipfile.ZipFile(p) as z:
            return z.read("modelDescription.xml").decode("utf-8"), None
    except (zipfile.BadZipFile, KeyError) as e:
        return "", f"cannot read modelDescription.xml from {p}: {e}"


def parse_variables(md_xml: str) -> tuple[dict, str, str | None]:
    """Parse ModelVariables -> {name: {vr, causality, type, unit}}. Also returns the
    declared FMI version string. Handles FMI 2.0 (ScalarVariable/Real) and 3.0 (typed
    element per variable)."""
    try:
        root = ET.fromstring(md_xml)
    except ET.ParseError as e:
        return {}, "", f"malformed modelDescription.xml: {e}"
    fmi_version = root.get("fmiVersion", "")
    mv = root.find("ModelVariables")
    if mv is None:
        return {}, fmi_version, "modelDescription.xml has no <ModelVariables>"
    out: dict[str, dict] = {}
    for var in mv:
        name = var.get("name")
        vr = var.get("valueReference")
        if name is None or vr is None:
            continue
        causality = var.get("causality") or "local"
        if var.tag == "ScalarVariable":  # FMI 2.0: type + unit in the child element
            child = next(iter(var), None)
            vtype = child.tag if child is not None else "Unknown"
            unit = child.get("unit") if child is not None else None
        else:  # FMI 3.0: the element tag IS the type; unit is an attribute
            vtype = var.tag
            unit = var.get("unit")
        try:
            vr_int = int(vr)
        except ValueError:
            continue
        out[name] = {"vr": vr_int, "causality": causality, "type": vtype, "unit": unit}
    return out, fmi_version, None


def resolve_role(variables: dict, name: str, role: str,
                 expect_unit: str | None, errors: list[str]) -> dict | None:
    """Validate one mapped variable (role = 'input' | 'output') and return its info."""
    v = variables.get(name)
    if v is None:
        errors.append(f"{role} variable '{name}' not found in modelDescription "
                      f"(available: {sorted(variables)[:12]})")
        return None
    if v["type"] not in REAL_TYPES:
        errors.append(f"{role} '{name}' is type {v['type']}, but FmuPlant drives only "
                      f"real-valued I/O ({sorted(REAL_TYPES)}) -- pick a Float64/Real variable")
    if role == "input" and v["causality"] != "input":
        errors.append(f"input '{name}' has causality '{v['causality']}', expected 'input'")
    if role == "output" and v["causality"] not in OUTPUT_CAUSALITIES:
        errors.append(f"output '{name}' has causality '{v['causality']}', expected one of "
                      f"{sorted(OUTPUT_CAUSALITIES)}")
    if expect_unit is not None and v["unit"] != expect_unit:
        errors.append(f"{role} '{name}' unit is {v['unit']!r}, expected {expect_unit!r}")
    return v


def resolve(md_xml: str, input_name: str | None, output_name: str,
            expect_input_unit: str | None = None,
            expect_output_unit: str | None = None) -> tuple[dict, list[str]]:
    variables, fmi_version, err = parse_variables(md_xml)
    if err:
        return {}, [err]
    errors: list[str] = []
    result: dict = {"fmi_version": fmi_version}
    if input_name is not None:
        vi = resolve_role(variables, input_name, "input", expect_input_unit, errors)
        if vi:
            result["input_vr"] = vi["vr"]
            result["input_unit"] = vi["unit"]
    vo = resolve_role(variables, output_name, "output", expect_output_unit, errors)
    if vo:
        result["output_vr"] = vo["vr"]
        result["output_unit"] = vo["unit"]
    return result, errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    src = parser.add_mutually_exclusive_group(required=True)
    src.add_argument("--fmu", help="path to a .fmu (reads its modelDescription.xml)")
    src.add_argument("--model-description", help="path to a modelDescription.xml")
    parser.add_argument("--input", help="FMU input variable name (command); omit for autonomous FMU")
    parser.add_argument("--output", required=True, help="FMU output variable name (feedback)")
    parser.add_argument("--expect-input-unit", default=None)
    parser.add_argument("--expect-output-unit", default=None)
    parser.add_argument("--json", action="store_true", help="emit JSON instead of shell vars")
    args = parser.parse_args(argv)

    md_xml, err = load_model_description(args)
    if err:
        print(f"error: {err}", file=sys.stderr)
        return 2

    result, errors = resolve(md_xml, args.input, args.output,
                             args.expect_input_unit, args.expect_output_unit)
    for e in errors:
        print(f"error: {e}", file=sys.stderr)
    if errors:
        return 1

    if args.json:
        print(json.dumps(result))
    else:
        if "input_vr" in result:
            print(f"RESOLVED_FMU_INPUT_VR={result['input_vr']}")
        print(f"RESOLVED_FMU_OUTPUT_VR={result['output_vr']}")
        print(f"RESOLVED_FMU_FMI_VERSION='{result['fmi_version']}'")
    return 0


if __name__ == "__main__":
    sys.exit(main())
