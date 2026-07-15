#!/usr/bin/env python3
"""Self-contained checks for domain_lease.py -- no pytest, just asserts.

Run: python3 tools/sils/run_coordinator/test_domain_lease.py
"""

from __future__ import annotations

import json
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import domain_lease as dl  # noqa: E402


class Args:
    """Minimal stand-in for argparse.Namespace so we can call cmd_* directly."""

    def __init__(self, lease_dir, **kw):
        self.lease_dir = lease_dir
        for k, v in kw.items():
            setattr(self, k, v)


def fresh_dir():
    return tempfile.mkdtemp(prefix="sils-lease-test-")


def _allocate(d, run_id, pool="90-92", ttl=3600):
    import io
    import contextlib

    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = dl.cmd_allocate(Args(d, pool=pool, ttl=ttl, run_id=run_id))
    out = buf.getvalue().strip()
    return rc, out


def test_parse_pool():
    assert dl.parse_pool("90-99") == (90, 99)
    for bad in ["90", "99-90", "-5-3"]:
        try:
            dl.parse_pool(bad)
            assert False, f"expected ValueError for {bad}"
        except ValueError:
            pass


def test_sequential_allocation_is_unique():
    d = fresh_dir()
    rc1, a = _allocate(d, "run-a")
    rc2, b = _allocate(d, "run-b")
    rc3, c = _allocate(d, "run-c")
    assert rc1 == rc2 == rc3 == 0
    assert {a, b, c} == {"90", "91", "92"}, f"expected distinct 90,91,92 got {a},{b},{c}"


def test_pool_exhaustion_returns_distinct_code():
    d = fresh_dir()
    _allocate(d, "r1")
    _allocate(d, "r2")
    _allocate(d, "r3")
    rc, out = _allocate(d, "r4")  # pool 90-92 has only 3 domains
    assert rc == dl.EXIT_EXHAUSTED, f"expected EXIT_EXHAUSTED({dl.EXIT_EXHAUSTED}), got {rc}"


def test_release_frees_a_domain():
    d = fresh_dir()
    _allocate(d, "r1")
    _, b = _allocate(d, "r2")
    _allocate(d, "r3")
    # Pool full now. Release r2, then a new run should reclaim its domain.
    dl.cmd_release(Args(d, run_id="r2"))
    rc, out = _allocate(d, "r4")
    assert rc == 0 and out == b, f"expected reclaimed domain {b}, got rc={rc} out={out}"


def test_release_is_idempotent():
    d = fresh_dir()
    assert dl.cmd_release(Args(d, run_id="never-existed")) == 0
    assert dl.cmd_release(Args(d, run_id="never-existed")) == 0


def test_expired_lease_is_reaped_and_reclaimed():
    d = fresh_dir()
    # Fill the pool with already-expired leases by hand (expires in the past).
    now = time.time()
    for domain in (90, 91, 92):
        (Path(d) / f"{domain}.lease").write_text(
            json.dumps({"domain": domain, "run_id": f"dead-{domain}", "expires": now - 10})
        )
    # allocate reaps expired leases first, so it should succeed despite a "full" pool.
    rc, out = _allocate(d, "fresh")
    assert rc == 0, f"expected reclaim of expired domain, got rc={rc}"
    assert out in {"90", "91", "92"}


def test_live_lease_is_not_reaped():
    d = fresh_dir()
    _allocate(d, "alive", ttl=3600)
    reaped = dl.reap_expired(Path(d), time.time())
    assert reaped == 0, f"a live lease must not be reaped, got {reaped}"


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
