#!/usr/bin/env bash
#
# check_autoware_adapi.sh -- does OUR Autoware speak the interface scenario_simulator_v2 expects?
#
# WHY
#
# Stage 3 (B-full) runs SS_v2's scenario runner against our own Autoware
# (docker-compose-integration.yaml: oss-adsw-{decision,control,perception}), with
# launch_autoware:=false -- which means "Autoware is already up", NOT "no Autoware".
# In that configuration SS_v2's concealer::FieldOperatorApplication talks to Autoware over a
# fixed set of ROS topics and services.
#
# The risk is VERSION DRIFT, not absence. SS_v2 pins architecture_type
# "awf/universe/20240605" and its EgoEntity accepts any string containing "awf/universe",
# gating some interfaces on `architecture_type >= "awf/universe/20230906"` -- a LEXICOGRAPHIC
# date compare. So a mismatch does not fail loudly at startup; it shows up as an ego that
# never engages, or a service call that hangs. This script turns that into a table.
#
# The contract below is read from concealer at the pinned upstream commit
# (docker/sils/ss2-upstream.pin) -- see docs/sils/evidence/ss2-concealer-survey-2026-08-07.md:
#   external/concealer/src/field_operator_application.cpp   (name strings)
#   external/concealer/include/concealer/field_operator_application.hpp   (types)
#
# WHAT "MISSING" MEANS depends on the row's severity, and the severities are NOT all equally
# well established:
#   core    VERIFIED from source: nothing moves without it.
#   adapi   VERIFIED: the three together ARE the state source at architecture_type
#           >= awf/universe/20230906 (concealer::LegacyAutowareState derives from them).
#   legacy  the pre-20230906 state source, /autoware/state. Alternative to `adapi`.
#   drive   INFERRED -- needed to get the ego driving (engage / route / localize).
#   feature INFERRED -- used by particular scenario actions; absence limits which
#           scenarios run, it does not stop bring-up.
#
# ⚠ /api/autoware/set/velocity_limit was `feature` here until 2026-08-10, when a real run
# proved otherwise: RoutingAction.AcquirePositionAction -- one ego, no NPC, no RTC -- died
# with `Service "/api/autoware/set/velocity_limit" is not ready after 180 seconds`. It is
# `drive` now. The lesson generalises: concealer constructs clients for ALL ten services,
# and its failure mode for a missing server is a 180 s silence followed by a throw, so
# "this stub does not need that feature" is not a judgement we can make from the call site.
# Treat the remaining `feature` rows as unproven in the same direction.
# Treat `drive`/`feature` as a starting hypothesis, not a finding.
#
# Usage:
#   tools/sils/ss2/check_autoware_adapi.sh [--domain 42] [--image sils-ss2:humble]
#   tools/sils/ss2/check_autoware_adapi.sh --json out.json
# Exit: 0 all `core` rows present with matching types, 1 a core row failed, 2 usage/setup.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

EXIT_OK=0
EXIT_FAIL=1
EXIT_USAGE=2

DOMAIN="${SILS_DOMAIN:-42}"
IMAGE="${SILS_SS2_IMAGE:-sils-ss2:humble}"
JSON_OUT=""

die() { echo "check_autoware_adapi: $*" >&2; exit "${EXIT_USAGE}"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --domain) DOMAIN="$2"; shift 2 ;;
        --image)  IMAGE="$2"; shift 2 ;;
        --json)   JSON_OUT="$2"; shift 2 ;;
        -h|--help) sed -n '2,40p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit "${EXIT_USAGE}" ;;
        *) die "unknown argument: $1" ;;
    esac
done

[[ "${DOMAIN}" =~ ^[0-9]+$ ]] || die "--domain must be a number"

