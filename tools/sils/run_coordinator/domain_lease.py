#!/usr/bin/env python3
"""SiLS DDS domain lease manager -- architecture v3 Final §12.1/§12.2.

Domain-only run isolation (P5) means every concurrent SiLS run needs a distinct
DDS domain id, and the supply is finite. This allocates domain ids from a bounded
pool with leases that:

  - are acquired atomically (O_CREAT|O_EXCL per-domain file -- two runners racing
    for the last free domain, only one wins),
  - carry a TTL + owner pid so a run that dies without releasing (Coordinator
    crash, §14.4) can be reaped and its domain reclaimed instead of leaking,
  - live under a per-runner lease dir so different runners don't fight over the
    same files (§12.2: "runner별 domain pool 을 분리").

Subcommands:
  allocate --pool LO-HI --ttl SEC --run-id ID   -> prints allocated domain id, or
                                                    exit 3 if the pool is exhausted
  release  --run-id ID                          -> idempotent
  reap                                          -> drop expired leases, print count
  list                                          -> active leases as JSONL

max_concurrent is the pool size itself (HI-LO+1); a separate lower cap can be
layered by the CI scheduler on top. `allocation: external` in the topology means
CI owns which pool a runner gets -- this tool is the mechanism, not the policy.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from pathlib import Path

DEFAULT_LEASE_DIR = os.environ.get("SILS_DOMAIN_LEASE_DIR", "/tmp/sils-domain-leases")
EXIT_EXHAUSTED = 3  # distinct code so a CI scheduler can tell "queue & retry" from a real error


def lease_dir(args) -> Path:
    d = Path(args.lease_dir)
    d.mkdir(parents=True, exist_ok=True)
    return d


def lease_path(d: Path, domain: int) -> Path:
    return d / f"{domain}.lease"


def parse_pool(pool: str) -> tuple[int, int]:
    if "-" not in pool:
        raise ValueError(f"pool must be LO-HI, got '{pool}'")
    lo_s, hi_s = pool.split("-", 1)
    lo, hi = int(lo_s), int(hi_s)
    if lo > hi:
        raise ValueError(f"pool LO ({lo}) > HI ({hi})")
    if lo < 0:
        raise ValueError("DDS domain ids must be >= 0")
    return lo, hi


def read_lease(path: Path) -> dict | None:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (FileNotFoundError, json.JSONDecodeError):
        return None


def is_expired(lease: dict, now: float) -> bool:
    return float(lease.get("expires", 0)) <= now


def reap_expired(d: Path, now: float) -> int:
    """Remove expired lease files. Best-effort: a file vanishing under us (another
    reaper/releaser) is fine, not an error."""
    count = 0
    for path in d.glob("*.lease"):
        lease = read_lease(path)
        if lease is None or is_expired(lease, now):
            try:
                path.unlink()
                count += 1
            except FileNotFoundError:
                pass
    return count


def cmd_allocate(args) -> int:
    d = lease_dir(args)
    lo, hi = parse_pool(args.pool)
    now = time.time()

    # Reap first so domains held by dead runs become claimable.
    reap_expired(d, now)

    payload_expires = now + args.ttl
    for domain in range(lo, hi + 1):
        path = lease_path(d, domain)
        try:
            # Atomic claim: O_EXCL fails if the lease file already exists.
            fd = os.open(str(path), os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o644)
        except FileExistsError:
            continue
        with os.fdopen(fd, "w") as f:
            json.dump(
                {
                    "domain": domain,
                    "run_id": args.run_id,
                    "owner_pid": os.getpid(),
                    "created": now,
                    "expires": payload_expires,
                    "ttl_sec": args.ttl,
                },
                f,
            )
        print(domain)
        return 0

    print(
        f"# error: domain pool {lo}-{hi} exhausted "
        f"({hi - lo + 1} domains all leased) -- queue and retry",
        file=sys.stderr,
    )
    return EXIT_EXHAUSTED


def cmd_release(args) -> int:
    d = lease_dir(args)
    released = 0
    for path in d.glob("*.lease"):
        lease = read_lease(path)
        if lease is not None and lease.get("run_id") == args.run_id:
            try:
                path.unlink()
                released += 1
            except FileNotFoundError:
                pass
    # Idempotent: releasing a run that holds nothing is success.
    print(f"# released {released} lease(s) for run_id={args.run_id}", file=sys.stderr)
    return 0


def cmd_reap(args) -> int:
    d = lease_dir(args)
    n = reap_expired(d, time.time())
    print(f"# reaped {n} expired lease(s)", file=sys.stderr)
    print(n)
    return 0


def cmd_list(args) -> int:
    d = lease_dir(args)
    now = time.time()
    for path in sorted(d.glob("*.lease")):
        lease = read_lease(path)
        if lease is None:
            continue
        lease["expired"] = is_expired(lease, now)
        print(json.dumps(lease))
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lease-dir", default=DEFAULT_LEASE_DIR)
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_alloc = sub.add_parser("allocate")
    p_alloc.add_argument("--pool", required=True, help="domain id range LO-HI")
    p_alloc.add_argument("--ttl", type=int, default=3600, help="lease TTL seconds")
    p_alloc.add_argument("--run-id", required=True)
    p_alloc.set_defaults(func=cmd_allocate)

    p_rel = sub.add_parser("release")
    p_rel.add_argument("--run-id", required=True)
    p_rel.set_defaults(func=cmd_release)

    p_reap = sub.add_parser("reap")
    p_reap.set_defaults(func=cmd_reap)

    p_list = sub.add_parser("list")
    p_list.set_defaults(func=cmd_list)

    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except ValueError as e:
        print(f"# error: {e}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
