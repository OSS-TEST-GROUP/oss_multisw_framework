#!/bin/bash

# Multi-architecture Docker image build script for OSS Multi-Middleware Framework.
#
# Builds runtime-mmf-{core,ext-ros2,ext-agl} for both linux/amd64 and
# linux/arm64 and pushes a multi-arch manifest list to the company
# registry as :latest.
#
# Implementation note — TARGETARCH naming mismatch
# ------------------------------------------------
# Dockerfile.rel selects the right runtime stage via
#   FROM runtime-mmf-core-${TARGETARCH} AS runtime-mmf-core
# but its named stages are runtime-mmf-core-x86_64 / runtime-mmf-core-aarch64
# (kernel arch). Docker auto-fills TARGETARCH per platform with `amd64` /
# `arm64` (Docker arch), which does NOT match the kernel-arch suffix.
#
# A single `docker buildx build --platform=linux/amd64,linux/arm64` cannot
# pass a per-platform --build-arg, so we build each platform separately
# (with explicit TARGETARCH override), push them as
# `:latest-amd64` / `:latest-arm64`, and then assemble the multi-arch
# manifest list at `:latest` using `docker buildx imagetools create`.
#
# Usage:
#   BUILD_TYPE=Release ./docker/build_multiarch.sh

set -e

REGISTRY="registry.obigo.com"
BUILD_TYPE="${BUILD_TYPE:-Release}"

# 레지스트리 로그인 헬퍼 (ossuser 단일 계정). REGISTRY_ADDRESS 를 REGISTRY 와 맞춰 source.
REGISTRY_ADDRESS="${REGISTRY}"
source "$(dirname "$0")/registry-auth.sh"

# ── 소스 스냅샷 메타 ────────────────────────────────────────────────────────
# 이미지를 :latest 와 함께 불변 스냅샷 태그(<branch>-<date>-<shortsha>)로도 push 하고,
# 소스 정보를 OCI 라벨로 baked 한다. 재현성 보장을 위해 커밋되지 않은(tracked) 변경이
# 있으면 push 를 차단한다 (untracked 빌드 산출물 output/ 등은 무시).
if ! git diff --quiet HEAD 2>/dev/null; then
  echo "ERROR: 커밋되지 않은 변경이 있습니다 — 커밋 후 다시 실행하세요 (스냅샷 태그/재현성 보장)." >&2
  git status --short --untracked-files=no >&2 || true
  exit 1
fi
GIT_REV="$(git rev-parse HEAD)"
GIT_BRANCH="$(git rev-parse --abbrev-ref HEAD | sed 's/[^A-Za-z0-9._-]/-/g')"
GIT_SHORT="$(git rev-parse --short HEAD)"

# 배포 정책: :latest + 스냅샷 태그 push 는 default 브랜치(develop)에서만 허용한다.
# feature/기타 브랜치에서 실수로 :latest 를 덮어쓰는 것을 방지. 정책상 default
# 브랜치 이외의 빌드는 여기서 중단한다 (필요 시 DEFAULT_BRANCH 로 정책 조정).
DEFAULT_BRANCH="${DEFAULT_BRANCH:-develop}"
if [ "${GIT_BRANCH}" != "${DEFAULT_BRANCH}" ]; then
  echo "ERROR: 현재 브랜치 '${GIT_BRANCH}' — :latest + 스냅샷 push 는 default 브랜치 '${DEFAULT_BRANCH}' 에서만 허용됩니다." >&2
  echo "       '${DEFAULT_BRANCH}' 로 머지 후 그 브랜치에서 빌드하세요." >&2
  exit 1
fi
BUILD_DATE_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
SNAP="${GIT_BRANCH}-$(date -u +%Y%m%d)-${GIT_SHORT}"
OCI_LABELS=(
  --label "org.opencontainers.image.revision=${GIT_REV}"
  --label "org.opencontainers.image.source=github.com/obigo-gov/oss_multisw_framework"
  --label "org.opencontainers.image.ref.name=${GIT_BRANCH}"
  --label "org.opencontainers.image.created=${BUILD_DATE_UTC}"
  --label "org.opencontainers.image.version=${SNAP}"
)

# Maps Docker auto-fill arch (amd64/arm64) → SUT kernel-arch stage suffix.
declare -A ARCH_MAP=( [amd64]=x86_64 [arm64]=aarch64 )
ARCHS=(amd64 arm64)
TARGETS=(core ext-ros2 ext-agl)

echo "=== OSS Multi-Middleware Framework Multi-arch Build ==="
echo "Architectures: ${ARCHS[*]}"
echo "Build Type:    ${BUILD_TYPE}"
echo "Registry:      ${REGISTRY}"
echo "Snapshot tag:  ${SNAP}"
echo ""

# Create buildx builder if not exists.
echo "Creating buildx builder..."
docker buildx create --name mmf-buildx --driver docker-container --use 2>/dev/null || \
  docker buildx use mmf-buildx

build_target() {
  local tgt="$1"
  local base="${REGISTRY}/oss/oss-mmf-${tgt}"
  local snap_tags=()

  for arch in "${ARCHS[@]}"; do
    local kernel_arch="${ARCH_MAP[$arch]}"
    local snap_arch="${base}:${SNAP}-${arch}"

    echo ""
    echo "=== Building runtime-mmf-${tgt} for linux/${arch} (TARGETARCH=${kernel_arch}) ==="
    docker buildx build \
      --platform="linux/${arch}" \
      --build-arg "BUILD_TYPE=${BUILD_TYPE}" \
      --build-arg "TARGETARCH=${kernel_arch}" \
      "${OCI_LABELS[@]}" \
      -f docker/Dockerfile.rel \
      --target "runtime-mmf-${tgt}" \
      --push \
      -t "${snap_arch}" \
      .
    snap_tags+=("${snap_arch}")
  done

  echo ""
  echo "=== Assembling multi-arch manifests (:latest + :${SNAP}) for ${tgt} ==="
  docker buildx imagetools create \
    -t "${base}:latest" \
    -t "${base}:${SNAP}" \
    "${snap_tags[@]}"
}

# ossuser 로그인 (pull·push 공용). 실패 시 중단.
registry_login || { echo "레지스트리 로그인 실패 — 중단"; exit 1; }

for tgt in "${TARGETS[@]}"; do
  build_target "$tgt"
done

echo ""
echo "=== Build Complete ==="
echo "Multi-arch manifests pushed (:latest + :${SNAP}):"
for tgt in "${TARGETS[@]}"; do
  echo "  - ${REGISTRY}/oss/oss-mmf-${tgt}:latest  +  :${SNAP}   (linux/amd64 + linux/arm64)"
done
