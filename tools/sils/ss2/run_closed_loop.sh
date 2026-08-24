#!/usr/bin/env bash
#
# run_closed_loop.sh -- stage 2 end to end: bring the five pieces up IN ORDER and say
# whether the ego actually moved.
#
#   [scenario] ──/entity/status──▶ [ss2-adapter] ──▶ [vECU] ──▶ [ss2-control] ──┐
#       ▲                                                                        │
#       └──────────────── vehicle model ◀── /control/command/* ◀─────────────────┘
#
# ★ ORDER IS NOT COSMETIC, AND THE SCENARIO GOES LAST.
#
# Four of the five are idle-safe: a bridge with nothing to bridge waits quietly, and the
# vECU with no state produces no command. The scenario is the ONLY one with a clock -- it
# runs global_timeout seconds and then exits, whether or not anything was listening. Start
# it first and the rest of the bring-up burns the very window it was meant to measure.
# That is exactly how the first attempt failed: by the time the adapter and the vECU were
# up, the thing they were supposed to observe had already finished.
#
# The stub goes FIRST for a different reason: concealer asks for its services during
# EgoEntity construction and, if they are missing, waits 180 SECONDS in complete silence
# before throwing (concealer/service.hpp). A stub started late does not fail, it stalls.
#
# WHAT A PASS MEANS, AND WHAT IT DOES NOT
#
# PASS = the ego's speed rose on /entity/status, which is the SIMULATOR's own report from
# downstream of the vehicle model. Nothing else in this configuration commands
# acceleration, so the loop is closed.
#
# PASS does NOT mean the scenario succeeded. The reference vECU behaviour is a longitudinal
# P-controller with steering pinned to 0 (vehicle_behavior.hpp), so the ego drives straight
# and will still miss an AcquirePosition goal that needs lane following. Closing the loop
# and completing a scenario are two different milestones; this script measures the first.
#
# Usage:
#   tools/sils/ss2/run_closed_loop.sh
#   tools/sils/ss2/run_closed_loop.sh --bin-dir <dir>   # if several probe copies exist
#   tools/sils/ss2/run_closed_loop.sh --watch 20 --domain 42
#   tools/sils/ss2/run_closed_loop.sh --keep      # leave everything up afterwards
# Exit: 0 the ego moved, 1 it did not, 2 setup/usage.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

EXIT_OK=0
EXIT_FAIL=1
EXIT_USAGE=2

DOMAIN="${SILS_DOMAIN:-42}"
IMAGE="${SILS_SS2_IMAGE:-sils-ss2:humble}"
ADAPTER_IMAGE="${SILS_SS2_ADAPTER_IMAGE:-sils-ss2-adapter:humble}"
COMPOSE="${REPO_ROOT}/docker/compose/docker-compose-sils-ss2.yaml"
OUT_DIR="${REPO_ROOT}/runs/closed-loop"
RUN_ID="closed-loop"
# Seconds to let the scenario load the map and spawn the ego before we start measuring.
# Observed on this pin: Run at t+4s, map loaded t+6s, `driving` at t+7s.
SETTLE=18
WATCH=20
ZMQ_PORT="${SILS_SS2_ZMQ_PORT:-5555}"
STUB="sils-closed-loop-stub"
KEEP=0
SCENARIO_REL="scenario/RoutingAction.AcquirePositionAction.yaml"
SCENARIO_ABS=""
# Passed straight through to run_vecu_wire.sh. Needed whenever the tree holds more than
# one built copy of the probe -- that script refuses to pick for you, which is right:
# silently running a stale binary is how a "wiring works" result gets earned by code
# that no longer exists.
BIN_DIR=""
# Decides which vehicle model the simulator uses, and therefore whether our acceleration
# command is read at all. See the check in step 5.
VEHICLE_MODEL="${SILS_SS2_VEHICLE_MODEL:-sample_vehicle}"

