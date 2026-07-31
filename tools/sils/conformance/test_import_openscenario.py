"""Tests for import_openscenario.py -- OpenSCENARIO (.xosc) -> requirements import."""
from __future__ import annotations

import sys
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


def test_main_writes_yaml(tmp_path):
    out = tmp_path / "req.yaml"
    rc = imp.main([str(SAMPLE), "-o", str(out)])
    assert rc == 0 and out.exists()
    import yaml
    spec, err = ev.load_requirements(out)
    assert err is None and len(spec["requirements"]) == 2
