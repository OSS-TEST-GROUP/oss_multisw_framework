#!/usr/bin/env bash
#
# Run BOTH halves of the vehicle_interface seat's return path against a live Autoware
# partition: canonical EgoState -> everything autoware_simple_planning_simulator used to
# publish.
#
#     aw_vehicle_status_bridge    -> /vehicle/status/*      14 subscriptions (measured)
#     aw_localization_bridge      -> /localization/*        38 subscriptions (measured)
#
# Docs: docs/sils/sils-integration-topology.md §3.
# Pairs with tools/sils/ss2/run_control_partition_sils.sh, which is what turns the vendor
# simulator off in the first place. Running these two while the simulator is UP is not a
# test of anything: there would be two publishers per topic and Autoware would mix them.
#
# ★ TWO DOMAINS, TWO DDS VENDORS, ONE PROCESS EACH
#
# Each bridge is an rclcpp node on the Autoware side and a native Fast DDS participant on
# the canonical side, so it needs BOTH domain ids and they are NOT the same number:
#
#     ROS_DOMAIN_ID        Autoware's, Cyclone   (43 in the stage 3 rig)
#     VISS_DDS_DOMAIN_ID   the SiLS bus, Fast DDS (42)
#
# RMW_IMPLEMENTATION must be Cyclone to match the partitions. The adapter image carries
# rmw_cyclonedds_cpp for exactly this (docker/sils/Dockerfile.ss2-adapter).
#
# ★ WHAT FEEDS THEM
#
# Something must publish canonical EgoState on sils/ego/state. In order of preference:
#   1. a real plant adapter (MORAI / Web.Auto)          <- the destination
#   2. ss2_ego_bridge, with scenario_simulator_v2 up    <- available today
#   3. bin/sils_ego_source, the open-loop test fixture  <- --with-ego-source below
#
# Option 3 proves WIRING ONLY. It ramps vx from a standstill at the MAP ORIGIN, which is not
# on the lanelet map, so Autoware will localize nowhere useful and will not plan a route.
# "The subscriptions matched and the starvation messages stopped" is a real result; "Autoware
# drove" is not something this fixture can show.
#
# Usage:
#     tools/sils/ss2/run_vehicle_seat_bridges.sh [--with-ego-source] [--duration 60]
#
# Env:  ROS_DOMAIN_ID (43)  VISS_DDS_DOMAIN_ID (42)  SILS_DURABILITY (transient-local)
#       SILS_SS2_ADAPTER_IMAGE (sils-ss2-adapter:humble)

set -euo pipefail

ROS_DOMAIN="${ROS_DOMAIN_ID:-43}"
SILS_DOMAIN="${VISS_DDS_DOMAIN_ID:-42}"
DURABILITY="${SILS_DURABILITY:-transient-local}"
IMAGE="${SILS_SS2_ADAPTER_IMAGE:-sils-ss2-adapter:humble}"
WITH_SOURCE=false
DURATION=60
# Default start pose: a point ON the sample-map-planning lanelet map, taken from the pose
# that drove the partition on 08-10 (/workspace/log/pose.json "start"). At (0,0) the fixture
# sits at the map ORIGIN, which is on no lanelet, so Autoware localizes nowhere and never
# plans a route -- see evidence/vehicle-seat-no-simulator-2026-08-11.md §5.1. Override for a
# different map.
EGO_X0="${SILS_EGO_X0:-3638.69}"
EGO_Y0="${SILS_EGO_Y0:-73605.25}"
EGO_YAW0="${SILS_EGO_YAW0:--1.4441}"
# Cap the ramp. Uncapped, 0.5 m/s^2 reaches 150 m/s over a 300 s run and leaves the map.
EGO_VMAX="${SILS_EGO_VMAX:-8.0}"

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${here}/../../.." && pwd)"

die() { echo "run_vehicle_seat_bridges: $*" >&2; exit 1; }

while [ "${1:-}" != "" ]; do
    case "$1" in
        --with-ego-source) WITH_SOURCE=true ;;
        --duration)        DURATION="$2"; shift ;;
        --x0)              EGO_X0="$2"; shift ;;
        --y0)              EGO_Y0="$2"; shift ;;
        --yaw0)            EGO_YAW0="$2"; shift ;;
        --vmax)            EGO_VMAX="$2"; shift ;;
        --help|-h)         sed -n '2,45p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *)                 die "unknown option: $1" ;;
    esac
    shift