die() { echo "run_closed_loop: $*" >&2; exit "${EXIT_USAGE}"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --domain)   DOMAIN="$2"; shift 2 ;;
        --scenario) SCENARIO_ABS="$2"; shift 2 ;;
        --settle)   SETTLE="$2"; shift 2 ;;
        --watch)    WATCH="$2"; shift 2 ;;
        --out)      OUT_DIR="$2"; shift 2 ;;
        --zmq-port) ZMQ_PORT="$2"; shift 2 ;;
        --bin-dir)  BIN_DIR="$2"; shift 2 ;;
        --vehicle-model) VEHICLE_MODEL="$2"; shift 2 ;;
        --keep)     KEEP=1; shift ;;
        -h|--help)  sed -n '2,40p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit "${EXIT_USAGE}" ;;
        *) die "unknown argument: $1" ;;
    esac
done

[[ "${DOMAIN}" =~ ^[0-9]+$ ]] || die "--domain must be a number"
command -v docker >/dev/null 2>&1 || die "docker is not on PATH"
docker image inspect "${IMAGE}" >/dev/null 2>&1 \
    || die "image '${IMAGE}' not found. Build it: docker/sils/build-ss2.sh"
docker image inspect "${ADAPTER_IMAGE}" >/dev/null 2>&1 \
    || die "image '${ADAPTER_IMAGE}' not found -- this is the one that carries BOTH bridges.
  Build it: docker/sils/build-ss2-adapter.sh
  (B2-3 added ss2_control_bridge to that package, so an image built before it has only
  ss2_ego_bridge and the return path will not exist.)"
mkdir -p "${OUT_DIR}" "${REPO_ROOT}/runs/ss2-scenario" 2>/dev/null \
    || die "cannot create ${OUT_DIR} (a container may own it: sudo chown -R \$USER ${REPO_ROOT}/runs)"

IN_CONTAINER_ENV='source /opt/ros/${ROS_DISTRO}/setup.bash >/dev/null 2>&1
source /home/ubuntu/Desktop/scenario_simulator_ws/install/local_setup.bash >/dev/null 2>&1
export PYTHONPATH=/stub:${PYTHONPATH:-}'

PIN="$(sed -n 's/^commit=//p' "${REPO_ROOT}/docker/sils/ss2-upstream.pin" 2>/dev/null | head -1)"

echo "run_closed_loop: stage 2 -- does the ego move because of OUR vECU?"
echo "  domain    ${DOMAIN}   (ROS and DDS alike; every piece must agree)"
echo "  images    ${IMAGE} / ${ADAPTER_IMAGE}   (ss2 pin ${PIN:-unrecorded})"
echo "  out       ${OUT_DIR}"

if [[ -z "${SCENARIO_ABS}" ]]; then
    share="$(docker run --rm --entrypoint bash "${IMAGE}" -lc "${IN_CONTAINER_ENV}
ros2 pkg prefix --share scenario_test_runner 2>/dev/null" 2>/dev/null | tr -d '\r' | tail -1)"
    [[ -n "${share}" ]] || die "could not resolve scenario_test_runner's share dir in ${IMAGE}"
    SCENARIO_ABS="${share}/${SCENARIO_REL}"
fi
echo "  scenario  ${SCENARIO_ABS}"
echo

# --- 0. clean slate ----------------------------------------------------------
#
# By name as well as through compose: `docker compose rm` only removes what compose still
# recognises as its own, and a container left by a killed run survives it. That leftover is
# not merely untidy -- its simulator still holds the ZMQ port, and the next interpreter
# connects to the OLD simulator with an ESTABLISHED socket and a completely healthy-looking
# `ss` output while the two talk about different maps.
echo "[0/6] clearing previous runs"
docker rm -f "${STUB}" sils-ego-stub sils-ss2-scenario >/dev/null 2>&1 || true
docker compose -f "${COMPOSE}" rm -sf ss2-scenario ss2-adapter ss2-control >/dev/null 2>&1 || true
docker rm -f sils-ss2-adapter sils-ss2-control >/dev/null 2>&1 || true

if (exec 3<>/dev/tcp/127.0.0.1/"${ZMQ_PORT}") 2>/dev/null; then
    exec 3<&- 2>/dev/null || true
    docker ps --format '    {{.Names}}  ({{.Image}})'
    die "ZMQ port ${ZMQ_PORT} is still held after cleanup. A new simulator cannot bind it and
  the interpreter would talk to whatever is already there -- a hang that looks like a DDS
  fault. Stop it, or pass --zmq-port."
