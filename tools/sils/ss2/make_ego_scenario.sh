#!/usr/bin/env bash
#
# make_ego_scenario.sh -- produce a .xosc that spawns a REAL EgoEntity, for the B1-2 test.
#
# WHY THIS EXISTS
#
# B1-2's one remaining question is whether an EgoEntity can exist without Autoware, because
# EgoEntity is what brings the vehicle model + AutowareUniverse command adapter into
# simple_sensor_simulator (docs/sils/evidence/ss2-concealer-survey-2026-08-07.md).
#
# cpp_mock_scenarios cannot answer it. Upstream leaves the only EgoEntity mock out of the
# build -- "require EgoEntity and cannot be used for CI" -- so every scenario that IS built
# spawns a VehicleEntity, including the ones that name it "ego". Only the .xosc path
# (openscenario_interpreter) spawns a real one.
#
# And upstream's own example is not runnable as shipped: openscenario_interpreter/example/
# contains lane_change.xosc and NOTHING ELSE, while the scenario says
#     <LogicFile filepath="./"/>
# i.e. "the map is next to me". There is no map there. The map lives in a separate package
# (kashiwanoha_map). Patching that one path is the whole job.
#
# WHAT IT DOES: copy upstream's lane_change.xosc, rewrite LogicFile to the in-image map
# directory, write it to scenarios/ss2/. Nothing else is touched -- in particular the
# scenario's own content is left exactly as upstream wrote it, so a failure is upstream's
# behaviour and not our edit.
#
# A lane change is also the right shape for later: it is the manoeuvre that settled V1
# (vehicle vs map frame), so the same asset re-confirms V1 on a real EgoEntity.
#
# Usage:
#   tools/sils/ss2/make_ego_scenario.sh [--out <path>] [--image <img>] [--force]
# Then:
#   SILS_SS2_SCENARIO=/scenarios/ego-lane-change.xosc \
#     docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-scenario
#
# Exit: 0 written, 2 usage/IO error.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

EXIT_OK=0
EXIT_USAGE=2

UPSTREAM_REL="openscenario/openscenario_interpreter/example/lane_change.xosc"
OUT="${REPO_ROOT}/scenarios/ss2/ego-lane-change.xosc"
IMAGE="${SILS_SS2_IMAGE:-sils-ss2:humble}"
FORCE=0

die() { echo "make_ego_scenario: $*" >&2; exit "${EXIT_USAGE}"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --out)   OUT="$2"; shift 2 ;;
        --image) IMAGE="$2"; shift 2 ;;
        --force) FORCE=1; shift ;;
        -h|--help) sed -n '2,32p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit "${EXIT_USAGE}" ;;
        *) die "unknown argument: $1" ;;
    esac
done

if [[ -e "${OUT}" && "${FORCE}" -eq 0 ]]; then
    die "${OUT} already exists. Pass --force to overwrite (it is a generated asset)."
fi

# --- 1. the source scenario ---------------------------------------------------
#
# Prefer the host clone (.ss2-src, created by docker/sils/build-ss2.sh) over the image:
# it is the same tree the image was built from, needs no container, and is present even
# when the image is not.
SRC=""
if [[ -f "${REPO_ROOT}/.ss2-src/${UPSTREAM_REL}" ]]; then
    SRC="${REPO_ROOT}/.ss2-src/${UPSTREAM_REL}"
    echo "make_ego_scenario: source = ${SRC}"
else
    command -v docker >/dev/null 2>&1 \
        || die "no .ss2-src checkout and no docker. Run docker/sils/build-ss2.sh first."
    echo "make_ego_scenario: no .ss2-src; extracting from ${IMAGE}"
    TMP_SRC="$(mktemp)"
    trap 'rm -f "${TMP_SRC}"' EXIT
    docker run --rm --entrypoint cat "${IMAGE}" \
        "/home/ubuntu/Desktop/scenario_simulator_ws/src/${UPSTREAM_REL}" > "${TMP_SRC}" 2>/dev/null \
        || die "cannot read ${UPSTREAM_REL} from ${IMAGE}. Is the image built, and is the
  workspace path still /home/ubuntu/Desktop/scenario_simulator_ws? (upstream may have moved it)"
    [[ -s "${TMP_SRC}" ]] || die "extracted an empty file from ${IMAGE}"
    SRC="${TMP_SRC}"
fi

grep -q '<LogicFile' "${SRC}" \
    || die "no <LogicFile> in ${SRC} -- upstream changed the scenario; re-check by hand."