# --- the contract -------------------------------------------------------------
#
# kind|severity|name|type          -- `type` is what concealer compiles against.
#
# A NAME match with a TYPE mismatch is the dangerous case and the reason this script
# compares types at all: ROS 2 will happily let two nodes disagree about a topic's type,
# they simply never match, and neither side logs anything.
#
# ⚠ THERE ARE TWO WAYS TO READ AUTOWARE'S STATE, and only one has to work.
# concealer::LegacyAutowareState is built either
#   (legacy) straight from /autoware/state, or
#   (adapi)  DERIVED from localization + routing + operation_mode state,
# and which one applies is gated on architecture_type >= "awf/universe/20230906".
# The pinned default is awf/universe/20240605, i.e. the adapi path. So the three `adapi`
# rows below are jointly the state source, `legacy` is the fallback, and the gate passes
# when EITHER path is complete. Marking all four `core` (the first version of this file
# did) would fail a perfectly good modern Autoware for lacking a deprecated topic.
CONTRACT=$(cat <<'EOF'
topic|core|/control/command/control_cmd|autoware_control_msgs/msg/Control
topic|legacy|/autoware/state|autoware_system_msgs/msg/AutowareState
topic|adapi|/api/operation_mode/state|autoware_adapi_v1_msgs/msg/OperationModeState
topic|adapi|/api/routing/state|autoware_adapi_v1_msgs/msg/RouteState
topic|adapi|/api/localization/initialization_state|autoware_adapi_v1_msgs/msg/LocalizationInitializationState
topic|feature|/api/fail_safe/mrm_state|autoware_adapi_v1_msgs/msg/MrmState
topic|feature|/api/external/get/emergency|tier4_external_api_msgs/msg/Emergency
topic|feature|/api/external/get/rtc_status|tier4_rtc_msgs/msg/CooperateStatusArray
topic|feature|/control/command/turn_indicators_cmd|autoware_vehicle_msgs/msg/TurnIndicatorsCommand
topic|feature|/planning/scenario_planning/lane_driving/behavior_planning/path_with_lane_id|
service|drive|/api/external/set/engage|tier4_external_api_msgs/srv/Engage
service|drive|/api/localization/initialize|autoware_adapi_v1_msgs/srv/InitializeLocalization
service|drive|/api/routing/set_route_points|autoware_adapi_v1_msgs/srv/SetRoutePoints
service|drive|/api/routing/set_route|autoware_adapi_v1_msgs/srv/SetRoute
service|drive|/api/routing/clear_route|autoware_adapi_v1_msgs/srv/ClearRoute
service|drive|/api/operation_mode/enable_autoware_control|autoware_adapi_v1_msgs/srv/ChangeOperationMode
service|feature|/api/operation_mode/change_to_stop|autoware_adapi_v1_msgs/srv/ChangeOperationMode
service|drive|/api/autoware/set/velocity_limit|tier4_external_api_msgs/srv/SetVelocityLimit
service|feature|/api/external/set/rtc_commands|tier4_rtc_msgs/srv/CooperateCommands
service|feature|/api/external/set/rtc_auto_mode|tier4_rtc_msgs/srv/AutoModeWithModule
EOF
)
# path_with_lane_id has no type above on purpose: concealer wraps it in its own
# priority::PathWithLaneId, which resolves to a different package across Autoware
# generations (tier4_planning_msgs vs autoware_internal_planning_msgs). Comparing a guess
# would produce a false MISMATCH, so the name is checked and the observed type reported.

# --- gather from the live system ----------------------------------------------
#
# Prefer a host ros2 if there is one; otherwise borrow the SS_v2 image, which has rclcpp and
# every message package concealer needs. Either way ROS_DOMAIN_ID must be Autoware's --
# a wrong domain reports "everything missing", which reads like a version problem and is not.
gather() {
    if command -v ros2 >/dev/null 2>&1; then
        ROS_DOMAIN_ID="${DOMAIN}" ROS_LOCALHOST_ONLY=0 bash -c \
            'ros2 topic list -t 2>/dev/null; echo "---SERVICES---"; ros2 service list -t 2>/dev/null'
    else
        command -v docker >/dev/null 2>&1 \
            || die "neither ros2 nor docker on PATH -- cannot inspect the graph"
        docker run --rm --network host --ipc host \
            -e ROS_DOMAIN_ID="${DOMAIN}" -e ROS_LOCALHOST_ONLY=0 \
            -e RMW_IMPLEMENTATION="${SILS_RMW:-rmw_fastrtps_cpp}" \
            --entrypoint bash "${IMAGE}" -lc \
            'source /opt/ros/${ROS_DISTRO}/setup.bash >/dev/null 2>&1
             source /home/ubuntu/Desktop/scenario_simulator_ws/install/local_setup.bash >/dev/null 2>&1
             ros2 topic list -t 2>/dev/null
             echo "---SERVICES---"
             ros2 service list -t 2>/dev/null' 2>/dev/null
    fi
}

echo "check_autoware_adapi: SS_v2 <-> Autoware interface check"
echo "  domain   ${DOMAIN}     (must be Autoware's ROS_DOMAIN_ID)"
echo "  contract concealer @ $(sed -n 's/^commit=//p' "${REPO_ROOT}/docker/sils/ss2-upstream.pin" 2>/dev/null | head -1 || echo unrecorded)"
echo

GRAPH="$(gather)"
TOPIC_PART="${GRAPH%%---SERVICES---*}"
SVC_PART="${GRAPH#*---SERVICES---}"

# Emptiness must be judged AFTER splitting: gather() always emits the ---SERVICES---
# marker, so `-z "${GRAPH}"` is never true and an unreachable graph would otherwise be
# reported as 20 MISSING rows -- which reads like total version incompatibility and is
# actually "nothing is running".
if [[ -z "$(printf '%s' "${TOPIC_PART}${SVC_PART}" | tr -d '[:space:]')" ]]; then
    die "the ROS graph came back EMPTY -- no topics and no services on domain ${DOMAIN}.
  This is not a compatibility result; nothing was reachable. Check, in order:
    1. Autoware is up:
         docker compose -f docker/compose/docker-compose-integration.yaml up -d \\
           oss-adsw-decision oss-adsw-control oss-adsw-perception
    2. --domain matches its ROS_DOMAIN_ID (compose defaults to 42)
    3. same RMW on both sides (SILS_RMW, default rmw_fastrtps_cpp)"
