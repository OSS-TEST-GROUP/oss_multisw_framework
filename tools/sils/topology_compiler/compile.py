#!/usr/bin/env python3
"""SiLS Topology Compiler -- architecture v3 Final §11.

Resolves a Topology YAML (+ ECU Descriptors + Endpoint Contract Catalog + QoS
Catalog) into a concrete, validated deployment plan:

    Topology + Descriptors + Contract Catalog + QoS Catalog
                        |
                        v
                 Topology Compiler
              /          |          \\
    resolved-run.yaml  saved-service-list.yaml  validation report
      (canonical,        (adsw_manager-              (errors/warnings;
       hash-stamped)      compatible, generated)       non-zero exit on error)

lifecycle_mgr/adsw_manager stay untouched (architecture v3 Final §4/§18): this
tool only produces the flat deployment artifacts they already know how to
consume via the existing UpdateServiceListRequest/StartContainerRequest path.

Scope for this increment (deliberately not the whole §9.5 validation list):
  - descriptor/contract/qos-profile existence
  - single-owner contract producer-count violation (§9.5)
  - container/instance name collisions
  - domain_env consistency (all referenced envs bound to the same run domain)
Not yet implemented: JSON-Schema-level structural validation, K3s/compose
backends (docker only), lockstep/clock master wiring, relay contracts.
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path
from typing import Any

import yaml


class CompileError(Exception):
    """Raised for CLI-level fatal errors (bad args, unreadable files)."""


def repo_root() -> Path:
    # tools/sils/topology_compiler/compile.py -> repo root is 3 parents up.
    return Path(__file__).resolve().parents[3]


def load_yaml(path: Path) -> Any:
    if not path.exists():
        raise CompileError(f"file not found: {path}")
    with path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def sha256_of(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_contract_catalog(root: Path, rel_path: str) -> dict[str, dict]:
    data = load_yaml(root / rel_path)
    contracts = {}
    for entry in data.get("contracts", []):
        contracts[entry["id"]] = entry
    return contracts


def load_qos_catalog(root: Path, rel_path: str) -> dict[str, dict]:
    return load_yaml(root / rel_path) or {}


def expand_replicas(vecu_entry: dict, descriptor: dict) -> list[dict]:
    """Expand one Topology `vecus[]` entry (with optional replicas/params) into
    concrete (container_name, instance_id) pairs. Pure function, no file I/O,
    so it's independently testable without a real Contract Catalog entry --
    see test_compile.py's test_expand_replicas_with_params.
    """
    base_name = vecu_entry["name"]
    replicas = vecu_entry.get("replicas", 1)
    params = vecu_entry.get("params")

    if params is not None and len(params) != replicas:
        raise CompileError(
            f"vecu '{base_name}': replicas={replicas} but params has {len(params)} entries"
        )

    if replicas == 1 and not params:
        # Single instance, no per-replica params: keep the descriptor's own
        # instance_id (matches Phase 1's viss_probe.yaml usage verbatim).
        return [
            {
                "container_name": base_name,
                "instance_id": descriptor.get("instance_id", base_name),
                "params": {},
                "index": 1,
            }
        ]

    out = []
    for i in range(replicas):
        if params:
            p = params[i]
            suffix = "-".join(str(v) for v in p.values())
        else:
            suffix = str(i + 1)
        out.append(
            {
                "container_name": f"{base_name}-{suffix}",
                "instance_id": f"{base_name}-{suffix}",
                "params": params[i] if params else {},
                "index": i + 1,
            }
        )
    return out


# Descriptor behavior 'type' -> the probe's SILS_BEHAVIOR kind (behavior.hpp MakeBehavior).
BEHAVIOR_KIND = {
    "stub": "viss-request", "viss-request": "viss-request",
    "accumulate": "accumulate", "accumulator": "accumulate",
    "p-controller": "p-controller",
}


def _normalize_faults(faults) -> str:
    """A scenario fault schedule in the probe's spec form 'tick:fault[,tick:fault...]'.
    Accepts a ready string, a list of {tick, fault} dicts, or a {tick: fault} mapping."""
    if isinstance(faults, str):
        return faults
    if isinstance(faults, dict):
        return ",".join(f"{k}:{v}" for k, v in faults.items())
    if isinstance(faults, list):
        parts = []
        for f in faults:
            if isinstance(f, dict) and "tick" in f and "fault" in f:
                parts.append(f"{f['tick']}:{f['fault']}")
        return ",".join(parts)
    return ""


def resolve_runtime_env(descriptor: dict) -> dict[str, str]:
    """Translate a Descriptor's port declarations (clock/behavior/plant/scenario/
    attestation/protocol) into the SILS_* environment the probe reads (§9.2). Pure and
    file-I/O-free so it's unit-testable without a build. This is the declarative front
    door: a Descriptor that says `plant: {type: fmu}` now drives the FmuPlant port via
    env, instead of an operator hand-exporting SILS_PLANT. Per-run identity (SILS_RUN_ID,
    SILS_VECU_INSTANCE_ID) and the resolved per-instance topic (SILS_TOPIC) are NOT set
    here -- the Run Coordinator owns those. Values are returned as strings (env-ready)."""
    env: dict[str, str] = {}  # dict is insertion-ordered (py3.7+): stable emit order

    clock = descriptor.get("clock") or {}
    if clock.get("mode"):
        env["SILS_CLOCK_MODE"] = str(clock["mode"])

    b = descriptor.get("behavior") or {}
    if b.get("type"):
        env["SILS_BEHAVIOR"] = BEHAVIOR_KIND.get(b["type"], str(b["type"]))
    spec = b.get("spec") or {}
    for key, envname in (("period_ms", "SILS_VISS_PROBE_PERIOD_MS"),
                         ("response_topic_prefix", "SILS_VISS_PROBE_RESPONSE_TOPIC_PREFIX"),
                         ("request_action", "SILS_VISS_PROBE_REQUEST_ACTION"),
                         ("request_path", "SILS_VISS_PROBE_REQUEST_PATH"),
                         ("kp", "SILS_CTRL_KP"), ("setpoint", "SILS_CTRL_SETPOINT")):
        if key in spec:
            env[envname] = str(spec[key])

    proto = descriptor.get("protocol") or {}
    if proto.get("mode"):
        env["SILS_PROTOCOL"] = str(proto["mode"])
    if proto.get("response_topic"):
        env["SILS_RESPONSE_TOPIC"] = str(proto["response_topic"])

    p = descriptor.get("plant") or {}
    ptype = p.get("type")
    if ptype and ptype != "none":
        env["SILS_PLANT"] = str(ptype)
        pspec = p.get("spec") or {}
        for key, envname in (("drag", "SILS_PLANT_DRAG"), ("init", "SILS_PLANT_INIT"),
                             ("replay_file", "SILS_PLANT_REPLAY_FILE")):
            if key in pspec:
                env[envname] = str(pspec[key])
        fmu = p.get("fmu") or pspec.get("fmu") or {}
        if fmu.get("file"):
            env["SILS_FMU_FILE"] = str(fmu["file"])
        if "input_vr" in fmu:
            env["SILS_FMU_INPUT_VR"] = str(fmu["input_vr"])
        if "output_vr" in fmu:
            env["SILS_FMU_OUTPUT_VR"] = str(fmu["output_vr"])
        if fmu.get("allowlist"):
            env["SILS_FMU_ALLOWLIST"] = str(fmu["allowlist"])
        if fmu.get("allowlist_id"):
            env["SILS_FMU_ALLOWLIST_ID"] = str(fmu["allowlist_id"])

    s = descriptor.get("scenario") or {}
    if s.get("type"):
        env["SILS_SCENARIO"] = str(s["type"])
        faults = _normalize_faults(s.get("faults", ""))
        if faults:
            env["SILS_SCENARIO_FAULTS"] = faults

    a = descriptor.get("attestation") or {}
    for key in ("arch", "gateway", "protocol"):
        if a.get(key):
            env[f"SILS_ATTEST_{key.upper()}"] = str(a[key])

    return env


def resolve_topology(
    topology: dict, root: Path, contracts: dict[str, dict], qos_catalog: dict[str, dict]
) -> tuple[list[dict], list[str], list[str]]:
    """Returns (resolved_containers, errors, warnings). resolved_containers is
    only meaningful (safe to emit) when errors is empty -- fail closed.
    """
    errors: list[str] = []
    warnings: list[str] = []
    resolved: list[dict] = []
    seen_names: set[str] = set()
    # resolved topic -> list of (container_name, contract_id, ownership) publishing to it.
    # §9.5 ownership is checked on the RESOLVED topic (post per-instance templating), so a
    # per-instance fleet -- N replicas each owning a distinct topic -- is valid, while N
    # producers on the SAME topic (single-owner, or a per-instance topic missing its
    # {instance_id} placeholder) is a violation.
    producers_by_topic: dict[str, list[tuple[str, str, str]]] = {}
    domain_envs_used: set[str] = set()

    run_domain_id = str(topology.get("run", {}).get("domain_id", "")).strip()
    if not run_domain_id:
        errors.append("run.domain_id is required (must match the SUT's domain, §3.4/§18-B)")

    for vecu_entry in topology.get("vecus", []):
        descriptor_rel = vecu_entry.get("descriptor")
        if not descriptor_rel:
            errors.append(f"vecu entry {vecu_entry.get('name', '?')}: missing 'descriptor'")
            continue
        descriptor_path = root / descriptor_rel
        try:
            descriptor = load_yaml(descriptor_path)
        except CompileError as e:
            errors.append(str(e))
            continue

        try:
            instances = expand_replicas(vecu_entry, descriptor)
        except CompileError as e:
            errors.append(str(e))
            continue

        endpoints_resolved = []
        for bus in descriptor.get("buses", []):
            for ep in bus.get("endpoints", []):
                contract_id = ep.get("contract_id")
                contract = contracts.get(contract_id)
                if contract is None:
                    errors.append(
                        f"vecu '{vecu_entry['name']}': endpoint references unknown "
                        f"contract_id '{contract_id}' (not in Contract Catalog)"
                    )
                    continue
                qos_name = contract.get("qos_profile")
                if qos_name not in qos_catalog:
                    errors.append(
                        f"contract '{contract_id}': qos_profile '{qos_name}' not in QoS Catalog"
                    )
                domain_envs_used.add(contract["domain_env"])
                endpoints_resolved.append(
                    {
                        "contract_id": contract_id,
                        "direction": ep.get("direction"),
                        "topic": contract.get("topic"),
                        "domain_env": contract.get("domain_env"),
                        "qos_profile": qos_name,
                        "ownership": contract.get("ownership"),
                        "late_sample_policy": contract.get("late_sample_policy", "next_tick"),
                    }
                )

        for inst in instances:
            name = inst["container_name"]
            if name in seen_names:
                errors.append(f"duplicate container/instance name: '{name}'")
            seen_names.add(name)

            # Resolve per-instance topics: substitute {instance_id}/{index} in the
            # contract topic template so each replica owns a distinct endpoint (§9.5,
            # §13 multi-instance). Non-templated topics pass through unchanged.
            inst_eps = []
            for ep in endpoints_resolved:
                topic = str(ep["topic"]).replace("{instance_id}", inst["instance_id"]) \
                    .replace("{index}", str(inst.get("index", "")))
                ie = dict(ep, topic=topic)
                inst_eps.append(ie)
                if ep["direction"] == "publish":
                    producers_by_topic.setdefault(topic, []).append(
                        (name, ep["contract_id"], ep.get("ownership")))

            resolved.append(
                {
                    "name": name,
                    "instance_id": inst["instance_id"],
                    "descriptor": descriptor_rel,
                    "ecu": descriptor.get("ecu"),
                    "fidelity": descriptor.get("fidelity"),
                    "params": inst["params"],
                    # Port declarations -> SILS_* env the probe reads (§9.2 declarative path).
                    "env": resolve_runtime_env(descriptor),
                    "endpoints": inst_eps,
                }
            )

    # §9.5: an exclusively-owned topic must have exactly one producer. Checked on the
    # RESOLVED topic -- ownership 'multi'/'shared' is exempt (co-producers allowed).
    for topic, producers in producers_by_topic.items():
        if len(producers) <= 1:
            continue
        ownership = producers[0][2]
        if ownership in ("multi", "shared"):
            continue
        cids = sorted({c for _, c, _ in producers})
        names = ", ".join(n for n, _, _ in producers)
        errors.append(
            f"topic '{topic}' has {len(producers)} producers ({names}) but contract(s) "
            f"{cids} are {ownership!r}-owned -- expected exactly 1 (per-instance contracts "
            "must template {instance_id} into the topic so replicas own distinct topics)"
        )

    if len(domain_envs_used) > 1:
        warnings.append(
            "multiple domain envs referenced in one topology: "
            f"{sorted(domain_envs_used)} -- all will be bound to the same run.domain_id"
        )

    return resolved, errors, warnings


def render_saved_service_list(resolved: list[dict], image: str, run_domain_id: str) -> dict:
    containers = []
    for c in resolved:
        domain_envs = sorted({ep["domain_env"] for ep in c["endpoints"] if ep.get("domain_env")})
        extra_args = ["--network", "host"]
        for env_name in domain_envs:
            extra_args += ["-e", f"{env_name}={run_domain_id}"]
        extra_args += ["-e", "SILS_RUN_ID=${SILS_RUN_ID}"]
        extra_args += ["-e", f"SILS_VECU_INSTANCE_ID={c['instance_id']}"]
        # Port declarations (§9.2): pass the Descriptor-derived SILS_* env into the
        # container so the docker path drives the same ports as local.
        for env_name, env_val in (c.get("env") or {}).items():
            extra_args += ["-e", f"{env_name}={env_val}"]

        containers.append(
            {
                "name": c["name"],
                "image": image,
                "container-type": "docker",
                "extra_args": extra_args,
                "services": [
                    {
                        "package": "",
                        "name": c["name"],
                        "launchfile": "",
                        "type": "NORMAL",
                        "recovery": "MANUAL",
                        "enabled": "true",
                        "max": "0",
                        "exec_type": "container",
                    }
                ],
            }
        )
    return {"specs": [{"containers": containers}]}


def render_resolved_run(topology: dict, resolved: list[dict], hashes: dict[str, str]) -> dict:
    return {
        "topology": topology.get("topology"),
        "run": topology.get("run", {}),
        "deployment": topology.get("deployment", {}),
        "clock": topology.get("clock", {}),
        "containers": resolved,
        "input_hashes": hashes,
    }


def compile_topology(
    topology_path: Path,
    root: Path,
    contract_catalog_rel: str,
    qos_catalog_rel: str,
    image: str,
) -> tuple[dict | None, dict | None, list[str], list[str]]:
    """Returns (resolved_run, saved_service_list, errors, warnings).
    First two are None if there were any errors (fail closed, §9.5)."""
    topology = load_yaml(topology_path)
    contracts = load_contract_catalog(root, contract_catalog_rel)
    qos_catalog = load_qos_catalog(root, qos_catalog_rel)

    resolved, errors, warnings = resolve_topology(topology, root, contracts, qos_catalog)
    if errors:
        return None, None, errors, warnings

    hashes = {
        "topology": sha256_of(topology_path),
        "contracts": sha256_of(root / contract_catalog_rel),
        "qos": sha256_of(root / qos_catalog_rel),
    }
    for c in resolved:
        hashes[f"descriptor:{c['name']}"] = sha256_of(root / c["descriptor"])

    run_domain_id = str(topology.get("run", {}).get("domain_id", ""))
    resolved_run = render_resolved_run(topology, resolved, hashes)
    saved_service_list = render_saved_service_list(resolved, image, run_domain_id)
    return resolved_run, saved_service_list, errors, warnings


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("topology", type=Path, help="path to a Topology YAML")
    parser.add_argument("--repo-root", type=Path, default=repo_root())
    parser.add_argument(
        "--contracts", default="config/sils/contracts/mmf-endpoints.yaml",
        help="Contract Catalog path, relative to --repo-root",
    )
    parser.add_argument(
        "--qos", default="config/sils/qos/qos-profiles.yaml",
        help="QoS Catalog path, relative to --repo-root",
    )
    parser.add_argument(
        "--image", default="registry.obigo.com/oss/oss-mmf-sils-vecu:latest",
        help="image reference used in the generated saved-service-list",
    )
    parser.add_argument("--out-dir", type=Path, default=None,
                         help="write resolved-run.yaml / saved-service-list.yaml here")
    args = parser.parse_args(argv)

    try:
        resolved_run, saved_service_list, errors, warnings = compile_topology(
            args.topology, args.repo_root, args.contracts, args.qos, args.image
        )
    except CompileError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    for w in warnings:
        print(f"warning: {w}", file=sys.stderr)
    for e in errors:
        print(f"error: {e}", file=sys.stderr)

    if errors:
        print(f"validation FAILED: {len(errors)} error(s) -- no artifacts written", file=sys.stderr)
        return 1

    print(f"validation OK: {len(resolved_run['containers'])} container(s) resolved", file=sys.stderr)

    if args.out_dir:
        args.out_dir.mkdir(parents=True, exist_ok=True)
        (args.out_dir / "resolved-run.yaml").write_text(
            yaml.safe_dump(resolved_run, sort_keys=False)
        )
        (args.out_dir / "saved-service-list.yaml").write_text(
            yaml.safe_dump(saved_service_list, sort_keys=False)
        )
        print(f"wrote {args.out_dir}/resolved-run.yaml and saved-service-list.yaml", file=sys.stderr)
    else:
        print(yaml.safe_dump(resolved_run, sort_keys=False))

    return 0


if __name__ == "__main__":
    sys.exit(main())
