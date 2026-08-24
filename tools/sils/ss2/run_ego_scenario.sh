#!/usr/bin/env bash
#
# run_ego_scenario.sh -- B1-4: spawn a REAL EgoEntity and let concealer talk to our stub.
#
# WHY THIS IS THE DECISIVE TEST
#
# smoke_autoware_stub.sh proved the stub does what we designed it to do, but WE played
# concealer's part. The one unknown it cannot touch is whether the real concealer accepts
# the result -- and concealer only appears when an EgoEntity is constructed, which only the
# .xosc/.yaml path does. This script constructs one.
#
# WHAT B1-4 TURNED OUT TO BE (survey at pin 3370c817)
#
# The task was written as "find a TIER IV Scenario Format v2.0 -> .xosc conversion path".
# There is nothing to build: scenario_test_runner ALREADY converts, internally, on the way
# in (scenario_test_runner.py:45-49 -- .xosc passes through, anything else goes through
# openscenario_utility.conversion.convert). So B1-4 is an asset CHOICE, not a conversion job.
#
#   29 of upstream's 82 scenario yaml files declare isEgo: true
#   0  of the 32 scenarios upstream CI runs (config/workflow.txt) declare it
#
# So ego scenarios exist and are shipped, but NOTHING upstream exercises them in CI -- the
# same gap cpp_mock_scenarios has ("require EgoEntity and cannot be used for CI"). Expect
# rough edges; a failure here is as likely to be upstream's as ours.
#
# WHY THIS SCENARIO
#
# RoutingAction.AcquirePositionAction.yaml:
#   - exactly one entity, isEgo: true. No NPC, no sensors, no traffic signals to confuse a
#     failure with something unrelated to the handshake.
#   - LogicFile is $(find-pkg-share kashiwanoha_map)/map/lanelet2_map.osm -- resolved inside
#     the image at run time, so unlike openscenario_interpreter/example/lane_change.xosc
#     (filepath="./", no map there) there is no path to patch and nothing to mount.
#   - AcquirePositionAction is precisely what concealer's plan() turns into
#     /api/routing/set_route_points, i.e. the service our stub answers.
#   - it only succeeds if the ego MOVES, so the same asset keeps working as the stage-2
#     goal once B2-3 exists. Right now it will not move -- see the verdict section.
#
# WHY launch_autoware:=false STILL EXERCISES concealer
#
# It skips only the process spawn: ego_entity.cpp:72 makes concealer::ros2_launch()
# conditional and returns 0 otherwise. EgoEntity still derives from
# FieldOperatorApplication, whose constructor arms initialize_duration (default 30 s) and
# calls requestInitialPose("/api/localization/initialize", ...). It talks ADAPI to whoever
# answers. That is the stub.
#
# Usage:
#   tools/sils/ss2/run_ego_scenario.sh
#   tools/sils/ss2/run_ego_scenario.sh --scenario <in-image path> --timeout 60
#   tools/sils/ss2/run_ego_scenario.sh --keep         # leave both up to poke at
# Exit: 0 concealer reached `driving`, 1 it did not, 2 setup/usage.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

EXIT_OK=0
EXIT_FAIL=1
EXIT_USAGE=2

DOMAIN="${SILS_DOMAIN:-42}"
IMAGE="${SILS_SS2_IMAGE:-sils-ss2:humble}"
OUT_DIR="${REPO_ROOT}/runs/ego-scenario"
SCENARIO_REL="scenario/RoutingAction.AcquirePositionAction.yaml"
SCENARIO_ABS=""
# Must EXCEED concealer's own availability timeout, which is 180 s.
#
# concealer::Service::operator() spins on client->service_is_ready() and only throws
# `Service "..." is not ready after 180 seconds` once that deadline passes
# (external/concealer/include/concealer/service.hpp). At 90 s we were tearing the run down
# before concealer could say anything, and reading the resulting silence as "it never
# called us" -- when the truth may be "it was still waiting to be able to".
#
# Waiting the extra ~2 minutes buys the single most informative line in the whole exercise:
# concealer naming the service it could not reach.
TIMEOUT=200
KEEP=0
# The interpreter <-> simulator ZMQ port (traffic_simulator api.hpp reads it as `port`,
# default 5555). Exposed here only so a busy host can move it.
ZMQ_PORT="${SILS_SS2_ZMQ_PORT:-5555}"
STUB="sils-ego-stub"
COMPOSE="${REPO_ROOT}/docker/compose/docker-compose-sils-ss2.yaml"