fi
echo "      done (ZMQ ${ZMQ_PORT} free)"

cleanup() {
    if [[ ${KEEP} -eq 1 ]]; then
        echo
        echo "  --keep: everything left running."
        echo "    tools/sils/ss2/check_closed_loop.sh --domain ${DOMAIN}"
        echo "    docker rm -f ${STUB}; docker compose -f ${COMPOSE} down"
    else
        docker rm -f "${STUB}" >/dev/null 2>&1 || true
        docker compose -f "${COMPOSE}" rm -sf ss2-scenario ss2-adapter ss2-control \
            >/dev/null 2>&1 || true
        docker rm -f sils-ss2-scenario sils-ss2-adapter sils-ss2-control >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

# --- 1. the stub -------------------------------------------------------------
echo
echo "[1/6] autoware_stub  (first: concealer stalls 180s in silence without it)"
docker run -d --network host --ipc host --name "${STUB}" \
    -e ROS_DOMAIN_ID="${DOMAIN}" -e ROS_LOCALHOST_ONLY=0 \
    -e RMW_IMPLEMENTATION="${SILS_RMW:-rmw_fastrtps_cpp}" -e PYTHONUNBUFFERED=1 \
    -v "${REPO_ROOT}/adapters/autoware-stub:/stub:ro" \
    --entrypoint bash "${IMAGE}" -lc "${IN_CONTAINER_ENV}
exec python3 -m autoware_stub.stub_node" >/dev/null || die "could not start the stub"
sleep 3
docker ps --format '{{.Names}}' | grep -qx "${STUB}" || {
    docker logs "${STUB}" 2>&1 | tail -20 | sed 's/^/      /'
    die "the stub exited immediately (output above)"
}
echo "      up"

# --- 2/3. the two bridges ----------------------------------------------------
#
# Both before the scenario, both idle-safe. The outbound one will log "no samples" until
# the simulator exists; that warning is expected here and is not a fault.
echo
echo "[2/6] ss2-adapter   (state out: /entity/status -> sils/ego/state)"
# STATE_TOPIC is namespaced here, unlike stage 1. scenario_test_runner launches the
# interpreter with namespace="simulation", and traffic_simulator's "entity/status" is a
# RELATIVE name, so it comes out as /simulation/entity/status. cpp_mock_scenarios (stage 1)
# runs its scenario node with no namespace and publishes /entity/status. Same code, two
# names, and subscribing to the wrong one is silent -- it cost us a full run that reported
# "NOTHING RECEIVED" while the simulator was publishing perfectly.
STATE_TOPIC="${SILS_SS2_STATE_TOPIC:-/simulation/entity/status}"
echo "      state topic ${STATE_TOPIC}"
SILS_DOMAIN="${DOMAIN}" SILS_RUN_ID="${RUN_ID}" SILS_DURABILITY="transient-local" \
SILS_SS2_STATE_TOPIC="${STATE_TOPIC}" \
    docker compose -f "${COMPOSE}" up -d ss2-adapter >/dev/null 2>&1 \
    || die "compose could not start ss2-adapter"
echo "      up"

echo
echo "[3/6] ss2-control   (command back: sils/ego/cmd -> /control/command/*)"
SILS_DOMAIN="${DOMAIN}" \
    docker compose -f "${COMPOSE}" up -d ss2-control >/dev/null 2>&1 \
    || die "compose could not start ss2-control"
sleep 2
if ! docker ps --format '{{.Names}}' | grep -qx sils-ss2-control; then
    docker logs sils-ss2-control 2>&1 | tail -20 | sed 's/^/      /'
    die "ss2-control exited immediately (output above).
  If it says 'executable not found', the adapter image predates B2-3 -- rebuild it:
  docker/sils/build-ss2-adapter.sh"
fi
echo "      up"

# --- 4. the vECU -------------------------------------------------------------
#
# Delegated to run_vecu_wire.sh rather than reimplemented: that script already wipes the
# inherited SILS_*/VISS_* environment (a leaked SILS_RUN_ID from an earlier session cost us
# a run once), resolves the binary without guessing among several copies, warns when the
# binary predates the trace fields, and analyses the state path at the end. Duplicating any
# of that here would give us a second place for those rules to drift.
#
# Its duration must outlast our measurement window, or the vECU stops commanding halfway
# through and ss2-control's watchdog withdraws the command -- which would read as a
# stationary car.
VECU_DURATION=$(( SETTLE + WATCH + 20 ))
echo
echo "[4/6] vECU          (run_vecu_wire.sh, ${VECU_DURATION}s)"
"${SCRIPT_DIR}/run_vecu_wire.sh" \
    --domain "${DOMAIN}" --run-id "${RUN_ID}" --duration "${VECU_DURATION}" \
    ${BIN_DIR:+--bin-dir "${BIN_DIR}"} \
    --out "${OUT_DIR}/vecu" > "${OUT_DIR}/vecu-wire.log" 2>&1 &
VECU_PID=$!
sleep 3
kill -0 "${VECU_PID}" 2>/dev/null || {
    tail -25 "${OUT_DIR}/vecu-wire.log" | sed 's/^/      /'
    if grep -q "copies of" "${OUT_DIR}/vecu-wire.log" 2>/dev/null; then
        die "several built copies of the probe exist and run_vecu_wire will not choose for
  you. Pick one from the list above and pass it to THIS script:

      tools/sils/ss2/run_closed_loop.sh --bin-dir <path>

  Prefer the NEWEST unless you have a reason not to -- an older copy predates the trace
  fields the analysis reads, and it fails as inconclusive results rather than as an error."
    fi
    die "the vECU driver exited immediately (output above; full log ${OUT_DIR}/vecu-wire.log)"
}
echo "      running (log ${OUT_DIR}/vecu-wire.log)"

# --- 5. the scenario, LAST ---------------------------------------------------
#
# global_timeout is generous on purpose: it is the only clock in the system, and the
# measurement window has to fit inside it with room to spare.
echo
echo "[5/6] scenario      (LAST -- it is the only piece with a clock)"

# ★ Which vehicle model will the simulator use? Ask BEFORE starting, because the wrong
#   answer produces a perfectly healthy pipeline and a car that never moves.
#
#   simple_sensor_simulator: getParameter<std::string>("vehicle_model_type",
#                                                      "IDEAL_STEER_VEL")
#   and for the *_VEL family, ego_entity_simulation.cpp does
#       input(0) = speed;      // = control_command.longitudinal.velocity
#   which is the one number our canonical contract does not carry. We publish 0.0 there.
#   The acceleration we DO carry is never read. Result: 755 commands delivered, vx 0.000.
#
#   Nobody sets vehicle_model_type directly: the launch derives it from
#   <vehicle_model>_description/config/simulator_model.param.yaml, and skips that whole
#   step when vehicle_model is empty -- which is the upstream default.
MODEL_TYPE=""
if [[ -n "${VEHICLE_MODEL}" ]]; then
    MODEL_TYPE="$(docker run --rm --entrypoint bash "${IMAGE}" -lc "${IN_CONTAINER_ENV}
share=\$(ros2 pkg prefix --share ${VEHICLE_MODEL}_description 2>/dev/null) || exit 3
sed -n 's/.*vehicle_model_type:[[:space:]]*[\"'\'']*\([A-Z_]*\).*/\1/p' \
    \"\${share}/config/simulator_model.param.yaml\" 2>/dev/null | head -1" 2>/dev/null \
        | tr -d '\r' | tail -1)"
