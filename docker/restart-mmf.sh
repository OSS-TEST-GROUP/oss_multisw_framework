#!/bin/bash
# =============================================================================
# restart-mmf.sh — restart the local integration stack (mmf-core + mmf-ext-ros2)
# with a stale-SHM sweep in between.
#
# WHY: FastDDS' same-host SHM transport leaves /dev/shm/fastdds_* segments
# behind when a container is killed/restarted. The core↔ext-ros2 link RELIES
# on SHM (same-host UDP loopback is unreliable in this container setup), so we
# can't just disable SHM. Instead we sweep the ORPHANED segments — the ones no
# live process still holds — every time the stack comes up, so zombies from a
# previous run can't corrupt DDS writer-proxy registration (which made
# DiagReport / NotifyServiceState stop reaching lfc_monitor after a restart).
#
# The sweep runs on the host while the stack is stopped, and `fuser` protects
# any segment still held by a live process, so it is safe even if some other
# DDS process is running on the host.
#
# Usage (from SUT root or anywhere):
#   ./docker/restart-mmf.sh            # restart core + ext-ros2 with SHM sweep
#   ./docker/restart-mmf.sh --no-sweep # plain restart, skip the sweep
# =============================================================================
set -e

cd "$(dirname "$0")/.."   # → SUT root
COMPOSE="docker compose -f docker/compose/docker-compose-integration.yaml"
SERVICES="mmf-core mmf-ext-ros2"
SWEEP=1
[ "$1" = "--no-sweep" ] && SWEEP=0

sweep_shm() {
    # Remove ALL FastDDS SHM segments. This runs ONLY after `compose stop` has
    # brought mmf-core + mmf-ext-ros2 down, so every /dev/shm/fastdds_* is an
    # orphan of the just-stopped stack — safe to delete wholesale. (An earlier
    # fuser-guarded version kept some segments that a lingering handle still
    # "held", leaving a stale DiagReport phantom that the bridge then pinned;
    # a full wipe is what actually clears it.)
    #
    # sudo because privileged containers create root-owned segments that the
    # compose invoker (non-root) otherwise can't remove. NOPASSWD assumed;
    # falls back to a plain rm for whatever is owner-removable.
    local before after
    before=$(ls /dev/shm 2>/dev/null | grep -c '^fastdds' || true)
    sudo rm -f /dev/shm/fastdds_* /dev/shm/fast_datasharing_* 2>/dev/null || \
        rm -f /dev/shm/fastdds_* /dev/shm/fast_datasharing_* 2>/dev/null || true
    after=$(ls /dev/shm 2>/dev/null | grep -c '^fastdds' || true)
    echo "[restart-mmf] SHM sweep: ${before} → ${after} fastdds segment(s)"
}

echo "[restart-mmf] stopping ${SERVICES} ..."
$COMPOSE stop $SERVICES

if [ "$SWEEP" -eq 1 ]; then
    sweep_shm
else
    echo "[restart-mmf] --no-sweep: skipping SHM sweep"
fi

echo "[restart-mmf] starting mmf-core ..."
$COMPOSE up -d mmf-core
sleep 8   # let core's DDS come up before the bridge subscribes
echo "[restart-mmf] starting mmf-ext-ros2 ..."
$COMPOSE up -d mmf-ext-ros2

echo "[restart-mmf] done. Tail logs with:"
echo "  docker logs -f oss-mmf-core"
echo "  docker logs -f oss-mmf-ext-ros2"
