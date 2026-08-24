#!/usr/bin/env bash
#
# smoke_autoware_stub.sh -- run adapters/autoware-stub for real, without a simulator.
#
# WHY
#
# The stub's unit tests cover state.py, which has no ROS in it (12 passed). Everything
# between that state machine and concealer is untested: do Autoware's message packages even
# exist in this image, are the constants the stub resolves BY NAME still there, does the QoS
# match, do the services answer. This script closes that gap the cheapest way available --
# no scenario, no Autoware, no GPU.
#
# It is a COMPONENT test, not stage 2. Proving concealer accepts the stub needs an EgoEntity,
# which needs an isEgo=true scenario asset we do not have yet (B1-4). What this settles is
# the other half: that the stub does what we designed it to do. If it fails, B1-4 and B2-3
# would both be wasted effort until it is fixed -- which is why it goes first.
#
# NO GPU REQUIRED. The GPU dependency in this project belongs to the real Autoware
# partitions (oss-adsw-*, stage 3), which perceive and render. The stub publishes 3 topics
# and answers 7 services; the SS_v2 image is borrowed purely for rclpy and the message
# definitions, and the simulator itself is never started.
#
# WHAT IT DOES
#   1. pre-flight: image present, message packages + named constants present
#   2. pre-flight: the domain is EMPTY -- otherwise we would be reading someone else's
#      /api/routing/state and calling the result ours
#   3. start the stub in one container, run the probe in another (a real DDS crossing,
#      which is what concealer will do)
#   4. walk initialize -> set_route -> engage, checking the wire after each
#
# Usage:
#   tools/sils/ss2/smoke_autoware_stub.sh
#   tools/sils/ss2/smoke_autoware_stub.sh --domain 91 --out runs/stub-smoke
#   tools/sils/ss2/smoke_autoware_stub.sh --check-only     # pre-flight, start nothing
#   tools/sils/ss2/smoke_autoware_stub.sh --keep           # leave the stub running to poke
# Exit: 0 PASS, 1 FAIL (a real result), 2 setup/usage (not a result).

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

EXIT_OK=0
EXIT_FAIL=1
EXIT_USAGE=2

# A DEDICATED domain, not the usual 42. This test asserts on values read off the bus, so a
# second publisher of /api/routing/state -- a real Autoware, a previous stub, the simulator --
# would silently corrupt the result. 42 is where all of those live.
DOMAIN="${SILS_STUB_DOMAIN:-91}"
IMAGE="${SILS_SS2_IMAGE:-sils-ss2:humble}"
OUT_DIR="${REPO_ROOT}/runs/stub-smoke"
STEP_TIMEOUT="5"
CHECK_ONLY=0
KEEP=0
CONTAINER="sils-stub-smoke"

die() { echo "smoke_autoware_stub: $*" >&2; exit "${EXIT_USAGE}"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --domain)     DOMAIN="$2"; shift 2 ;;
        --image)      IMAGE="$2"; shift 2 ;;
        --out)        OUT_DIR="$2"; shift 2 ;;
        --timeout)    STEP_TIMEOUT="$2"; shift 2 ;;
        --check-only) CHECK_ONLY=1; shift ;;
        --keep)       KEEP=1; shift ;;
        -h|--help)    sed -n '2,45p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit "${EXIT_USAGE}" ;;
        *) die "unknown argument: $1" ;;
    esac
done

[[ "${DOMAIN}" =~ ^[0-9]+$ ]] || die "--domain must be a number"
command -v docker >/dev/null 2>&1 || die "docker is not on PATH"
docker image inspect "${IMAGE}" >/dev/null 2>&1 \
    || die "image '${IMAGE}' not found locally. Build it: docker/sils/build-ss2.sh
  (it is never pulled -- these images exist in no registry)"

STUB_DIR="${REPO_ROOT}/adapters/autoware-stub"
[[ -f "${STUB_DIR}/autoware_stub/stub_node.py" ]] || die "stub sources not at ${STUB_DIR}"

# Output dir. docker has a habit of creating bind-mount targets as root, so a plain mkdir
# failure here is usually ownership rather than a missing parent -- name the owner instead
# of leaving the user to guess. Same trap run_vecu_wire.sh hit.
if ! mkdir -p "${OUT_DIR}" 2>/dev/null; then
    anc="${OUT_DIR}"
    while [[ ! -e "${anc}" && "${anc}" != "/" ]]; do anc="$(dirname "${anc}")"; done
    die "cannot create ${OUT_DIR}
  first existing ancestor: ${anc}  (owner: $(stat -c '%U:%G' "${anc}" 2>/dev/null \
       || stat -f '%Su:%Sg' "${anc}" 2>/dev/null || echo unknown))
  if that is root, a container created it. Fix: sudo chown -R \$USER ${REPO_ROOT}/runs"
fi

PIN="$(sed -n 's/^commit=//p' "${REPO_ROOT}/docker/sils/ss2-upstream.pin" 2>/dev/null | head -1)"
PIN="${PIN:-unrecorded}"

echo "smoke_autoware_stub: autoware_stub component test (no simulator, no Autoware, no GPU)"
echo "  image     ${IMAGE}"
echo "  ss2 pin   ${PIN}"
echo "  domain    ${DOMAIN}   (dedicated -- NOT 42, so nothing else can answer for us)"
echo "  out       ${OUT_DIR}"
echo

# Sourcing both: the message packages may sit in the ROS install or in the SS_v2 workspace
# depending on how they were pulled in, and we do not want the answer to depend on that.
IN_CONTAINER_ENV='source /opt/ros/${ROS_DISTRO}/setup.bash >/dev/null 2>&1
source /home/ubuntu/Desktop/scenario_simulator_ws/install/local_setup.bash >/dev/null 2>&1
export PYTHONPATH=/stub:/tools:${PYTHONPATH:-}'

