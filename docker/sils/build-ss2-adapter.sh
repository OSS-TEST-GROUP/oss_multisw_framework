#!/usr/bin/env bash
#
# Build the loop-B adapter image (work item B2-1).
#
# Requires the SS_v2 image first (docker/sils/build-ss2.sh) -- the adapter needs
# traffic_simulator_msgs from that workspace.
#
# Usage:
#   docker/sils/build-ss2-adapter.sh
#   FASTDDSGEN_REF=v3.3.0 docker/sils/build-ss2-adapter.sh     # pin once a combo works
#   SS2_IMAGE=sils-ss2:humble ADAPTER_IMAGE=sils-ss2-adapter:humble docker/sils/build-ss2-adapter.sh
#
# Docs: adapters/ss2/README.md · docs/sils/sils-ss2-bringup.md

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# The DEV image, not the runtime one -- upstream's runtime stage strips CMake configs and
# headers, so traffic_simulator_msgs cannot be found there. build-ss2.sh tags both.
SS2_IMAGE="${SS2_IMAGE:-sils-ss2-dev:humble}"
ADAPTER_IMAGE="${ADAPTER_IMAGE:-sils-ss2-adapter:humble}"
# Generator line is pinned by the image's FAST CDR version, not just Fast DDS.
# Measured in sils-ss2-dev:humble (2026-08-07):
#     ros-humble-fastcdr  1.0.29     -> Fast CDR 1.0.x, no include/fastcdr/cdr/
#     ros-humble-fastrtps 2.6.11     -> Fast DDS 2.6
# v3.x generators emit `#include <fastcdr/cdr/...>` and fail here; v2.x is the match.
FASTDDSGEN_REF="${FASTDDSGEN_REF:-v2.5.3}"

die() { echo "ERROR: $*" >&2; exit 1; }

command -v docker >/dev/null || die "docker not found"
docker info >/dev/null 2>&1 || die "docker daemon not reachable"

# Fail here rather than 10 minutes into a gradle build.
docker image inspect "${SS2_IMAGE}" >/dev/null 2>&1 || die "\
base image '${SS2_IMAGE}' not found -- run docker/sils/build-ss2.sh first.

  If you built before 2026-08-07, that script only produced the runtime image. Re-run it;
  it now also tags the development stage, which is the one that can COMPILE against
  traffic_simulator_msgs (runtime has its CMake configs and headers stripped). The re-run
  is cheap -- every layer is already cached."

export DOCKER_BUILDKIT=1

echo "==> building ${ADAPTER_IMAGE} (base ${SS2_IMAGE}, Fast-DDS-Gen ${FASTDDSGEN_REF})"
echo "    note: the fastddsgen gradle build is slow on first run and cached after."
docker build \
    --build-arg "SS2_IMAGE=${SS2_IMAGE}" \
    --build-arg "FASTDDSGEN_REF=${FASTDDSGEN_REF}" \
    --build-arg ROS_DISTRO=humble \
    -t "${ADAPTER_IMAGE}" \
    -f "${REPO_ROOT}/docker/sils/Dockerfile.ss2-adapter" \
    "${REPO_ROOT}"

cat <<EOF

==> done: ${ADAPTER_IMAGE}

next (stage 1 wiring -- three terminals or -d):
    docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-mock
    docker compose -f docker/compose/docker-compose-sils-ss2.yaml up ss2-adapter
    # then point a vECU at the same domain with SILS_CHANNELMAP=config/sils/channelmap/ego-min.yaml

first thing to confirm: the adapter logs "published=N" with N rising, and the vECU's
trace shows state_received. See adapters/ss2/README.md "동작 확인".
EOF
