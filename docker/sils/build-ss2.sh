#!/usr/bin/env bash
#
# Build the loop-B simulator image: upstream scenario_simulator_v2, then the thin
# SiLS layer on top (docker/sils/Dockerfile.ss2).
#
# Two-step on purpose. Upstream owns the hard part (vcs import + rosdep + colcon)
# and that recipe changes; we build it verbatim from their tree rather than copying
# it here, so we inherit their fixes instead of drifting from them.
#
#   step 1  clone tier4/scenario_simulator_v2 at SS2_REF, docker build --target runtime
#   step 2  docker build our layer FROM that image
#
# The resolved upstream commit SHA is written to the pin file so a run is
# reproducible even when SS2_REF is a moving branch. Record it in the bring-up
# report -- an interop measurement without the simulator SHA is not a measurement.
#
# Usage:
#   docker/sils/build-ss2.sh                        # SS2_REF=master (records SHA)
#   SS2_REF=<sha-or-tag> docker/sils/build-ss2.sh   # pinned (recommended once known)
#   SS2_WORKDIR=/scratch/ss2 docker/sils/build-ss2.sh
#   docker/sils/build-ss2.sh --upstream-only        # stop after step 1
#
# Requires: Linux x86_64, docker with BuildKit, git, ~30GB disk, no GPU.
# Docs: docs/sils/sils-ss2-bringup.md

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

SS2_REPO="${SS2_REPO:-https://github.com/tier4/scenario_simulator_v2.git}"
# PINNED, not `master`.
#
# This is the commit the 1계단 (stage-1 wiring) verdict was measured on -- V2 interop,
# V1 vehicle frame, V3 the 50 ms fixed step (docs/sils/evidence/ss2-stage1-verdict-2026-08-07.md).
# A moving branch would let those findings silently stop applying: the next build could
# change the twist frame or the step size and nothing here would notice.
#
# To move it deliberately: build with the new ref, RE-RUN tools/sils/ss2/run_vecu_wire.sh,
# confirm V1/V3 still read the same, then update this line and the pin file together.
#   SS2_REF=master docker/sils/build-ss2.sh      # track upstream tip (re-verify after!)
SS2_REF="${SS2_REF:-3370c817efc5458fbe69cc3c02a9bc8956cb8a81}"
SS2_WORKDIR="${SS2_WORKDIR:-${REPO_ROOT}/.ss2-src}"
UPSTREAM_IMAGE="${UPSTREAM_IMAGE:-sils-ss2-upstream:humble}"
UPSTREAM_DEV_IMAGE="${UPSTREAM_DEV_IMAGE:-sils-ss2-dev:humble}"
SILS_IMAGE="${SILS_IMAGE:-sils-ss2:humble}"
PIN_FILE="${PIN_FILE:-${REPO_ROOT}/docker/sils/ss2-upstream.pin}"

upstream_only=0
[[ "${1:-}" == "--upstream-only" ]] && upstream_only=1

die() { echo "ERROR: $*" >&2; exit 1; }
note() { echo "==> $*"; }

# --- preflight ---------------------------------------------------------------
# Fail loudly here rather than 40 minutes into a colcon build.

command -v docker >/dev/null || die "docker not found"
command -v git >/dev/null || die "git not found"
docker info >/dev/null 2>&1 || die "docker daemon not reachable"

host_arch="$(uname -m)"
if [[ "${host_arch}" != "x86_64" ]]; then
    cat >&2 <<EOF
ERROR: host arch is ${host_arch}, expected x86_64.

  scenario_simulator_v2 and the Autoware message packages target amd64. Building
  under qemu emulation is slow and, more importantly, makes the B1-3 interop
  measurement meaningless -- the numbers would not describe the deployment target.

  ARM64 is a separate axis in this project (L4 target parity, see
  docs/sils/sils-target-parity.md). Do not conflate the two here.

  Override only if you know why:  SILS_ALLOW_ARCH=1 $0
EOF
    [[ "${SILS_ALLOW_ARCH:-0}" == "1" ]] || exit 1
    echo "WARNING: proceeding on ${host_arch} because SILS_ALLOW_ARCH=1" >&2
fi

# Upstream's Dockerfile uses `RUN --mount=type=cache`, which is BuildKit-only.
# Without it the build fails on an opaque parse error.
export DOCKER_BUILDKIT=1

# --- step 1: upstream --------------------------------------------------------

