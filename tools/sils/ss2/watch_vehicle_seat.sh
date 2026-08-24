#!/usr/bin/env bash
#
# watch_vehicle_seat.sh -- is the stage-3 vehicle_interface seat still connected?
#
# One screen, one verdict, repeatable. Pairs with run_vehicle_seat_bridges.sh, which brings
# the seat up; this one tells you it is still alive an hour later.
#
# ★ WHY A DEDICATED WATCHER
#
# Every failure this rig has produced was SILENT. Not one of them printed "error" anywhere a
# person was looking:
#
#   - the DDS TypeObject fault killed the Autoware DECISION partition, and the only trace
#     was `invalid type object` buried in a vendor log file outside this repo
#   - a QoS or domain mismatch never matches at all, and "no publisher" looks identical to
#     "publisher is fine but I am on the wrong domain"
#   - the stamp anchor going stale leaves every counter healthy while Autoware quietly
#     rejects the messages as too old
#   - a fixture with --count exhausts and exits cleanly, so the plant just... stops
#
# So the checks below are not a generic health screen. Each one is a fault this rig has
# actually produced, and the wording says which.
#
# ★ WHAT IS CHECKED
#
#   1. containers      partitions + bridges, up or gone
#   2. type faults     `invalid type object` in the partition logs   <- the 08-11 regression
#   3. plant           something is writing canonical EgoState
#   4. bridges         received AND published RISING, not merely nonzero
#   5. anchor          stamp anchor latched and not stale
#   6. Autoware        the /vehicle/status + /localization topics have a live publisher
#
# Usage:
#   watch_vehicle_seat.sh                 one pass, then exit
#   watch_vehicle_seat.sh --interval 10   repeat every 10 s until Ctrl-C
#   watch_vehicle_seat.sh --domain 43     partitions on another domain
#   watch_vehicle_seat.sh --deep          also measure real topic rates (slow, ~25 s)
#
# Exit: 0 all green · 1 degraded (something is down or not moving) · 2 usage

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

DOMAIN="${ROS_DOMAIN_ID:-42}"
INTERVAL=0
DEEP=false
PARTITION_LOG_DIR="${PARTITION_LOG_DIR:-$HOME/oss/oss_adsw/log}"
# Names as run_vehicle_seat_bridges.sh creates them; the ad-hoc ones this session used are
# accepted too so a hand-started rig is still watchable.
BRIDGE_NAMES=(sils-aw-status sils-aw-localization sils-aw-status42 sils-aw-loc42)

die() { echo "watch_vehicle_seat: $*" >&2; exit 2; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --interval) INTERVAL="$2"; shift 2 ;;
        --domain)   DOMAIN="$2"; shift 2 ;;
        --deep)     DEEP=true; shift ;;
        -h|--help)  sed -n '2,45p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 2 ;;
        *) die "unknown argument: $1" ;;
    esac
done

GREEN=$'\033[32m'; RED=$'\033[31m'; YEL=$'\033[33m'; DIM=$'\033[2m'; OFF=$'\033[0m'
[ -t 1 ] || { GREEN=""; RED=""; YEL=""; DIM=""; OFF=""; }

ok()   { printf '  %sOK  %s %s\n'   "$GREEN" "$OFF" "$1"; }
bad()  { printf '  %sBAD %s %s\n'   "$RED"   "$OFF" "$1"; DEGRADED=1; }
warn() { printf '  %s??  %s %s\n'   "$YEL"   "$OFF" "$1"; }
note() { printf '      %s%s%s\n'    "$DIM" "$1" "$OFF"; }

# Counter deltas need memory between passes. Keep it in the run dir rather than /tmp so a
# watcher restarted in another terminal picks up where the last one left off.
STATE_DIR="${REPO_ROOT}/runs/.seat-watch"
mkdir -p "${STATE_DIR}" 2>/dev/null || STATE_DIR="$(mktemp -d)"

# Read the last "received=N published=M" the bridge logged. Deliberately the LAST line and
# not a sum: these are cumulative counters, and what matters is whether they still move.
bridge_counters() {   # $1 = container -> "received published" or empty
    docker logs --tail 400 "$1" 2>&1 \
        | grep -oE 'received=[0-9]+ published=[0-9]+' | tail -1 \
        | sed -E 's/received=([0-9]+) published=([0-9]+)/\1 \2/'
}

live_containers() {
    docker ps --format '{{.Names}}' 2>/dev/null
}

