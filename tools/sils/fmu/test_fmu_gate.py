#!/usr/bin/env python3
"""Self-contained checks for fmu_gate.py -- no pytest, just asserts.

Run: python3 tools/sils/fmu/test_fmu_gate.py
"""

from __future__ import annotations

import sys
import tempfile
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import fmu_gate as g  # noqa: E402

FMI3_MD = """<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription fmiVersion="3.0" modelName="Feedthrough">
  <CoSimulation modelIdentifier="Feedthrough"/>
  <ModelVariables>
    <Float64 name="in" valueReference="7" causality="input"/>
    <Float64 name="out" valueReference="8" causality="output"/>
  </ModelVariables>
</fmiModelDescription>"""

FMI2_MD = """<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription fmiVersion="2.0" modelName="Speed">
  <CoSimulation modelIdentifier="SpeedCS"/>
  <ModelVariables/>
</fmiModelDescription>"""

SHA_A = "aaaa000000000000000000000000000000000000000000000000000000000000"
SHA_B = "bbbb000000000000000000000000000000000000000000000000000000000000"


def _make_fmu(dirpath: Path, name: str, md_xml: str) -> Path:
    p = dirpath / name
    with zipfile.ZipFile(p, "w") as z:
        z.writestr("modelDescription.xml", md_xml)
    return p


def test_allow_by_sha_match():
    al = [{"id": "x", "sha256": SHA_A, "fmi_version": "3.0", "model_identifier": "Feedthrough"}]
    ok, reasons, warns, entry = g.gate(SHA_A, {"fmi_version": "3.0", "model_identifier": "Feedthrough"}, al, None)
    assert ok and not reasons and not warns, (reasons, warns)
    assert entry["id"] == "x"


def test_deny_unknown_sha():
    al = [{"id": "x", "sha256": SHA_A}]
    ok, reasons, _, entry = g.gate(SHA_B, {}, al, None)
    assert not ok and entry is None
    assert any("not in the allowlist" in r for r in reasons), reasons


def test_deny_expect_id_missing():
    al = [{"id": "x", "sha256": SHA_A}]
    ok, reasons, _, _ = g.gate(SHA_A, {}, al, "nope")
    assert not ok and any("no entry" in r for r in reasons), reasons


def test_deny_expect_id_hash_mismatch():
    # entry 'x' is allowlisted, but the bytes we have (SHA_B) are not the approved ones.
    al = [{"id": "x", "sha256": SHA_A}]
    ok, reasons, _, _ = g.gate(SHA_B, {}, al, "x")
    assert not ok and any("does not match the bytes approved" in r for r in reasons), reasons


def test_allow_expect_id_match():
    al = [{"id": "x", "sha256": SHA_A}, {"id": "y", "sha256": SHA_B}]
    ok, reasons, _, entry = g.gate(SHA_A, {}, al, "x")
    assert ok and entry["id"] == "x", (ok, reasons)


def test_metadata_drift_warns_not_denies():
    al = [{"id": "x", "sha256": SHA_A, "fmi_version": "2.0", "model_identifier": "Feedthrough"}]
    ok, reasons, warns, _ = g.gate(SHA_A, {"fmi_version": "3.0", "model_identifier": "Feedthrough"}, al, None)
    assert ok, reasons  # bytes verified -> still allowed
    assert any("fmi_version" in w and "stale" in w for w in warns), warns


def test_read_model_description_fmi3():
    with tempfile.TemporaryDirectory() as d:
        fmu = _make_fmu(Path(d), "ft.fmu", FMI3_MD)
        md, err = g.read_model_description(fmu)
        assert err is None, err
        assert md["fmi_version"] == "3.0" and md["model_identifier"] == "Feedthrough"


def test_read_model_description_fmi2_identifier():
    with tempfile.TemporaryDirectory() as d:
        fmu = _make_fmu(Path(d), "s.fmu", FMI2_MD)
        md, err = g.read_model_description(fmu)
        assert err is None and md["model_identifier"] == "SpeedCS", (md, err)


def test_read_model_description_not_a_zip():
    with tempfile.TemporaryDirectory() as d:
        bad = Path(d) / "bad.fmu"
        bad.write_text("not a zip", encoding="utf-8")
        _, err = g.read_model_description(bad)
        assert err and "cannot read modelDescription" in err, err


def test_sha256_stable_and_bytewise():
    with tempfile.TemporaryDirectory() as d:
        p = Path(d) / "f.bin"
        p.write_bytes(b"hello sils")
        import hashlib
        assert g.sha256_of(p) == hashlib.sha256(b"hello sils").hexdigest()


def test_load_allowlist_missing_denies():
    fmus, err = g.load_allowlist(Path("/no/such/allowlist.yaml"))
    assert fmus == [] and err and "not found" in err, (fmus, err)


def test_provenance_fields():
    with tempfile.TemporaryDirectory() as d:
        fmu = _make_fmu(Path(d), "ft.fmu", FMI3_MD)
        md, _ = g.read_model_description(fmu)
        prov = g.build_provenance(fmu, SHA_A, md, {"id": "x", "supplier": "acme"})
        assert prov["fmu"]["sha256"] == SHA_A
        assert prov["fmu"]["model_identifier"] == "Feedthrough"
        assert prov["allowlist"] == {"id": "x", "supplier": "acme"}
        assert prov["fmu"]["size_bytes"] > 0


def test_record_stanza_has_hash_and_id():
    s = g.record_stanza(SHA_A, {"fmi_version": "3.0", "model_identifier": "Feedthrough"})
    assert SHA_A in s and "id: feedthrough" in s and "FILL-IN" in s, s


def test_to_yaml_quotes_empty_and_colon():
    prov = {"fmu": {"file": "a.fmu", "model_identifier": "", "fmi_version": "3.0"},
            "allowlist": {"id": "x", "supplier": None}}
    y = g.to_yaml(prov)
    assert 'model_identifier: ""' in y and "supplier: " in y and "file: a.fmu" in y, y


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