run_in_image() {  # <name-suffix> <command...>
    docker run --rm --network host --ipc host \
        --name "${CONTAINER}-$1" \
        -e ROS_DOMAIN_ID="${DOMAIN}" \
        -e ROS_LOCALHOST_ONLY=0 \
        -e RMW_IMPLEMENTATION="${SILS_RMW:-rmw_fastrtps_cpp}" \
        -v "${STUB_DIR}:/stub:ro" \
        -v "${SCRIPT_DIR}:/tools:ro" \
        -v "${OUT_DIR}:/out" \
        --entrypoint bash "${IMAGE}" -lc "${IN_CONTAINER_ENV}
$2"
}

# --- 1. messages ---------------------------------------------------------------
echo "[1/4] message packages and constants"
run_in_image preflight 'python3 /tools/stub_smoke_probe.py --check-messages' \
    2>&1 | tee "${OUT_DIR}/preflight.txt" | sed 's/^/      /'
pf=${PIPESTATUS[0]}
if [[ ${pf} -ne 0 ]]; then
    echo
    echo "  STOP: the stub cannot start against this image's messages. Nothing was launched."
    echo "  This is the failure the stub's _const() was written to surface, and it is a"
    echo "  finding about the IMAGE, not about the stub's logic."
    exit "${EXIT_FAIL}"
fi

# --- 2. domain is clear --------------------------------------------------------
echo
echo "[2/4] domain ${DOMAIN} is unoccupied"
# Our own leftover comes down first: it is named by us and step 3 kills it anyway, so
# checking before removing it would report our own previous run as an intruder.
docker rm -f "${CONTAINER}-stub" >/dev/null 2>&1 || true
# The sleep is not padding -- `ros2 topic list` prints whatever discovery finished by the
# time it runs, and a fresh container reports a partial graph that reads like a mystery peer.
existing="$(run_in_image scan 'sleep 5; ros2 topic list 2>/dev/null' 2>/dev/null \
            | grep -E '^/api/(routing/state|operation_mode/state|localization/initialization_state)$' || true)"
if [[ -n "${existing}" ]]; then
    echo "      OCCUPIED -- these are already published on domain ${DOMAIN}:"
    printf '        %s\n' ${existing}
    die "another publisher owns the topics this test asserts on; its values would be read as
  ours. Our own leftover was already removed above, so this is something else -- stop it,
  or pass a free --domain."
fi
echo "      clear"

if [[ ${CHECK_ONLY} -eq 1 ]]; then
    echo
    echo "  --check-only: pre-flight passed, nothing started."
    exit "${EXIT_OK}"
fi

# --- 3. start the stub ---------------------------------------------------------
echo
echo "[3/4] starting the stub"

cleanup() {
    if [[ ${KEEP} -eq 1 ]]; then
        echo
        echo "  --keep: stub still running as ${CONTAINER}-stub."
        echo "    docker logs -f ${CONTAINER}-stub"
        echo "    docker rm -f ${CONTAINER}-stub      # when done"
    else
        docker rm -f "${CONTAINER}-stub" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

docker run -d --network host --ipc host \
    --name "${CONTAINER}-stub" \
    -e ROS_DOMAIN_ID="${DOMAIN}" \
    -e ROS_LOCALHOST_ONLY=0 \
    -e RMW_IMPLEMENTATION="${SILS_RMW:-rmw_fastrtps_cpp}" \
    -e PYTHONUNBUFFERED=1 \
    -v "${STUB_DIR}:/stub:ro" \
    --entrypoint bash "${IMAGE}" -lc "${IN_CONTAINER_ENV}
exec python3 -m autoware_stub.stub_node" >/dev/null \
    || die "could not start the stub container"

# Running it as a module off PYTHONPATH rather than `ros2 run` on purpose: colcon-building
# an ament_python package into this image just to smoke-test it would add a build step whose
# failures are unrelated to what is being measured.

sleep 3
if ! docker ps --format '{{.Names}}' | grep -qx "${CONTAINER}-stub"; then
    echo "      the stub exited immediately. Its output:"
    docker logs "${CONTAINER}-stub" 2>&1 | sed 's/^/        /'
    docker logs "${CONTAINER}-stub" > "${OUT_DIR}/stub.log" 2>&1 || true
    exit "${EXIT_FAIL}"
fi
echo "      up (container ${CONTAINER}-stub)"

# --- 4. drive it ---------------------------------------------------------------
echo
echo "[4/4] walking concealer's sequence"
echo
run_in_image probe "python3 /tools/stub_smoke_probe.py --domain ${DOMAIN} \
    --timeout ${STEP_TIMEOUT} --json /out/stub-smoke.json" 2>&1 \
    | tee "${OUT_DIR}/probe.txt"
verdict=${PIPESTATUS[0]}

docker logs "${CONTAINER}-stub" > "${OUT_DIR}/stub.log" 2>&1 || true
echo
echo "  stub log   ${OUT_DIR}/stub.log"
echo "  probe log  ${OUT_DIR}/probe.txt"
[[ -f "${OUT_DIR}/stub-smoke.json" ]] && echo "  json       ${OUT_DIR}/stub-smoke.json"

# The stub's own view, next to the probe's. They are independent observations of the same
# run -- the stub logs what it believes it published, the probe reports what actually
# arrived. Printing both means a divergence is visible rather than something to go looking
# for, and a divergence here would mean the wire, not the state machine, is the problem.
echo
echo "  --- what the stub itself logged ---"
grep -E 'derived autoware state|->' "${OUT_DIR}/stub.log" 2>/dev/null | sed 's/^/  /' \
    || echo "  (no transition lines)"

exit "${verdict}"