done

docker image inspect "$IMAGE" >/dev/null 2>&1 \
    || die "image $IMAGE not found. Build it: docker/sils/build-ss2-adapter.sh"

# Refuse to run against a partition that still has its own simulator. Both bridges would
# publish onto topics that already have a publisher, Autoware would receive an interleaving
# of two sources, and every number afterwards would be meaningless -- silently.
CONTROL="adsw-control-${ROS_DOMAIN}"
if docker ps --format '{{.Names}}' | grep -qx "$CONTROL"; then
    if docker exec "$CONTROL" bash -lc \
        'source /opt/ros/humble/setup.bash && ros2 node list 2>/dev/null' \
        | grep -q simple_planning_simulator; then
        die "simple_planning_simulator is STILL RUNNING in ${CONTROL}. These bridges would be
  a second publisher on every topic it owns and the run would measure an interleaving of
  two sources. Restart the partition with the simulator off first:
      tools/sils/ss2/run_control_partition_sils.sh --domain-id ${ROS_DOMAIN}"
    fi
else
    echo "warning: ${CONTROL} is not running; starting the bridges anyway (they will report" >&2
    echo "         no subscribers, which is the honest answer, not a crash)." >&2
fi

echo "==> vehicle_interface seat, return path"
echo "    ROS 2 domain      ${ROS_DOMAIN}  (Cyclone, Autoware)"
echo "    SiLS DDS domain   ${SILS_DOMAIN}  (Fast DDS, canonical)"
echo "    durability        ${DURABILITY}"
echo

for pair in "aw_vehicle_status_bridge:sils-aw-status" "aw_localization_bridge:sils-aw-localization"; do
    exe="${pair%%:*}"; name="${pair##*:}"
    docker rm -f "$name" >/dev/null 2>&1 || true
    docker run -d --name "$name" --rm --net=host --ipc=host \
        -e "ROS_DOMAIN_ID=${ROS_DOMAIN}" \
        -e "RMW_IMPLEMENTATION=rmw_cyclonedds_cpp" \
        -e "ROS_LOCALHOST_ONLY=0" \
        -e "VISS_DDS_DOMAIN_ID=${SILS_DOMAIN}" \
        -e "SILS_DURABILITY=${DURABILITY}" \
        "$IMAGE" \
        ros2 run ss2_ego_bridge "$exe" >/dev/null
    echo "    started ${name}  (${exe})"
done

if [ "$WITH_SOURCE" = true ]; then
    # Prefer the build tree over the bin/ copy: bin/ is a hand-staged snapshot and has gone
    # stale before. If both exist and differ, the build tree is the one that matches the
    # source you are reading.
    SRC="${REPO_ROOT}/output/x86_64/Debug/extension/ros2/bin/sils_ego_source"
    [ -x "$SRC" ] || SRC="${REPO_ROOT}/bin/sils_ego_source"
    [ -x "$SRC" ] || die "sils_ego_source not built. Build it:  ./build.sh --build-sils"
    LIBDIR="${REPO_ROOT}/output/x86_64/lib"
    mkdir -p "${REPO_ROOT}/runs"
    echo
    echo "    starting sils_ego_source (OPEN LOOP TEST FIXTURE; it ignores Autoware's"
    echo "      commands and drives straight, so this is not a closed loop)"
    echo "      binary  ${SRC#"${REPO_ROOT}/"}"
    echo "      pose    x0=${EGO_X0} y0=${EGO_Y0} yaw0=${EGO_YAW0} rad, vmax=${EGO_VMAX} m/s"
    VISS_DDS_DOMAIN_ID="${SILS_DOMAIN}" SILS_DURABILITY="${DURABILITY}" \
    LD_LIBRARY_PATH="${LIBDIR}:${LD_LIBRARY_PATH:-}" \
        "$SRC" --rate-ms 20 --count $(( DURATION * 50 )) \
               --x0 "${EGO_X0}" --y0 "${EGO_Y0}" --yaw0 "${EGO_YAW0}" --vmax "${EGO_VMAX}" \
        > "${REPO_ROOT}/runs/ego-source.jsonl" 2>&1 &
    echo "      pid $!  log runs/ego-source.jsonl"
fi

echo
echo "watch:   docker logs -f sils-aw-status"
echo "         docker logs -f sils-aw-localization"
echo "stop:    docker rm -f sils-aw-status sils-aw-localization"