# --- 2. where the map lives INSIDE the container -------------------------------
#
# Resolved with `ros2 pkg prefix` rather than hardcoded: the workspace layout is upstream's
# and has moved before. The path is baked into the .xosc, so it must be the CONTAINER path,
# not a host one.
MAP_DIR=""
if command -v docker >/dev/null 2>&1; then
    MAP_DIR="$(docker run --rm --entrypoint bash "${IMAGE}" -lc \
        'source /opt/ros/${ROS_DISTRO}/setup.bash >/dev/null 2>&1;
         source /home/ubuntu/Desktop/scenario_simulator_ws/install/local_setup.bash >/dev/null 2>&1;
         printf "%s/share/kashiwanoha_map/map" "$(ros2 pkg prefix kashiwanoha_map)"' \
        2>/dev/null | tr -d '\r' | tail -1)"
fi
if [[ -z "${MAP_DIR}" || "${MAP_DIR}" != /* ]]; then
    MAP_DIR="/home/ubuntu/Desktop/scenario_simulator_ws/install/kashiwanoha_map/share/kashiwanoha_map/map"
    echo "make_ego_scenario: WARNING could not resolve kashiwanoha_map via ros2 pkg prefix;"
    echo "                   falling back to ${MAP_DIR}"
    echo "                   If the scenario fails to load a map, check this path first."
else
    echo "make_ego_scenario: map dir  = ${MAP_DIR}  (resolved in ${IMAGE})"
fi

# --- 3. rewrite ONLY the map path ----------------------------------------------
mkdir -p "$(dirname "${OUT}")" || die "cannot create $(dirname "${OUT}")"

python3 - "${SRC}" "${OUT}" "${MAP_DIR}" <<'PY' || die "rewrite failed"
import re, sys
src, out, map_dir = sys.argv[1], sys.argv[2], sys.argv[3]
text = open(src, encoding="utf-8").read()

# Replace the filepath attribute of <LogicFile> only. A blunt whole-file substitution would
# also hit <SceneGraphFile>, which is a different asset and legitimately absent here.
new, n = re.subn(r'(<LogicFile\s+filepath=")[^"]*(")',
                 lambda m: m.group(1) + map_dir + "/" + m.group(2), text, count=1)
if n != 1:
    sys.exit(f"expected exactly 1 <LogicFile filepath=...>, patched {n}")

# NO "--" ANYWHERE IN THIS BANNER. XML forbids a double hyphen inside a comment
# (XML 1.0 §2.5), so the house style used everywhere else in this repo produces a
# file that is not well-formed. openscenario_preprocessor validates before it does
# anything else, and scenario_test_runner SWALLOWS that failure -- the run just
# exits after ~6 s with no error. Cost us a day. Keep the assertion below.
banner = (f"<!-- GENERATED by tools/sils/ss2/make_ego_scenario.sh. Do not edit by hand.\n"
          f"     Source: upstream {sys.argv[1]}\n"
          f"     Only <LogicFile filepath> was changed (upstream ships \"./\" with no map\n"
          f"     beside it). Everything else is upstream's, so a failure here is upstream\n"
          f"     behaviour rather than our edit.\n"
          f"     Purpose: B1-2, can an EgoEntity exist without Autoware?\n"
          f"     See docs/sils/evidence/ss2-concealer-survey-2026-08-07.md -->\n")
if "--" in banner[4:-4]:
    sys.exit("banner contains '--', which is illegal inside an XML comment")
# After the XML declaration if there is one, else at the top.
m = re.match(r'^\s*<\?xml[^>]*\?>\s*\n', new)
new = (new[:m.end()] + banner + new[m.end():]) if m else (banner + new)

# Parse what we are about to write. The consumer's validator runs in a container and
# reports only a line/column, so catching it here is the difference between a clear
# message and an afternoon of bisecting a scenario.
import xml.etree.ElementTree as ET
try:
    ET.fromstring(new)
except ET.ParseError as e:
    sys.exit(f"generated scenario is not well-formed XML: {e}")

open(out, "w", encoding="utf-8").write(new)
PY

echo
echo "make_ego_scenario: wrote ${OUT}"
grep -n '<LogicFile' "${OUT}" | sed 's/^/  /'
echo
echo "Run it:"
echo "  export SILS_DOMAIN=42"
echo "  export SILS_SS2_SCENARIO=/scenarios/$(basename "${OUT}")"
echo "  docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-scenario"
echo
echo "launch_autoware defaults to false in that service -- that IS the experiment."
echo "What to watch for:"
echo "  simulation advances, entities publish  -> EgoEntity works without Autoware (B1-2 done)"
echo "  repeated waiting-for-autoware-state    -> blocked as feared; stub needed"
echo "  immediate exit                         -> a different problem (map, vehicle_model)"
exit "${EXIT_OK}"
