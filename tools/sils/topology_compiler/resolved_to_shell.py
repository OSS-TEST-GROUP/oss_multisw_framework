#!/usr/bin/env python3
"""Emit shell-sourceable variables from a compiled resolved-run.yaml.

Bridges the Topology Compiler's output (compile.py) to the Run Coordinator
(service/sils/run_coordinator/run_phase1.sh), which is a bash script and
shouldn't hand-roll YAML parsing. Given a resolved-run.yaml and a container
index, prints `KEY='value'` lines the coordinator can `eval`:

    RESOLVED_TOPOLOGY, RESOLVED_BACKEND, RESOLVED_DOMAIN,
    RESOLVED_CONTAINER_COUNT, RESOLVED_NAME, RESOLVED_INSTANCE_ID,
    RESOLVED_TOPIC, RESOLVED_ENV_KEYS, and `export SILS_*` port-env lines (§9.2)

Values are single-quoted with embedded single-quotes escaped, so `eval` is
safe even though the inputs here (domain ids, instance names) are already
tame. Exits non-zero with a message on a missing file or out-of-range index.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import yaml


def sh_quote(value: str) -> str:
    # Wrap in single quotes; a literal ' becomes '\'' (close, escaped quote, reopen).
    return "'" + str(value).replace("'", "'\\''") + "'"


def emit(resolved_path: Path, index: int) -> tuple[list[str], str | None]:
    if not resolved_path.exists():
        return [], f"resolved-run file not found: {resolved_path}"
    data = yaml.safe_load(resolved_path.read_text(encoding="utf-8")) or {}

    containers = data.get("containers", [])
    count = len(containers)
    lines = [
        f"RESOLVED_TOPOLOGY={sh_quote(data.get('topology', ''))}",
        f"RESOLVED_BACKEND={sh_quote(data.get('deployment', {}).get('backend', ''))}",
        f"RESOLVED_DOMAIN={sh_quote(data.get('run', {}).get('domain_id', ''))}",
        f"RESOLVED_CONTAINER_COUNT={sh_quote(count)}",
    ]

    if count == 0:
        return lines, "resolved-run has no containers"
    if index < 0 or index >= count:
        return lines, f"container index {index} out of range (0..{count - 1})"

    c = containers[index]
    lines.append(f"RESOLVED_NAME={sh_quote(c.get('name', ''))}")
    lines.append(f"RESOLVED_INSTANCE_ID={sh_quote(c.get('instance_id', ''))}")
    # First publish endpoint's resolved topic -> SILS_TOPIC, so a per-instance fleet
    # member publishes to its own topic (e.g. /VIN/WheelSpeed/<instance>, §9.5/§13).
    eps = c.get("endpoints", []) or []
    pub = next((e for e in eps if e.get("direction") == "publish"), None)
    lines.append(f"RESOLVED_TOPIC={sh_quote(pub.get('topic', '') if pub else '')}")
    # Descriptor-derived port env (§9.2): `export SILS_*` so the local probe inherits them
    # (and the coordinator's own fmu_preflight sees SILS_FMU_FILE). RESOLVED_ENV_KEYS lists
    # the names so the docker path can rebuild `-e NAME=value` from the exported values.
    env = c.get("env") or {}
    for env_name, env_val in env.items():
        lines.append(f"export {env_name}={sh_quote(env_val)}")
    lines.append(f"RESOLVED_ENV_KEYS={sh_quote(' '.join(env.keys()))}")
    return lines, None


def env_args(resolved_path: Path, index: int) -> tuple[list[str], str | None]:
    """Raw, UNQUOTED `SILS_KEY=value` lines for the container's port env -- suitable as
    `env`/`docker -e` arguments (the shell/docker, not eval, consumes them). Distinct from
    emit()'s sh-quoted `export` lines. Used by the fleet launch path."""
    if not resolved_path.exists():
        return [], f"resolved-run file not found: {resolved_path}"
    data = yaml.safe_load(resolved_path.read_text(encoding="utf-8")) or {}
    containers = data.get("containers", [])
    if index < 0 or index >= len(containers):
        return [], f"container index {index} out of range (0..{len(containers) - 1})"
    env = containers[index].get("env") or {}
    return [f"{k}={v}" for k, v in env.items()], None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("resolved_run", type=Path, help="path to a resolved-run.yaml")
    parser.add_argument("--index", type=int, default=0, help="container index (default 0)")
    parser.add_argument("--env-args", action="store_true",
                        help="print raw KEY=VALUE port-env lines (for env/docker -e), not shell vars")
    args = parser.parse_args(argv)

    if args.env_args:
        lines, err = env_args(args.resolved_run, args.index)
        for line in lines:
            print(line)
        if err:
            print(f"# error: {err}", file=sys.stderr)
            return 1
        return 0

    lines, err = emit(args.resolved_run, args.index)
    # Always emit the run-level vars we do have, even on a per-container error,
    # so the caller can surface RESOLVED_CONTAINER_COUNT in its diagnostics.
    for line in lines:
        print(line)
    if err:
        print(f"# error: {err}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
