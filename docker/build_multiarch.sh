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
#
# ⚠️ 선행 조건 — 이 스크립트는 **컴파일하지 않는다.** `output/<arch>/${BUILD_TYPE}/…` 에 이미
#    있는 바이너리를 이미지에 복사할 뿐이다. 그래서 먼저 소스를 빌드해야 한다:
#
#      ./build.sh --release                                  # x86_64 (core+extension)
#      ./build.sh --release --aarch64 --set-extension-platform=ROS2
#      ./build.sh --release --aarch64 --set-extension-platform=AGL
#
#    ❗ `build.sh` 의 기본 모드는 **Debug** 다(`--release` 를 빼면 Debug 로 나간다). 반면 이
#      스크립트와 Dockerfile 의 기본값은 **Release** 다. 두 기본값이 어긋나므로, `--release`
#      없이 빌드한 뒤 이미지를 만들면 **방금 빌드한 코드가 아니라 예전 Release 산출물이 조용히
#      들어간다.** 아래 가드가 그 사고를 막는다(존재 + 신선도 검사).

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
# 선택적 스냅샷 prefix — 같은 소스(같은 커밋)로 여러 변종 이미지를 만들 때
# 스냅샷 태그가 서로 덮어쓰는 것을 막는다. 예) BUILD_TYPE=Debug 빌드:
#   SNAP_PREFIX=debug BUILD_TYPE=Debug ./docker/build_multiarch.sh
#   → debug-develop-20260801-abc1234
SNAP_PREFIX="$(printf '%s' "${SNAP_PREFIX:-}" | sed 's/[^A-Za-z0-9._-]/-/g')"
SNAP="${SNAP_PREFIX:+${SNAP_PREFIX}-}${GIT_BRANCH}-$(date -u +%Y%m%d)-${GIT_SHORT}"
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

# ── 산출물 가드 (BUILD_TYPE ↔ output/ 정합) ─────────────────────────────────
# 실패 모드가 "조용히 잘못된 이미지" 라서 빌드 전에 막는다.
#   (a) 존재: Dockerfile 이 복사할 디렉토리가 없으면 build 가 애매한 에러로 죽거나,
#             더 나쁘게는 다른 타겟의 stale 산출물만으로 이미지가 만들어질 수 있다.
#   (b) 신선도: 산출물이 현재 커밋보다 오래되면 이미지에 옛 코드가 들어간다. 실측 사례 —
#             x86_64/Release 가 3주 전에 멈춘 상태로 남아 있었다(2026-08-11 확인).
# 검사만 하고 컴파일은 하지 않는다(이 스크립트의 책임 분리). 의도적으로 옛 산출물을 쓰려면
# ALLOW_STALE_OUTPUT=1 로 신선도 검사만 경고로 낮춘다.
declare -A TARGET_SUBPATH=( [core]=core [ext-ros2]=extension/ros2 [ext-agl]=extension/agl )
# 신선도 기준은 **HEAD 가 아니라 그 타겟의 소스를 마지막으로 바꾼 커밋**이다. HEAD 로 비교하면
# 문서·스크립트만 고친 커밋에도 정상 산출물이 stale 로 잡혀(오탐) 가드가 무력화된다.
declare -A TARGET_SRC=(
  [core]="service/core idl CMakeLists.txt"
  [ext-ros2]="service/extension/ros2 idl CMakeLists.txt"
  [ext-agl]="service/extension/agl idl CMakeLists.txt"
)
guard_errors=0

for arch in "${ARCHS[@]}"; do
  kernel_arch="${ARCH_MAP[$arch]}"
  for tgt in "${TARGETS[@]}"; do
    bin_dir="output/${kernel_arch}/${BUILD_TYPE}/${TARGET_SUBPATH[$tgt]}/bin"

    if [ ! -d "${bin_dir}" ]; then
      echo "ERROR: 산출물 없음 — ${bin_dir}" >&2
      echo "       BUILD_TYPE=${BUILD_TYPE} 로 먼저 소스를 빌드하세요:" >&2
      case "${tgt}" in
        core)     echo "         ./build.sh --release${arch:+$([ "$arch" = arm64 ] && echo ' --aarch64')}" >&2 ;;
        ext-ros2) echo "         ./build.sh --release$([ "$arch" = arm64 ] && echo ' --aarch64') --set-extension-platform=ROS2" >&2 ;;
        ext-agl)  echo "         ./build.sh --release$([ "$arch" = arm64 ] && echo ' --aarch64') --set-extension-platform=AGL" >&2 ;;
      esac
      guard_errors=$((guard_errors+1))
      continue
    fi

    # 산출물 최신 mtime vs 그 타겟 소스의 마지막 커밋 시각.
    newest_epoch="$(find "${bin_dir}" -type f -printf '%T@\n' 2>/dev/null \
                    | sort -rn | head -1 | cut -d. -f1)"
    newest_epoch="${newest_epoch:-0}"
    # shellcheck disable=SC2086  # 경로 목록을 의도적으로 단어 분리
    src_epoch="$(git log -1 --format=%ct -- ${TARGET_SRC[$tgt]} 2>/dev/null || echo 0)"
    src_epoch="${src_epoch:-0}"
    if [ "${src_epoch}" -gt 0 ] && [ "${newest_epoch}" -lt "${src_epoch}" ]; then
      lag=$(( src_epoch - newest_epoch ))
      if [ "${lag}" -ge 86400 ]; then age_txt="약 $(( lag / 86400 ))일"; else age_txt="약 $(( lag / 3600 ))시간"; fi
      msg="산출물이 소스 변경보다 ${age_txt} 뒤처짐 — ${bin_dir}
       (소스 최종변경 $(date -d "@${src_epoch}" '+%m-%d %H:%M') > 산출물 $(date -d "@${newest_epoch}" '+%m-%d %H:%M'))"
      if [ "${ALLOW_STALE_OUTPUT:-0}" = "1" ]; then
        echo "WARNING: ${msg} — ALLOW_STALE_OUTPUT=1 로 통과" >&2
      else
        echo "ERROR: ${msg}" >&2
        echo "       재빌드하거나, 의도한 것이면 ALLOW_STALE_OUTPUT=1 을 주세요." >&2
        guard_errors=$((guard_errors+1))
      fi
    fi
  done
done

if [ "${guard_errors}" -gt 0 ]; then
  echo "" >&2
  echo "산출물 가드 실패 ${guard_errors}건 — 이미지를 만들지 않았습니다." >&2
  exit 1
fi
echo "산출물 가드 통과 (BUILD_TYPE=${BUILD_TYPE}, ${#ARCHS[@]} arch × ${#TARGETS[@]} target)"
echo ""

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
