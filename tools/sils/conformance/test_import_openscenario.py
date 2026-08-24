"""Tests for import_openscenario.py -- OpenSCENARIO (.xosc) -> requirements import."""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import evaluate as ev  # noqa: E402
import import_openscenario as imp  # noqa: E402

HERE = Path(__file__).resolve().parent
SAMPLE = HERE / "samples" / "example.xosc"

XOSC_MIN = """<?xml version="1.0"?>
<OpenSCENARIO>
  <FileHeader author="t" description="d"/>
  <Entities><ScenarioObject name="Ego"/></Entities>
  <Storyboard><StopTrigger><ConditionGroup>
    <Condition name="spd"><ByEntityCondition><EntityCondition>
      <SpeedCondition rule="lessThan" value="30.0"/>
    </EntityCondition></ByEntityCondition></Condition>
    <Condition name="acc"><ByEntityCondition><EntityCondition>
      <AccelerationCondition rule="greaterThan" value="-8.0"/>
    </EntityCondition></ByEntityCondition></Condition>
    <Condition name="crash"><ByEntityCondition><EntityCondition>
      <CollisionCondition/>
    </EntityCondition></ByEntityCondition></Condition>
  </ConditionGroup></StopTrigger></Storyboard>
</OpenSCENARIO>"""


def test_maps_speed_and_acceleration():
    r = imp.parse_xosc(XOSC_MIN, src="t.xosc")
    reqs = {req["name"]: req for req in r["requirements"]}
    assert len(reqs) == 2
    assert reqs["spd"]["signal"] == "state_received.vx"
    assert reqs["spd"]["assert"] == {"in_range": [None, 30.0]}      # lessThan -> upper bound
    assert reqs["acc"]["signal"] == "control_published.acceleration"
    assert reqs["acc"]["assert"] == {"in_range": [-8.0, None]}       # greaterThan -> lower bound
    # traceability carried back to the .xosc condition
    assert reqs["spd"]["source"]["condition"] == "SpeedCondition"


def test_unmapped_conditions_reported_not_dropped():
    r = imp.parse_xosc(XOSC_MIN)
    kinds = {u["condition"] for u in r["unmapped"]}
    assert "CollisionCondition" in kinds  # no collision signal -> reported, not silently lost


def test_scenario_metadata_extracted():
    r = imp.parse_xosc(XOSC_MIN)
    assert r["scenario"]["author"] == "t"
    assert r["scenario"]["entities"] == ["Ego"]


def test_generated_requirements_feed_analyze_requirements():
    # Round trip: .xosc -> requirements -> evaluate.analyze_requirements judges a trace.
    r = imp.parse_xosc(XOSC_MIN)
    spec = {"requirements": r["requirements"]}
    events = [
        {"event": "control_published", "tick_seq": 1, "acceleration": 1.0, "steering_angle": 0.0},
        {"event": "state_received", "tick_seq": 1, "vx": 5.0},
    ]
    info, viol = ev.analyze_requirements(events, spec)
    assert info["mode"] == "checked" and info["total"] == 2
    assert info["passed"] == 2 and viol == []  # vx=5<=30, accel=1>=-8


def test_committed_sample_parses():
    r = imp.parse_xosc(SAMPLE.read_text(encoding="utf-8"), src=SAMPLE.name)
    assert len(r["requirements"]) == 2      # Speed + Acceleration
    assert len(r["unmapped"]) == 2          # Collision + TraveledDistance
    # YAML render is valid + consumable
    import yaml
    body = yaml.safe_load(imp.to_yaml(r))
    assert len(body["requirements"]) == 2


def test_main_writes_yaml():
    # tempfile (not pytest's tmp_path) so this file also self-runs under plain python3.
    out = Path(tempfile.mkdtemp(prefix="sils-xosc-test-")) / "req.yaml"
    rc = imp.main([str(SAMPLE), "-o", str(out)])
    assert rc == 0 and out.exists()
    spec, err = ev.load_requirements(out)
    assert err is None and len(spec["requirements"]) == 2


