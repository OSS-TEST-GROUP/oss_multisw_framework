#!/usr/bin/env bash
#
# check_closed_loop.sh -- one screen for the whole stage-2 loop, and one verdict.
#
# WHY
#
# Stage 2 is four processes with four separate logs, and reading all four still does not
# answer the only question that matters:
#
#   [scenario] ──/entity/status──▶ [ss2-adapter] ──▶ [vECU] ──▶ [ss2-control] ──▶ back in
#       ▲                                                                            │
#       └────────────────────────── vehicle model ◀──────────────────────────────────┘
#
# Every counter in that chain can look perfect while the loop is open. `published=500` on
# the outbound bridge proves only that state went out; `forwarded=500` on the return bridge
# proves only that a command was written to a topic. Neither says the vehicle model applied
# it -- a gear the simulator reads as NONE zeroes the acceleration, and both bridges still
# report full health.
#
# So this script does two things: it lays out who is up and what each one last said, and
# then it asks the SIMULATOR whether the ego is actually moving (closed_loop_probe.py).
# The speed on /entity/status is downstream of the vehicle model, which is the one place a
# healthy-looking pipe cannot fake.
#
# Read-only. It starts nothing and stops nothing.
#
# Usage:
#   tools/sils/ss2/check_closed_loop.sh
#   tools/sils/ss2/check_closed_loop.sh --seconds 20 --domain 42
# Exit: 0 the ego is moving under command, 1 it is not, 2 setup/usage.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

EXIT_OK=0
EXIT_FAIL=1
EXIT_USAGE=2

DOMAIN="${SILS_DOMAIN:-42}"
IMAGE="${SILS_SS2_IMAGE:-sils-ss2:humble}"
SECONDS_TO_WATCH=10
EGO_NAME="${SILS_SS2_EGO_NAME:-}"

die() { echo "check_closed_loop: $*" >&2; exit "${EXIT_USAGE}"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --seconds)  SECONDS_TO_WATCH="$2"; shift 2 ;;
        --domain)   DOMAIN="$2"; shift 2 ;;
        --image)    IMAGE="$2"; shift 2 ;;
        --ego-name) EGO_NAME="$2"; shift 2 ;;
        -h|--help)  sed -n '2,30p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit "${EXIT_USAGE}" ;;
        *) die "unknown argument: $1" ;;
    esac
done

command -v docker >/dev/null 2>&1 || die "docker is not on PATH"

echo "check_closed_loop: stage-2 loop status (read-only)"
echo "  domain ${DOMAIN}"
echo

# --- 1. who is up ------------------------------------------------------------
#
# The vECU is deliberately NOT in this table. It usually runs on the host rather than in a
# container, so `docker ps` cannot see it -- and asking docker would report it missing when
# it is fine. Its liveness is reported instead by the return bridge's `matched` count,
# which is a direct DDS observation of "somebody is writing our topic".
printf '  %-22s %-9s %s\n' PROCESS STATE 'LAST LINE'
printf '  %.0s-' {1..100}; printf '\n'

up_count=0
report_container() {  # <display> <container> <grep-pattern>
    local label="$1" name="$2" pat="$3" state last
    if docker ps --format '{{.Names}}' | grep -qx "${name}"; then
        state="up"
        up_count=$((up_count + 1))
    elif docker ps -a --format '{{.Names}}' | grep -qx "${name}"; then
        state="EXITED"
    else
        state="absent"
    fi
    last="$(docker logs --tail 200 "${name}" 2>&1 | grep -aE "${pat}" | tail -1 \
            | cut -c1-140)"
    printf '  %-22s %-9s %s\n' "${label}" "${state}" "${last:-–}"
}

report_container "autoware_stub"   "sils-ego-stub"      'derived autoware state|first contact'
report_container "scenario (SS_v2)" "sils-ss2-scenario" 'Run |AutowareError|\[ERROR\]'
report_container "ss2-adapter  out" "sils-ss2-adapter"  'received=|no samples'
report_container "ss2-control  in"  "sils-ss2-control"  'forwarded=|no VehicleControl'

echo
if [[ ${up_count} -eq 0 ]]; then
    die "none of the four containers is running -- there is no loop to check.
  Bring-up order matters: stub, scenario, ss2-adapter, vECU, ss2-control.
  See adapters/ss2/README.md §9.6."
fi

# The two failures worth naming before spending the observation window on them, because
# each has a specific fix and neither is visible in the speed reading.
ctl_log="$(docker logs --tail 200 sils-ss2-control 2>&1 || true)"
if grep -q "matched=0" <<< "${ctl_log}"; then
    echo "  ⚠ ss2-control reports matched=0 -- NOTHING is writing sils/ego/cmd."
    echo "    The vECU is down, on another VISS_DDS_DOMAIN_ID, or its channelmap has no"
    echo "    `out` channel. No command can reach the simulator in this state."
    echo
fi
if grep -qE "bad_gear=[1-9]" <<< "${ctl_log}"; then
    echo "  ⚠ ss2-control reports bad_gear>0 -- an unrecognised canonical gear is being"
    echo "    sent as NONE, and the simulator multiplies acceleration by sign_of(NONE)=0."
    echo "    The car cannot move while this persists, however healthy everything looks."
    echo
fi
if grep -qE "stale_holds=[1-9]" <<< "${ctl_log}"; then
    echo "  ⚠ ss2-control reports stale_holds>0 -- the vECU stopped publishing at some"
    echo "    point and the command was withdrawn. Do not trust this run's result."
    echo
fi

# --- 2. the only question that matters ---------------------------------------
echo "  asking the simulator whether the ego is moving (${SECONDS_TO_WATCH}s)..."
echo

docker run --rm --network host --ipc host \
    -e ROS_DOMAIN_ID="${DOMAIN}" -e ROS_LOCALHOST_ONLY=0 \
    -e RMW_IMPLEMENTATION="${SILS_RMW:-rmw_fastrtps_cpp}" \
    -e PYTHONUNBUFFERED=1 \
    -v "${SCRIPT_DIR}:/tools:ro" \
    --entrypoint bash "${IMAGE}" -lc "
source /opt/ros/\${ROS_DISTRO}/setup.bash >/dev/null 2>&1
source /home/ubuntu/Desktop/scenario_simulator_ws/install/local_setup.bash >/dev/null 2>&1
python3 /tools/closed_loop_probe.py --seconds ${SECONDS_TO_WATCH} \
    ${EGO_NAME:+--ego-name ${EGO_NAME}}" 2>&1 | sed 's/^/  /'

verdict=${PIPESTATUS[0]}
exit "${verdict}"
