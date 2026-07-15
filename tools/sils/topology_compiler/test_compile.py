#!/usr/bin/env python3
"""Self-contained checks for compile.py -- no pytest dependency, just asserts.

Run directly: python3 tools/sils/topology_compiler/test_compile.py
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import compile as tc  # noqa: E402  (path insert must come first)
import resolved_to_shell as rts  # noqa: E402

ROOT = tc.repo_root()
TESTDATA = Path(__file__).resolve().parent / "testdata"


def test_valid_topology_compiles():
    topology_path = ROOT / "config/sils/topologies/viss_probe_smoke.yaml"
    resolved_run, saved_service_list, errors, warnings = tc.compile_topology(
        topology_path, ROOT,
        "config/sils/contracts/mmf-endpoints.yaml",
        "config/sils/qos/qos-profiles.yaml",
        "registry.obigo.com/oss/oss-mmf-sils-vecu:latest",
    )
    assert errors == [], f"expected no errors, got: {errors}"
    assert resolved_run is not None
    assert saved_service_list is not None
    containers = resolved_run["containers"]
    assert len(containers) == 1, f"expected 1 container, got {len(containers)}"
    assert containers[0]["name"] == "viss-probe"
    assert containers[0]["instance_id"] == "viss-probe-1"
    assert len(containers[0]["endpoints"]) == 1
    ep = containers[0]["endpoints"][0]
    assert ep["contract_id"] == "viss-request"
    assert ep["topic"] == "/VIN/Vehicle"
    assert ep["domain_env"] == "VISS_DDS_DOMAIN_ID"

    ssl_containers = saved_service_list["specs"][0]["containers"]
    assert len(ssl_containers) == 1
    assert "VISS_DDS_DOMAIN_ID=0" in " ".join(ssl_containers[0]["extra_args"])
    assert "SILS_VECU_INSTANCE_ID=viss-probe-1" in " ".join(ssl_containers[0]["extra_args"])


def test_replica_violation_detected():
    topology_path = TESTDATA / "topology_replica_violation.yaml"
    resolved_run, saved_service_list, errors, warnings = tc.compile_topology(
        topology_path, ROOT,
        "config/sils/contracts/mmf-endpoints.yaml",
        "config/sils/qos/qos-profiles.yaml",
        "registry.obigo.com/oss/oss-mmf-sils-vecu:latest",
    )
    assert resolved_run is None, "must fail closed: no artifacts on validation error"
    assert saved_service_list is None
    assert len(errors) == 1, f"expected exactly 1 error, got: {errors}"
    # replicas of a single-owner, non-templated topic collide on one topic (§9.5).
    assert "viss-request" in errors[0]
    assert "single" in errors[0] and "expected exactly 1" in errors[0], errors[0]


def test_per_instance_fleet_is_valid():
    # 4 replicas of a per-instance contract -> 4 distinct resolved topics -> §9.5-valid.
    resolved_run, saved_service_list, errors, warnings = tc.compile_topology(
        ROOT / "config/sils/topologies/wheelspeed_fleet.yaml", ROOT,
        "config/sils/contracts/fleet-example.yaml",
        "config/sils/qos/qos-profiles.yaml",
        "registry.obigo.com/oss/oss-mmf-sils-vecu:latest",
    )
    assert errors == [], f"expected no errors, got: {errors}"
    containers = resolved_run["containers"]
    assert len(containers) == 4, f"expected 4, got {len(containers)}"
    topics = [c["endpoints"][0]["topic"] for c in containers]
    assert topics == [f"/VIN/WheelSpeed/wheelspeed-{i}" for i in (1, 2, 3, 4)], topics
    assert len(set(topics)) == 4, "per-instance topics must be distinct"
    assert all(c["endpoints"][0]["ownership"] == "per-instance" for c in containers)


def test_per_instance_missing_placeholder_collides():
    # A per-instance contract whose topic lacks {instance_id}: replicas collide on one
    # topic -> §9.5 violation (this is exactly what the placeholder prevents).
    contracts = {"wheelspeed": {
        "id": "wheelspeed", "topic": "/VIN/WheelSpeed",  # <-- no {instance_id}
        "domain_env": "VISS_DDS_DOMAIN_ID", "qos_profile": "viss-sils-verifiable",
        "ownership": "per-instance"}}
    qos = tc.load_qos_catalog(ROOT, "config/sils/qos/qos-profiles.yaml")
    topo = {"run": {"domain_id": "0"}, "vecus": [{
        "name": "wheelspeed", "replicas": 3,
        "descriptor": "service/sils/vecu/descriptors/wheelspeed_probe.yaml"}]}
    resolved, errors, warnings = tc.resolve_topology(topo, ROOT, contracts, qos)
    assert any("expected exactly 1" in e and "/VIN/WheelSpeed" in e for e in errors), errors


def test_expand_replicas_with_params():
    # Pure unit test of the naming/expansion mechanism, independent of any
    # real Contract Catalog entry (uses a descriptor with no DDS endpoints at
    # all, a legitimate degenerate case) -- our only real contract today is
    # single-owner, so this is the honest way to cover multi-instance naming
    # without fabricating a fake multi-producer contract in the real catalog.
    vecu_entry = {
        "name": "wheelspeed",
        "descriptor": "irrelevant-for-this-test.yaml",
        "replicas": 4,
        "params": [{"pos": "FL"}, {"pos": "FR"}, {"pos": "RL"}, {"pos": "RR"}],
    }
    descriptor = {"ecu": "wheelspeed_ecu", "buses": []}

    instances = tc.expand_replicas(vecu_entry, descriptor)

    names = [i["container_name"] for i in instances]
    assert names == ["wheelspeed-FL", "wheelspeed-FR", "wheelspeed-RL", "wheelspeed-RR"], names
    assert [i["instance_id"] for i in instances] == names
    assert instances[0]["params"] == {"pos": "FL"}


def test_expand_replicas_params_length_mismatch():
    vecu_entry = {"name": "wheelspeed", "replicas": 4, "params": [{"pos": "FL"}, {"pos": "FR"}]}
    descriptor = {"ecu": "wheelspeed_ecu", "buses": []}
    try:
        tc.expand_replicas(vecu_entry, descriptor)
        assert False, "expected CompileError for replicas/params length mismatch"
    except tc.CompileError as e:
        assert "replicas=4" in str(e)
        assert "2 entries" in str(e)


def test_expand_replicas_single_no_params_keeps_descriptor_instance_id():
    vecu_entry = {"name": "viss-probe", "descriptor": "x.yaml"}
    descriptor = {"ecu": "viss_probe", "instance_id": "viss-probe-1", "buses": []}
    instances = tc.expand_replicas(vecu_entry, descriptor)
    assert len(instances) == 1
    assert instances[0]["container_name"] == "viss-probe"
    assert instances[0]["instance_id"] == "viss-probe-1"


def test_resolved_to_shell_roundtrip(tmp_out=Path("/tmp/sils-test-compile-out")):
    # Compile the real smoke topology to disk, then confirm resolved_to_shell
    # extracts exactly what the Run Coordinator needs to feed --from-resolved.
    topology_path = ROOT / "config/sils/topologies/viss_probe_smoke.yaml"
    resolved_run, _ssl, errors, _w = tc.compile_topology(
        topology_path, ROOT,
        "config/sils/contracts/mmf-endpoints.yaml",
        "config/sils/qos/qos-profiles.yaml",
        "registry.obigo.com/oss/oss-mmf-sils-vecu:latest",
    )
    assert errors == []
    tmp_out.mkdir(parents=True, exist_ok=True)
    import yaml
    resolved_path = tmp_out / "resolved-run.yaml"
    resolved_path.write_text(yaml.safe_dump(resolved_run, sort_keys=False))

    lines, err = rts.emit(resolved_path, 0)
    assert err is None, f"unexpected error: {err}"
    joined = "\n".join(lines)
    assert "RESOLVED_BACKEND='docker'" in joined
    assert "RESOLVED_DOMAIN='0'" in joined
    assert "RESOLVED_INSTANCE_ID='viss-probe-1'" in joined
    assert "RESOLVED_CONTAINER_COUNT='1'" in joined
    assert "RESOLVED_TOPIC='/VIN/Vehicle'" in joined  # -> SILS_TOPIC for the probe


def test_resolved_to_shell_out_of_range():
    resolved_path = Path("/tmp/sils-test-compile-out/resolved-run.yaml")
    # Depends on the roundtrip test having written the file; call it if missing.
    if not resolved_path.exists():
        test_resolved_to_shell_roundtrip()
    lines, err = rts.emit(resolved_path, 5)
    assert err is not None and "out of range" in err
    # Run-level vars must still be emitted so the caller can report the count.
    assert any(l.startswith("RESOLVED_CONTAINER_COUNT=") for l in lines)


def test_resolved_to_shell_sh_quote_escapes_single_quote():
    assert rts.sh_quote("a'b") == "'a'\\''b'"


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
        except Exception as e:  # noqa: BLE001 -- test runner, want to see any failure
            failed += 1
            print(f"ERROR {t.__name__}: {type(e).__name__}: {e}")
    print(f"\n{len(tests) - failed}/{len(tests)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
