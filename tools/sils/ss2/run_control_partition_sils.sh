#!/usr/bin/env bash
#
# Restart ONLY the Autoware Control partition with autoware_simple_planning_simulator OFF,
# so the SiLS bridges occupy the vehicle_interface seat for real.
#
# Docs: docs/sils/sils-integration-topology.md §3, docs/sils/sils-stage3-autoware-partition.md
#
# ★ WHY A SEPARATE SCRIPT
#
# The upstream runner (oss_adsw/partition/run_partitions.sh) starts the control container
# with /autoware/start_script/adsw-control.sh, which is baked into the vendor IMAGE and runs
#
#     ros2 launch obigo_launch sample_adsw_control_run.launch.xml ...
#
# That launch file hardcodes launch_simulator_dummy_vehicle as a `value=`, which no command
# line can override (see the header of sils_adsw_control_run.launch.xml). Patching either
# the image or the upstream repo would put our experiment inside someone else's tree, so
# instead we replace the CONTAINER's command with our own launch file, mounted in.
#
# ★ THE DUPLICATION, AND WHY IT IS DELIBERATE
#
# The docker flags below are copied from run_partitions.sh (COMMON_ARGS + DDS_ARGS). That is
# duplication and it CAN drift -- if the partitions behave oddly, diff this block against
# the upstream script first. The alternative was to source that script, but it starts all
# three partitions and traps EXIT to stop them, so sourcing it does not compose.
#
# Perception and Decision are NOT touched: leave run_partitions.sh running, it only tails
# logs after start-up.
#
# ★ USAGE
#
#     tools/sils/ss2/run_control_partition_sils.sh                 # simulator OFF (the point)
#     tools/sils/ss2/run_control_partition_sils.sh --simulator-on   # vendor behaviour, for A/B
#
# Then supply what the simulator was publishing, or Autoware blocks:
#
#     aw_vehicle_status_bridge    /vehicle/status/*        (25 subscriptions)
#     aw_localization_bridge      /localization/*          (38 subscriptions)
#
# Env:  ROS_DOMAIN_ID (default 43)   ADSW_REPO   ADSW_MAP_PATH   ADSW_WORKSPACE

set -euo pipefail

REPO="${ADSW_REPO:-ghcr.io/oss-test-group/autoware-partition}"
ROS_DOMAIN="${ROS_DOMAIN_ID:-43}"
MAP_PATH="${ADSW_MAP_PATH:-$HOME/autoware_map/sample-map-planning}"
WORKSPACE_ROOT="${ADSW_WORKSPACE:-$HOME/oss/oss_adsw}"
HOST_SOURCE_DIR="${HOST_SOURCE_DIR:-$HOME/source}"
SIMULATOR=false

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCH_SRC="${here}/sils_adsw_control_run.launch.xml"

die() { echo "run_control_partition_sils: $*" >&2; exit 1; }

while [ "${1:-}" != "" ]; do
    case "$1" in
        --simulator-on)  SIMULATOR=true ;;
        --domain-id)     ROS_DOMAIN="$2"; shift ;;
        --map-path)      MAP_PATH="$2"; shift ;;
        --help|-h)       sed -n '2,45p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *)               die "unknown option: $1" ;;
    esac
    shift
done

[ -d "$MAP_PATH" ]         || die "map path does not exist: $MAP_PATH"
[ -d "$WORKSPACE_ROOT" ]   || die "workspace does not exist: $WORKSPACE_ROOT (set ADSW_WORKSPACE)"
[ -f "$LAUNCH_SRC" ]       || die "launch file missing: $LAUNCH_SRC"

NAME="adsw-control-${ROS_DOMAIN}"

# The launch file has to be reachable INSIDE the container. /workspace is a bind mount of
# WORKSPACE_ROOT, so copying it there is enough -- no extra -v, and it survives the restart.
mkdir -p "${WORKSPACE_ROOT}/log"
cp "$LAUNCH_SRC" "${WORKSPACE_ROOT}/sils_adsw_control_run.launch.xml"

echo "==> replacing ${NAME}"
echo "    image      ${REPO}:adsw-control"
echo "    domain     ${ROS_DOMAIN}"
echo "    map        ${MAP_PATH}"
echo "    simulator  $([ "$SIMULATOR" = true ] && echo 'ON (vendor behaviour)' || echo 'OFF  <-- SiLS bridges must feed Autoware')"

docker rm -f "$NAME" >/dev/null 2>&1 || true

# Copied from run_partitions.sh -- see the duplication note above.
docker run -d \
    --name "$NAME" \
    --rm --net=host --pid=host --ipc=host \
    -e "LOCAL_UID=$(id -u)" \
    -e "LOCAL_GID=$(id -g)" \
    -e "LOCAL_USER=$(id -un)" \
    -e "LOCAL_GROUP=$(id -gn)" \
    -e "FASTDDS_BUILTIN_TRANSPORTS=UDPv4" \
    -e "ROS_DOMAIN_ID=${ROS_DOMAIN}" \
    -e "RMW_IMPLEMENTATION=rmw_cyclonedds_cpp" \
    -e "ROS_LOCALHOST_ONLY=0" \
    ${CYCLONEDDS_URI:+-e "CYCLONEDDS_URI=${CYCLONEDDS_URI}"} \
    -v /etc/localtime:/etc/localtime:ro \
    -v "${WORKSPACE_ROOT}:/workspace" \
    -v "${MAP_PATH}:/autoware_map:ro" \
    -v "${HOST_SOURCE_DIR}/fastdds:/fastdds:rw" \
    -v "${WORKSPACE_ROOT}:/exec" \
    -v "${HOST_SOURCE_DIR}/ros2:/ros2" \
    "${REPO}:adsw-control" \
    bash -lc "mkdir -p /workspace/log && \
        ros2 launch /workspace/sils_adsw_control_run.launch.xml \
            map_path:=/autoware_map \
            vehicle_model:=sample_vehicle \
            sensor_model:=sample_sensor_kit \
            launch_simulator_dummy_vehicle:=${SIMULATOR} \
        > /workspace/log/control_log.txt 2>&1" >/dev/null

echo "    started. log: ${WORKSPACE_ROOT}/log/control_log.txt"
echo
echo "confirm the simulator really is $([ "$SIMULATOR" = true ] && echo on || echo off):"
echo "    docker exec ${NAME} bash -lc 'source /opt/ros/humble/setup.bash &&"
echo "        ros2 node list | grep simple_planning_simulator'"
echo "  -- empty output means it is off and Autoware is now waiting on the SiLS bridges."