die() { echo "run_ego_scenario: $*" >&2; exit "${EXIT_USAGE}"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --scenario) SCENARIO_ABS="$2"; shift 2 ;;
        --domain)   DOMAIN="$2"; shift 2 ;;
        --image)    IMAGE="$2"; shift 2 ;;
        --out)      OUT_DIR="$2"; shift 2 ;;
        --timeout)  TIMEOUT="$2"; shift 2 ;;
        --zmq-port) ZMQ_PORT="$2"; shift 2 ;;
        --keep)     KEEP=1; shift ;;
        -h|--help)  sed -n '2,52p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit "${EXIT_USAGE}" ;;
        *) die "unknown argument: $1" ;;
    esac
done

[[ "${DOMAIN}" =~ ^[0-9]+$ ]] || die "--domain must be a number"
command -v docker >/dev/null 2>&1 || die "docker is not on PATH"
docker image inspect "${IMAGE}" >/dev/null 2>&1 \
    || die "image '${IMAGE}' not found. Build it: docker/sils/build-ss2.sh"
mkdir -p "${OUT_DIR}" "${REPO_ROOT}/runs/ss2-scenario" 2>/dev/null \
    || die "cannot create ${OUT_DIR} (a container may own it: sudo chown -R \$USER ${REPO_ROOT}/runs)"

IN_CONTAINER_ENV='source /opt/ros/${ROS_DISTRO}/setup.bash >/dev/null 2>&1
source /home/ubuntu/Desktop/scenario_simulator_ws/install/local_setup.bash >/dev/null 2>&1
export PYTHONPATH=/stub:${PYTHONPATH:-}'

PIN="$(sed -n 's/^commit=//p' "${REPO_ROOT}/docker/sils/ss2-upstream.pin" 2>/dev/null | head -1)"

echo "run_ego_scenario: B1-4 -- real EgoEntity, concealer <-> autoware_stub"
echo "  image    ${IMAGE}   (ss2 pin ${PIN:-unrecorded})"
echo "  domain   ${DOMAIN}"
echo "  out      ${OUT_DIR}"

# --- resolve the scenario inside the image -------------------------------------
#
# Asked of the image rather than hardcoded: the install prefix is a property of how the
# workspace was built, and a wrong guess fails as "scenario not found" much later, after
# the simulator has already started.
if [[ -z "${SCENARIO_ABS}" ]]; then
    share="$(docker run --rm --entrypoint bash "${IMAGE}" -lc "${IN_CONTAINER_ENV}
ros2 pkg prefix --share scenario_test_runner 2>/dev/null" 2>/dev/null | tr -d '\r' | tail -1)"
    [[ -n "${share}" ]] || die "could not resolve scenario_test_runner's share dir in ${IMAGE}"
    SCENARIO_ABS="${share}/${SCENARIO_REL}"
fi
docker run --rm --entrypoint bash "${IMAGE}" -lc "test -f '${SCENARIO_ABS}'" 2>/dev/null \
    || die "scenario not found in the image: ${SCENARIO_ABS}"
echo "  scenario ${SCENARIO_ABS}"
echo

# --- 0. clear OUR leftovers, then check nobody else answers for Autoware --------
#
# Our own containers come down FIRST. They are named by us and step 1 kills them anyway;
# checking occupancy before removing them just reports our own previous run as an intruder,
# which is a false alarm that teaches the reader to ignore the check.
docker rm -f "${STUB}" >/dev/null 2>&1 || true
docker compose -f "${COMPOSE}" rm -sf ss2-scenario >/dev/null 2>&1 || true
# ...and by NAME as well. `docker compose rm` only removes what compose still recognises as
# its own; a container left by a SIGKILLed run, or started from another directory (different
# project name), survives it. That leftover is not merely untidy -- see the port check below.
docker rm -f sils-ss2-scenario >/dev/null 2>&1 || true

# The ZMQ port must be FREE, and this is the hazard the name-based removal above exists for.
#
# simple_sensor_simulator binds ${ZMQ_PORT} (zmq_multi_server) and traffic_simulator, inside
# the interpreter, connects to it. Every container here runs network_mode: host, so that
# port is the HOST's. If a previous run's simulator still holds it, the new interpreter
# connects to the OLD simulator: the socket is ESTABLISHED, `ss` looks perfectly healthy,
# and the two are talking about different maps and different entities. It hangs, silently,
# with no error on either side -- which is indistinguishable from the DDS problems we were
# chasing, and cost us several runs.
if (exec 3<>/dev/tcp/127.0.0.1/"${ZMQ_PORT}") 2>/dev/null; then
    exec 3<&- 2>/dev/null || true
    echo "  ZMQ port ${ZMQ_PORT} is still held after cleanup:"
    docker ps --format '    {{.Names}}  ({{.Image}})' | sed 's/^/  /'
    die "something is already listening on ${ZMQ_PORT}. A new simulator cannot bind it, and the
  interpreter would connect to whatever is there instead -- a hang that looks like a DDS
  fault. Stop it, or pass --zmq-port with a free one."
