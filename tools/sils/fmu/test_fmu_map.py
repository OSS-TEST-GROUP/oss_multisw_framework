#!/usr/bin/env python3
"""Self-contained checks for fmu_map.py -- no pytest, just asserts.

Run: python3 tools/sils/fmu/test_fmu_map.py
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import fmu_map as fm  # noqa: E402

# FMI 3.0 -- a subset of the real Reference-FMUs Feedthrough (typed elements).
FMI3 = """<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription fmiVersion="3.0" modelName="Feedthrough">
  <ModelVariables>
    <Float64 name="time" valueReference="0" causality="independent"/>
    <Float32 name="Float32_continuous_input" valueReference="1" causality="input" start="0"/>
    <Float32 name="Float32_continuous_output" valueReference="2" causality="output"/>
    <Float64 name="Float64_continuous_input" valueReference="7" causality="input" start="0"/>
    <Float64 name="Float64_continuous_output" valueReference="8" causality="output"/>
  </ModelVariables>
</fmiModelDescription>"""

# FMI 2.0 -- ScalarVariable/Real with units.
FMI2 = """<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription fmiVersion="2.0" modelName="Speed">
  <ModelVariables>
    <ScalarVariable name="throttle" valueReference="0" causality="input"><Real unit="1" start="0"/></ScalarVariable>
    <ScalarVariable name="speed" valueReference="3" causality="output"><Real unit="m/s"/></ScalarVariable>
  </ModelVariables>
</fmiModelDescription>"""


def test_fmi3_feedthrough_resolves():
    r, errors = fm.resolve(FMI3, "Float64_continuous_input", "Float64_continuous_output")
    assert errors == [], errors
    assert r["input_vr"] == 7 and r["output_vr"] == 8, r
    assert r["fmi_version"] == "3.0"


def test_fmi2_real_resolves_with_unit():
    r, errors = fm.resolve(FMI2, "throttle", "speed",
                           expect_output_unit="m/s")
    assert errors == [], errors
    assert r["input_vr"] == 0 and r["output_vr"] == 3
    assert r["fmi_version"] == "2.0"
    assert r["output_unit"] == "m/s"


def test_float32_input_rejected():
    _, errors = fm.resolve(FMI3, "Float32_continuous_input", "Float64_continuous_output")
    assert any("Float32" in e and "real-valued" in e for e in errors), errors


def test_unknown_name_errors():
    _, errors = fm.resolve(FMI3, "nope", "Float64_continuous_output")
    assert any("not found" in e for e in errors), errors


def test_wrong_causality_errors():
    # use the output var as the input -> causality mismatch
    _, errors = fm.resolve(FMI3, "Float64_continuous_output", "Float64_continuous_output")
    assert any("causality" in e and "expected 'input'" in e for e in errors), errors


def test_unit_mismatch_errors():
    _, errors = fm.resolve(FMI2, "throttle", "speed", expect_output_unit="km/h")
    assert any("unit" in e and "km/h" in e for e in errors), errors


def test_autonomous_output_only():
    # no input mapped (autonomous FMU): only the output resolves.
    r, errors = fm.resolve(FMI3, None, "Float64_continuous_output")
    assert errors == [], errors
    assert "input_vr" not in r and r["output_vr"] == 8


def test_malformed_xml():
    _, errors = fm.resolve("<not-xml", "a", "b")
    assert any("malformed" in e for e in errors), errors


def main() -> int:
    tests = [v for k, v in globals().items() if k.startswith("test_") and callable(v)]
    failed = 0
    for t in tests:
        try:
            t()
            print(f"PASS {t.__name__}")
        except AssertionError as e:
            failed += 1
            print(f"FAIL {t.__name__}: {e}")
    print(f"\n{len(tests) - failed}/{len(tests)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
