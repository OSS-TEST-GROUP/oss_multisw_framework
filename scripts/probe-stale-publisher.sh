#!/bin/bash
# probe-stale-publisher.sh
#
# Diagnose phantom publishers of mmf/lifecyclemgr/StartServiceResponse.
#
# Symptom: lifecycle_mgr.log shows recurring `Service Start Failure 1` even
# while adsw_manager.log has stopped writing StartServiceResponse. Some other
# FastDDS participant on the same domain is publishing FAILED responses.
#
# Strategy: SIGSTOP each candidate process one at a time, watch the lifecycle_mgr
# log for `Service Start Failure 1` arrivals during the freeze window, then
# SIGCONT it. The candidate that silences the failures is the phantom writer.
#
# Usage:
#   probe-stale-publisher.sh [--lifecycle-log PATH] [--freeze-secs N]
#
# Defaults:
#   --lifecycle-log  /proc/<lifecycle_mgr-pid>/root/oss/lifecycle_mgr.log
#   --freeze-secs    35  (covers two ~15s FAILED intervals)
#
# Safety: SIGSTOP is reversible — every candidate is SIGCONT'd before moving
# to the next. The script aborts (and SIGCONTs all stopped PIDs) on Ctrl-C.

set -u

FREEZE_SECS=35
LIFECYCLE_LOG=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --lifecycle-log)  LIFECYCLE_LOG="$2"; shift 2 ;;
        --freeze-secs)    FREEZE_SECS="$2";  shift 2 ;;
        -h|--help)
            sed -n '2,22p' "$0"; exit 0 ;;
        *) echo "Unknown arg: $1" >&2; exit 2 ;;
    esac
done

# Locate lifecycle_mgr's log via /proc if not given.
if [[ -z "$LIFECYCLE_LOG" ]]; then
    lc_pid=$(pgrep -x lifecycle_mgr | head -1)
    if [[ -z "$lc_pid" ]]; then
        echo "lifecycle_mgr not running — pass --lifecycle-log explicitly." >&2
        exit 1
    fi
    LIFECYCLE_LOG="/proc/${lc_pid}/root/oss/lifecycle_mgr.log"
fi
if [[ ! -r "$LIFECYCLE_LOG" ]]; then
    echo "Cannot read lifecycle log: $LIFECYCLE_LOG" >&2
    exit 1
fi

# Discover candidate publishers: every process holding a FastDDS shm region,
# minus lifecycle_mgr/adsw_manager (the known-good endpoints).
mapfile -t CANDIDATES < <(
    lsof /dev/shm 2>/dev/null \
        | awk '/fastdds_/ {print $2}' \
        | sort -u \
        | while read -r pid; do
              name=$(ps -p "$pid" -o comm= 2>/dev/null || true)
              [[ -z "$name" ]] && continue
              [[ "$name" == "lifecycle_mgr" || "$name" == "adsw_manager" ]] && continue
              echo "${pid}:${name}"
          done
)
# Also probe lifecycle_mgr/adsw_manager peers that don't use shm — discover
# them by matching DDS UDP listeners. We *don't* stop these; we just list them
# so the operator can rule them out separately if every shm candidate is clean.
mapfile -t UDP_PEERS < <(
    pgrep -af 'lifecycle_mgr|adsw_manager|dds_bridge|ota_client|OtaManager|vehicle_info_manager_app' \
        | awk '{print $1":"$2}' \
        | sed 's|.*/||'
)

# Baseline failure rate.
count_failures() {
    local since="$1" until="$2"
    awk -v since="$since" -v until="$until" '
        /Service Start Failure/ {
            # log format: [YYYY-MM-DD HH:MM:SS] ...
            ts=$1" "$2; gsub(/[\[\]]/, "", ts)
            if (ts >= since && ts <= until) c++
        }
        END { print c+0 }
    ' "$LIFECYCLE_LOG"
}
now_ts() { date -u '+%Y-%m-%d %H:%M:%S'; }

# Match log timezone: the file uses UTC ("2026-05-20 15:42:02"). `date -u` gives
# UTC. If your build emits local-time logs, drop the -u above.

cleanup() {
    # Resume any candidate we left stopped on abort.
    for entry in "${STOPPED[@]:-}"; do
        local pid="${entry%%:*}"
        kill -CONT "$pid" 2>/dev/null || true
    done
}
trap cleanup EXIT INT TERM
STOPPED=()

echo "lifecycle log:  $LIFECYCLE_LOG"
echo "freeze window:  ${FREEZE_SECS}s per candidate"
echo
echo "UDP-only DDS peers (not stopped, listed for visibility):"
for p in "${UDP_PEERS[@]}"; do echo "  - $p"; done
echo
echo "Stoppable shm-using candidates: ${#CANDIDATES[@]}"
for c in "${CANDIDATES[@]}"; do echo "  - $c"; done
echo

if [[ ${#CANDIDATES[@]} -eq 0 ]]; then
    echo "No shm-using candidates found. The phantom writer may be UDP-only —"
    echo "stop the UDP peers above one at a time manually and re-measure."
    exit 0
fi

# Baseline: count failures in the previous FREEZE_SECS without stopping anything.
echo "[baseline] sampling ${FREEZE_SECS}s with everything running..."
b_start="$(now_ts)"
sleep "$FREEZE_SECS"
b_end="$(now_ts)"
BASELINE=$(count_failures "$b_start" "$b_end")
echo "[baseline] $BASELINE failures in window"
echo

for entry in "${CANDIDATES[@]}"; do
    pid="${entry%%:*}"
    name="${entry##*:}"
    if ! kill -0 "$pid" 2>/dev/null; then
        echo "[skip]   $name ($pid) — already gone"
        continue
    fi
    echo "[freeze] $name ($pid) — SIGSTOP for ${FREEZE_SECS}s..."
    kill -STOP "$pid"
    STOPPED+=("$pid:$name")
    s_start="$(now_ts)"
    sleep "$FREEZE_SECS"
    s_end="$(now_ts)"
    kill -CONT "$pid" 2>/dev/null || true
    STOPPED=("${STOPPED[@]/$pid:$name}")
    n=$(count_failures "$s_start" "$s_end")
    if [[ "$n" -lt "$BASELINE" ]]; then
        verdict="!! SUSPECT — failures dropped from $BASELINE to $n"
    else
        verdict="ok    (failures: $n vs baseline $BASELINE)"
    fi
    printf "[result] %-32s %s\n" "$name ($pid)" "$verdict"
done

echo
echo "Done. A SUSPECT line above is your phantom writer of"
echo "  mmf/lifecyclemgr/StartServiceResponse"
echo "Inspect that process's source for unintended publishes on the topic,"
echo "or stop/restart it cleanly and confirm Service Start Failure 1 stops."