note "upstream source -> ${SS2_WORKDIR} (ref ${SS2_REF})"
if [[ -d "${SS2_WORKDIR}/.git" ]]; then
    git -C "${SS2_WORKDIR}" fetch --depth 1 origin "${SS2_REF}"
    git -C "${SS2_WORKDIR}" checkout -q FETCH_HEAD
else
    rm -rf "${SS2_WORKDIR}"
    git clone --depth 1 --branch "${SS2_REF}" "${SS2_REPO}" "${SS2_WORKDIR}" 2>/dev/null \
        || {
            # --branch does not accept a raw SHA; fall back to fetch-by-sha.
            rm -rf "${SS2_WORKDIR}"
            git init -q "${SS2_WORKDIR}"
            git -C "${SS2_WORKDIR}" remote add origin "${SS2_REPO}"
            git -C "${SS2_WORKDIR}" fetch --depth 1 origin "${SS2_REF}"
            git -C "${SS2_WORKDIR}" checkout -q FETCH_HEAD
        }
fi

ss2_sha="$(git -C "${SS2_WORKDIR}" rev-parse HEAD)"
note "upstream commit ${ss2_sha}"

[[ -f "${SS2_WORKDIR}/Dockerfile" ]] \
    || die "no Dockerfile at ${SS2_WORKDIR} -- upstream layout changed, re-check the ref"
[[ -f "${SS2_WORKDIR}/dependency_humble.repos" ]] \
    || die "no dependency_humble.repos -- upstream may have moved off Humble; see docs/sils/sils-ss2-bringup.md"

note "building upstream runtime stage -> ${UPSTREAM_IMAGE}  (long: colcon from source)"
docker build \
    --target runtime \
    --build-arg ROS_DISTRO=humble \
    -t "${UPSTREAM_IMAGE}" \
    -f "${SS2_WORKDIR}/Dockerfile" \
    "${SS2_WORKDIR}"

# Also tag the DEVELOPMENT stage. Upstream's runtime stage deliberately strips the build
# interface to slim the image:
#     find install -name cmake -type d -exec rm -rf {} + ; rm -rf install/include
# so `find_package(traffic_simulator_msgs)` cannot work there -- runtime runs, it does not
# compile. The adapter (adapters/ss2) needs those headers and CMake configs, so it builds
# against this image instead.
#
# Nearly free: `development` is the base `runtime` was built from, so every layer is
# already in the cache from the build above.
note "tagging upstream development stage -> ${UPSTREAM_DEV_IMAGE}  (needed to BUILD the adapter)"
docker build \
    --target development \
    --build-arg ROS_DISTRO=humble \
    -t "${UPSTREAM_DEV_IMAGE}" \
    -f "${SS2_WORKDIR}/Dockerfile" \
    "${SS2_WORKDIR}"

# Record the pin only after a successful build -- a pin file pointing at a ref
# that does not build is worse than none.
cat > "${PIN_FILE}" <<EOF
# Resolved upstream scenario_simulator_v2 commit for the loop-B image.
# Written by docker/sils/build-ss2.sh on a successful build. Commit this file:
# it is what makes a B1-3 interop measurement reproducible.
repo=${SS2_REPO}
ref=${SS2_REF}
commit=${ss2_sha}
image=${UPSTREAM_IMAGE}
dev_image=${UPSTREAM_DEV_IMAGE}
EOF
note "pin written -> ${PIN_FILE}"

if [[ "${upstream_only}" == "1" ]]; then
    note "done (--upstream-only)"
    exit 0
fi

# --- step 2: SiLS layer ------------------------------------------------------
# Context is the repo root: the layer copies our Fast DDS profile and probe script.

note "building SiLS layer -> ${SILS_IMAGE}"
docker build \
    --build-arg "SS2_UPSTREAM_IMAGE=${UPSTREAM_IMAGE}" \
    --build-arg ROS_DISTRO=humble \
    -t "${SILS_IMAGE}" \
    -f "${REPO_ROOT}/docker/sils/Dockerfile.ss2" \
    "${REPO_ROOT}"

cat <<EOF

==> done
    upstream (runtime) : ${UPSTREAM_IMAGE}   (commit ${ss2_sha})   -- runs the simulator
    upstream (dev)     : ${UPSTREAM_DEV_IMAGE}                     -- BUILDS the adapter
    sils               : ${SILS_IMAGE}

next:
    docker compose -f docker/compose/docker-compose-sils-ss2.yaml run --rm ss2-probe
        -> writes the B1-3 interop report to runs/ss2-probe/

see docs/sils/sils-ss2-bringup.md
EOF