fi

# `ros2 topic list -t` prints:  /name [pkg/msg/Type]
lookup() {  # kind name -> observed type, or empty
    local part
    [[ "$1" == "topic" ]] && part="${TOPIC_PART}" || part="${SVC_PART}"
    printf '%s\n' "${part}" \
        | awk -v n="$2" '$1 == n { gsub(/[][]/, "", $2); print $2; exit }'
}

printf '%-8s %-8s %-62s %s\n' KIND SEV NAME RESULT
printf '%.0s-' {1..110}; printf '\n'

core_fail=0; drive_missing=0; feature_missing=0; found=0
adapi_ok=0; adapi_total=0; legacy_ok=0
JSON_ROWS=""
while IFS='|' read -r kind sev name want; do
    [[ -z "${kind}" ]] && continue
    [[ "${sev}" == "adapi" ]] && adapi_total=$((adapi_total+1))
    got="$(lookup "${kind}" "${name}")"
    if [[ -z "${got}" ]]; then
        result="MISSING"
        case "${sev}" in core) core_fail=$((core_fail+1));;
                          drive) drive_missing=$((drive_missing+1));;
                          adapi|legacy) ;;   # scored jointly below
                          *) feature_missing=$((feature_missing+1));; esac
    elif [[ -n "${want}" && "${got}" != "${want}" ]]; then
        result="TYPE MISMATCH  want=${want}  got=${got}"
        [[ "${sev}" == "core" ]] && core_fail=$((core_fail+1))
    else
        [[ "${sev}" == "adapi" ]] && adapi_ok=$((adapi_ok+1))
        [[ "${sev}" == "legacy" ]] && legacy_ok=1
        result="ok${want:+}"
        [[ -z "${want}" ]] && result="ok (type unchecked: ${got})"
        found=$((found+1))
    fi
    printf '%-8s %-8s %-62s %s\n' "${kind}" "${sev}" "${name}" "${result}"
    JSON_ROWS="${JSON_ROWS}$(printf '\n    {"kind":"%s","severity":"%s","name":"%s","expected":"%s","observed":"%s"},' \
        "${kind}" "${sev}" "${name}" "${want}" "${got}")"
done <<< "${CONTRACT}"

# State is readable if EITHER path is whole -- see the contract note.
STATE_PATH="none"
[[ "${adapi_ok}" -eq "${adapi_total}" && "${adapi_total}" -gt 0 ]] && STATE_PATH="adapi"
[[ "${STATE_PATH}" == "none" && "${legacy_ok}" -eq 1 ]] && STATE_PATH="legacy"
[[ "${STATE_PATH}" == "none" ]] && core_fail=$((core_fail+1))

echo
echo "  present ${found}   core failures ${core_fail}   drive missing ${drive_missing}   feature missing ${feature_missing}"
echo "  state source: ${STATE_PATH}   (adapi ${adapi_ok}/${adapi_total}, legacy $([[ ${legacy_ok} -eq 1 ]] && echo present || echo absent))"

if [[ -n "${JSON_OUT}" ]]; then
    { echo '{'
      echo "  \"domain\": ${DOMAIN},"
      echo "  \"state_source\": \"${STATE_PATH}\","
      echo "  \"core_failures\": ${core_fail},"
      echo '  "rows": ['
      echo "${JSON_ROWS%,}"
      echo '  ]'
      echo '}'
    } > "${JSON_OUT}" || die "cannot write ${JSON_OUT}"
    echo "  json    ${JSON_OUT}"
fi

echo
if [[ "${core_fail}" -gt 0 ]]; then
    echo "  VERDICT: NOT COMPATIBLE as-is."
    if [[ "${STATE_PATH}" == "none" ]]; then
        echo "  NEITHER state path is whole: concealer cannot tell what Autoware is doing,"
        echo "  so waitForAutowareStateToBe() never returns and the ego never engages."
        echo "  It needs the three adapi rows together, OR /autoware/state alone."
    fi
    echo "  Before concluding version drift, rule out: wrong --domain, Autoware not fully"
    echo "  up yet, a namespace prefix on our partitions."
    exit "${EXIT_FAIL}"
fi
echo "  VERDICT: core interfaces present, state readable via '${STATE_PATH}'."
if [[ "${drive_missing}" -gt 0 ]]; then
    echo "  ${drive_missing} 'drive' row(s) missing -- the ego may never engage or route."
    echo "  These severities are INFERRED, not verified; check them against a real run"
    echo "  rather than trusting this line."
fi
exit "${EXIT_OK}"