fi

# Now the real question. The verdict is read off the stub's log, but concealer picks its
# peer off the BUS. Domain 42 is where the rest of this project lives -- a real Autoware, an
# integration run, someone else's session. Any of them would answer concealer's services
# first and we would report a PASS earned by something else. The smoke test dodged this with
# a dedicated domain; here the domain has to stay the project's, so check instead.
#
# The sleep is not padding. `ros2 service list` prints whatever discovery has completed by
# the time it runs, so a fresh short-lived container reports a PARTIAL graph -- one service
# out of seven, which reads like a mystery peer rather than a slow snapshot.
occupied="$(docker run --rm --network host --ipc host \
    -e ROS_DOMAIN_ID="${DOMAIN}" -e ROS_LOCALHOST_ONLY=0 \
    -e RMW_IMPLEMENTATION="${SILS_RMW:-rmw_fastrtps_cpp}" \
    --entrypoint bash "${IMAGE}" -lc "${IN_CONTAINER_ENV}
sleep 5
ros2 service list 2>/dev/null" 2>/dev/null \
    | grep -E '^/api/(localization/initialize|routing/set_route_points|external/set/engage)$' || true)"
if [[ -n "${occupied}" ]]; then
    echo "  ADAPI services are ALREADY served on domain ${DOMAIN}:"
    printf '    %s\n' ${occupied}
    echo
    echo "  containers currently up:"
    docker ps --format '    {{.Names}}  ({{.Image}})' | sed 's/^/  /'
    die "something else would answer concealer instead of our stub, and the run would look
  like a PASS we did not earn. Our own leftovers were already removed above, so this is
  something else -- stop it, or pass a free --domain."
fi

