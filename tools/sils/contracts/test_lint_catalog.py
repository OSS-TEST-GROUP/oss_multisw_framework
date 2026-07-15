#!/usr/bin/env python3
"""Self-contained checks for lint_catalog.py -- no pytest, just asserts.

Run: python3 tools/sils/contracts/test_lint_catalog.py
"""

from __future__ import annotations

import copy
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import lint_catalog as lc  # noqa: E402

ROOT = lc.tc.repo_root()
REAL_IDL = "idl/dds/vehicle_info_mgr/VissRequest.idl"  # exists in the repo


def good_qos():
    return {"p1": {"reliability": "RELIABLE", "durability": "VOLATILE",
                   "history": {"kind": "KEEP_LAST", "depth": 10}}}


def good_contract():
    return {
        "id": "c1", "semantic_version": "0.1.0", "transport": "dds",
        "domain_env": "VISS_DDS_DOMAIN_ID", "idl": REAL_IDL, "struct": "VissRequestTopic",
        "registered_type": "VissRequestTopic", "topic": "/VIN/Vehicle",
        "qos_profile": "p1", "ownership": "single",
        "roles": {"sut": "subscribe", "vecu": "publish"},
    }


def run(contracts, qos=None):
    return lc.lint({"contracts": contracts}, qos if qos is not None else good_qos(), ROOT)


def has(msgs, *needles):
    return any(all(n in m for n in needles) for m in msgs)


def test_clean_passes():
    errors, warnings = run([good_contract()])
    assert errors == [], errors
    assert warnings == [], warnings


def test_missing_required_field():
    c = good_contract(); del c["domain_env"]
    errors, _ = run([c])
    assert has(errors, "missing required field", "domain_env"), errors


def test_duplicate_id():
    errors, _ = run([good_contract(), good_contract()])
    assert has(errors, "duplicate contract id"), errors


def test_unknown_domain_env_typo():
    c = good_contract(); c["domain_env"] = "VIS_DDS_DOMAIN_ID"  # typo
    errors, _ = run([c])
    assert has(errors, "domain_env", "unknown"), errors


def test_dangling_qos_reference():
    c = good_contract(); c["qos_profile"] = "nope"
    errors, _ = run([c])
    assert has(errors, "qos_profile 'nope' not in QoS Catalog"), errors


def test_missing_idl_file():
    c = good_contract(); c["idl"] = "idl/dds/does/not/exist.idl"
    errors, _ = run([c])
    assert has(errors, "idl file", "not found"), errors


def test_invalid_enums():
    c = good_contract(); c["ownership"] = "many"; c["transport"] = "someip"
    errors, _ = run([c])
    assert has(errors, "ownership 'many' invalid"), errors
    assert has(errors, "transport 'someip' invalid"), errors


def test_duplicate_topic_domain():
    c2 = good_contract(); c2["id"] = "c2"  # same topic + domain_env as c1
    errors, _ = run([good_contract(), c2])
    assert has(errors, "ambiguous"), errors


def test_bad_semver():
    c = good_contract(); c["semantic_version"] = "1.0"
    errors, _ = run([c])
    assert has(errors, "semantic_version", "malformed"), errors


def test_roles_missing_and_invalid():
    c = good_contract(); c["roles"] = {"sut": "sideways"}  # missing vecu, bad sut dir
    errors, _ = run([c])
    assert has(errors, "roles missing 'vecu'"), errors
    assert has(errors, "roles.sut 'sideways' invalid"), errors


def test_roles_no_subscriber_warns():
    c = good_contract(); c["roles"] = {"sut": "publish", "vecu": "publish"}
    errors, warnings = run([c])
    assert errors == [], errors
    assert has(warnings, "no subscribing role"), warnings