fi

if [[ -z "${VEHICLE_MODEL}" ]]; then
    echo "      ⚠ vehicle_model is EMPTY -- the simulator falls back to IDEAL_STEER_VEL,"
    echo "        which reads longitudinal.velocity (we publish 0.0) and ignores our"
    echo "        acceleration entirely. The ego will not move. Pass --vehicle-model."
elif [[ -z "${MODEL_TYPE}" ]]; then
    echo "      ⚠ could not read vehicle_model_type from ${VEHICLE_MODEL}_description."
    echo "        Either the package is not in this image or its simulator_model.param.yaml"
    echo "        has a shape this check does not parse. Proceeding, but if the ego stays"
    echo "        still that is the first thing to verify by hand."
elif [[ "${MODEL_TYPE}" == *VEL ]]; then
    echo "      ⚠ ${VEHICLE_MODEL} resolves to ${MODEL_TYPE} -- a VELOCITY model."
    echo "        It reads longitudinal.velocity, which our canonical contract does not"
    echo "        carry, so the ego will not move however healthy the counters look."
    echo "        Pick a vehicle model whose type is an *_ACC* variant."
else
    echo "      vehicle model ${VEHICLE_MODEL} -> ${MODEL_TYPE}  (acceleration is read)"
fi

SILS_SS2_VEHICLE_MODEL="${VEHICLE_MODEL}" \
SILS_DOMAIN="${DOMAIN}" \
SILS_SS2_SCENARIO="${SCENARIO_ABS}" \
SILS_SS2_LAUNCH_AUTOWARE=false \
SILS_SS2_GLOBAL_TIMEOUT=$(( SETTLE + WATCH + 60 )) \
SILS_SS2_ZMQ_PORT="${ZMQ_PORT}" \
    docker compose -f "${COMPOSE}" up -d ss2-scenario >/dev/null 2>&1 \
    || die "compose could not start ss2-scenario"