check_once() {
    DEGRADED=0
    echo
    echo "===== vehicle_interface seat @ $(date '+%H:%M:%S')  ROS domain ${DOMAIN} ====="

    local up; up="$(live_containers)"

    # --- 1. partitions ---------------------------------------------------------
    echo
    echo "[1] Autoware partitions"
    local found_partition=false
    for role in perception decision control; do
        local name="adsw-${role}-${DOMAIN}"
        if grep -qx "${name}" <<<"${up}"; then
            ok "${name}"
            found_partition=true
        elif docker ps -a --format '{{.Names}} {{.Status}}' 2>/dev/null | grep -q "^${name} "; then
            bad "${name} is GONE -- $(docker ps -a --format '{{.Names}}\t{{.Status}}' | grep "^${name}" | cut -f2)"
            note "a partition that exits on its own is the TypeObject fault until proven otherwise; see [2]"
        else
            warn "${name} not found (never started on this domain?)"
        fi
    done

    # --- 2. the 08-11 regression ----------------------------------------------
    #
    # This is the canary for the fix in sils_ego_source.cc. A NONZERO count here means some
    # participant is propagating a TypeObject that Cyclone cannot ingest -- and the last
    # time that happened it took down the decision partition within a second.
    echo
    echo "[2] DDS type faults in partition logs"
    if [ -d "${PARTITION_LOG_DIR}" ]; then
        local total=0
        for role in perception decision control; do
            local f="${PARTITION_LOG_DIR}/${role}_log.txt"
            [ -f "$f" ] || continue
            # grep -c already prints 0 when nothing matches -- it just exits 1.
            # `|| echo 0` would append a SECOND line and break the arithmetic below.
            local n; n="$(grep -c 'invalid type object' "$f" 2>/dev/null)"; n="${n:-0}"
            total=$(( total + n ))
            [ "$n" -gt 0 ] && bad "${role}: ${n} x 'invalid type object'"
        done
        if [ "$total" -eq 0 ]; then
            ok "no 'invalid type object' in any partition log"
        else
            note "a Fast DDS participant is propagating a TypeObject Cyclone rejects."
            note "every canonical-type writer needs type propagation off:"
            note "  Fast DDS 3.x: pqos property fastdds.type_propagation=disabled"
            note "  Fast DDS 2.6: TopicDataType::auto_fill_type_object(false)"
            note "see docs/sils/sils-stage3-autoware-partition.md, domain-42 section"
        fi
        note "logs are OVERWRITTEN per run and shared across domains (${PARTITION_LOG_DIR})"
    else
        warn "partition log dir not found: ${PARTITION_LOG_DIR}"
        note "set PARTITION_LOG_DIR if the katech repo lives elsewhere"
    fi

    # --- 3. the plant ----------------------------------------------------------
    echo
    echo "[3] plant (canonical EgoState writer)"
    # -x (process NAME), not -f (whole COMMAND LINE): -f also matches any shell whose argv
    # happens to mention the binary -- including one that is merely grepping for it.
    #
    # ⚠ -x compares against /proc/<pid>/comm, which the kernel TRUNCATES AT 15 CHARACTERS.
    # "sils_ego_source" is exactly 15, so this works -- by one character. Rename the binary
    # to anything longer and this check silently stops matching and reports "no plant"
    # forever. (run_vecu_wire.sh cannot use -x for that reason: its
    # sils_vecu_vehicle_probe is 23 characters and -x never matches it.)
    if pgrep -x sils_ego_source >/dev/null 2>&1; then
        ok "sils_ego_source running (OPEN-LOOP FIXTURE, not a plant)"
        note "it ignores Autoware's commands; --count exhausts and it exits cleanly"
    elif grep -qx 'sils-ss2-adapter' <<<"${up}" || grep -qx 'sils-ss2-scenario' <<<"${up}"; then
        ok "SS_v2 path up"
    else
        bad "nothing is writing canonical EgoState"
        note "the bridges will sit at 'no EgoState received yet' with no other symptom"
    fi

    # --- 4/5. the bridges ------------------------------------------------------
    echo
    echo "[4] bridges -- counters must RISE, not merely be nonzero"
    local any_bridge=false
    for name in "${BRIDGE_NAMES[@]}"; do
        grep -qx "${name}" <<<"${up}" || continue
        any_bridge=true

        local cur; cur="$(bridge_counters "${name}")"
        if [ -z "${cur}" ]; then
            bad "${name}: up, but has never reported a counter line"
            note "it reports every 5 s; if this persists it never matched the writer"
            continue
        fi
        local rx tx; read -r rx tx <<<"${cur}"

        local sf="${STATE_DIR}/${name}"
        if [ -f "${sf}" ]; then
            local prx ptx; read -r prx ptx < "${sf}"
            local drx=$(( rx - prx )) dtx=$(( tx - ptx ))
            if [ "${drx}" -le 0 ]; then
                bad "${name}: STALLED at received=${rx} (no change since last pass)"
                note "matched once and then stopped: check the plant first, then QoS durability"
            elif [ "${dtx}" -le 0 ]; then
                # RECEIVING BUT NOT PUBLISHING. This screen once reported "connected" for a
                # bridge in exactly this state, because it only checked `received`. The
                # bridge was dropping every sample and Autoware was starving.
                bad "${name}: received +${drx} but published +0 -- RECEIVING, NOT FORWARDING"
                note "the plant is fine and Autoware is starving; nothing else shows this"
                note "usual cause: the stamp anchor never latches, so every sample is dropped"
                note "  -> check for TWO writers on sils/ego/state (a leftover plant process)"
                note "     pgrep -xa sils_ego_source   |   docker ps | grep ss2"
            else
                ok "${name}: received +${drx}  published +${dtx}"
            fi
        else
            ok "${name}: received=${rx} published=${tx}  (baseline; deltas from next pass)"
        fi
        printf '%s %s\n' "${rx}" "${tx}" > "${sf}"

        # published < received is NORMAL only for the warm-up drop; anything later is loss.
        local dropped; dropped="$(docker logs --tail 200 "${name}" 2>&1 \
            | grep -oE 'warmup_dropped=[0-9]+' | tail -1 | cut -d= -f2)"
        dropped="${dropped:-0}"
        local gap=$(( rx - tx ))
        if [ "${gap}" -gt "${dropped}" ]; then
            warn "${name}: received-published=${gap} exceeds warmup_dropped=${dropped}"
            note "the excess is unexplained loss, not the TRANSIENT_LOCAL backlog drop"
        fi

        # --- 5. anchor -----------------------------------------------------------
        local anchor; anchor="$(docker logs --tail 200 "${name}" 2>&1 \
            | grep -oE 'stamp anchor ok: max deviation [-0-9.]+ ms|STAMP ANCHOR STALE: [0-9]+ samples past tolerance, max deviation [-0-9.]+ ms' \
            | tail -1)"
        if [ -z "${anchor}" ]; then
            note "${name}: no anchor line yet (stamp_policy=source, or still warming up)"
        elif [[ "${anchor}" == STAMP* ]]; then
            bad "${name}: ${anchor}"
            note "stamps are drifting out of Autoware's freshness window -- counters stay green"
            note "sim time is not advancing at wall rate; check the plant's realtime_factor"
        else
            ok "${name}: ${anchor}"
        fi
    done
    [ "${any_bridge}" = true ] || bad "no bridge container is running"

    # --- 6. the Autoware side --------------------------------------------------
    #
    # Publisher count, not a rate: it is cheap, and it distinguishes the two failures that
    # look identical from the bridge side -- "nobody is publishing" vs "publishing onto a
    # domain nothing is listening on".
    echo
    echo "[6] Autoware-side topics"
    local ctl="adsw-control-${DOMAIN}"
    if grep -qx "${ctl}" <<<"${up}"; then
        local topics="/vehicle/status/velocity_status /localization/kinematic_state"
        local out
        out="$(docker exec "${ctl}" bash -lc "
            source /opt/ros/humble/setup.bash >/dev/null 2>&1
            source /opt/autoware/setup.bash >/dev/null 2>&1
            for t in ${topics}; do
                n=\$(timeout 8 ros2 topic info \$t 2>/dev/null | grep -oE 'Publisher count: [0-9]+' | grep -oE '[0-9]+')
                echo \"\$t \${n:-?}\"
            done" 2>/dev/null)"
        if [ -z "${out}" ]; then
            warn "could not query topics inside ${ctl}"
        else
            while read -r topic n; do
                [ -z "${topic}" ] && continue
                if [ "${n}" = "?" ]; then
                    warn "${topic}: query failed"
                elif [ "${n}" -ge 1 ]; then
                    ok "${topic}: ${n} publisher(s)"
                else
                    bad "${topic}: NO publisher -- Autoware is starving on this one"
                fi
            done <<<"${out}"
        fi

        if [ "${DEEP}" = true ]; then
            echo "      measuring real rates (~25 s)..."
            docker exec "${ctl}" bash -lc "
                source /opt/ros/humble/setup.bash >/dev/null 2>&1
                source /opt/autoware/setup.bash >/dev/null 2>&1
                for t in ${topics}; do
                    printf '      %-42s ' \$t
                    timeout 12 ros2 topic hz \$t 2>/dev/null | grep -m1 'average rate' || echo 'NO DATA'
                done" 2>/dev/null
        fi
    else
        warn "${ctl} not up -- skipping Autoware-side checks"
    fi

    echo
    if [ "${DEGRADED}" -eq 0 ]; then
        printf '%sVERDICT: connected%s\n' "$GREEN" "$OFF"
    else
        printf '%sVERDICT: DEGRADED%s -- see BAD lines above\n' "$RED" "$OFF"
    fi
    return "${DEGRADED}"
}

if [ "${INTERVAL}" -le 0 ]; then
    check_once
    exit "$?"
fi

# First pass only establishes counter baselines, so deltas are meaningless until the second.
echo "watching every ${INTERVAL}s (Ctrl-C to stop). First pass sets the counter baseline."
while true; do
    check_once || true
    sleep "${INTERVAL}"
done
