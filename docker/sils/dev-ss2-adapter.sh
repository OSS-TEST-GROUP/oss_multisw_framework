#!/usr/bin/env bash
#
# Interactive build shell for the SS_v2 adapter (adapters/ss2).
#
# Why this exists: `docker build` shows exactly ONE compile error per round trip, discards
# the failed layer, and the error is buried in a log that terminals like to truncate.
# Iterating on C++ that way costs a full rebuild per fix. This drops you into the
# toolchain image with the adapter source BIND-MOUNTED, so you edit on the host, rebuild
# in the shell, and see the complete error list each time.
#
#   docker/sils/dev-ss2-adapter.sh
#   # then, inside:
#   build          # colcon build, full error output
#   run            # run the bridge
#
# Once it compiles clean, bake the image normally:
#   docker/sils/build-ss2-adapter.sh
#
# Docs: adapters/ss2/README.md

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

SS2_IMAGE="${SS2_IMAGE:-sils-ss2-dev:humble}"
TOOLCHAIN_IMAGE="${TOOLCHAIN_IMAGE:-sils-ss2-toolchain:humble}"
FASTDDSGEN_REF="${FASTDDSGEN_REF:-v2.5.3}"
WS=/home/ubuntu/Desktop/scenario_simulator_ws

die() { echo "ERROR: $*" >&2; exit 1; }

command -v docker >/dev/null || die "docker not found"
docker image inspect "${SS2_IMAGE}" >/dev/null 2>&1 \
    || die "base image '${SS2_IMAGE}' not found -- run docker/sils/build-ss2.sh first"

export DOCKER_BUILDKIT=1

# Cheap: every layer is shared with the full adapter build, so after one successful
# adapter build attempt this is a cache hit.
echo "==> toolchain image: ${TOOLCHAIN_IMAGE}"
docker build \
    --target toolchain \
    --build-arg "SS2_IMAGE=${SS2_IMAGE}" \
    --build-arg "FASTDDSGEN_REF=${FASTDDSGEN_REF}" \
    --build-arg ROS_DISTRO=humble \
    -t "${TOOLCHAIN_IMAGE}" \
    -f "${REPO_ROOT}/docker/sils/Dockerfile.ss2-adapter" \
    "${REPO_ROOT}"

# Convenience commands, written into the container's bashrc so the shell is usable
# immediately rather than after copy-pasting three sourcing lines.
init_script=$(cat <<'INIT'
source /opt/ros/humble/setup.bash
source /home/ubuntu/Desktop/scenario_simulator_ws/install/local_setup.bash 2>/dev/null || true
cd /home/ubuntu/Desktop/scenario_simulator_ws

build() {
    colcon build --packages-select ss2_ego_bridge \
        --cmake-args -DCMAKE_BUILD_TYPE=Release \
        --event-handlers console_direct+ "$@"
}
run() {
    source /home/ubuntu/Desktop/scenario_simulator_ws/install/local_setup.bash
    ros2 run ss2_ego_bridge ss2_ego_bridge "$@"
}

cat <<'BANNER'
--------------------------------------------------------------------
 SS_v2 adapter dev shell.  Source is BIND-MOUNTED from the host:
   host  adapters/ss2/  <->  ws/src/ss2_ego_bridge/
 Edit on the host, then in here:

   build      colcon build with full (untruncated) compiler output
   run        run the bridge

 console_direct+ is on, so you get the WHOLE error list, not the
 one-error-per-round-trip that `docker build` gives you.
--------------------------------------------------------------------
BANNER
INIT
)

echo "${init_script}" > "${REPO_ROOT}/.ss2-devshell-init.sh"
trap 'rm -f "${REPO_ROOT}/.ss2-devshell-init.sh"' EXIT

echo "==> shell (source bind-mounted from ${REPO_ROOT}/adapters/ss2)"
docker run --rm -it \
    --network host --ipc host \
    -v "${REPO_ROOT}/adapters/ss2:${WS}/src/ss2_ego_bridge" \
    -v "${REPO_ROOT}/.ss2-devshell-init.sh:/etc/ss2-devshell-init.sh:ro" \
    -e "ROS_DOMAIN_ID=${SILS_DOMAIN:-0}" \
    -e "VISS_DDS_DOMAIN_ID=${SILS_DOMAIN:-0}" \
    -e "RMW_IMPLEMENTATION=${SILS_RMW:-rmw_fastrtps_cpp}" \
    "${TOOLCHAIN_IMAGE}" \
    bash --rcfile /etc/ss2-devshell-init.sh
