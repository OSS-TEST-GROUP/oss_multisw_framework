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


# --- Descriptor port declarations -> SILS_* env (§9.2 declarative path) -------

def test_resolve_runtime_env_full_ports():
    # A rich descriptor exercising every port -> the exact SILS_* the probe reads.
    descriptor = {
        "clock": {"mode": "lockstep"},
        "behavior": {"type": "p-controller",
                     "spec": {"period_ms": 100, "kp": 0.8, "setpoint": 10.0,
                              "request_action": "set", "request_path": "Vehicle.Actuation"}},
        "protocol": {"mode": "reqresp", "response_topic": "sils/viss/response"},
        "plant": {"type": "fmu",
                  "fmu": {"file": "vehicle.fmu", "input_vr": 7, "output_vr": 8,
                          "allowlist_id": "reference-feedthrough"}},
        "scenario": {"type": "scheduled", "faults": "3:drop,7:corrupt"},
        "attestation": {"arch": "aarch64", "gateway": "real", "protocol": "real"},
    }
    env = tc.resolve_runtime_env(descriptor)
    assert env["SILS_CLOCK_MODE"] == "lockstep"
    assert env["SILS_BEHAVIOR"] == "p-controller"
    assert env["SILS_CTRL_KP"] == "0.8" and env["SILS_CTRL_SETPOINT"] == "10.0"
    assert env["SILS_VISS_PROBE_PERIOD_MS"] == "100"
    assert env["SILS_PROTOCOL"] == "reqresp" and env["SILS_RESPONSE_TOPIC"] == "sils/viss/response"
    assert env["SILS_PLANT"] == "fmu" and env["SILS_FMU_FILE"] == "vehicle.fmu"
    assert env["SILS_FMU_INPUT_VR"] == "7" and env["SILS_FMU_OUTPUT_VR"] == "8"
    assert env["SILS_FMU_ALLOWLIST_ID"] == "reference-feedthrough"
    assert env["SILS_SCENARIO"] == "scheduled" and env["SILS_SCENARIO_FAULTS"] == "3:drop,7:corrupt"
    assert env["SILS_ATTEST_ARCH"] == "aarch64" and env["SILS_ATTEST_GATEWAY"] == "real"


def test_resolve_runtime_env_stub_maps_to_viss_request():
    # 'stub' behavior -> the probe's default viss-request kind; plant none / empty scenario
    # emit nothing (so the L0 smoke descriptor's run is unchanged).
    env = tc.resolve_runtime_env({"clock": {"mode": "wall"}, "behavior": {"type": "stub"},
                                  "plant": {"type": "none"}, "scenario": {}})
    assert env["SILS_BEHAVIOR"] == "viss-request"
    assert env["SILS_CLOCK_MODE"] == "wall"
    assert "SILS_PLANT" not in env and "SILS_SCENARIO" not in env


def test_resolve_runtime_env_faults_forms():
    assert tc._normalize_faults("3:drop,7:corrupt") == "3:drop,7:corrupt"
    assert tc._normalize_faults([{"tick": 3, "fault": "drop"}, {"tick": 7, "fault": "corrupt"}]) \
        == "3:drop,7:corrupt"
    assert tc._normalize_faults({4: "duplicate"}) == "4:duplicate"


def test_smoke_descriptor_env_carried_into_resolved():
    resolved_run, ssl, errors, _w = tc.compile_topology(
        ROOT / "config/sils/topologies/viss_probe_smoke.yaml", ROOT,
        "config/sils/contracts/mmf-endpoints.yaml",
        "config/sils/qos/qos-profiles.yaml",
        "registry.obigo.com/oss/oss-mmf-sils-vecu:latest",
    )
    assert errors == [], errors
    env = resolved_run["containers"][0]["env"]
    assert env["SILS_CLOCK_MODE"] == "wall"
    assert env["SILS_BEHAVIOR"] == "viss-request"
    assert env["SILS_VISS_PROBE_REQUEST_PATH"] == "Vehicle.Speed"
    # docker path: the port env must also appear as -e flags in the saved-service-list
    joined = " ".join(ssl["specs"][0]["containers"][0]["extra_args"])
    assert "-e SILS_CLOCK_MODE=wall" in joined.replace("'", "") or "SILS_CLOCK_MODE=wall" in joined


def test_scenario_example_topology_wires_ports():
    # The shipped example: behavior=accumulate + scheduled faults + attestation, all
    # resolved into env by the compiler (no hand-exported vars).
    resolved_run, _ssl, errors, _w = tc.compile_topology(
        ROOT / "config/sils/topologies/viss_scenario.yaml", ROOT,
        "config/sils/contracts/mmf-endpoints.yaml",
        "config/sils/qos/qos-profiles.yaml",
        "registry.obigo.com/oss/oss-mmf-sils-vecu:latest",
    )
    assert errors == [], errors
    env = resolved_run["containers"][0]["env"]
    assert env["SILS_BEHAVIOR"] == "accumulate"
    assert env["SILS_SCENARIO"] == "scheduled"
    assert env["SILS_SCENARIO_FAULTS"] == "3:drop,7:duplicate"
    assert env["SILS_ATTEST_ARCH"] == "x86_64"  # sim run -> not target-like (by design)


def test_resolved_to_shell_emits_export_and_env_keys(tmp_out=Path("/tmp/sils-test-env-out")):
    resolved_run = {
        "topology": "t", "run": {"domain_id": "0"}, "deployment": {"backend": "local"},
        "containers": [{
            "name": "c", "instance_id": "c-1",
            "env": {"SILS_CLOCK_MODE": "wall", "SILS_SCENARIO": "scheduled",
                    "SILS_SCENARIO_FAULTS": "3:drop"},
            "endpoints": [{"direction": "publish", "topic": "/VIN/Vehicle"}],
        }],
    }
    tmp_out.mkdir(parents=True, exist_ok=True)
    import yaml
    rp = tmp_out / "resolved-run.yaml"
    rp.write_text(yaml.safe_dump(resolved_run, sort_keys=False))

    lines, err = rts.emit(rp, 0)
    assert err is None, err
    joined = "\n".join(lines)
    assert "export SILS_CLOCK_MODE='wall'" in joined
    assert "export SILS_SCENARIO_FAULTS='3:drop'" in joined
    # RESOLVED_ENV_KEYS lists names so the docker path can rebuild -e NAME=$NAME
    assert "SILS_CLOCK_MODE SILS_SCENARIO SILS_SCENARIO_FAULTS" in joined

    # --env-args: raw unquoted KEY=VALUE for env / docker -e
    args, err2 = rts.env_args(rp, 0)
    assert err2 is None and "SILS_CLOCK_MODE=wall" in args and "SILS_SCENARIO_FAULTS=3:drop" in args


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