def test_qos_enum_and_depth():
    bad_qos = {
        "bad-rel": {"reliability": "SORTA", "durability": "VOLATILE"},
        "bad-dur": {"reliability": "RELIABLE", "durability": "FOREVER"},
        "bad-hist": {"history": {"kind": "KEEP_SOME", "depth": 1}},
        "no-depth": {"history": {"kind": "KEEP_LAST"}},
        "zero-depth": {"history": {"kind": "KEEP_LAST", "depth": 0}},
    }
    # reference them all so they aren't also flagged as orphans (focus on enum errors)
    contracts = []
    for i, name in enumerate(bad_qos):
        c = good_contract(); c["id"] = f"c{i}"; c["topic"] = f"/t{i}"; c["qos_profile"] = name
        contracts.append(c)
    errors, _ = lc.lint({"contracts": contracts}, bad_qos, ROOT)
    assert has(errors, "reliability 'SORTA' invalid"), errors
    assert has(errors, "durability 'FOREVER' invalid"), errors
    assert has(errors, "history.kind 'KEEP_SOME' invalid"), errors
    assert has(errors, "no-depth", "positive integer depth"), errors
    assert has(errors, "zero-depth", "positive integer depth"), errors


def test_orphan_qos_warning():
    qos = good_qos(); qos["unused"] = {"reliability": "RELIABLE"}
    _, warnings = run([good_contract()], qos)
    assert has(warnings, "qos-profile 'unused'", "dead config"), warnings


def test_encoding_warning():
    c = good_contract()
    c["payload"] = {"fields": [{"name": "payload", "type": "string", "encoding": "protobuf"}]}
    errors, warnings = run([c])
    assert errors == [], errors
    assert has(warnings, "encoding 'protobuf' unrecognized"), warnings


def _write_catalogs(contracts, qos):
    import yaml
    d = Path(tempfile.mkdtemp(prefix="sils-lint-"))
    (d / "cat.yaml").write_text(yaml.safe_dump({"contracts": contracts}))
    (d / "qos.yaml").write_text(yaml.safe_dump(qos))
    return d


def test_main_exit_codes(capsys=None):
    d = _write_catalogs([good_contract()], good_qos())
    argv_ok = ["--catalog", "cat.yaml", "--qos", "qos.yaml", "--repo-root", str(ROOT)]
    # NOTE: --catalog is relative to --repo-root; point repo-root at the temp dir but
    # the idl path must still resolve, so keep repo-root=ROOT and use absolute-ish paths.
    # Simpler: write catalogs under ROOT-relative temp is awkward; instead pass repo-root=d
    # and an idl that exists there. Recreate with a real idl copied in.
    (d / "idl").mkdir(parents=True, exist_ok=True)
    idlp = d / REAL_IDL
    idlp.parent.mkdir(parents=True, exist_ok=True)
    idlp.write_text("// stub idl for lint test\n")
    rc = lc.main(["--catalog", "cat.yaml", "--qos", "qos.yaml", "--repo-root", str(d)])
    assert rc == 0, "clean catalog should exit 0"

    # error case: dangling qos
    c = good_contract(); c["qos_profile"] = "ghost"
    d2 = _write_catalogs([c], good_qos())
    (d2 / REAL_IDL).parent.mkdir(parents=True, exist_ok=True)
    (d2 / REAL_IDL).write_text("// stub\n")
    assert lc.main(["--catalog", "cat.yaml", "--qos", "qos.yaml", "--repo-root", str(d2)]) == 1

    # strict: an orphan-qos warning becomes a failure
    qos = good_qos(); qos["unused"] = {"reliability": "RELIABLE"}
    d3 = _write_catalogs([good_contract()], qos)
    (d3 / REAL_IDL).parent.mkdir(parents=True, exist_ok=True)
    (d3 / REAL_IDL).write_text("// stub\n")
    assert lc.main(["--catalog", "cat.yaml", "--qos", "qos.yaml", "--repo-root", str(d3)]) == 0
    assert lc.main(["--catalog", "cat.yaml", "--qos", "qos.yaml", "--repo-root", str(d3), "--strict"]) == 1

    # missing file -> usage error
    assert lc.main(["--catalog", "nope.yaml", "--qos", "qos.yaml", "--repo-root", str(d3)]) == 2


def test_real_catalog_is_clean():
    # The shipped catalog must lint clean (errors == 0); orphan-qos warnings are OK.
    catalog = lc.tc.load_yaml(ROOT / "config/sils/contracts/mmf-endpoints.yaml")
    qos = lc.tc.load_yaml(ROOT / "config/sils/qos/qos-profiles.yaml")
    errors, _ = lc.lint(catalog, qos, ROOT)
    assert errors == [], errors


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
        except Exception as e:  # noqa: BLE001
            failed += 1
            print(f"ERROR {t.__name__}: {type(e).__name__}: {e}")
    print(f"\n{len(tests) - failed}/{len(tests)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