def test_main_fails_loudly_without_pyyaml(monkeypatch=None):
    """PyYAML absent must be exit 2, never a placeholder file written with exit 0 --
    otherwise gate.py --requirements fails later with the cause hidden."""
    import builtins
    out = Path(tempfile.mkdtemp(prefix="sils-xosc-noyaml-")) / "req.yaml"
    real_import = builtins.__import__

    def blocked(name, *a, **kw):
        if name == "yaml":
            raise ImportError("blocked for test")
        return real_import(name, *a, **kw)

    builtins.__import__ = blocked
    try:
        rc = imp.main([str(SAMPLE), "-o", str(out)])
    finally:
        builtins.__import__ = real_import
    assert rc == 2, f"expected exit 2 without PyYAML, got {rc}"
    assert not out.exists(), "must not write a placeholder requirements file"


# --- parameterized scenarios (Web.Auto Scenario Editor emits these) ------------------

def _xosc_param(decls: str, value: str) -> str:
    return f'''<?xml version="1.0"?>
<OpenSCENARIO>
  <FileHeader author="editor" description="parameterized"/>
  <ParameterDeclarations>{decls}</ParameterDeclarations>
  <Entities><ScenarioObject name="ego"/></Entities>
  <Storyboard><StopTrigger><ConditionGroup>
    <Condition name="spd" conditionEdge="sticky"><ByEntityCondition><EntityCondition>
      <SpeedCondition rule="lessThan" value="{value}"/>
    </EntityCondition></ByEntityCondition></Condition>
  </ConditionGroup></StopTrigger></Storyboard>
</OpenSCENARIO>'''


DECL = '<ParameterDeclaration name="vmax" parameterType="double" value="16.7"/>'


def test_parameter_reference_is_resolved():
    r = imp.parse_xosc(_xosc_param(DECL, "$vmax"), src="p.xosc")
    assert len(r["requirements"]) == 1, r["unmapped"]
    req = r["requirements"][0]
    assert req["assert"] == {"in_range": [None, 16.7]}
    # the authored form survives for traceability
    assert req["source"]["parameter"] == "$vmax" and req["source"]["value"] == 16.7
    assert r["scenario"]["parameters"]["vmax"] == "16.7"


def test_literal_value_still_works_and_carries_no_parameter_key():
    r = imp.parse_xosc(_xosc_param(DECL, "16.7"))
    assert len(r["requirements"]) == 1
    assert "parameter" not in r["requirements"][0]["source"]


def test_chained_parameter_reference():
    decls = ('<ParameterDeclaration name="a" value="$b"/>'
             '<ParameterDeclaration name="b" value="9.5"/>')
    r = imp.parse_xosc(_xosc_param(decls, "$a"))
    assert r["requirements"][0]["assert"] == {"in_range": [None, 9.5]}


def test_cyclic_parameter_is_reported_not_hung():
    decls = ('<ParameterDeclaration name="a" value="$b"/>'
             '<ParameterDeclaration name="b" value="$a"/>')
    r = imp.parse_xosc(_xosc_param(decls, "$a"))
    assert r["requirements"] == []
    assert "cyclic" in r["unmapped"][0]["reason"]


def test_undeclared_parameter_is_reported():
    r = imp.parse_xosc(_xosc_param(DECL, "$nope"))
    assert r["requirements"] == []
    assert "undeclared parameter '$nope'" in r["unmapped"][0]["reason"]


def test_expression_is_not_evaluated():
    r = imp.parse_xosc(_xosc_param(DECL, "${vmax + 1}"))
    assert r["requirements"] == []
    assert "not evaluated" in r["unmapped"][0]["reason"]


def test_shadowed_parameter_keeps_first_and_warns():
    decls = ('<ParameterDeclaration name="vmax" value="16.7"/>'
             '<ParameterDeclaration name="vmax" value="99.0"/>')
    r = imp.parse_xosc(_xosc_param(decls, "$vmax"))
    assert r["requirements"][0]["assert"] == {"in_range": [None, 16.7]}   # first wins
    assert r["scenario"]["parameters_shadowed"] == "vmax"
    assert "WARNING" in imp.to_yaml(r)


def main() -> int:
    tests = [v for k, v in sorted(globals().items())
             if k.startswith("test_") and callable(v)]
    failed = 0
    for t in tests:
        try:
            t()
            print(f"PASS {t.__name__}")
        except AssertionError as e:
            failed += 1
            print(f"FAIL {t.__name__}: {e}")
        except Exception as e:  # noqa: BLE001
            failed += 1
            print(f"ERROR {t.__name__}: {type(e).__name__}: {e}")
    print(f"\n{len(tests) - failed}/{len(tests)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
