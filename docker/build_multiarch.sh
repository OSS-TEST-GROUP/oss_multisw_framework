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

# Maps Docker auto-fill arch (amd64/arm64) → SUT kernel-arch stage suffix.
declare -A ARCH_MAP=( [amd64]=x86_64 [arm64]=aarch64 )
ARCHS=(amd64 arm64)
TARGETS=(core ext-ros2 ext-agl)

echo "=== OSS Multi-Middleware Framework Multi-arch Build ==="
echo "Architectures: ${ARCHS[*]}"
echo "Build Type:    ${BUILD_TYPE}"
echo "Registry:      ${REGISTRY}"
echo ""

# Create buildx builder if not exists.
echo "Creating buildx builder..."
docker buildx create --name mmf-buildx --driver docker-container --use 2>/dev/null || \
  docker buildx use mmf-buildx

build_target() {
  local tgt="$1"
  local manifest_tag="${REGISTRY}/oss/oss-mmf-${tgt}:latest"
  local platform_tags=()

  for arch in "${ARCHS[@]}"; do
    local kernel_arch="${ARCH_MAP[$arch]}"
    local platform_tag="${manifest_tag}-${arch}"

    echo ""
    echo "=== Building runtime-mmf-${tgt} for linux/${arch} (TARGETARCH=${kernel_arch}) ==="
    docker buildx build \
      --platform="linux/${arch}" \
      --build-arg "BUILD_TYPE=${BUILD_TYPE}" \
      --build-arg "TARGETARCH=${kernel_arch}" \
      -f docker/Dockerfile.rel \
      --target "runtime-mmf-${tgt}" \
      --push \
      -t "${platform_tag}" \
      .
    platform_tags+=("${platform_tag}")
  done

  echo ""
  echo "=== Assembling multi-arch manifest ${manifest_tag} ==="
  docker buildx imagetools create \
    -t "${manifest_tag}" \
    "${platform_tags[@]}"
}

# ossuser 로그인 (pull·push 공용). 실패 시 중단.
registry_login || { echo "레지스트리 로그인 실패 — 중단"; exit 1; }

for tgt in "${TARGETS[@]}"; do
  build_target "$tgt"
done

echo ""
echo "=== Build Complete ==="
echo "Multi-arch manifests pushed:"
for tgt in "${TARGETS[@]}"; do
  echo "  - ${REGISTRY}/oss/oss-mmf-${tgt}:latest  (linux/amd64 + linux/arm64)"
done