cleanup() {
    if [[ ${KEEP} -eq 1 ]]; then
        echo
        echo "  --keep: ${STUB} and sils-ss2-scenario left running."
        echo "    docker rm -f ${STUB}; docker compose -f ${COMPOSE} down"
    else
        docker rm -f "${STUB}" >/dev/null 2>&1 || true
        docker compose -f "${COMPOSE}" rm -sf ss2-scenario >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

# --- 1. stub FIRST -------------------------------------------------------------
#
# Order matters and is not cosmetic. concealer arms initialize_duration the moment
# EgoEntity is constructed and starts calling services immediately; if the stub is not
# there yet it burns that budget on an absent service. The stub's TRANSIENT_LOCAL state
# topics also mean a concealer that subscribes late still gets the current value -- but
# only if the writer already exists. Stage 1 lost a run to exactly this ordering.
echo "[1/3] autoware_stub on domain ${DOMAIN}"
docker run -d --network host --ipc host --name "${STUB}" \
    -e ROS_DOMAIN_ID="${DOMAIN}" -e ROS_LOCALHOST_ONLY=0 \
    -e RMW_IMPLEMENTATION="${SILS_RMW:-rmw_fastrtps_cpp}" -e PYTHONUNBUFFERED=1 \
    -v "${REPO_ROOT}/adapters/autoware-stub:/stub:ro" \
    --entrypoint bash "${IMAGE}" -lc "${IN_CONTAINER_ENV}
exec python3 -m autoware_stub.stub_node" >/dev/null || die "could not start the stub"

sleep 3
docker ps --format '{{.Names}}' | grep -qx "${STUB}" || {
    echo "      the stub exited immediately:"
    docker logs "${STUB}" 2>&1 | sed 's/^/        /'
    exit "${EXIT_FAIL}"
}
echo "      up"

# --- 2. the scenario -----------------------------------------------------------
echo
echo "[2/3] scenario (launch_autoware:=false, up to ${TIMEOUT}s)"
docker compose -f "${COMPOSE}" rm -sf ss2-scenario >/dev/null 2>&1 || true
SILS_DOMAIN="${DOMAIN}" \
SILS_SS2_SCENARIO="${SCENARIO_ABS}" \
SILS_SS2_LAUNCH_AUTOWARE=false \
SILS_SS2_GLOBAL_TIMEOUT="${TIMEOUT}" \
SILS_SS2_ZMQ_PORT="${ZMQ_PORT}" \
    docker compose -f "${COMPOSE}" up -d ss2-scenario >/dev/null 2>&1 \
    || die "compose could not start ss2-scenario"

# Poll for the milestone rather than sleeping the full timeout.
#
# The marker is the stub's own `derived autoware state -> driving`, NOT a particular
# service's log line: concealer reaches driving through requestEngage, but
# enable_autoware_control gets there too, and matching one service name would report FAIL
# on a run that actually succeeded through the other.
#
# Progress is echoed as the stub's current state rather than bare dots, because "waiting"
# with nothing else on screen cannot distinguish "concealer has not started yet" (SS_v2 is
# still loading a 39 MB pointcloud and building the routing graph, which is slow on a first
# run) from "concealer is stuck". Those need opposite responses -- wait longer vs stop and
# look. concealer itself throws AutowareError once initialize_duration (default 30 s from
# EgoEntity construction) elapses, so a long quiet stretch with no error is startup.
echo "      waiting for concealer. Two of its own deadlines matter here, and both are"
echo "      longer than they look: service availability 180s (Service::operator()) and"
echo "      initialize_duration 30s from EgoEntity construction. Quiet != stuck."
deadline=$((SECONDS + TIMEOUT))
reached=0
last_seen=""
while [[ ${SECONDS} -lt ${deadline} ]]; do
    if docker logs "${STUB}" 2>&1 | grep -q "derived autoware state -> driving"; then
        reached=1; break
    fi
    now_state="$(docker logs "${STUB}" 2>&1 \
                 | grep -oE 'derived autoware state -> [a-z_]+' | tail -1)"
    now_state="${now_state##* }"
    # Print on change, and otherwise every 20 s. Change-only looked like the script had
    # died: on the first hung run it printed one line at t+12s and nothing for the next 78,
    # which is indistinguishable from a broken loop right when the reader most wants to
    # know something is still being watched.
    if [[ "${now_state}" != "${last_seen}" ]]; then
        echo "        t+${SECONDS}s  stub state: ${now_state:-<none>}"
        last_seen="${now_state}"
        last_beat=${SECONDS}
    elif [[ $((SECONDS - ${last_beat:-0})) -ge 20 ]]; then
        echo "        t+${SECONDS}s  still ${now_state:-<none>} (scenario container alive)"
        last_beat=${SECONDS}
    fi
    if ! docker ps --format '{{.Names}}' | grep -qx sils-ss2-scenario; then
        echo "        t+${SECONDS}s  the scenario container exited"
        break
    fi
    sleep 2
done

docker logs "${STUB}" > "${OUT_DIR}/stub.log" 2>&1 || true
docker logs sils-ss2-scenario > "${OUT_DIR}/scenario.log" 2>&1 || true

# --- 3. verdict ----------------------------------------------------------------
echo
echo "[3/3] what happened"
echo
echo "  --- stub: which of our services concealer actually called ---"
# `first contact` must be in this filter. The heading promises "which of our services
# concealer actually called", and a call that moves no state -- change_to_stop from the
# all-UNKNOWN boot, or any of the three no-op services -- has no `->` in its line. Filtering
# on `->` alone hid exactly the evidence the heading claims to show, and we had to go read
# `docker logs` by hand to find it.
if grep -qE ': .* -> ' "${OUT_DIR}/stub.log"; then
    grep -E ': .* -> |derived autoware state|first contact' "${OUT_DIR}/stub.log" \
        | sed 's/^/  /'
else
    echo "  (none -- concealer never called us)"
fi
echo
echo "  --- scenario: errors ---"
# Two filters, both earned the hard way on the first real run.
#
# ROSBAG2_TRANSPORT is excluded: record_option makes rosbag2 try to record every topic on
# the domain, and anything whose type is not in this image produces a WARN that embeds the
# ENTIRE ament prefix path -- thousands of characters, containing the word "error" often
# enough to match, and long enough to push the actual failure off the screen. It says
# nothing about our run; it says rosbridge shares domain 42.
#
# cut: one runaway line must not cost us the other eleven.
if grep -aiE "autowareerror|\[ERROR\]|exception|terminate|what\(\)" "${OUT_DIR}/scenario.log" 2>/dev/null \
        | grep -av "ROSBAG2_TRANSPORT" | cut -c1-220 | head -12 | grep -q .; then
    grep -aiE "autowareerror|\[ERROR\]|exception|terminate|what\(\)" "${OUT_DIR}/scenario.log" \
        | grep -av "ROSBAG2_TRANSPORT" | cut -c1-220 | head -12 | sed 's/^/  /'
else
    echo "  (none -- so it did not throw; it simply never got far enough)"
fi

# The tail is where a launch that gave up says why, and it is the one part that is useful
# even when nothing matches an error pattern.
echo
echo "  --- scenario: last 20 lines ---"
tail -20 "${OUT_DIR}/scenario.log" 2>/dev/null | cut -c1-200 | sed 's/^/  /'

echo
echo "  full logs: ${OUT_DIR}/stub.log  ${OUT_DIR}/scenario.log"
echo

if [[ ${reached} -eq 1 ]]; then
    echo "  VERDICT: PASS -- the REAL concealer walked our stub to \`driving\`."
    echo "  B1-2b's remaining unknown is closed: an EgoEntity can exist without Autoware,"
    echo "  and a scenario asset that spawns one is available with no upstream patch."
    echo
    echo "  NOW EXPECTED TO FAIL LATER, and that is not this test's business: nothing"
    echo "  publishes /control/command/control_cmd yet, so the ego sits still and the"
    echo "  scenario times out without reaching its goal. ss2_control_bridge (B2-3) fixes"
    echo "  the sitting-still part -- but NOT the goal: the vECU's reference behaviour is a"
    echo "  LONGITUDINAL P-controller with steering pinned to 0, so the ego will drive"
    echo "  straight and still miss an AcquirePosition goal that needs lane following."
    echo "  Closing the loop and completing this scenario are two different milestones."
    exit "${EXIT_OK}"
fi

echo "  VERDICT: FAIL -- concealer did not reach \`driving\`."
last="$(grep -oE ': [a-z_]+ -> [a-z_]+' "${OUT_DIR}/stub.log" 2>/dev/null | tail -1)"
# `first contact` is the discriminator that matters, and it is NOT the same question as
# "did a transition happen". concealer's very first act on EgoEntity construction is
# requestChangeToStop, and from our all-UNKNOWN boot state that call moves no derived
# state -- so a constructed ego and a never-constructed one look identical unless the stub
# reports the call itself. It does now.
# `grep -c` prints 0 AND exits 1 when there is no match, so `|| echo 0` appends a SECOND
# line and the variable becomes "0\n0" -- which breaks the -gt comparison and made this
# report "the EgoEntity WAS constructed" on a run where concealer never called us at all.
# Strip to digits instead of trusting the exit status.
contacted="$(grep -c "first contact on this service" "${OUT_DIR}/stub.log" 2>/dev/null | head -1)"
contacted="${contacted//[^0-9]/}"
contacted="${contacted:-0}"
if [[ -n "${last}" ]]; then
    echo "  It DID reach us -- the last transition was '${last#: }', so the stub is being"
    echo "  talked to and the problem is the step after it, not discovery."
elif [[ "${contacted}" -gt 0 ]]; then
    echo "  concealer DID call us (see 'first contact' above) but never moved us off"
    echo "  \`initializing\`. So the EgoEntity WAS constructed and discovery works; what did"
    echo "  not happen is initialize() -- the interpreter is stuck before or inside Init."
else
    echo "  No service of ours was called at all -- not even change_to_stop, which is the"
    echo "  first thing concealer's constructor does."
    echo
    echo "  That is TWO different situations and this line cannot tell them apart:"
    echo "    (a) the EgoEntity was never constructed, or"
    echo "    (b) it was, and concealer is spinning in Service::operator() because"
    echo "        client->service_is_ready() is false -- i.e. it cannot SEE our services."
    echo "  In case (b) concealer stays silent for a full 180s before throwing."
    echo
    echo "  Settle it from inside the scenario container while it is still up (--keep):"
    echo "    docker exec sils-ss2-scenario bash -lc 'source /opt/ros/\$ROS_DISTRO/setup.bash;"
    echo "      ros2 service list | grep \"^/api/\"'"
    echo "  Services listed -> discovery is fine, so (a). Nothing listed -> (b), and the"
    echo "  problem is DDS reachability between the two containers on domain ${DOMAIN}."
fi
echo
echo "  Remember upstream CI runs ZERO isEgo:true scenarios -- a failure here may well be"
echo "  upstream's rather than the stub's. Compare against smoke_autoware_stub.sh, which"
echo "  exercises the same services with a known-good caller."
exit "${EXIT_FAIL}"