echo "      up; settling ${SETTLE}s (map load + ego spawn + concealer handshake)"

for _ in $(seq 1 $(( SETTLE / 3 ))); do
    sleep 3
    if ! docker ps --format '{{.Names}}' | grep -qx sils-ss2-scenario; then
        echo "      the scenario exited during settle -- see ${OUT_DIR}/scenario.log"
        break
    fi
done

state="$(docker logs "${STUB}" 2>&1 | grep -oE 'derived autoware state -> [a-z_]+' | tail -1)"
echo "      stub reports: ${state:-<nothing>}"
if [[ "${state}" != *driving ]]; then
    echo "      ⚠ concealer has not reached \`driving\`. The ego is not drivable yet, so a"
    echo "        stationary result below would be about the HANDSHAKE, not the control path."
    echo "        Diagnose that first: tools/sils/ss2/run_ego_scenario.sh"
fi

# --- 6. the verdict ----------------------------------------------------------
echo
echo "[6/6] watching the ego for ${WATCH}s"
echo
docker run --rm --network host --ipc host \
    -e ROS_DOMAIN_ID="${DOMAIN}" -e ROS_LOCALHOST_ONLY=0 \
    -e RMW_IMPLEMENTATION="${SILS_RMW:-rmw_fastrtps_cpp}" -e PYTHONUNBUFFERED=1 \
    -v "${SCRIPT_DIR}:/tools:ro" \
    --entrypoint bash "${IMAGE}" -lc "${IN_CONTAINER_ENV}
python3 /tools/closed_loop_probe.py --seconds ${WATCH}" 2>&1 | tee "${OUT_DIR}/probe.txt" \
    | sed 's/^/  /'
verdict=${PIPESTATUS[0]}

# Logs first, then the counters -- the counters are only interpretable next to the verdict.
for c in "${STUB}" sils-ss2-scenario sils-ss2-adapter sils-ss2-control; do
    docker logs "${c}" > "${OUT_DIR}/${c}.log" 2>&1 || true
done

echo
echo "  --- counters at the end ---"
printf '  %-16s %s\n' "ss2-adapter" \
    "$(grep -aE 'received=' "${OUT_DIR}/sils-ss2-adapter.log" 2>/dev/null | tail -1 | cut -c1-120)"
printf '  %-16s %s\n' "ss2-control" \
    "$(grep -aE 'forwarded=|no VehicleControl' "${OUT_DIR}/sils-ss2-control.log" 2>/dev/null | tail -1 | cut -c1-120)"

echo
echo "  logs  ${OUT_DIR}/"
echo "  vECU  ${OUT_DIR}/vecu-wire.log   (its own stage-1 state analysis runs at the end)"

# Let the vECU driver finish so its analysis lands in the log rather than being killed
# mid-write by the cleanup trap.
wait "${VECU_PID}" 2>/dev/null || true

exit "${verdict}"
