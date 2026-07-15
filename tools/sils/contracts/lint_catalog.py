#!/usr/bin/env python3
"""SiLS Contract Catalog + QoS Catalog standalone linter -- architecture v3 §9.1/§9.3.

The Topology Compiler (tools/sils/topology_compiler/compile.py) validates catalog
entries only in the CONTEXT of a topology: it checks the contract_ids a topology
references, the qos_profile of a USED contract, single-owner producer counts, and
domain_env consistency. It never lints the catalog on its own, so authoring errors
in an entry no topology happens to use -- or errors the compiler would only hit as
a crash -- go unnoticed until much later.

This lints the catalogs standalone, complementary to (not overlapping with) the
compiler:

  ERRORS (block):
    - duplicate contract id (the compiler's dict silently overwrites these)
    - missing required contract field (the compiler KeyErrors on some, e.g. domain_env)
    - qos_profile referencing a profile absent from the QoS Catalog (even if unused)
    - idl path that doesn't exist on disk (the compiler never stat()s it)
    - unknown domain_env (typo guard: must be a known MMF/VISS/ROS env, §3.4)
    - invalid enum: transport / ownership / role direction / QoS reliability,
      durability, history.kind (+ KEEP_LAST needs a positive depth)
    - duplicate (topic, domain_env) across contracts (ambiguous binding)
    - malformed semantic_version (not x.y.z)

  WARNINGS (non-block unless --strict):
    - QoS profile defined but referenced by no contract (dead config)
    - unknown payload field encoding
    - a contract with no subscribing role (both roles publish)

Exit: 0 = clean, 1 = errors (or warnings under --strict), 2 = usage/IO error.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "topology_compiler"))
import compile as tc  # noqa: E402  (reuse repo_root/load_yaml)

# §3.4: the three DDS domain envs MMF isolates on. A contract's domain_env typo
# (e.g. VIS_DDS_DOMAIN_ID) would otherwise bind a vECU to the wrong/undefined domain.
KNOWN_DOMAIN_ENVS = {"VISS_DDS_DOMAIN_ID", "MMF_DDS_DOMAIN_ID", "ROS_DOMAIN_ID"}
TRANSPORTS = {"dds"}
# single: one owner of the topic. per-instance: templated topic ({instance_id}), each
# replica owns its own resolved topic (§9.5/§13 fleet). shared/multi: co-producers allowed.
OWNERSHIPS = {"single", "per-instance", "shared", "multi"}
ROLE_DIRECTIONS = {"publish", "subscribe"}
ENCODINGS = {"json", "cdr", "none", "raw"}
RELIABILITY = {"RELIABLE", "BEST_EFFORT"}
DURABILITY = {"VOLATILE", "TRANSIENT_LOCAL", "TRANSIENT", "PERSISTENT"}
HISTORY_KINDS = {"KEEP_LAST", "KEEP_ALL"}
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")

REQUIRED_CONTRACT_FIELDS = [
    "id", "transport", "domain_env", "idl", "struct",
    "registered_type", "topic", "qos_profile", "ownership", "roles",
]


def lint_qos_profile(name: str, prof: dict, errors: list[str]) -> None:
    where = f"qos-profile '{name}'"
    if not isinstance(prof, dict):
        errors.append(f"{where}: must be a mapping")
        return
    rel = prof.get("reliability")
    if rel is not None and rel not in RELIABILITY:
        errors.append(f"{where}: reliability '{rel}' invalid (expected one of {sorted(RELIABILITY)})")
    dur = prof.get("durability")
    if dur is not None and dur not in DURABILITY:
        errors.append(f"{where}: durability '{dur}' invalid (expected one of {sorted(DURABILITY)})")
    hist = prof.get("history")
    if isinstance(hist, dict):
        kind = hist.get("kind")
        if kind not in HISTORY_KINDS:
            errors.append(f"{where}: history.kind '{kind}' invalid (expected one of {sorted(HISTORY_KINDS)})")
        if kind == "KEEP_LAST":
            depth = hist.get("depth")
            if not isinstance(depth, int) or depth <= 0:
                errors.append(f"{where}: history KEEP_LAST requires a positive integer depth (got {depth!r})")
    elif hist is not None:
        errors.append(f"{where}: history must be a mapping with kind/depth")


def lint(catalog: dict, qos: dict, root: Path) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []

    qos = qos or {}
    for name, prof in qos.items():
        lint_qos_profile(name, prof, errors)

    contracts = (catalog or {}).get("contracts")
    if not isinstance(contracts, list):
        errors.append("catalog: top-level 'contracts' must be a list")
        return errors, warnings

    seen_ids: set[str] = set()
    seen_topic_domain: dict[tuple, str] = {}
    referenced_qos: set[str] = set()

    for i, c in enumerate(contracts):
        if not isinstance(c, dict):
            errors.append(f"contract #{i}: must be a mapping")
            continue
        cid = c.get("id", f"#{i}")
        where = f"contract '{cid}'"

        # required fields
        for f in REQUIRED_CONTRACT_FIELDS:
            if f not in c or c[f] in (None, ""):
                errors.append(f"{where}: missing required field '{f}'")

        # duplicate id
        if "id" in c:
            if c["id"] in seen_ids:
                errors.append(f"{where}: duplicate contract id (ids must be unique; the compiler would silently overwrite)")
            seen_ids.add(c["id"])

        # enum guards
        if c.get("transport") not in (None, "") and c["transport"] not in TRANSPORTS:
            errors.append(f"{where}: transport '{c['transport']}' invalid (expected one of {sorted(TRANSPORTS)})")
        if c.get("domain_env") not in (None, "") and c["domain_env"] not in KNOWN_DOMAIN_ENVS:
            errors.append(f"{where}: domain_env '{c['domain_env']}' unknown "
                          f"(expected one of {sorted(KNOWN_DOMAIN_ENVS)}; typo? §3.4)")
        if c.get("ownership") not in (None, "") and c["ownership"] not in OWNERSHIPS:
            errors.append(f"{where}: ownership '{c['ownership']}' invalid (expected one of {sorted(OWNERSHIPS)})")
        # per-instance contracts must template {instance_id} into the topic, else
        # replicas collide on one topic and fail the §9.5 producer check at compile.
        if c.get("ownership") == "per-instance" and "{instance_id}" not in str(c.get("topic", "")):
            errors.append(f"{where}: per-instance contract topic '{c.get('topic')}' must contain "
                          "the {instance_id} placeholder (else replicas collide, §9.5/§13)")

        # semantic_version format (optional field, but if present must be x.y.z)
        sv = c.get("semantic_version")
        if sv is not None and not SEMVER_RE.match(str(sv)):
            errors.append(f"{where}: semantic_version '{sv}' malformed (expected x.y.z)")

        # qos reference resolves
        qp = c.get("qos_profile")
        if qp not in (None, ""):
            referenced_qos.add(qp)
            if qp not in qos:
                errors.append(f"{where}: qos_profile '{qp}' not in QoS Catalog")

        # idl file exists on disk
        idl = c.get("idl")
        if idl not in (None, "") and not (root / idl).exists():
            errors.append(f"{where}: idl file '{idl}' not found (relative to repo root {root})")

        # (topic, domain_env) uniqueness
        if c.get("topic") and c.get("domain_env"):
            key = (c["topic"], c["domain_env"])
            if key in seen_topic_domain:
                errors.append(f"{where}: topic '{c['topic']}' on domain_env {c['domain_env']} "
                              f"already bound by contract '{seen_topic_domain[key]}' (ambiguous)")
            else:
                seen_topic_domain[key] = c.get("id", f"#{i}")

        # roles
        roles = c.get("roles")
        if isinstance(roles, dict):
            for role_name in ("sut", "vecu"):
                if role_name not in roles:
                    errors.append(f"{where}: roles missing '{role_name}'")
                elif roles[role_name] not in ROLE_DIRECTIONS:
                    errors.append(f"{where}: roles.{role_name} '{roles[role_name]}' invalid "
                                  f"(expected one of {sorted(ROLE_DIRECTIONS)})")
            directions = {roles.get("sut"), roles.get("vecu")}
            if directions == {"publish"}:
                warnings.append(f"{where}: no subscribing role (both sut and vecu publish)")
        elif roles is not None:
            errors.append(f"{where}: roles must be a mapping with sut/vecu directions")

        # payload encoding (optional)
        payload = c.get("payload")
        if isinstance(payload, dict):
            for fld in payload.get("fields", []) or []:
                enc = fld.get("encoding")
                if enc is not None and enc not in ENCODINGS:
                    warnings.append(f"{where}: payload field '{fld.get('name')}' encoding '{enc}' unrecognized "
                                    f"(known: {sorted(ENCODINGS)})")

    # orphan QoS profiles (defined, referenced by no contract)
    for name in qos:
        if name not in referenced_qos:
            warnings.append(f"qos-profile '{name}': defined but referenced by no contract (dead config)")

    return errors, warnings


def main(argv: list[str] | None = None) -> int:
    root = tc.repo_root()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", default="config/sils/contracts/mmf-endpoints.yaml",
                        help="Contract Catalog path (relative to --repo-root)")
    parser.add_argument("--qos", default="config/sils/qos/qos-profiles.yaml",
                        help="QoS Catalog path (relative to --repo-root)")
    parser.add_argument("--repo-root", type=Path, default=root)
    parser.add_argument("--strict", action="store_true", help="treat warnings as errors")
    args = parser.parse_args(argv)

    catalog_path = args.repo_root / args.catalog
    qos_path = args.repo_root / args.qos
    for p in (catalog_path, qos_path):
        if not p.exists():
            print(f"# error: file not found: {p}", file=sys.stderr)
            return 2
    try:
        catalog = tc.load_yaml(catalog_path)
        qos = tc.load_yaml(qos_path)
    except Exception as e:  # noqa: BLE001
        print(f"# error: cannot parse YAML: {e}", file=sys.stderr)
        return 2

    errors, warnings = lint(catalog, qos, args.repo_root)

    for w in warnings:
        print(f"warning: {w}", file=sys.stderr)
    for e in errors:
        print(f"error: {e}", file=sys.stderr)

    n_contracts = len((catalog or {}).get("contracts", []) or [])
    if errors:
        print(f"catalog lint FAILED: {len(errors)} error(s), {len(warnings)} warning(s) "
              f"across {n_contracts} contract(s)", file=sys.stderr)
        return 1
    if warnings and args.strict:
        print(f"catalog lint FAILED (--strict): {len(warnings)} warning(s)", file=sys.stderr)
        return 1
    print(f"catalog lint OK: {n_contracts} contract(s), {len(warnings)} warning(s)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
