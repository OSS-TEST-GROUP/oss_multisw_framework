#!/usr/bin/env python3
"""Emit shell-sourceable variables from a compiled resolved-run.yaml.

Bridges the Topology Compiler's output (compile.py) to the Run Coordinator
(service/sils/run_coordinator/run_phase1.sh), which is a bash script and
shouldn't hand-roll YAML parsing. Given a resolved-run.yaml and a container
index, prints `KEY='value'` lines the coordinator can `eval`:

    RESOLVED_TOPOLOGY, RESOLVED_BACKEND, RESOLVED_DOMAIN,
    RESOLVED_CONTAINER_COUNT, RESOLVED_NAME, RESOLVED_INSTANCE_ID

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
    return lines, None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("resolved_run", type=Path, help="path to a resolved-run.yaml")
    parser.add_argument("--index", type=int, default=0, help="container index (default 0)")
    args = parser.parse_args(argv)

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
