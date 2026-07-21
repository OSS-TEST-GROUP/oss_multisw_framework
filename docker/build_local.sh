#!/bin/bash
# Local x86_64 runtime image builder.
#
# Builds the runtime-mmf-{core,ext-ros2} stages of docker/Dockerfile.rel
# from the host's output/x86_64/${BUILD_TYPE} tree (produced earlier by
# ./build.sh) and tags the result :local for use with the
# docker/compose/docker-compose-integration.yaml profile.
#
# Sibling of build_multiarch.sh, which targets multi-arch + push to the
# company registry. This one is single-arch (linux/amd64), --load (no
# push), and pinned to the kernel-arch stage suffix the SUT uses
# (`x86_64`, not Docker's `amd64`).
#
# Usage (from anywhere):
#   ./docker/build_local.sh
#   BUILD_TYPE=Release ./docker/build_local.sh
#   ./docker/build_local.sh --core-only
#   ./docker/build_local.sh --ext-ros2-only

set -e

cd "$(dirname "$0")/.."   # → SUT root

BUILD_TYPE="${BUILD_TYPE:-Debug}"
PLATFORM="linux/amd64"
# Dockerfile.rel uses ${TARGETARCH} as a stage suffix
# (`runtime-mmf-core-${TARGETARCH}`) but its stages are named with kernel
# arch (`x86_64`/`aarch64`), not Docker's auto-fill (`amd64`/`arm64`).
# Override explicitly so the local stage resolves.
TARGETARCH="x86_64"

BUILD_CORE=1
BUILD_EXT_ROS2=1
BUILD_SILS_VECU=0

usage() {
  cat >&2 <<'USAGE'
Usage: docker/build_local.sh [options]

Build the SUT's runtime docker images for local x86_64 verification.
Tags: oss-mmf-core:local, oss-mmf-ext-ros2:local, oss-mmf-sils-vecu:local

Options:
  --core-only         Build only oss-mmf-core:local
  --ext-ros2-only     Build only oss-mmf-ext-ros2:local
  --sils-vecu-only    Build only oss-mmf-sils-vecu:local (SiLS vECU 이미지)
  -h, --help          Show this message

Environment:
  BUILD_TYPE        Debug | Release (default: Debug)

Pre-requisite:
  ./build.sh must have populated output/x86_64/${BUILD_TYPE}/{core,extension/ros2}/{bin,lib}
  sils-vecu 이미지는 ./build.sh --build-sils 로 sils_vecu_* 바이너리가
  output/x86_64/${BUILD_TYPE}/core/bin 에 있어야 한다.
USAGE
}

while [ $# -gt 0 ]; do
  case "$1" in
    --core-only)      BUILD_EXT_ROS2=0; BUILD_SILS_VECU=0; shift;;
    --ext-ros2-only)  BUILD_CORE=0; BUILD_SILS_VECU=0; shift;;
    --sils-vecu-only) BUILD_CORE=0; BUILD_EXT_ROS2=0; BUILD_SILS_VECU=1; shift;;
    -h|--help)        usage; exit 0;;
    *) echo "ERR: unknown option: $1" >&2; usage; exit 2;;
  esac
done

case "$BUILD_TYPE" in
  Debug|Release) ;;
  *) echo "ERR: BUILD_TYPE must be Debug or Release (got '$BUILD_TYPE')" >&2; exit 2;;
esac

if [ ! -d "output/x86_64/${BUILD_TYPE}/core/bin" ]; then
  echo "ERR: output/x86_64/${BUILD_TYPE}/core/bin missing — run ./build.sh first" >&2
  exit 1
fi

build_target() {
  local tgt="$1"
  local tag="oss-mmf-${tgt}:local"
  echo ""
  echo "── building runtime-mmf-${tgt} → ${tag} (BUILD_TYPE=${BUILD_TYPE}) ──"
  docker buildx build \
    --platform="${PLATFORM}" \
    --build-arg "BUILD_TYPE=${BUILD_TYPE}" \
    --build-arg "TARGETARCH=${TARGETARCH}" \
    -f docker/Dockerfile.rel \
    --target "runtime-mmf-${tgt}" \
    --load \
    -t "${tag}" \
    .
}

[ "$BUILD_CORE" -eq 1 ]      && build_target core
[ "$BUILD_EXT_ROS2" -eq 1 ]  && build_target ext-ros2
[ "$BUILD_SILS_VECU" -eq 1 ] && build_target sils-vecu

echo ""
echo "✓ Done. Local images:"
docker image ls --format "{{.Repository}}:{{.Tag}}\t{{.Size}}\t{{.CreatedSince}}" \
  | grep "^oss-mmf-.*:local" \
  | sed 's/^/  /' \
  || echo "  (none)"

echo ""
echo "Next:"
echo "  docker compose -f docker/compose/docker-compose-integration.yaml up -d"
